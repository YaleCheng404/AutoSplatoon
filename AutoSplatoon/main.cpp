#include "mainwindow.h"
#include "flashdialog.h"

#include <QApplication>
#include <QTimer>
#include <QPainter>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScreen>
#include <QTranslator>
#include <QLibraryInfo>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationName("AutoSplatoon");
    a.setApplicationName("AutoSplatoon");
    a.setApplicationVersion("1.0.3");
    a.setStyle("Fusion");
    QTranslator translations;
#ifdef Q_OS_MACOS
    const QString translationPath = a.applicationDirPath() + "/../Resources/translations";
#else
    const QString translationPath = a.applicationDirPath() + "/translations";
#endif
    if (translations.load("qtbase_zh_CN", translationPath) ||
        translations.load("qtbase_zh_CN", QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
        a.installTranslator(&translations);
    verifiedFirmware();
    AutoSplatoon w;
    w.show();
    if (a.arguments().contains("--smoke-test")) {
        QImage sample(640, 240, QImage::Format_RGB32); sample.fill(Qt::white);
        QPainter painter(&sample); painter.setPen(QPen(Qt::black, 5));
        painter.drawEllipse(80, 30, 180, 180); painter.drawLine(320, 200, 580, 35); painter.end();
        w.loadImage(sample);
        auto* poll = new QTimer(&w);
        QObject::connect(poll, &QTimer::timeout, &w, [&] {
            if (w.outputImage().isNull()) return;
            const QString directory = qEnvironmentVariable("AUTOSPLATOON_SMOKE_OUTPUT");
            if (!directory.isEmpty()) {
                w.grab().save(directory + "/window.png");
                w.outputImage().save(directory + "/output.png");
                QFile metadata(directory + "/window.json");
                if (metadata.open(QIODevice::WriteOnly)) metadata.write(QJsonDocument(QJsonObject{
                    {"width", w.width()}, {"height", w.height()},
                    {"screenWidth", w.screen()->availableGeometry().width()},
                    {"screenHeight", w.screen()->availableGeometry().height()},
                    {"dpr", w.devicePixelRatioF()}}).toJson());
            }
            a.exit(0);
        });
        poll->start(100);
        QTimer::singleShot(10000, &a, [&] { a.exit(2); });
    }
    return a.exec();
}
