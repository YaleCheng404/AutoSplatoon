#include "drawingplanner.h"
#include "actionqueueplayer.h"
#include "imageprocessor.h"
#include "canvasview.h"
#include "mainwindow.h"
#include "flashdialog.h"
#include "serialconnection.h"
#include <QtTest>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QSettings>
#include <QTemporaryDir>
#include <QPainter>
#include <QPlainTextEdit>
#include <QLinearGradient>
#include <QDir>

class CoreTests : public QObject {
    Q_OBJECT
    QTemporaryDir settings;
private slots:
    void initTestCase()
    {
        QCoreApplication::setOrganizationName("AutoSplatoonTests");
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
        QSettings().setValue("guideSeen", true);
        AutoSplatoon window;
        QVERIFY(QFontMetrics(QApplication::font()).inFontUcs4(0x56fe));
    }
    void plannerReplay_data()
    {
        QTest::addColumn<int>("startRow"); QTest::addColumn<int>("startColumn");
        QTest::newRow("origin") << 0 << 0;
        QTest::newRow("partial-even") << 0 << 3;
        QTest::newRow("partial-odd") << 1 << 4;
        QTest::newRow("last-pixel") << 4 << 6;
    }
    void plannerReplay()
    {
        QFETCH(int, startRow); QFETCH(int, startColumn);
        QImage image(7, 5, QImage::Format_RGB32); image.fill(Qt::white);
        const QList<QPoint> points = {{0,0}, {6,0}, {0,1}, {6,1}, {3,2}, {0,4}, {6,4}};
        for (const QPoint& point : points) image.setPixelColor(point, Qt::black);
        const auto plan = makeDrawPlan(image, startRow, startColumn, 70, true);
        int x = startColumn, y = startRow, saves = 0;
        QSet<QPoint> painted;
        qint64 elapsed = 0;
        for (const auto& frame : plan.frames) {
            QVERIFY(x >= 0 && x < image.width()); QVERIFY(y >= 0 && y < image.height());
            elapsed += frame.durationMs;
            if (frame.action == InputEmulator::BTN_A) painted.insert(QPoint(x, y));
            if (frame.action == InputEmulator::DPAD_R) ++x;
            if (frame.action == InputEmulator::DPAD_L) --x;
            if (frame.action == InputEmulator::DPAD_D) ++y;
            if (frame.action == InputEmulator::BTN_MINUS) ++saves;
        }
        QSet<QPoint> expected;
        for (const auto& p : points)
            if (p.y() > startRow || (p.y() == startRow &&
                (startRow % 2 == 0 ? p.x() >= startColumn : p.x() <= startColumn))) expected.insert(p);
        QCOMPARE(painted, expected); QCOMPARE(elapsed, plan.durationMs); QCOMPARE(saves, 1);
        QCOMPARE(plan.skippedPixels, int(points.size() - expected.size()));
    }
    void plannerSinglePixelAndEmpty()
    {
        QImage image(1, 1, QImage::Format_RGB32); image.fill(Qt::black);
        auto plan = makeDrawPlan(image, 0, 0, 70, false);
        QCOMPARE(plan.frames.size(), 2); QCOMPARE(plan.frames[0].action, InputEmulator::BTN_A);
        image.fill(Qt::white); QVERIFY(makeDrawPlan(image, 0, 0, 70, false).frames.isEmpty());
        QVERIFY_EXCEPTION_THROWN(makeDrawPlan(image, 1, 0, 70, false), std::invalid_argument);
    }
    void presets_data()
    {
        QTest::addColumn<int>("mode");
        for (int i = 0; i < 4; ++i) QTest::newRow(qPrintable(QString::number(i))) << i;
    }
    void presets()
    {
        QFETCH(int, mode);
        // Odd source stride, alpha, text, gradient and hard edges.
        QImage image(641, 241, QImage::Format_ARGB32); image.fill(Qt::transparent);
        QPainter p(&image); p.fillRect(0, 0, 640, 240, Qt::white);
        p.setPen(Qt::black); p.drawText(QRect(10, 10, 300, 100), "Splatoon");
        p.drawEllipse(100, 50, 120, 120);
        QLinearGradient gradient(360, 0, 640, 0); gradient.setColorAt(0, Qt::black); gradient.setColorAt(1, Qt::white);
        p.fillRect(360, 40, 260, 180, gradient); p.end();
        ProcessOptions options; options.preset = ImagePreset(mode);
        const QImage result = processImage(image, options);
        QCOMPARE(result.size(), QSize(320, 120)); QCOMPARE(result.devicePixelRatio(), 1.);
        QCOMPARE(result, processImage(image, options));
        const auto qa = qEnvironmentVariable("AUTOSPLATOON_QA_OUTPUT");
        if (!qa.isEmpty()) {
            QDir().mkpath(qa);
            image.save(qa + "/preset-source.png");
            result.save(qa + QString("/preset-%1.png").arg(mode));
        }
        for (int y = 0; y < 120; ++y) for (int x = 0; x < 320; ++x) {
            int value = qGray(result.pixel(x, y)); QVERIFY(value == 0 || value == 255);
        }
        options.invert = true;
        const QImage inverse = processImage(image, options);
        for (int y = 0; y < 120; ++y) for (int x = 0; x < 320; ++x)
            QCOMPARE(qGray(result.pixel(x,y)) + qGray(inverse.pixel(x,y)), 255);
    }
    void uniformAndTinyImages()
    {
        for (QColor color : {QColor(Qt::black), QColor(Qt::white)}) {
            QImage image(1, 1, QImage::Format_RGB32); image.fill(color);
            for (ImagePreset preset : {ImagePreset::LineArt, ImagePreset::Pencil, ImagePreset::Threshold, ImagePreset::Dither}) {
                ProcessOptions options; options.preset = preset;
                auto result = processImage(image, options);
                if (preset != ImagePreset::Pencil) {
                    QCOMPARE(result.pixelColor(0, 0), color); QCOMPARE(result.pixelColor(319, 119), color);
                } else {
                    QCOMPARE(result.size(), QSize(320,120));
                    QCOMPARE(result, processImage(image, options));
                }
            }
        }
    }
    void photoComparison()
    {
        QImage photo(QFINDTESTDATA("data/fruits.jpg"));
        QVERIFY(!photo.isNull());
        CanvasView canvas(true); canvas.setImage(photo);
        QImage comparison(660, 320, QImage::Format_RGB32); comparison.fill(Qt::white);
        QPainter painter(&comparison);
        const QStringList titles = {QString::fromUtf8("清晰线稿"), QString::fromUtf8("铅笔素描"),
            QString::fromUtf8("纯黑白"), QString::fromUtf8("经典抖动")};
        for (int mode = 0; mode < 4; ++mode) {
            ProcessOptions options; options.preset = ImagePreset(mode);
            const auto result = processImage(canvas.composition(), options);
            QCOMPARE(result.size(), QSize(320,120));
            QCOMPARE(result, processImage(canvas.composition(), options));
            int black = 0;
            for (int y = 0; y < 120; ++y) for (int x = 0; x < 320; ++x) {
                const int value = qGray(result.pixel(x,y)); QVERIFY(value == 0 || value == 255);
                if (value == 0) ++black;
            }
            // Regression: default pencil conversion used to turn the fruit region almost solid black.
            if (mode == int(ImagePreset::Pencil)) QVERIFY(black > 0 && black < 9600);
            const int x = 5 + mode % 2 * 330, y = 20 + mode / 2 * 160;
            painter.drawText(x, y, titles[mode]); painter.drawImage(x, y + 8, result);
        }
        painter.end();
        const auto qa = qEnvironmentVariable("AUTOSPLATOON_QA_OUTPUT");
        if (!qa.isEmpty()) comparison.save(qa + "/photo-comparison.png");
    }
    void playerPauseResumeAndStop()
    {
        ActionQueuePlayer player;
        DrawPlan plan; plan.frames = {{InputEmulator::BTN_A, 30, 0, 0}, {InputEmulator::NO_INPUT, 30, 0, 0}};
        QSignalSpy action(&player, &ActionQueuePlayer::sendAction);
        QSignalSpy finished(&player, &ActionQueuePlayer::finished);
        player.start(plan); player.pause();
        QTRY_VERIFY_WITH_TIMEOUT(player.isPaused(), 500);
        QCOMPARE(action.last().at(0).toULongLong(), InputEmulator::NO_INPUT);
        const int index = player.currentIndex(); QTest::qWait(50); QCOMPARE(player.currentIndex(), index);
        player.resume(); QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 500);
        player.start(plan); player.stop();
        const int count = action.size(); QTest::qWait(80); QCOMPARE(action.size(), count);
        QVERIFY(!player.isRunning()); QCOMPARE(action.last().at(0).toULongLong(), InputEmulator::NO_INPUT);
    }
    void playerReentrantStop()
    {
        ActionQueuePlayer player; DrawPlan plan;
        plan.frames = {{InputEmulator::BTN_A, 20, 0, 0}, {InputEmulator::BTN_A, 20, 0, 1}};
        connect(&player, &ActionQueuePlayer::progressChanged, &player, [&player] { player.stop(); });
        QSignalSpy actions(&player, &ActionQueuePlayer::sendAction);
        player.start(plan); QTest::qWait(60); QCOMPARE(actions.size(), 2); QVERIFY(!player.isRunning());
    }
    void packetCompatibility()
    {
        QCOMPARE(controllerPacket(InputEmulator::BTN_A).left(8).toHex(), QByteArray("0004088080808000"));
        QCOMPARE(controllerPacket(InputEmulator::DPAD_R).left(8).toHex(), QByteArray("0000028080808000"));
        QCOMPARE(controllerPacket(InputEmulator::BTN_MINUS).left(8).toHex(), QByteArray("0100088080808000"));
        // Independent polynomial long division, including all eight payload bytes.
        for (quint64 value : {InputEmulator::NO_INPUT, InputEmulator::BTN_A, InputEmulator::DPAD_R}) {
            auto packet = controllerPacket(value); unsigned remainder = 0;
            for (int i = 0; i < 8; ++i) {
                remainder ^= quint8(packet[i]) << 8;
                for (int bit = 0; bit < 8; ++bit) {
                    remainder <<= 1;
                    if (remainder & 0x10000) remainder ^= 0x10700;
                }
            }
            QCOMPARE(quint8(packet[8]), quint8(remainder >> 8));
        }
    }
    void serialPairingAndPeriodicReports()
    {
        SerialController serial;
        serial.ready = true; // UART handshake completed, before Bluetooth pairing.
        QSignalSpy failed(&serial, &SerialController::failed);
        QSignalSpy activity(&serial, &SerialController::hidActivityChanged);
        serial.handleResponse({});
        serial.checkHidActivity(60000);
        QVERIFY(serial.isReady()); QVERIFY(!serial.hasHidReports()); QCOMPARE(failed.size(), 0);
        serial.handleResponse(QByteArray(3, char(0x90))); // Unsolicited, periodic HID reports.
        QVERIFY(serial.hasHidReports()); QCOMPARE(activity.size(), 1);
        serial.checkHidActivity(serial.lastHidReport + 4999);
        QVERIFY(serial.hasHidReports());
        serial.checkHidActivity(serial.lastHidReport + 5001);
        QVERIFY(serial.isReady()); QVERIFY(!serial.hasHidReports()); QCOMPARE(failed.size(), 0);
        QCOMPARE(activity.size(), 2);
        serial.handleResponse(QByteArray(1, char(0x90)));
        QVERIFY(serial.hasHidReports()); QCOMPARE(activity.size(), 3);
        serial.handleResponse(QByteArray(1, char(0x92)));
        QVERIFY(!serial.isReady()); QCOMPARE(failed.size(), 1);
    }
    void pairingAllowsManualButBlocksDrawing()
    {
        AutoSplatoon window; window.show();
        QImage image(320,120,QImage::Format_RGB32); image.fill(Qt::black);
        window.loadImage(image);
        QTRY_VERIFY_WITH_TIMEOUT(!window.outputImage().isNull(), 10000);
        auto* serial = window.findChild<SerialController*>();
        auto* player = window.findChild<ActionQueuePlayer*>();
        auto* start = window.findChild<QPushButton*>("startButton");
        auto* manual = window.findChild<QPushButton*>("manualButton");
        serial->ready = true; emit serial->connectionChanged(true);
        QVERIFY(manual->isEnabled()); QVERIFY(!start->isEnabled());
        serial->checkHidActivity(60000);
        QVERIFY(manual->isEnabled()); QVERIFY(serial->isReady());
        serial->handleResponse(QByteArray(1, char(0x90)));
        QVERIFY(start->isEnabled());
        QTest::mouseClick(start, Qt::LeftButton); QVERIFY(player->isRunning());
        serial->checkHidActivity(serial->lastHidReport + 5001);
        QVERIFY(!player->isRunning()); QVERIFY(serial->isReady());
        QVERIFY(manual->isEnabled()); QVERIFY(!start->isEnabled());
        serial->handleResponse(QByteArray(1, char(0x90)));
        QVERIFY(start->isEnabled());
    }
    void handshakeTimerNotStarvedByHidStream()
    {
        SerialController serial;
        QSignalSpy parsed(&serial.replyTimer, &QTimer::timeout);
        serial.handleResponse(QByteArray(1, char(0x03)));
        QTimer stream;
        connect(&stream, &QTimer::timeout, &serial, [&serial] {
            serial.handleResponse(QByteArray(1, char(0x90)));
        });
        stream.start(5);
        QTRY_VERIFY_WITH_TIMEOUT(parsed.size() > 0, 500);
        stream.stop();
    }
    void firmwareBundled()
    {
        QCOMPARE(verifiedFirmware().size(), 648768);
    }
    void flashProcess_data()
    {
        QTest::addColumn<QString>("failure");
        QTest::newRow("success") << QString();
        QTest::newRow("unsupported-chip") << QString("probe");
        QTest::newRow("write-failure") << QString("write");
        QTest::newRow("cancel") << QString("cancel");
    }
    void flashProcess()
    {
        QFETCH(QString, failure);
        qputenv("AUTOSPLATOON_FAKE_FAILURE", failure.toUtf8());
        QString executable = QCoreApplication::applicationDirPath() + "/fake_esptool";
#ifdef Q_OS_WIN
        executable += ".exe";
#endif
        FlashDialog dialog("TEST_PORT", nullptr, executable);
        QSignalSpy flashed(&dialog, &FlashDialog::flashed);
        dialog.show();
        auto* start = dialog.findChild<QPushButton*>("flashStart");
        QTest::mouseClick(start, Qt::LeftButton);
        if (failure == "cancel") {
            QTRY_VERIFY_WITH_TIMEOUT(dialog.findChild<QPlainTextEdit*>()->toPlainText().contains("50 %"), 3000);
            QTest::mouseClick(dialog.findChild<QPushButton*>("flashCancel"), Qt::LeftButton);
        }
        QTRY_VERIFY_WITH_TIMEOUT(start->isEnabled(), 5000);
        QCOMPARE(flashed.size(), failure.isEmpty() ? 1 : 0);
        const auto log = dialog.findChild<QPlainTextEdit*>()->toPlainText();
        if (failure == "probe") QVERIFY(!log.contains("Writing"));
        if (failure == "cancel") QVERIFY(log.contains(QString::fromUtf8("取消")));
        qunsetenv("AUTOSPLATOON_FAKE_FAILURE");
    }
    void canvasIsIndependentOfViewport()
    {
        CanvasView view(true); QImage image(641, 241, QImage::Format_ARGB32); image.fill(Qt::transparent);
        image.setPixelColor(320, 120, Qt::black);
        view.setImage(image); const auto before = view.composition();
        view.resize(2560, 1440); view.show(); QTest::qWait(10);
        QCOMPARE(view.composition(), before); view.resize(600, 300); QCOMPARE(view.composition(), before);
        QCOMPARE(before.pixelColor(0,0), QColor(Qt::white));
    }
    void latestProcessingWins()
    {
        AutoSplatoon window; window.show();
        QImage dark(640,240,QImage::Format_RGB32); dark.fill(Qt::black);
        QImage white(640,240,QImage::Format_RGB32); white.fill(Qt::white);
        window.loadImage(dark); window.loadImage(white);
        window.findChild<QComboBox*>("preset")->setCurrentIndex(2);
        QTRY_VERIFY_WITH_TIMEOUT(!window.outputImage().isNull(), 10000);
        QCOMPARE(window.outputImage().pixelColor(0,0), QColor(Qt::white));
        QVERIFY(!window.findChild<QPushButton*>("startButton")->isEnabled());
        window.resize(800,600); QTest::qWait(10);
        QCOMPARE(window.outputImage().size(), QSize(320,120));
    }
};
QTEST_MAIN(CoreTests)
#include "test_core.moc"
