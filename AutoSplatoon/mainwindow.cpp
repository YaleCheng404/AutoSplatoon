#include "mainwindow.h"
#include "flashdialog.h"
#include "manualcontroldialog.h"
#include "samplelibrary.h"
#include "uilanguage.h"
#include "workbenchstyle.h"
#include <QtConcurrent>
#include <QAccessibilityHints>
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
#include <QMenu>
#include <QPainter>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QTabBar>
#include <QToolButton>

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

class WorkbenchLayout : public QVBoxLayout {
public:
    using QVBoxLayout::QVBoxLayout;

    // The scroll area should fit the canvas to the viewport. A wrapped caption's
    // preferred height would otherwise force the whole workbench to scroll.
    bool hasHeightForWidth() const override { return false; }
};

class WorkbenchToolbar : public QWidget {
public:
    WorkbenchToolbar(QWidget* brand, QWidget* files, QWidget* preferences)
        : row(new QBoxLayout(QBoxLayout::LeftToRight, this)),
          controls(new QBoxLayout(QBoxLayout::LeftToRight)), brand(brand), files(files), preferences(preferences)
    {
        setObjectName("workbenchToolbar");
        setAttribute(Qt::WA_StyledBackground);
        row->setContentsMargins(14, 5, 16, 6);
        row->setSpacing(8);
        row->addWidget(brand, 1);
        controls->setSpacing(8);
        controls->addWidget(files, 0, Qt::AlignLeft);
        controls->addWidget(preferences, 0, Qt::AlignLeft);
        row->addLayout(controls);
    }

    QSize minimumSizeHint() const override
    {
        return {qMax(files->minimumSizeHint().width(), preferences->minimumSizeHint().width()) + 30,
                row->minimumSize().height()};
    }

protected:
    void resizeEvent(QResizeEvent* event) override
    {
        QWidget::resizeEvent(event);
        const int controlWidth = files->minimumSizeHint().width() + preferences->minimumSizeHint().width() + controls->spacing();
        const int available = width() - row->contentsMargins().left() - row->contentsMargins().right();
        row->setDirection(available < controlWidth + brand->sizeHint().width() + row->spacing()
            ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
        controls->setDirection(available < controlWidth ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
    }

private:
    QBoxLayout* row;
    QBoxLayout* controls;
    QWidget *brand, *files, *preferences;
};

class InkTabBar : public QTabBar {
public:
    InkTabBar()
    {
        setExpanding(false);
        setDrawBase(false);
        connect(this, &QTabBar::currentChanged, this, [this](int index) {
            if (index < 0) return;
            const QRectF from = marker.isEmpty() ? QRectF(tabRect(index)) : marker;
            const QRectF to = tabRect(index);
            UiMotion::instance().animate(this, 0, 1, 150, [this, from, to](qreal t) {
                marker = QRectF(from.topLeft() + (to.topLeft() - from.topLeft()) * t,
                    from.size() + (to.size() - from.size()) * t);
                update();
            });
        });
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        if (!count()) return;
        const QRectF track = tabRect(0).united(tabRect(count() - 1));
        const QRectF selection = marker.isEmpty() ? QRectF(tabRect(currentIndex())) : marker;
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        const bool dark = palette().windowText().color().lightness() > 128;
        painter.setPen(Qt::NoPen);
        painter.setBrush(dark ? palette().window() : palette().midlight());
        painter.drawRoundedRect(track.adjusted(0, 0, -1, -1), 8, 8);
        painter.setPen(QPen(palette().dark().color(), .7));
        painter.setBrush(dark ? palette().midlight() : palette().base());
        painter.drawRoundedRect(selection.adjusted(2.5, 2.5, -3, -3), 6, 6);
        for (int i = 0; i < count(); ++i) {
            const bool active = i == currentIndex();
            QFont textFont = font();
            if (active) textFont.setWeight(QFont::DemiBold);
            painter.setFont(textFont);
            painter.setPen(active ? palette().highlight().color() : palette().windowText().color());
            painter.drawText(tabRect(i), Qt::AlignCenter, tabText(i));
        }
        if (hasFocus()) {
            painter.setPen(QPen(palette().highlight().color(), 1.3));
            painter.setBrush(Qt::NoBrush);
            painter.drawRoundedRect(QRectF(tabRect(currentIndex())).adjusted(2, 2, -2, -2), 6, 6);
        }
    }

    void resizeEvent(QResizeEvent* event) override
    {
        QTabBar::resizeEvent(event);
        UiMotion::instance().cancel(this);
        marker = tabRect(currentIndex());
    }

private:
    QRectF marker;
};

class PreviewTabs : public QTabWidget {
public:
    PreviewTabs()
    {
        setTabBar(new InkTabBar);
        setDocumentMode(true);
    }
};

QLabel* label(const QString& value, const char* role)
{
    auto* widget = new QLabel(value);
    widget->setProperty(role, true);
    return widget;
}
}
AutoSplatoon::AutoSplatoon(QWidget* parent) : QMainWindow(parent), serial(this), player(this)
{
    Q_INIT_RESOURCE(resources);
    static const int fontId = QFontDatabase::addApplicationFont(":/fonts/SourceHanSansCN-VF.ttf");
    const auto families = QFontDatabase::applicationFontFamilies(fontId);
    if (!families.isEmpty()) {
        QFont font = qApp->font();
        font.setFamily(families.first());
        font.setWeight(QFont::Normal);
        font.setPointSizeF(qMax(10., font.pointSizeF()));
        qApp->setFont(font);
    }
    setWindowTitle(tr("AutoSplatoon · 图像工作台"));
    setWindowIcon(QIcon(":/icons/app.svg"));
    auto* central = new QWidget;
    auto* root = new WorkbenchLayout(central);
    root->setContentsMargins(0, 0, 0, 0); root->setSpacing(0);
    auto* brand = WorkbenchStyle::dialogHeading("AutoSplatoon", tr("广场涂鸦工作台"), "squid-yellow");
    auto* controls = new QWidget;
    auto* toolbar = new QHBoxLayout(controls);
    toolbar->setContentsMargins(0, 0, 0, 0);
    importButton = new QToolButton;
    importButton->setObjectName("importButton");
    importButton->setText(tr("导入图片"));
    importButton->setProperty("primary", true);
    importButton->setPopupMode(QToolButton::MenuButtonPopup);
    auto* imports = new QMenu(importButton);
    imports->addAction(tr("从文件导入…"), this, &AutoSplatoon::importImage);
    auto* samples = imports->addMenu(tr("示例素材"));
    importButton->setMenu(imports);
    exportButton = button(tr("导出 PNG"), "exportButton");
    auto* help = button(tr("使用指南"), "guideButton");
    theme = new QComboBox;
    theme->setObjectName("theme");
    theme->setAccessibleName(tr("界面主题"));
    theme->addItems({tr("跟随系统"), tr("浅色"), tr("深色")});
    theme->setCurrentIndex(qBound(0, QSettings().value("theme", 0).toInt(), 2));
    auto* reduceMotion = new QCheckBox(tr("减少动态效果"));
    reduceMotion->setObjectName("reduceMotion");
    reduceMotion->setChecked(QSettings().value("ui/reduceMotion", false).toBool());
    reduceMotion->setToolTip(tr("勾选后停用装饰动画。未勾选时遵循系统的动态效果偏好。"));
    auto* language = new QComboBox;
    language->setObjectName("language");
    auto* languageLabel = new QLabel(QStringLiteral("Language / 语言"));
    languageLabel->setBuddy(language);
    language->setAccessibleName(languageLabel->text());
    language->addItem(tr("跟随系统"), "system");
    language->addItem(QStringLiteral("简体中文"), "zh_CN");
    language->addItem(QStringLiteral("English"), "en");
    const QString startupPreference = UiLanguage::preference();
    language->setCurrentIndex(language->findData(startupPreference));
    auto* pending = label(tr("下次启动生效"), "muted");
    pending->setObjectName("languagePending");
    pending->setContentsMargins(14, 2, 16, 0);
    pending->setAlignment(Qt::AlignRight);
    pending->setWordWrap(true);
    pending->hide();
    connect(language, &QComboBox::currentIndexChanged, this, [language, pending, startupPreference] {
        const QString preference = language->currentData().toString();
        QSettings().setValue(UiLanguage::settingKey, preference);
        pending->setVisible(preference != startupPreference);
    });
    toolbar->addWidget(importButton);
    toolbar->addWidget(exportButton); toolbar->addWidget(help); toolbar->addWidget(reduceMotion);
    auto* preferences = new QWidget;
    auto* preferenceRow = new QHBoxLayout(preferences);
    preferenceRow->setContentsMargins(0, 0, 0, 0);
    preferenceRow->addWidget(theme);
    preferenceRow->addWidget(languageLabel);
    preferenceRow->addWidget(language);
    root->addWidget(new WorkbenchToolbar(brand, controls, preferences));
    root->addWidget(pending);
    auto* splitter = new QSplitter;
    splitter->setChildrenCollapsible(false);
    splitter->setHandleWidth(7);
    splitter->setMinimumHeight(220);
    auto* canvasPane = new QWidget;
    auto* canvasPaneLayout = new QVBoxLayout(canvasPane);
    canvasPaneLayout->setContentsMargins(14, 10, 14, 8);
    canvasPaneLayout->setSpacing(8);
    auto* canvasHeader = new QHBoxLayout;
    canvasHeader->addWidget(label(tr("画布"), "paneTitle"));
    canvasHeader->addStretch();
    canvasHeader->addWidget(label(tr("320 × 120 像素"), "muted"));
    canvasPaneLayout->addLayout(canvasHeader);
    workspace = new QStackedWidget;
    workspace->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
    workspace->setMinimumHeight(180);
    canvasPaneLayout->addWidget(workspace, 1);
    gallery = new ArtPanel(ArtPanel::Surface::Gallery);
    gallery->setObjectName("sampleGallery");
    auto* galleryLayout = new QVBoxLayout(gallery);
    galleryLayout->setContentsMargins(20, 16, 20, 16);
    auto* galleryContent = new QWidget;
    auto* galleryBody = new QVBoxLayout(galleryContent);
    galleryBody->setContentsMargins(0, 0, 0, 0);
    galleryBody->setSpacing(7);
    galleryBody->addWidget(label(tr("广场涂鸦  /  示例素材"), "eyebrow"));
    galleryBody->addWidget(label(tr("选一张图开始涂鸦"), "heading"));
    auto* intro = label(tr("试试下方示例，或导入自己的图片。构图、预览和导出都可以离线完成。"), "muted");
    intro->setWordWrap(true);
    galleryBody->addWidget(intro);
    auto* cards = new QHBoxLayout;
    const auto& library = sampleLibrary();
    for (int index = 0; index < int(library.size()); ++index) {
        const auto& sample = library[index];
        auto* action = samples->addAction(sample.title, this, [this, index] { loadSample(index); });
        action->setObjectName("sampleAction-" + sample.id);
        action->setEnabled(!sample.image.isNull());
        auto* tile = new QToolButton;
        tile->setObjectName(sample.id);
        tile->setProperty("sample", true);
        tile->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        tile->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        tile->setText(sample.title);
        tile->setIconSize(QSize(120, 45));
        tile->setIcon(QPixmap::fromImage(sample.image));
        tile->setToolTip(sample.description + "\n" + sample.credit);
        tile->setAccessibleDescription(sample.description);
        tile->setEnabled(!sample.image.isNull());
        connect(tile, &QToolButton::clicked, this, [this, index] { loadSample(index); });
        auto* column = new QVBoxLayout;
        column->setSpacing(4);
        column->addWidget(tile);
        auto* caption = label(sample.description, "muted");
        caption->setAlignment(Qt::AlignHCenter);
        caption->setWordWrap(true);
        column->addWidget(caption);
        cards->addLayout(column, 1);
    }
    galleryBody->addSpacing(5);
    galleryBody->addLayout(cards);
    galleryLayout->addStretch();
    galleryLayout->addWidget(galleryContent);
    galleryLayout->addStretch();
    workspace->addWidget(gallery);
    QTimer::singleShot(0, galleryContent, [galleryContent] { UiMotion::instance().reveal(galleryContent, 220, 6); });
    tabs = new PreviewTabs;
    resultView = new CanvasView(false);
    resultView->setObjectName("resultView");
    compositionView = new CanvasView(true);
    compositionView->setObjectName("compositionView");
    tabs->addTab(resultView, tr("最终效果"));
    auto* compositionPage = new QWidget;
    auto* compositionLayout = new QVBoxLayout(compositionPage);
    compositionLayout->addWidget(new QLabel(tr("拖动图片调整位置，滚轮缩放图片。")));
    compositionLayout->addWidget(compositionView, 1);
    auto* reset = button(tr("适应画布 / 重置构图"));
    compositionLayout->addWidget(reset);
    tabs->addTab(compositionPage, tr("调整构图"));
    connect(tabs, &QTabWidget::currentChanged, this, [this] {
        UiMotion::instance().reveal(tabs->currentWidget(), 120, 0, .78);
    });
    workspace->addWidget(tabs);
    splitter->addWidget(canvasPane);
    parameters = new QScrollArea;
    parameters->setObjectName("inspectorScroll");
    parameters->setWidgetResizable(true); parameters->setMinimumWidth(248);
    parameters->setMinimumHeight(120);
    auto* sidebar = new QWidget;
    sidebar->setObjectName("inspectorPanel");
    auto* side = new QVBoxLayout(sidebar);
    side->setContentsMargins(12, 10, 12, 8);
    side->setSpacing(6);
    side->addWidget(label(tr("检查器"), "paneTitle"));
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
    form->addRow(tr("预设"), preset);
    auto* presetHelp = button(tr("预设用途"), "presetHelp");
    presetHelp->setIcon(QIcon(":/icons/chevron-down.svg"));
    presetHelp->setCheckable(true);
    form->addRow(presetHelp);
    auto* presetDescription = label({}, "muted");
    presetDescription->setWordWrap(true);
    auto* descriptionPanel = new QWidget;
    descriptionPanel->setObjectName("presetDescriptionPanel");
    auto* descriptionLayout = new QVBoxLayout(descriptionPanel);
    descriptionLayout->setContentsMargins(0, 0, 0, 0);
    descriptionLayout->setSizeConstraint(QLayout::SetNoConstraint);
    descriptionLayout->addWidget(presetDescription);
    descriptionPanel->setMaximumHeight(0);
    form->addRow(descriptionPanel);
    connect(presetHelp, &QPushButton::toggled, this, [presetHelp, descriptionPanel, presetDescription](bool open) {
        presetHelp->setIcon(QIcon(open ? ":/icons/chevron-up.svg" : ":/icons/chevron-down.svg"));
        const int from = descriptionPanel->height();
        const int to = open ? qMax(presetDescription->sizeHint().height(),
            presetDescription->heightForWidth(descriptionPanel->width())) : 0;
        UiMotion::instance().animate(descriptionPanel, from, to, 180, [descriptionPanel, open, to](qreal height) {
            descriptionPanel->setMaximumHeight(open && qRound(height) == to ? QWIDGETSIZE_MAX : qRound(height));
        });
    });
    form->addRow(tr("细节"), detail);
    form->addRow(tr("去噪"), denoise); form->addRow(tr("线条粗细"), thickness);
    form->addRow(tr("阈值"), threshold); form->addRow(inverse);
    const QStringList descriptions{tr("提取清楚的轮廓，适合插画与有明显边线的图。"),
        tr("用深浅与笔画表现照片和绘画中的细节。"),
        tr("按阈值划分黑白，适合文字、标志和现有线稿。"),
        tr("用黑白点阵表现连续色调，适合渐变和阴影。")};
    for (int i = 0; i < descriptions.size(); ++i) preset->setItemData(i, descriptions[i], Qt::ToolTipRole);
    auto optionsChanged = [this, form, presetDescription, descriptions] {
        const int mode = preset->currentIndex();
        presetDescription->setText(descriptions.value(mode));
        preset->setToolTip(descriptions.value(mode));
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
    ports->setObjectName("ports");
    ports->setMinimumContentsLength(14);
    refreshButton = button(tr("刷新串口"), "refreshButton");
    deviceLayout->addWidget(ports);
    connectButton = button(tr("连接"), "connectButton"); flashButton = button(tr("烧录内置固件"), "flashButton");
    connectButton->setToolTip(tr("连接时自动重启 ESP32；无法配对时先断开再连接，然后在 Switch 上重新配对。"));
    manualButton = button(tr("手动控制 / 配对"), "manualButton");
    auto* serialActions = new QHBoxLayout;
    serialActions->addWidget(connectButton, 1);
    serialActions->addWidget(refreshButton);
    deviceLayout->addLayout(serialActions);
    deviceLayout->addWidget(manualButton);
    deviceLayout->addWidget(flashButton);
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
    auto* taskCard = new QFrame;
    taskCard->setObjectName("taskStrip");
    auto* taskLayout = new QVBoxLayout(taskCard);
    taskLayout->setContentsMargins(16, 7, 16, 7);
    taskLayout->setSpacing(5);
    auto* metrics = new QHBoxLayout;
    auto* roller = new QLabel;
    roller->setPixmap(WorkbenchStyle::artwork("paint-roller", QSize(20, 20), devicePixelRatioF()));
    metrics->addWidget(roller);
    metrics->addWidget(label(tr("绘图任务"), "metric"));
    metrics->addStretch();
    auto addMetric = [metrics](const QString& title) {
        auto* column = new QVBoxLayout;
        column->setSpacing(0);
        column->addWidget(label(title, "muted"));
        auto* value = label(QStringLiteral("—"), "metric");
        value->setAccessibleName(title);
        column->addWidget(value);
        metrics->addLayout(column);
        return value;
    };
    metrics->setSpacing(13);
    pixelCount = addMetric(tr("黑色像素"));
    estimate = addMetric(tr("预计时间"));
    actionCount = addMetric(tr("动作数量"));
    taskLayout->addLayout(metrics);
    progress = new QProgressBar;
    progress->setObjectName("drawingProgress");
    progress->setAccessibleName(tr("绘图进度"));
    progress->setRange(0, 100);
    progress->setValue(0);
    progress->setTextVisible(false);
    taskLayout->addWidget(progress);
    auto* taskButtons = new QHBoxLayout;
    state = label(tr("选择示例或导入图片"), "muted");
    state->setObjectName("taskState");
    state->setWordWrap(true);
    taskButtons->addWidget(state, 1);
    completion = new QLabel;
    completion->setPixmap(WorkbenchStyle::artwork("squid-purple", QSize(24, 24), devicePixelRatioF()));
    completion->hide();
    taskButtons->addWidget(completion);
    elapsed = label(tr("已用 %1").arg(duration(0)), "muted"); taskButtons->addWidget(elapsed);
    startButton = button(tr("开始绘图"), "startButton"); startButton->setProperty("primary", true);
    pauseButton = button(tr("暂停"), "pauseButton"); stopButton = button(tr("停止"), "stopButton");
    stopButton->setProperty("stop", true);
    taskButtons->addWidget(startButton); taskButtons->addWidget(pauseButton); taskButtons->addWidget(stopButton);
    taskLayout->addLayout(taskButtons);
    auto* workbench = new QScrollArea;
    workbench->setObjectName("workbenchScroll");
    workbench->setWidgetResizable(true); workbench->setFrameShape(QFrame::NoFrame);
    workbench->setWidget(central);
    auto* shell = new QWidget;
    auto* shellLayout = new QVBoxLayout(shell);
    shellLayout->setContentsMargins(0, 0, 0, 0);
    shellLayout->setSpacing(0);
    shellLayout->addWidget(workbench, 1);
    shellLayout->addWidget(taskCard);
    setCentralWidget(shell);
    connectionState = new QLabel(tr("未连接 · 可离线处理图片"));
    connectionState->setObjectName("connectionState");
    connectionState->setWordWrap(true);
    auto* deviceLamp = new QFrame;
    deviceLamp->setFixedSize(9, 9);
    auto setLamp = [deviceLamp](const QString& color) {
        deviceLamp->setStyleSheet("background: " + color + "; border-radius: 4px;");
    };
    auto updateLamp = [this, setLamp] {
        if (serial.isConnecting()) setLamp("#B39B35");
        else if (serial.hasHidReports()) setLamp("#179C94");
        else if (serial.isReady()) setLamp("#9B72E6");
        else setLamp("#82788E");
    };
    updateLamp();
    statusBar()->addWidget(deviceLamp);
    statusBar()->addWidget(connectionState, 1);
    debounce.setSingleShot(true); debounce.setInterval(80);
    connect(&debounce, &QTimer::timeout, this, &AutoSplatoon::runProcessing);
    connect(&worker, &QFutureWatcher<ProcessResult>::finished, this, [this] {
        const auto result = worker.result();
        if (workingGeneration != generation) { debounce.start(); return; }
        processing = false;
        output = result.image;
        resultView->setImage(output);
        if (!result.error.isEmpty()) state->setText(result.error);
        else {
            state->setText(tr("图片处理完成"));
            resultView->highlightResult();
        }
        updatePlan(); updateButtons();
    });
    connect(compositionView, &CanvasView::compositionChanged, this, &AutoSplatoon::requestProcessing);
    connect(reset, &QPushButton::clicked, compositionView, &CanvasView::resetComposition);
    connect(importButton, &QToolButton::clicked, this, &AutoSplatoon::importImage);
    connect(exportButton, &QPushButton::clicked, this, [this] {
        const auto filename = QFileDialog::getSaveFileName(this, tr("导出最终效果"), "drawing.png", "PNG (*.png)");
        if (!filename.isEmpty() && !output.save(filename, "PNG")) QMessageBox::warning(this, tr("导出失败"), tr("无法写入图片。"));
    });
    connect(help, &QPushButton::clicked, this, &AutoSplatoon::showGuide);
    connect(theme, &QComboBox::currentIndexChanged, this, [this] {
        QSettings().setValue("theme", theme->currentIndex()); applyTheme();
    });
    connect(qApp->styleHints(), &QStyleHints::colorSchemeChanged, this, [this] { applyTheme(); });
    connect(qApp->styleHints()->accessibility(), &QAccessibilityHints::contrastPreferenceChanged,
            this, [this] { applyTheme(); });
    connect(reduceMotion, &QCheckBox::toggled, this, [](bool reduced) {
        QSettings().setValue("ui/reduceMotion", reduced);
        UiMotion::instance().setReduced(reduced);
    });
    connect(refreshButton, &QPushButton::clicked, this, &AutoSplatoon::refreshPorts);
    connect(connectButton, &QPushButton::clicked, this, [this] {
        if (serial.isConnecting()) return;
        if (serial.isReady()) serial.close(); else if (!ports->currentData().toString().isEmpty()) serial.open(ports->currentData().toString());
    });
    connect(&serial, &SerialController::connectingChanged, this, [this, updateLamp] {
        updateLamp();
        updateButtons();
    });
    connect(&serial, &SerialController::connectionChanged, this, [this, updateLamp](bool connected) {
        if (!connected && player.isRunning()) player.stop();
        connectionState->setText(connected ? tr("串口已连接 · 等待 Switch 配对") : tr("未连接 · 可离线处理图片"));
        updateLamp();
        updateButtons();
    });
    connect(&serial, &SerialController::hidActivityChanged, this, [this, updateLamp](bool active) {
        if (!active && player.isRunning()) player.stop();
        connectionState->setText(active ? tr("Switch 手柄通信正常 · 可以绘图") : tr("等待 Switch 配对 · 可使用手动控制"));
        updateLamp();
        updateButtons();
    });
    connect(&serial, &SerialController::message, connectionState, &QLabel::setText);
    connect(&serial, &SerialController::failed, this, [this, setLamp](const QString& error) {
        player.stop();
        connectionState->setText(error);
        setLamp("#CF536C");
        updateButtons();
    });
    connect(flashButton, &QPushButton::clicked, this, [this] {
        serial.close(); FlashDialog dialog(ports->currentData().toString(), this); dialog.exec(); updateButtons();
    });
    connect(manualButton, &QPushButton::clicked, this, &AutoSplatoon::manualControl);
    for (auto* box : {interval, startRow, startColumn}) connect(box, &QSpinBox::valueChanged, this, &AutoSplatoon::updatePlan);
    connect(saveWhenFinished, &QCheckBox::toggled, this, &AutoSplatoon::updatePlan);
    connect(startButton, &QPushButton::clicked, this, [this] {
        if (processing || !serial.isReady() || !serial.hasHidReports() || output.isNull()) return;
        UiMotion::instance().setDrawing(true);
        completion->hide();
        updatePlan(); taskClock.start(); elapsedTimer.start(250); progress->setValue(0);
        elapsed->setText(tr("已用 %1").arg(duration(0)));
        player.start(plan); updateButtons();
    });
    connect(pauseButton, &QPushButton::clicked, this, [this] {
        if (player.isPaused()) { player.resume(); pauseButton->setText(tr("暂停")); }
        else { player.pause(); pauseButton->setText(tr("继续")); }
    });
    connect(stopButton, &QPushButton::clicked, &player, &ActionQueuePlayer::stop);
    connect(&player, &ActionQueuePlayer::sendAction, &serial, &SerialController::sendAction);
    connect(&player, &ActionQueuePlayer::progressChanged, this, [this](int current, int total, int row, int column) {
        const int percent = total ? current * 100 / total : 100;
        progress->setValue(percent);
        state->setText(tr("绘图 %1% · 行 %2，列 %3").arg(percent).arg(row).arg(column));
    });
    connect(&player, &ActionQueuePlayer::finished, this, [this] {
        elapsedTimer.stop(); progress->setValue(100); state->setText(tr("绘图完成")); updateButtons();
        elapsed->setText(tr("已用 %1").arg(duration(taskClock.elapsed())));
        UiMotion::instance().setDrawing(false);
        completion->show();
        UiMotion::instance().reveal(completion, 260);
    });
    connect(&player, &ActionQueuePlayer::stopped, this, [this] {
        UiMotion::instance().setDrawing(false);
        elapsedTimer.stop(); state->setText(tr("绘图已停止")); updateButtons();
    });
    connect(&player, &ActionQueuePlayer::timingOverrun, this, [this](int, int late) {
        pauseButton->setText(tr("继续")); state->setText(tr("调度延迟 %1 ms，已暂停；检查游戏光标后继续。").arg(late));
    });
    connect(&elapsedTimer, &QTimer::timeout, this, [this] { elapsed->setText(tr("已用 %1").arg(duration(taskClock.elapsed()))); });
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
    if (serial.isConnecting()) return;
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
    if (player.isRunning()) return;
    const QString file = QFileDialog::getOpenFileName(this, tr("导入图片"), {}, tr("图片 (*.png *.jpg *.jpeg *.bmp *.webp *.tif *.tiff *.pbm *.pgm *.ppm)"));
    if (file.isEmpty()) return;
    QImageReader reader(file); reader.setAutoTransform(true);
    const QSize size = reader.size();
    if (size.width() > 8192 || size.height() > 8192) reader.setScaledSize(size.scaled(8192, 8192, Qt::KeepAspectRatio));
    QImage image = reader.read();
    if (image.isNull()) {
        QMessageBox::warning(this, tr("无法读取图片"), tr("无法读取所选文件。\n%1").arg(reader.errorString()));
        return;
    }
    loadImage(image);
}
void AutoSplatoon::loadSample(int index)
{
    const auto& sample = sampleLibrary().at(index);
    if (player.isRunning() || sample.image.isNull()) return;
    const QSignalBlocker blockDetail(detail), blockDenoise(denoise), blockThickness(thickness);
    const QSignalBlocker blockThreshold(threshold), blockInverse(inverse);
    const ProcessOptions defaults;
    detail->setValue(defaults.detail);
    denoise->setValue(defaults.denoise);
    thickness->setValue(defaults.thickness);
    threshold->setValue(defaults.threshold);
    inverse->setChecked(defaults.invert);
    preset->setCurrentIndex(int(defaults.preset));
    loadImage(sample.image);
    tabs->setCurrentIndex(0);
}
void AutoSplatoon::loadImage(const QImage& image)
{
    if (image.isNull() || player.isRunning()) return;
    UiMotion::instance().cancel(resultView->viewport());
    resultView->setImage({});
    output = {};
    const bool firstImage = !hasImage;
    hasImage = true;
    workspace->setCurrentWidget(tabs);
    if (firstImage) UiMotion::instance().reveal(tabs->currentWidget(), 150, 0, .68);
    compositionView->setImage(image);
    updatePlan();
}
void AutoSplatoon::requestProcessing()
{
    if (!hasImage || player.isRunning()) return;
    ++generation;
    processing = true;
    completion->hide();
    UiMotion::instance().cancel(resultView->viewport());
    state->setText(tr("正在处理图片…"));
    debounce.start();
    updateButtons();
}
void AutoSplatoon::runProcessing()
{
    if (worker.isRunning() || !hasImage) return;
    workingGeneration = generation;
    ProcessOptions options{ImagePreset(preset->currentIndex()), detail->value(), denoise->value(), thickness->value(), threshold->value(), inverse->isChecked()};
    const QImage image = compositionView->composition();
    worker.setFuture(QtConcurrent::run([image, options] {
        try { return ProcessResult{processImage(image, options), {}}; }
        catch (const std::exception& e) { return ProcessResult{{}, tr("图像处理失败：%1").arg(QString::fromUtf8(e.what()))}; }
    }));
}
void AutoSplatoon::updatePlan()
{
    if (output.isNull()) {
        plan = {};
        for (auto* metric : {pixelCount, estimate, actionCount}) metric->setText(QStringLiteral("—"));
        pixelCount->setToolTip({});
        return;
    }
    plan = makeDrawPlan(output, startRow->value(), startColumn->value(), interval->value(), saveWhenFinished->isChecked());
    pixelCount->setText(plan.skippedPixels ? tr("%1（跳过 %2）").arg(plan.blackPixels).arg(plan.skippedPixels)
                                          : QString::number(plan.blackPixels));
    pixelCount->setToolTip(plan.skippedPixels ? tr("起点之前跳过 %1 个黑点").arg(plan.skippedPixels) : QString());
    estimate->setText(duration(plan.durationMs));
    actionCount->setText(QString::number(plan.frames.size()));
}
void AutoSplatoon::updateButtons()
{
    const bool running = player.isRunning();
    const bool connecting = serial.isConnecting();
    startButton->setEnabled(!running && !processing && !output.isNull() && serial.isReady() && serial.hasHidReports());
    startButton->setToolTip(serial.isReady() && !serial.hasHidReports() ? tr("请先在 Switch 上配对，收到手柄报告后即可绘图。") : QString());
    pauseButton->setEnabled(running); stopButton->setEnabled(running);
    if (!running) pauseButton->setText(tr("暂停"));
    parameters->setEnabled(!running);
    compositionView->setEnabled(!running);
    importButton->setEnabled(!running);
    theme->setEnabled(!running);
    findChild<QPushButton*>("guideButton")->setEnabled(!running);
    exportButton->setEnabled(!processing && !output.isNull());
    connectButton->setText(connecting ? tr("连接中…") : serial.isReady() ? tr("断开连接") : tr("连接"));
    connectButton->setEnabled(!running && !connecting && ports->count());
    ports->setEnabled(!serial.isReady() && !connecting);
    refreshButton->setEnabled(!running && !connecting);
    flashButton->setEnabled(!running && !connecting && ports->count());
    manualButton->setEnabled(!running && !connecting && serial.isReady());
}
void AutoSplatoon::applyTheme()
{
    const bool dark = theme->currentIndex() == 2 || (theme->currentIndex() == 0 && qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark);
    UiMotion::instance().finishAll();
    WorkbenchStyle::apply(dark);
    update();
}
void AutoSplatoon::showGuide()
{
    QDialog dialog(this); dialog.setWindowTitle(tr("首次使用指南"));
    auto* layout = new QVBoxLayout(&dialog);
    layout->addWidget(WorkbenchStyle::dialogHeading(tr("使用指南"), tr("连接 · 配对 · 开始涂鸦"), "pad-guide"));
    auto* guide = new QTextBrowser;
    guide->setPlainText(tr(
        "可以先在工作台载入示例，离线预览四种预设并导出 PNG。\n\n"
        "1. 用 USB 数据线连接传统 ESP32 开发板，选择串口。\n"
        "2. 首次使用点击“烧录内置固件”；无需另外下载工具或固件。\n"
        "3. 点击连接，软件自动重启开发板；连接完成后，在 Switch 的“更改握法 / 顺序”中配对。\n"
        "4. 通过手动控制回到涂鸦界面，清空画布、选择最小画笔、光标移到左上角。\n"
        "5. 导入图片，选择风格并调整构图，最终效果与导出、绘图使用同一图像。\n"
        "6. 点击开始绘图；默认按下和释放各 70 ms，完成后按 − 保存。\n\n"
        "请关闭主机自动休眠。发现错位时停止，清空画布后重新开始。\n"
        "无法配对时先断开再连接，重新配对后校准游戏光标。\n"
        "无设备也能处理、预览和导出图片。"));
    layout->addWidget(guide);
    auto* close = new QDialogButtonBox(QDialogButtonBox::Ok);
    close->button(QDialogButtonBox::Ok)->setText(tr("完成"));
    connect(close, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    layout->addWidget(close);
    dialog.resize(QSize(560, 460).boundedTo(screen()->availableGeometry().size() - QSize(32, 48)));
    dialog.exec();
    QSettings().setValue("guideSeen", true);
}
void AutoSplatoon::manualControl()
{
    if (player.isRunning() || !serial.isReady()) return;
    ManualControlDialog dialog(this);
    connect(&dialog, &ManualControlDialog::sendAction, &serial, &SerialController::sendAction);
    connect(&serial, &SerialController::connectionChanged, &dialog, [&dialog](bool ready) { if (!ready) dialog.reject(); });
    dialog.exec();
}
