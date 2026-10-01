#include "mainwindow.h"
#include "flashdialog.h"
#include <QtConcurrent>
#include <QApplication>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QScrollArea>
#include <QSplitter>
#include <QTabWidget>
#include <QGroupBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QStatusBar>
#include <QFileDialog>
#include <QImageReader>
#include <QSerialPortInfo>
#include <QMessageBox>
#include <QSettings>
#include <QStyleHints>
#include <QScreen>
#include <QStyle>
#include <QDialog>
#include <QDialogButtonBox>
#include <QTextBrowser>
#include <QFontDatabase>
#include <QFontMetrics>

namespace {
QPushButton* button(const QString& text, const QString& name = {})
{
    auto* value = new QPushButton(text);
    value->setObjectName(name);
    return value;
}
QSpinBox* spin(int minimum, int maximum, int value, const QString& name)
{
    auto* box = new QSpinBox;
    box->setRange(minimum, maximum); box->setValue(value); box->setObjectName(name);
    return box;
}
QString duration(qint64 ms)
{
    const qint64 seconds = ms / 1000;
    return QString("%1:%2:%3").arg(seconds / 3600, 2, 10, QLatin1Char('0'))
        .arg(seconds / 60 % 60, 2, 10, QLatin1Char('0')).arg(seconds % 60, 2, 10, QLatin1Char('0'));
}
}
AutoSplatoon::AutoSplatoon(QWidget* parent) : QMainWindow(parent), serial(this), player(this)
{
    Q_INIT_RESOURCE(resources);
    if (!QFontMetrics(qApp->font()).inFontUcs4(0x56fe)) {
        const int id = QFontDatabase::addApplicationFont(":/fonts/NotoSansCJKsc-Regular.otf");
        const auto families = QFontDatabase::applicationFontFamilies(id);
        if (!families.isEmpty()) {
            QFont font = qApp->font();
            font.setFamilies({font.family(), families.first()});
            qApp->setFont(font);
        }
    }
    setWindowTitle(tr("AutoSplatoon · 图像工作台"));
    setWindowIcon(QIcon(":/icons/app.svg"));
    auto* central = new QWidget;
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(16, 12, 16, 12); root->setSpacing(8);
    auto* toolbar = new QHBoxLayout;
    auto* title = new QLabel("AutoSplatoon");
    title->setObjectName("title");
    auto* open = button(tr("导入图片"), "importButton");
    exportButton = button(tr("导出 PNG"), "exportButton");
    auto* help = button(tr("使用指南"));
    theme = new QComboBox;
    theme->addItems({tr("跟随系统"), tr("浅色"), tr("深色")});
    theme->setCurrentIndex(QSettings().value("theme", 0).toInt());
    toolbar->addWidget(title); toolbar->addStretch(); toolbar->addWidget(open);
    toolbar->addWidget(exportButton); toolbar->addWidget(help); toolbar->addWidget(theme);
    root->addLayout(toolbar);
    auto* splitter = new QSplitter;
    auto* tabs = new QTabWidget;
    resultView = new CanvasView(false);
    resultView->setObjectName("resultView");
    compositionView = new CanvasView(true);
    compositionView->setObjectName("compositionView");
    tabs->addTab(resultView, tr("最终效果 · 320 × 120"));
    auto* compositionPage = new QWidget;
    auto* compositionLayout = new QVBoxLayout(compositionPage);
    compositionLayout->addWidget(new QLabel(tr("拖动图片调整位置，滚轮缩放图片。")));
    compositionLayout->addWidget(compositionView, 1);
    auto* reset = button(tr("适应画布 / 重置构图"));
    compositionLayout->addWidget(reset);
    tabs->addTab(compositionPage, tr("调整构图"));
    splitter->addWidget(tabs);
    parameters = new QScrollArea;
    parameters->setWidgetResizable(true); parameters->setMinimumWidth(260);
    auto* sidebar = new QWidget;
    auto* side = new QVBoxLayout(sidebar);
    auto* processingGroup = new QGroupBox(tr("图像风格"));
    auto* form = new QFormLayout(processingGroup);
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    preset = new QComboBox;
    preset->setObjectName("preset");
    preset->addItems({tr("清晰线稿"), tr("铅笔素描"), tr("纯黑白"), tr("经典抖动")});
    detail = spin(0, 100, 50, "detail");
    denoise = spin(0, 3, 1, "denoise");
    thickness = spin(1, 3, 1, "thickness");
    threshold = spin(0, 255, 160, "threshold");
    inverse = new QCheckBox(tr("黑白反色"));
    form->addRow(tr("预设"), preset); form->addRow(tr("细节"), detail);
    form->addRow(tr("去噪"), denoise); form->addRow(tr("线条粗细"), thickness);
    form->addRow(tr("阈值"), threshold); form->addRow(inverse);
    auto optionsChanged = [this, form] {
        const int mode = preset->currentIndex();
        form->setRowVisible(detail, mode <= 1);
        form->setRowVisible(denoise, mode != 3);
        form->setRowVisible(thickness, mode != 3);
        form->setRowVisible(threshold, mode == 1 || mode == 2);
        requestProcessing();
    };
    connect(preset, &QComboBox::currentIndexChanged, this, optionsChanged);
    for (auto* box : {detail, denoise, thickness, threshold})
        connect(box, &QSpinBox::valueChanged, this, optionsChanged);
    connect(inverse, &QCheckBox::toggled, this, optionsChanged);
    optionsChanged();
    side->addWidget(processingGroup);
    auto* device = new QGroupBox(tr("ESP32 设备"));
    auto* deviceLayout = new QVBoxLayout(device);
    ports = new QComboBox; ports->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    ports->setMinimumContentsLength(14);
    auto* refresh = button(tr("刷新串口"));
    deviceLayout->addWidget(ports); deviceLayout->addWidget(refresh);
    connectButton = button(tr("连接")); flashButton = button(tr("烧录内置固件"));
    manualButton = button(tr("手动控制 / 配对"), "manualButton");
    deviceLayout->addWidget(connectButton); deviceLayout->addWidget(flashButton); deviceLayout->addWidget(manualButton);
    auto* driver = new QLabel(tr("未出现串口？检查数据线和驱动。<br>Linux 串口权限需加入 dialout 组。<br>"
        "<a href='https://www.silabs.com/developer-tools/usb-to-uart-bridge-vcp-drivers'>CP210x 驱动</a> · "
        "<a href='https://www.wch.cn/downloads/CH341SER_EXE.html'>CH340 驱动</a>"));
    driver->setTextFormat(Qt::RichText);
    driver->setWordWrap(true); driver->setOpenExternalLinks(true);
    deviceLayout->addWidget(driver); side->addWidget(device);
    auto* drawing = new QGroupBox(tr("绘图设置"));
    auto* drawForm = new QFormLayout(drawing);
    drawForm->setRowWrapPolicy(QFormLayout::WrapLongRows);
    interval = spin(20, 500, 70, "interval"); interval->setSuffix(" ms");
    startRow = spin(0, 119, 0, "startRow"); startColumn = spin(0, 319, 0, "startColumn");
    saveWhenFinished = new QCheckBox(tr("完成后按 − 保存")); saveWhenFinished->setChecked(true);
    drawForm->addRow(tr("按下 / 释放间隔"), interval);
    drawForm->addRow(tr("起始行"), startRow); drawForm->addRow(tr("起始列"), startColumn);
    drawForm->addRow(saveWhenFinished);
    auto* cursorHint = new QLabel(tr("游戏光标需与起点一致，并选择最小画笔。"));
    cursorHint->setWordWrap(true); drawForm->addRow(cursorHint); side->addWidget(drawing); side->addStretch();
    parameters->setWidget(sidebar); splitter->addWidget(parameters);
    splitter->setStretchFactor(0, 1); splitter->setStretchFactor(1, 0);
    splitter->setSizes({760, 300});
    root->addWidget(splitter, 1);
    statistics = new QLabel(tr("导入图片后即可预览，无需连接设备。"));
    statistics->setWordWrap(true); root->addWidget(statistics);
    progress = new QProgressBar; progress->setRange(0, 100); root->addWidget(progress);
    auto* taskButtons = new QHBoxLayout;
    elapsed = new QLabel(tr("已用 00:00:00")); taskButtons->addWidget(elapsed); taskButtons->addStretch();
    startButton = button(tr("开始绘图"), "startButton"); startButton->setProperty("primary", true);
    pauseButton = button(tr("暂停"), "pauseButton"); stopButton = button(tr("停止"), "stopButton");
    taskButtons->addWidget(startButton); taskButtons->addWidget(pauseButton); taskButtons->addWidget(stopButton);
    root->addLayout(taskButtons);
    auto* workbench = new QScrollArea;
    workbench->setObjectName("workbenchScroll");
    workbench->setWidgetResizable(true); workbench->setFrameShape(QFrame::NoFrame);
    workbench->setWidget(central); setCentralWidget(workbench);
    state = new QLabel(tr("未连接 · 可离线处理图片")); statusBar()->addWidget(state, 1);
    debounce.setSingleShot(true); debounce.setInterval(80);
    connect(&debounce, &QTimer::timeout, this, &AutoSplatoon::runProcessing);
    connect(&worker, &QFutureWatcher<ProcessResult>::finished, this, [this] {
        const auto result = worker.result();
        if (workingGeneration != generation) { debounce.start(); return; }
        processing = false;
        if (!result.error.isEmpty()) { state->setText(result.error); output = {}; }
        else { output = result.image; resultView->setImage(output); state->setText(tr("图片处理完成")); }
        updatePlan(); updateButtons();
    });
    connect(compositionView, &CanvasView::compositionChanged, this, &AutoSplatoon::requestProcessing);
    connect(reset, &QPushButton::clicked, compositionView, &CanvasView::resetComposition);
    connect(open, &QPushButton::clicked, this, &AutoSplatoon::importImage);
    connect(exportButton, &QPushButton::clicked, this, [this] {
        const auto filename = QFileDialog::getSaveFileName(this, tr("导出最终效果"), "drawing.png", "PNG (*.png)");
        if (!filename.isEmpty() && !output.save(filename, "PNG")) QMessageBox::warning(this, tr("导出失败"), tr("无法写入图片。"));
    });
    connect(help, &QPushButton::clicked, this, &AutoSplatoon::showGuide);
    connect(theme, &QComboBox::currentIndexChanged, this, [this] {
        QSettings().setValue("theme", theme->currentIndex()); applyTheme();
    });
    connect(qApp->styleHints(), &QStyleHints::colorSchemeChanged, this, [this] { applyTheme(); });
    connect(refresh, &QPushButton::clicked, this, &AutoSplatoon::refreshPorts);
    connect(connectButton, &QPushButton::clicked, this, [this] {
        if (serial.isReady()) serial.close(); else if (!ports->currentData().toString().isEmpty()) serial.open(ports->currentData().toString());
    });
    connect(&serial, &SerialController::connectionChanged, this, [this](bool connected) {
        if (!connected && player.isRunning()) player.stop();
        state->setText(connected ? tr("串口已连接 · 等待 Switch 配对") : tr("未连接 · 可离线处理图片")); updateButtons();
    });
    connect(&serial, &SerialController::hidActivityChanged, this, [this](bool active) {
        if (!active && player.isRunning()) player.stop();
        state->setText(active ? tr("Switch 手柄通信正常 · 可以绘图") : tr("等待 Switch 配对 · 可使用手动控制"));
        updateButtons();
    });
    connect(&serial, &SerialController::message, state, &QLabel::setText);
    connect(&serial, &SerialController::failed, this, [this](const QString& error) {
        player.stop(); state->setText(error); updateButtons();
    });
    connect(flashButton, &QPushButton::clicked, this, [this] {
        serial.close(); FlashDialog dialog(ports->currentData().toString(), this); dialog.exec(); updateButtons();
    });
    connect(manualButton, &QPushButton::clicked, this, &AutoSplatoon::manualControl);
    for (auto* box : {interval, startRow, startColumn}) connect(box, &QSpinBox::valueChanged, this, &AutoSplatoon::updatePlan);
    connect(saveWhenFinished, &QCheckBox::toggled, this, &AutoSplatoon::updatePlan);
    connect(startButton, &QPushButton::clicked, this, [this] {
        if (processing || !serial.isReady() || !serial.hasHidReports() || output.isNull()) return;
        updatePlan(); taskClock.start(); elapsedTimer.start(250); progress->setValue(0);
        player.start(plan); updateButtons();
    });
    connect(pauseButton, &QPushButton::clicked, this, [this] {
        if (player.isPaused()) { player.resume(); pauseButton->setText(tr("暂停")); }
        else { player.pause(); pauseButton->setText(tr("继续")); }
    });
    connect(stopButton, &QPushButton::clicked, &player, &ActionQueuePlayer::stop);
    connect(&player, &ActionQueuePlayer::sendAction, &serial, &SerialController::sendAction);
    connect(&player, &ActionQueuePlayer::progressChanged, this, [this](int current, int total, int row, int column) {
        progress->setValue(total ? current * 100 / total : 100);
        state->setText(tr("绘图 %1/%2 · 行 %3，列 %4").arg(current).arg(total).arg(row).arg(column));
    });
    connect(&player, &ActionQueuePlayer::finished, this, [this] {
        elapsedTimer.stop(); progress->setValue(100); state->setText(tr("绘图完成")); updateButtons();
    });
    connect(&player, &ActionQueuePlayer::stopped, this, [this] {
        elapsedTimer.stop(); state->setText(tr("绘图已停止")); updateButtons();
    });
    connect(&player, &ActionQueuePlayer::timingOverrun, this, [this](int, int late) {
        pauseButton->setText(tr("继续")); state->setText(tr("调度延迟 %1 ms，已暂停；检查游戏光标后继续。").arg(late));
    });
    connect(&elapsedTimer, &QTimer::timeout, this, [this] { elapsed->setText(tr("已用 ") + duration(taskClock.elapsed())); });
    refreshPorts(); applyTheme(); updateButtons();
    const QSize available = screen()->availableGeometry().size();
    resize(QSize(1100, 720).boundedTo(available - QSize(32, 64)));
    if (!QSettings().value("guideSeen", false).toBool() && !qApp->arguments().contains("--smoke-test"))
        QTimer::singleShot(0, this, &AutoSplatoon::showGuide);
}
AutoSplatoon::~AutoSplatoon()
{
    player.stop(); serial.close(); worker.waitForFinished();
}
void AutoSplatoon::refreshPorts()
{
    const QString selected = ports->currentData().toString();
    ports->clear();
    for (const auto& port : QSerialPortInfo::availablePorts()) {
        ports->addItem(port.portName() + " · " + port.description(), port.systemLocation());
        if (port.systemLocation() == selected) ports->setCurrentIndex(ports->count() - 1);
    }
    updateButtons();
}
void AutoSplatoon::importImage()
{
    const QString file = QFileDialog::getOpenFileName(this, tr("导入图片"), {}, tr("图片 (*.png *.jpg *.jpeg *.bmp *.webp *.tif *.tiff *.pbm *.pgm *.ppm)"));
    if (file.isEmpty()) return;
    QImageReader reader(file); reader.setAutoTransform(true);
    const QSize size = reader.size();
    if (size.width() > 8192 || size.height() > 8192) reader.setScaledSize(size.scaled(8192, 8192, Qt::KeepAspectRatio));
    QImage image = reader.read();
    if (image.isNull()) { QMessageBox::warning(this, tr("无法读取图片"), reader.errorString()); return; }
    loadImage(image);
}
void AutoSplatoon::loadImage(const QImage& image)
{
    if (image.isNull() || player.isRunning()) return;
    hasImage = true; compositionView->setImage(image);
}
void AutoSplatoon::requestProcessing()
{
    if (!hasImage || player.isRunning()) return;
    ++generation; processing = true; debounce.start(); updateButtons();
}
void AutoSplatoon::runProcessing()
{
    if (worker.isRunning() || !hasImage) return;
    workingGeneration = generation;
    ProcessOptions options{ImagePreset(preset->currentIndex()), detail->value(), denoise->value(), thickness->value(), threshold->value(), inverse->isChecked()};
    const QImage image = compositionView->composition();
    state->setText(tr("正在处理图片…"));
    worker.setFuture(QtConcurrent::run([image, options] {
        try { return ProcessResult{processImage(image, options), {}}; }
        catch (const std::exception& e) { return ProcessResult{{}, QString::fromUtf8(e.what())}; }
    }));
}
void AutoSplatoon::updatePlan()
{
    if (output.isNull()) return;
    plan = makeDrawPlan(output, startRow->value(), startColumn->value(), interval->value(), saveWhenFinished->isChecked());
    statistics->setText(tr("黑色像素 %1 · 预计 %2 · 动作 %3%4").arg(plan.blackPixels).arg(duration(plan.durationMs))
        .arg(plan.frames.size()).arg(plan.skippedPixels ? tr(" · 起点之前跳过 %1 个黑点").arg(plan.skippedPixels) : QString()));
}
void AutoSplatoon::updateButtons()
{
    const bool running = player.isRunning();
    startButton->setEnabled(!running && !processing && !output.isNull() && serial.isReady() && serial.hasHidReports());
    startButton->setToolTip(serial.isReady() && !serial.hasHidReports() ? tr("请先在 Switch 上配对，收到手柄报告后即可绘图。") : QString());
    pauseButton->setEnabled(running); stopButton->setEnabled(running);
    if (!running) pauseButton->setText(tr("暂停"));
    parameters->setEnabled(!running);
    compositionView->setEnabled(!running);
    findChild<QPushButton*>("importButton")->setEnabled(!running);
    exportButton->setEnabled(!processing && !output.isNull());
    connectButton->setText(serial.isReady() ? tr("断开连接") : tr("连接"));
    connectButton->setEnabled(!running && ports->count());
    ports->setEnabled(!serial.isReady());
    flashButton->setEnabled(!running && ports->count());
    manualButton->setEnabled(!running && serial.isReady());
}
void AutoSplatoon::applyTheme()
{
    const bool dark = theme->currentIndex() == 2 || (theme->currentIndex() == 0 && qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark);
    QPalette palette = QApplication::style()->standardPalette();
    palette.setColor(QPalette::Window, QColor(dark ? "#202127" : "#f5f6fa"));
    palette.setColor(QPalette::WindowText, QColor(dark ? "#eeeeF5" : "#242633"));
    palette.setColor(QPalette::Base, QColor(dark ? "#292b33" : "#ffffff"));
    palette.setColor(QPalette::Text, palette.color(QPalette::WindowText));
    palette.setColor(QPalette::Button, palette.color(QPalette::Base));
    palette.setColor(QPalette::ButtonText, palette.color(QPalette::WindowText));
    palette.setColor(QPalette::Highlight, QColor("#635bdf"));
    palette.setColor(QPalette::Link, QColor(dark ? "#b7afff" : "#5146bf"));
    palette.setColor(QPalette::HighlightedText, Qt::white);
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(dark ? "#858793" : "#8c8e98"));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, palette.color(QPalette::Disabled, QPalette::Text));
    qApp->setPalette(palette);
    qApp->setStyleSheet("QGroupBox { border: 1px solid " + QString(dark ? "#424450" : "#dcdfe9") +
        "; border-radius: 10px; margin-top: 12px; padding: 14px 8px 8px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 5px; }"
        "QPushButton { padding: 8px 12px; border-radius: 6px; border: 1px solid " + QString(dark ? "#454754" : "#dde0e9") +
        "; background: " + QString(dark ? "#30323b" : "#ffffff") + "; }"
        "QPushButton:hover { border-color: #635bdf; }"
        "QPushButton[primary=true] { background: #635bdf; color: white; }"
        "QPushButton[primary=true]:disabled { background: #9894b7; }"
        "QSpinBox, QComboBox { padding: 5px; } QScrollArea { border: none; }"
        "QLabel#title { font-size: 22px; font-weight: bold; } QProgressBar { min-height: 16px; }");
}
void AutoSplatoon::showGuide()
{
    QDialog dialog(this); dialog.setWindowTitle(tr("首次使用指南"));
    auto* layout = new QVBoxLayout(&dialog);
    auto* guide = new QTextBrowser;
    guide->setPlainText(tr(
        "1. 用 USB 数据线连接传统 ESP32 开发板，选择串口。\n"
        "2. 首次使用点击“烧录内置固件”；无需另外下载工具或固件。\n"
        "3. 点击连接，在 Switch 的“更改握法 / 顺序”中进行配对。\n"
        "4. 通过手动控制回到涂鸦界面，清空画布、选择最小画笔、光标移到左上角。\n"
        "5. 导入图片，选择风格并调整构图，最终效果与导出、绘图使用同一图像。\n"
        "6. 点击开始绘图；默认按下和释放各 70 ms，完成后按 − 保存。\n\n"
        "请关闭主机自动休眠。发现错位时停止，清空画布后重新开始。\n"
        "无设备也能处理、预览和导出图片。"));
    layout->addWidget(guide);
    auto* close = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(close, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    layout->addWidget(close);
    dialog.resize(QSize(560, 460).boundedTo(screen()->availableGeometry().size() - QSize(32, 48)));
    dialog.exec();
    QSettings().setValue("guideSeen", true);
}
void AutoSplatoon::manualControl()
{
    QDialog dialog(this); dialog.setWindowTitle(tr("手动控制 / 配对"));
    auto* layout = new QVBoxLayout(&dialog);
    auto* instruction = new QLabel(tr("配对时按 L + R；松开按钮即释放。")); instruction->setWordWrap(true);
    layout->addWidget(instruction);
    auto* grid = new QGridLayout;
    using I = InputEmulator;
    const QList<QPair<QString, quint64>> actions = {{"↑", I::DPAD_U}, {"A", I::BTN_A}, {"B", I::BTN_B},
        {"←", I::DPAD_L}, {"↓", I::DPAD_D}, {"→", I::DPAD_R}, {"X", I::BTN_X}, {"Y", I::BTN_Y},
        {"L + R", I::BTN_L | I::BTN_R}, {"−", I::BTN_MINUS}, {"+", I::BTN_PLUS}, {"Home", I::BTN_HOME},
        {"L", I::BTN_L}, {"R", I::BTN_R}, {"L 点击", I::BTN_LCLICK}, {"ZL", I::BTN_ZL}, {"ZR", I::BTN_ZR}, {"截图", I::BTN_CAPTURE}};
    for (int i = 0; i < actions.size(); ++i) {
        auto* control = button(actions[i].first); grid->addWidget(control, i / 3, i % 3);
        connect(control, &QPushButton::pressed, &dialog, [this, action = actions[i].second] { serial.sendAction(action); });
        connect(control, &QPushButton::released, &dialog, [this] { serial.sendAction(I::NO_INPUT); });
    }
    layout->addLayout(grid);
    connect(&serial, &SerialController::connectionChanged, &dialog, [&dialog](bool ready) { if (!ready) dialog.reject(); });
    dialog.exec(); serial.sendAction(I::NO_INPUT);
}
