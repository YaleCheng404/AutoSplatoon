#include "flashdialog.h"
#include <QApplication>
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QRegularExpression>
#include <QScreen>
#include <stdexcept>

QString bundledEsptool()
{
    QString executable = "esptool";
#ifdef Q_OS_WIN
    executable += ".exe";
#endif
    // macOS: tools live inside Contents/Resources, not outside the bundle.
#ifdef Q_OS_MACOS
    return QApplication::applicationDirPath() + "/../Resources/tools/" + executable;
#else
    return QApplication::applicationDirPath() + "/tools/" + executable;
#endif
}
QByteArray verifiedFirmware()
{
    Q_INIT_RESOURCE(resources);
    QFile file(":/firmware/PRO-UART0.bin");
    if (!file.open(QIODevice::ReadOnly)) throw std::runtime_error("Bundled firmware is missing");
    const QByteArray data = file.readAll();
    if (QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex() !=
        "d8b2e49b221bb363ef1d72908582ce48690e3b9538500aeb77f470b433b66039")
        throw std::runtime_error("Firmware checksum mismatch");
    return data;
}
FlashDialog::FlashDialog(const QString& name, QWidget* parent, const QString& executable)
    : QDialog(parent), port(name), tool(executable)
{
    setWindowTitle(tr("烧录 ESP32 固件"));
    resize(QSize(640, 460).boundedTo(screen()->availableGeometry().size() - QSize(32, 48)));
    auto* layout = new QVBoxLayout(this);
    auto* description = new QLabel(tr("内置 UARTSwitchCon 1.2 · Pro Controller · UART0\n"
        "目标：%1 · 仅支持传统 ESP32，烧录会覆盖原固件。\n"
        "连接开发板；若连接失败，请按住 BOOT 并重新尝试。").arg(port));
    description->setWordWrap(true);
    layout->addWidget(description);
    log = new QPlainTextEdit;
    log->setReadOnly(true);
    layout->addWidget(log, 1);
    progress = new QProgressBar;
    layout->addWidget(progress);
    auto* buttons = new QHBoxLayout;
    start = new QPushButton(tr("烧录内置固件"));
    start->setObjectName("flashStart");
    cancel = new QPushButton(tr("关闭"));
    cancel->setObjectName("flashCancel");
    buttons->addStretch(); buttons->addWidget(start); buttons->addWidget(cancel);
    layout->addLayout(buttons);
    process.setProcessChannelMode(QProcess::MergedChannels);
    timeout.setSingleShot(true);
    connect(start, &QPushButton::clicked, this, &FlashDialog::begin);
    connect(cancel, &QPushButton::clicked, this, [this] {
        if (process.state() != QProcess::NotRunning) {
            process.kill(); finish(false, tr("烧录已取消，请重新烧录后再连接。"));
        } else reject();
    });
    connect(&timeout, &QTimer::timeout, this, [this] {
        process.kill(); finish(false, tr("烧录超时，请检查 USB 线和 BOOT 状态。"));
    });
    connect(&process, &QProcess::readyReadStandardOutput, this, &FlashDialog::appendOutput);
    connect(&process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) finish(false, process.errorString());
    });
    connect(&process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
        [this](int code, QProcess::ExitStatus status) {
            appendOutput();
            if (!active) { start->setEnabled(true); return; }
            if (code != 0 || status != QProcess::NormalExit) {
                finish(false, tr("烧录失败，请查看日志。")); return;
            }
            if (probing) {
                probing = false;
                process.start(tool, {"--chip", "esp32", "--port", port,
                    "--baud", "230400", "write-flash", "0x0", firmwarePath});
                timeout.start(120000);
            } else {
                finish(true, tr("烧录成功。关闭此窗口后连接开发板，并在 Switch 上进行配对。"));
                emit flashed();
            }
        });
}
FlashDialog::~FlashDialog()
{
    if (process.state() != QProcess::NotRunning) { process.kill(); process.waitForFinished(1000); }
}
void FlashDialog::begin()
{
    if (process.state() != QProcess::NotRunning) return;
    try {
        if (port.isEmpty()) throw std::runtime_error("No serial port selected");
        if (!QFileInfo(tool).isExecutable()) throw std::runtime_error("Bundled esptool is missing or not executable");
        const QByteArray data = verifiedFirmware();
        firmwarePath = temporary.filePath("PRO-UART0.bin");
        QFile file(firmwarePath);
        if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size())
            throw std::runtime_error("Cannot prepare firmware");
        file.close();
        log->clear(); progress->setValue(0); start->setEnabled(false);
        cancel->setText(tr("取消烧录"));
        probing = true;
        active = true;
        // esptool verifies the detected chip against --chip before any flash write.
        process.start(tool, {"--chip", "esp32", "--port", port, "chip-id"});
        timeout.start(30000);
    } catch (const std::exception& e) { finish(false, QString::fromUtf8(e.what())); }
}
void FlashDialog::appendOutput()
{
    QString text = QString::fromUtf8(process.readAllStandardOutput());
    text.remove(QRegularExpression("\x1b\\[[0-9;]*[A-Za-z]"));
    log->appendPlainText(text);
    auto matches = QRegularExpression("(\\d{1,3}(?:\\.\\d+)?)\\s*%").globalMatch(text);
    while (matches.hasNext()) progress->setValue(int(matches.next().captured(1).toDouble()));
}
void FlashDialog::finish(bool success, const QString& message)
{
    active = false;
    timeout.stop(); start->setEnabled(process.state() == QProcess::NotRunning); cancel->setText(tr("关闭"));
    if (success) progress->setValue(100);
    log->appendPlainText(message);
}
void FlashDialog::reject()
{
    if (process.state() == QProcess::NotRunning) QDialog::reject();
}
