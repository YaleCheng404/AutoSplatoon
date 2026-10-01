#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QThread>

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    const auto args = app.arguments();
    if (args.value(1) != "--chip" || args.value(2) != "esp32" ||
        args.value(3) != "--port" || args.value(4) != "TEST_PORT") return 10;
    QTextStream out(stdout);
    const QString failure = qEnvironmentVariable("AUTOSPLATOON_FAKE_FAILURE");
    if (args.contains("chip-id")) {
        out << "Chip is ESP32\n" << Qt::flush;
        return failure == "probe" ? 2 : 0;
    }
    if (!args.contains("write-flash") || args.value(args.indexOf("write-flash") + 1) != "0x0") return 11;
    QFile firmware(args.last());
    if (!firmware.open(QIODevice::ReadOnly) || firmware.size() != 648768) return 12;
    out << "Writing (50 %)\n" << Qt::flush;
    if (failure == "cancel") QThread::msleep(1000);
    out << "Writing (100 %)\n" << Qt::flush;
    return failure == "write" ? 3 : 0;
}
