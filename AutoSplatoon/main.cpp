#include "mainwindow.h"
#include "flashdialog.h"
#include "uilanguage.h"

#include <QApplication>
#include <QTimer>
#include <QPainter>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScreen>
#include <QTranslator>
#include <QPushButton>
#include <QMessageBox>
#include <QSettings>
#include <QCryptographicHash>
#include <QDataStream>
#include <exception>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationName("AutoSplatoon");
    a.setApplicationName("AutoSplatoon");
    a.setApplicationVersion(AUTOSPLATOON_VERSION);
    a.setStyle("Fusion");
    const bool smokeTest = a.arguments().contains("--smoke-test");
    if (smokeTest) {
        const QString settingsDirectory = qEnvironmentVariable("AUTOSPLATOON_SMOKE_SETTINGS");
        if (!settingsDirectory.isEmpty()) {
            QSettings::setDefaultFormat(QSettings::IniFormat);
            QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDirectory);
        }
    }
    const QString language = UiLanguage::resolve(UiLanguage::preference());
    QTranslator translator;
    if (!UiLanguage::install(translator, language)) {
        qWarning("Could not load UI translations for %s", qPrintable(language));
        if (smokeTest) return 3;
    }
    try {
        verifiedFirmware();
    } catch (const std::exception& error) {
        const auto message = QString::fromUtf8(error.what());
        if (smokeTest) qCritical("%s", qPrintable(message));
        else QMessageBox::critical(nullptr, a.applicationName(), message);
        return 2;
    }
    AutoSplatoon w;
    w.show();
    if (smokeTest) {
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
                const auto& plan = w.drawingPlan();
                QByteArray frames;
                QDataStream stream(&frames, QIODevice::WriteOnly);
                stream << plan.blackPixels << plan.skippedPixels << plan.durationMs;
                for (const auto& frame : plan.frames)
                    stream << frame.action << frame.durationMs << frame.row << frame.column;
                QFile metadata(directory + "/window.json");
                if (metadata.open(QIODevice::WriteOnly)) metadata.write(QJsonDocument(QJsonObject{
                    {"width", w.width()}, {"height", w.height()},
                    {"screenWidth", w.screen()->availableGeometry().width()},
                    {"screenHeight", w.screen()->availableGeometry().height()},
                    {"language", language}, {"title", w.windowTitle()},
                    {"exportLabel", w.findChild<QPushButton*>("exportButton")->text()},
                    {"planHash", QString::fromLatin1(QCryptographicHash::hash(frames, QCryptographicHash::Sha256).toHex())},
                    {"dpr", w.devicePixelRatioF()}}).toJson());
            }
            a.exit(0);
        });
        poll->start(100);
        QTimer::singleShot(10000, &a, [&] { a.exit(2); });
    }
    return a.exec();
}
