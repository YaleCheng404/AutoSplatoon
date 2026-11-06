#include "canvasview.h"
#include "flashdialog.h"
#include "mainwindow.h"
#include "manualcontroldialog.h"
#include "samplelibrary.h"
#include "uilanguage.h"

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QTranslator>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool passed, const QString& message)
{
    if (!passed) throw std::runtime_error(message.toUtf8().constData());
}

QString digest(const QString& path)
{
    QFile file(path);
    require(file.open(QIODevice::ReadOnly), "Cannot open " + path);
    return QString::fromLatin1(QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha256).toHex());
}

void save(const QImage& image, const QString& path)
{
    require(!image.isNull() && image.save(path, "PNG"), "Cannot save " + path);
}

void caption(QPainter& painter, int x, int y, const QString& text)
{
    QFont font = QApplication::font();
    font.setPixelSize(27);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(QColor("#241B36"));
    painter.drawText(x, y, text);
}

void screenshots(AutoSplatoon& window, QComboBox* theme, const QDir& directory)
{
    theme->setCurrentIndex(1);
    QTest::qWait(35);
    require(window.grab().save(directory.filePath("workbench-light.png")), "Cannot save light workbench");
    theme->setCurrentIndex(2);
    QTest::qWait(35);
    require(window.grab().save(directory.filePath("workbench-dark.png")), "Cannot save dark workbench");
    {
        ManualControlDialog dialog(&window);
        dialog.show();
        QTest::qWait(35);
        require(dialog.grab().save(directory.filePath("manual-control.png")), "Cannot save manual control");
    }
    {
        FlashDialog dialog("COM3", &window);
        dialog.show();
        QTest::qWait(35);
        require(dialog.grab().save(directory.filePath("flash-dialog.png")), "Cannot save flash dialog");
    }
    bool guideSaved = false;
    QTimer::singleShot(20, &window, [&] {
        auto* guide = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        if (!guide) guide = window.findChild<QDialog*>();
        if (guide) {
            guideSaved = guide->grab().save(directory.filePath("guide-dialog.png"));
            guide->accept();
        }
    });
    window.findChild<QPushButton*>("guideButton")->click();
    require(guideSaved, "Cannot save guide dialog");
    window.resize(800, 550);
    QTest::qWait(35);
    auto* scroller = window.findChild<QScrollArea*>("workbenchScroll");
    auto* result = window.findChild<CanvasView*>("resultView");
    auto* stop = window.findChild<QPushButton*>("stopButton");
    const QRect preview(result->viewport()->mapTo(scroller->viewport(), QPoint()), result->viewport()->size());
    const QRect stopGeometry(stop->mapTo(&window, QPoint()), stop->size());
    require(scroller->viewport()->rect().contains(preview) && window.rect().contains(stopGeometry),
        QStringLiteral("Small-window preview (%1,%2 %3x%4) exceeds the viewport (%5x%6)")
            .arg(preview.x()).arg(preview.y()).arg(preview.width()).arg(preview.height())
            .arg(scroller->viewport()->width()).arg(scroller->viewport()->height()));
    require(window.grab().save(directory.filePath("workbench-small.png")), "Cannot save small workbench");
    window.resize(1100, 720);
    theme->setCurrentIndex(1);
}
}

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.setOrganizationName("AutoSplatoonExamples");
    app.setApplicationName("AutoSplatoonExamples");
    app.setStyle("Fusion");
    const QTemporaryDir settings;
    try {
        const auto arguments = app.arguments();
        require(arguments.size() == 2 || arguments.size() == 3,
                "Usage: autosplatoon_examples <output-directory> [en|zh_CN]");
        const QString language = arguments.value(2, "zh_CN");
        require(language == "en" || language == "zh_CN", "Expected en or zh_CN");
        require(settings.isValid(), "Cannot create isolated settings");
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
        QSettings().setValue("guideSeen", true);
        QSettings().setValue("theme", 1);
        QSettings().setValue("ui/reduceMotion", true);
        QSettings().setValue(UiLanguage::settingKey, language);
        QTranslator translator;
        require(UiLanguage::install(translator, language), "Cannot load " + language + " translations");
        const QDir output(arguments.at(1));
        const QDir localized(output.filePath(language == "en" ? "en" : "zh-CN"));
        require(QDir().mkpath(localized.absolutePath()), "Cannot create output directory");
        AutoSplatoon window;
        window.resize(1100, 720);
        window.show();
        QTest::qWait(35);
        require(window.grab().save(localized.filePath("sample-gallery.png")), "Cannot save sample gallery");
        auto* canvas = window.findChild<CanvasView*>("compositionView");
        auto* preset = window.findChild<QComboBox*>("preset");
        auto* theme = window.findChild<QComboBox*>("theme");
        auto* exportButton = window.findChild<QPushButton*>("exportButton");
        const QStringList filenames{"line-art", "pencil", "threshold", "dither"};
        QJsonObject manifest;
        for (const auto& sample : sampleLibrary()) {
            auto* load = window.findChild<QAction*>("sampleAction-" + sample.id);
            require(load && load->isEnabled() && sample.image.size() == QSize(1280, 480), "Invalid sample " + sample.id);
            load->trigger();
            require(preset->currentIndex() == 0, "Sample did not reset to Line Art");
            const QString sourcePath = output.filePath(sample.id + "-source.png");
            save(sample.image, sourcePath);
            const QImage composition = canvas->composition();
            QImage grid(1344, 618, QImage::Format_RGB32);
            grid.fill(QColor("#F1EFF5"));
            QPainter comparison(&grid);
            QJsonObject hashes;
            for (int index = 0; index < filenames.size(); ++index) {
                preset->setCurrentIndex(index);
                ProcessOptions options;
                options.preset = ImagePreset(index);
                const QImage expected = processImage(composition, options);
                require(QTest::qWaitFor([&] { return exportButton->isEnabled() && !window.outputImage().isNull(); }, 12000),
                        "Processing timed out for " + sample.id + "/" + filenames[index]);
                const QImage actual = window.outputImage().convertToFormat(QImage::Format_Grayscale8);
                require(actual.size() == QSize(320, 120) &&
                        actual == expected.convertToFormat(QImage::Format_Grayscale8),
                        "Window output differs from processor for " + sample.id + "/" + filenames[index]);
                for (int y = 0; y < actual.height(); ++y) {
                    for (int x = 0; x < actual.width(); ++x)
                        require(actual.constScanLine(y)[x] == 0 || actual.constScanLine(y)[x] == 255, "Non-binary pixel");
                }
                const QString file = sample.id + "-" + filenames[index] + ".png";
                save(actual, output.filePath(file));
                hashes[file] = digest(output.filePath(file));
                const int x = 24 + (index % 2) * 656;
                const int y = 24 + (index / 2) * 300;
                caption(comparison, x, y + 25, preset->itemText(index));
                comparison.drawImage(x, y + 39, actual.scaled(640, 240, Qt::IgnoreAspectRatio, Qt::FastTransformation));
                if (index == 0) {
                    QImage pair(1344, 365, QImage::Format_RGB32);
                    pair.fill(QColor("#F1EFF5"));
                    QPainter painter(&pair);
                    caption(painter, 24, 42, sample.title);
                    caption(painter, 24, 78, QCoreApplication::translate("Examples", "01  示例原图"));
                    caption(painter, 680, 78, QCoreApplication::translate("Examples", "02  清晰线稿 · 320 × 120"));
                    painter.setRenderHint(QPainter::SmoothPixmapTransform);
                    painter.drawImage(QRect(24, 96, 640, 240), sample.image);
                    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
                    painter.drawImage(680, 96, actual.scaled(640, 240, Qt::IgnoreAspectRatio, Qt::FastTransformation));
                    painter.end();
                    save(pair, localized.filePath(sample.id + "-before-after.png"));
                    if (sample.id == "squid-party") screenshots(window, theme, localized);
                }
            }
            comparison.end();
            save(grid, localized.filePath(sample.id + "-presets.png"));
            manifest[sample.id] = QJsonObject{{"title", sample.title}, {"credit", sample.credit},
                {"source_sha256", digest(sourcePath)}, {"output_sha256", hashes}};
        }
        // Keep the shared manifest in its original language regardless of generation order.
        if (language == "en") {
            std::cout << "Verified 3 samples, 4 presets each, 320 x 120 binary output.\n";
            return 0;
        }
        QFile record(output.filePath("examples.json"));
        require(record.open(QIODevice::WriteOnly), "Cannot write verification manifest");
        const QByteArray json = QJsonDocument(QJsonObject{
            {"output", "320x120; black or white; checked against processImage"},
            {"settings", "detail=50, denoise=1, thickness=1, threshold=160, invert=false"},
            {"samples", manifest}}).toJson();
        require(record.write(json) == json.size(), "Cannot write verification manifest");
        std::cout << "Verified 3 samples, 4 presets each, 320 x 120 binary output.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
