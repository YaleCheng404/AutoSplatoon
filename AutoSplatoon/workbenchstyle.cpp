#include "workbenchstyle.h"

#include <QAccessibilityHints>
#include <QApplication>
#include <QEvent>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QSettings>
#include <QStyle>
#include <QStyleHints>
#include <QSvgRenderer>
#include <QTimer>
#include <QVariantAnimation>
#include <QVBoxLayout>

namespace WorkbenchStyle {
bool highContrast()
{
    return qApp->styleHints()->accessibility()->contrastPreference() == Qt::ContrastPreference::HighContrast;
}

QPixmap artwork(const QString& name, QSize size, qreal dpr)
{
    QSvgRenderer renderer(QStringLiteral(":/art/") + name + QStringLiteral(".svg"));
    if (!renderer.isValid()) return {};
    QPixmap pixmap(size * dpr);
    pixmap.setDevicePixelRatio(dpr);
    pixmap.fill(Qt::transparent);
    QSizeF fitted = renderer.defaultSize();
    fitted.scale(QSizeF(size), Qt::KeepAspectRatio);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    renderer.render(&painter, QRectF((size.width() - fitted.width()) / 2,
        (size.height() - fitted.height()) / 2, fitted.width(), fitted.height()));
    return pixmap;
}

void apply(bool dark)
{
    const bool contrast = highContrast();
    const QString background = dark ? "#202025" : "#F2F3F6";
    const QString surface = dark ? "#28282D" : "#FFFFFF";
    const QString text = dark ? "#F4F4F7" : "#202027";
    const QString muted = dark ? "#BCBCC6" : "#595963";
    const QString line = contrast ? text : dark ? "#4D4D56" : "#DADBE2";
    const QString hover = dark ? "#3A3646" : "#F0ECFB";
    const QString accent = dark ? "#C7B5FF" : "#5D40A5";
    const QString disabled = dark ? "#AAAAB6" : "#696974";
    QPalette palette = qApp->style()->standardPalette();
    palette.setColor(QPalette::Window, QColor(background));
    palette.setColor(QPalette::WindowText, QColor(text));
    palette.setColor(QPalette::Base, QColor(surface));
    palette.setColor(QPalette::AlternateBase, QColor(dark ? "#2E2E34" : "#F8F8FA"));
    palette.setColor(QPalette::Midlight, QColor(dark ? "#38383F" : "#E9E9EF"));
    palette.setColor(QPalette::Dark, QColor(line));
    palette.setColor(QPalette::Text, QColor(text));
    palette.setColor(QPalette::Button, QColor(surface));
    palette.setColor(QPalette::ButtonText, QColor(text));
    palette.setColor(QPalette::Mid, QColor(background));
    palette.setColor(QPalette::Link, QColor(accent));
    palette.setColor(QPalette::Highlight, QColor(accent));
    palette.setColor(QPalette::HighlightedText, dark ? QColor("#201A2D") : QColor(Qt::white));
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(disabled));
    palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(disabled));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(disabled));
    qApp->setPalette(palette);
    qApp->setStyleSheet(QStringLiteral(R"(
        QWidget#workbenchToolbar, QFrame#taskStrip {
            background: %1;
        }
        QWidget#workbenchToolbar { border-bottom: 1px solid %2; }
        QFrame#taskStrip { border-top: 1px solid %2; }
        QWidget#inspectorPanel, QScrollArea#inspectorScroll { background: palette(alternate-base); }
        QGroupBox {
            border: none; border-top: 1px solid %2; margin-top: 10px;
            padding: 15px 0 6px; font-weight: 600;
        }
        QGroupBox::title { subcontrol-origin: margin; left: 0; padding-right: 8px; background: palette(alternate-base); }
        QPushButton, QToolButton, QComboBox, QSpinBox {
            background: %1; border: 1px solid %2; border-radius: 7px; padding: 6px 9px;
        }
        QComboBox, QSpinBox { padding-right: 19px; }
        QComboBox::drop-down { subcontrol-origin: padding; width: 17px; border: none; }
        QComboBox::down-arrow { image: url(:/icons/chevron-down.svg); width: 9px; height: 6px; }
        QSpinBox::up-button, QSpinBox::down-button {
            subcontrol-origin: border; width: 17px; background: %4;
            border: none;
        }
        QSpinBox::up-button { subcontrol-position: top right; margin: 2px 2px 0 0; }
        QSpinBox::down-button { subcontrol-position: bottom right; margin: 0 2px 2px 0; }
        QSpinBox::up-arrow { image: url(:/icons/chevron-up.svg); width: 9px; height: 6px; }
        QSpinBox::down-arrow { image: url(:/icons/chevron-down.svg); width: 9px; height: 6px; }
        QCheckBox::indicator { width: 12px; height: 12px; border: 1px solid %2; border-radius: 4px; background: %1; }
        QCheckBox::indicator:checked { background: #6847BD; border-color: #6847BD; image: url(:/icons/checkmark.svg); }
        QCheckBox::indicator:disabled { background: %5; border-color: %2; }
        QPushButton:hover, QToolButton:hover, QComboBox:hover, QSpinBox:hover {
            border-color: %3; background: %4;
        }
        QPushButton:focus, QToolButton:focus, QComboBox:focus, QSpinBox:focus, QCheckBox:focus {
            border: 1px solid %3; outline: 1px solid %3;
        }
        QPushButton:pressed, QToolButton:pressed, QToolButton:checked {
            border-color: %3; background: %4; color: %3;
        }
        QPushButton[primary="true"], QToolButton[primary="true"] {
            background: #6847BD; border-color: #6847BD; color: white; font-weight: 600;
        }
        QPushButton[primary="true"]:hover, QToolButton[primary="true"]:hover { background: #7656C8; }
        QPushButton[primary="true"]:pressed, QToolButton[primary="true"]:pressed { background: #5637A8; }
        QToolButton#importButton { padding-right: 23px; }
        QToolButton#importButton::menu-button { border-left: 1px solid #977FDC; width: 18px; }
        QToolButton#importButton::menu-arrow { image: url(:/icons/chevron-light.svg); width: 9px; height: 6px; }
        QPushButton:disabled, QToolButton:disabled, QPushButton[primary="true"]:disabled,
        QToolButton[primary="true"]:disabled, QComboBox:disabled, QSpinBox:disabled {
            background: %5; border-color: %2; color: %6;
        }
        QPushButton[stop="true"]:enabled { color: %7; font-weight: 700; }
        QToolButton[sample="true"] {
            background: %1; border: 1px solid %2; border-radius: 10px; padding: 9px; font-weight: 600;
        }
        QToolButton[sample="true"]:hover, QToolButton[sample="true"]:focus {
            border-color: %3; background: %4;
        }
        QLabel[heading="true"] { font-size: 23px; font-weight: 700; }
        QLabel[paneTitle="true"] { font-size: 14px; font-weight: 600; }
        QLabel[muted="true"] { color: %8; }
        QLabel[metric="true"] { font-size: 14px; font-weight: 600; }
        QLabel[eyebrow="true"] { font-size: 11px; font-weight: 600; color: %3; }
        QLabel[bannerTitle="true"] { font-size: 18px; font-weight: 600; color: %10; }
        QLabel[bannerCaption="true"] { font-size: 11px; color: %8; }
        QLabel#connectionState { color: %8; padding: 2px 4px; }
        QScrollArea { border: none; }
        QSplitter::handle:horizontal { background: %2; margin: 0 3px; }
        QTabWidget::pane { border: none; background: %5; }
        QTabBar::tab { padding: 7px 15px; border: none; }
        QGraphicsView { border: none; }
        QTextBrowser, QPlainTextEdit { border: 1px solid %2; border-radius: 7px; }
        QProgressBar { border: none; border-radius: 4px; background: palette(midlight); text-align: center; }
        QProgressBar::chunk { background: %9; border-radius: 4px; }
        QProgressBar#drawingProgress { min-height: 5px; max-height: 5px; }
        QStatusBar { background: %1; }
    )").arg(surface, line, accent, hover, background, disabled,
            dark ? "#FFABB8" : "#9B263C", muted, dark ? "#A784FF" : "#7350CB", text));
}

QWidget* dialogHeading(const QString& title, const QString& caption, const QString& art)
{
    auto* banner = new ArtPanel(ArtPanel::Surface::Banner);
    auto* row = new QHBoxLayout(banner);
    row->setContentsMargins(6, 6, 10, 6);
    auto* picture = new QLabel;
    picture->setPixmap(artwork(art, QSize(32, 32), banner->devicePixelRatioF()));
    row->addWidget(picture);
    auto* column = new QVBoxLayout;
    column->setSpacing(2);
    auto* name = new QLabel(title);
    name->setProperty("bannerTitle", true);
    auto* subtitle = new QLabel(caption);
    subtitle->setProperty("bannerCaption", true);
    subtitle->setWordWrap(true);
    column->addWidget(name);
    column->addWidget(subtitle);
    row->addLayout(column, 1);
    QTimer::singleShot(0, picture, [picture] { UiMotion::instance().reveal(picture, 160); });
    return banner;
}
}

ArtPanel::ArtPanel(Surface type, QWidget* parent)
    : QFrame(parent), surface(type),
      texture(QPixmap(type == Surface::Banner ? ":/art/pattern-lines.png" : ":/art/pattern-dots.png")
                  .scaled(QSize(64, 64), Qt::KeepAspectRatio, Qt::SmoothTransformation)),
      ink(WorkbenchStyle::artwork(type == Surface::Banner ? "splatter-purple" : "ink-splash", QSize(176, 176)))
{
}

void ArtPanel::paintEvent(QPaintEvent*)
{
    const bool banner = surface == Surface::Banner;
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath shape;
    shape.addRoundedRect(QRectF(rect()).adjusted(.5, .5, -.5, -.5), 12, 12);
    painter.setClipPath(shape);
    painter.fillRect(rect(), palette().base().color());
    if (!WorkbenchStyle::highContrast()) {
        painter.setOpacity(banner ? .055 : .08);
        if (!banner || width() >= 410)
            painter.drawPixmap(width() - ink.width() + 28, -68, ink);
        if (!texture.isNull()) {
            painter.setOpacity(banner ? .04 : .025);
            const int offset = banner ? qMax(200, width() - 82) : 0;
            painter.drawTiledPixmap(QRect(offset, 0, width() - offset, height()), texture);
        }
    }
    painter.setOpacity(1);
    if (!banner) {
        painter.setPen(palette().dark().color());
        painter.drawPath(shape);
    }
}

UiMotion& UiMotion::instance()
{
    static auto* motion = new UiMotion(qApp);
    return *motion;
}

UiMotion::UiMotion(QObject* parent)
    : QObject(parent), reduced(QSettings().value("ui/reduceMotion", false).toBool())
{
    connect(qApp->styleHints()->accessibility(), &QAccessibilityHints::motionPreferenceChanged,
            this, [this] { if (!enabled()) finishAll(); });
}

bool UiMotion::enabled() const
{
    return !drawing && !reduced &&
        qApp->styleHints()->accessibility()->motionPreference() != Qt::MotionPreference::ReducedMotion;
}

void UiMotion::animate(QWidget* target, qreal from, qreal to, int ms, std::function<void(qreal)> update)
{
    cancel(target, false);
    if (!enabled() || !target->isVisible()) {
        update(to);
        return;
    }
    auto* animation = new QVariantAnimation(target);
    animation->setDuration(ms);
    animation->setStartValue(from);
    animation->setEndValue(to);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    target->installEventFilter(this);
    animations.insert(target, animation);
    connect(animation, &QVariantAnimation::valueChanged, animation, [update](const QVariant& value) {
        update(value.toReal());
    });
    connect(animation, &QVariantAnimation::finished, this, [this, target] { cancel(target, false); });
    connect(target, &QObject::destroyed, animation, [this](QObject* object) { animations.remove(object); });
    update(from);
    animation->start();
}

void UiMotion::reveal(QWidget* target, int ms, int lift, qreal from)
{
    cancel(target);
    if (!enabled() || !target->isVisible()) return;
    class LiftOpacity : public QGraphicsOpacityEffect {
    public:
        explicit LiftOpacity(int distance, QWidget* owner) : QGraphicsOpacityEffect(owner), lift(distance) {}
    protected:
        void draw(QPainter* painter) override
        {
            painter->save();
            painter->translate(0, lift * (1 - opacity()));
            QGraphicsOpacityEffect::draw(painter);
            painter->restore();
        }
    private:
        int lift;
    };
    auto* effect = new LiftOpacity(lift, target);
    target->setGraphicsEffect(effect);
    animate(target, from, 1, ms, [effect](qreal value) {
        effect->setOpacity(value);
        if (value >= 1) effect->setEnabled(false);
    });
}

void UiMotion::cancel(QWidget* target, bool finish)
{
    auto* animation = animations.take(target);
    if (!animation) return;
    target->removeEventFilter(this);
    animation->stop();
    if (finish) animation->setCurrentTime(animation->duration());
    animation->deleteLater();
}

void UiMotion::finishAll()
{
    for (auto* object : animations.keys()) cancel(static_cast<QWidget*>(object));
}

void UiMotion::setReduced(bool value)
{
    reduced = value;
    if (!enabled()) finishAll();
}

void UiMotion::setDrawing(bool value)
{
    drawing = value;
    if (drawing) finishAll();
}

bool UiMotion::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::Hide) cancel(static_cast<QWidget*>(watched));
    return QObject::eventFilter(watched, event);
}
