#include "manualcontroldialog.h"
#include "inputemulator.h"
#include "workbenchstyle.h"
#include <QEvent>
#include <QGridLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QVBoxLayout>

namespace {
class ControllerButton : public QPushButton {
public:
    ControllerButton(const QString& text, const QString& art)
        : QPushButton(text), artName(art)
    {
        setIconSize(QSize(25, 25));
        updateIcon();
    }

protected:
    void changeEvent(QEvent* event) override
    {
        QPushButton::changeEvent(event);
        if (event->type() == QEvent::PaletteChange) updateIcon();
    }

private:
    void updateIcon()
    {
        if (artName.isEmpty()) return;
        QPixmap art = WorkbenchStyle::artwork(artName, iconSize(), devicePixelRatioF());
        if (art.isNull()) return;
        QPainter painter(&art);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(QRect(QPoint(), iconSize()), palette().buttonText().color());
        painter.end();
        setIcon(QIcon(art));
    }

    QString artName;
};
}

// Layout adapted from nullstalgia/ClubchatGames, af2bdace90e0f0641aa4e21e0a24ca998a676ad0.
ManualControlDialog::ManualControlDialog(QWidget* parent) : QDialog(parent)
{
    Q_INIT_RESOURCE(resources);
    setObjectName("manualControlDialog");
    setWindowTitle(tr("手动控制 / 配对"));
    auto* root = new QVBoxLayout(this);
    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    auto* panel = new QWidget;
    auto* layout = new QVBoxLayout(panel);
    layout->addWidget(WorkbenchStyle::dialogHeading(tr("手动控制"), tr("校准画笔 · 配对 Switch"), "pad-guide"));
    auto* hint = new QLabel(tr("在 Switch 的“更改握法 / 顺序”中按住 L + R 配对。\n鼠标按住生效，松开或切换窗口即释放。L3 为左摇杆按压。"));
    hint->setWordWrap(true);
    layout->addWidget(hint);
    auto* shoulders = new QGridLayout;
    auto* controls = new QGridLayout;
    auto* system = new QGridLayout;
    using I = InputEmulator;
    auto add = [&](QGridLayout* grid, int row, int column, const QString& text,
                   const QString& name, quint64 action, const QString& icon = QString()) {
        const bool iconOnly = name == "A" || name == "B" || name == "X" || name == "Y";
        auto* button = new ControllerButton(iconOnly ? QString() : text, icon);
        button->setObjectName("manual_" + name);
        button->setAccessibleName(name == "LClick" ? tr("左摇杆按压") : text);
        button->setToolTip(tr("%1 · 按住生效，松开释放").arg(button->accessibleName()));
        button->setAutoDefault(false);
        button->setAutoRepeat(false);
        button->setFocusPolicy(Qt::NoFocus);
        button->setMinimumSize(64, 56);
        if (name == "LR") button->setProperty("primary", true);
        button->setMinimumSize(button->minimumSize().expandedTo(button->sizeHint()));
        grid->addWidget(button, row, column);
        connect(button, &QPushButton::pressed, this, [this, button, action] {
            held = button;
            emit sendAction(action);
        });
        connect(button, &QPushButton::released, this, &ManualControlDialog::releaseInput);
    };
    add(shoulders, 0, 0, "ZL", "ZL", I::BTN_ZL);
    add(shoulders, 0, 1, "L", "L", I::BTN_L);
    add(shoulders, 0, 2, tr("L + R 配对"), "LR", I::BTN_L | I::BTN_R);
    add(shoulders, 0, 3, "R", "R", I::BTN_R);
    add(shoulders, 0, 4, "ZR", "ZR", I::BTN_ZR);
    add(controls, 0, 1, tr("上"), "Up", I::DPAD_U, "pad-up");
    add(controls, 1, 0, tr("左"), "Left", I::DPAD_L, "pad-left");
    add(controls, 1, 1, "L3", "LClick", I::BTN_LCLICK, "pad-l3");
    add(controls, 1, 2, tr("右"), "Right", I::DPAD_R, "pad-right");
    add(controls, 2, 1, tr("下"), "Down", I::DPAD_D, "pad-down");
    controls->setColumnMinimumWidth(3, 24);
    add(controls, 0, 5, "X", "X", I::BTN_X, "pad-x");
    add(controls, 1, 4, "Y", "Y", I::BTN_Y, "pad-y");
    add(controls, 1, 6, "A", "A", I::BTN_A, "pad-a");
    add(controls, 2, 5, "B", "B", I::BTN_B, "pad-b");
    add(system, 0, 0, "−", "Minus", I::BTN_MINUS);
    add(system, 0, 1, "+", "Plus", I::BTN_PLUS);
    add(system, 0, 2, tr("截图"), "Capture", I::BTN_CAPTURE);
    add(system, 0, 3, tr("主页"), "Home", I::BTN_HOME);
    layout->addLayout(shoulders);
    layout->addSpacing(12);
    layout->addLayout(controls);
    layout->addSpacing(12);
    layout->addLayout(system);
    layout->addStretch();
    scroll->setWidget(panel);
    root->addWidget(scroll);
    resize(QSize(600, 485).boundedTo(screen()->availableGeometry().size() - QSize(32, 48)));
}

void ManualControlDialog::releaseInput()
{
    if (!held) return;
    auto* button = held;
    held = nullptr;
    button->setDown(false);
    if (QWidget::mouseGrabber() == button) button->releaseMouse();
    emit sendAction(InputEmulator::NO_INPUT);
}

bool ManualControlDialog::event(QEvent* event)
{
    if (event->type() == QEvent::WindowDeactivate || event->type() == QEvent::Hide)
        releaseInput();
    return QDialog::event(event);
}

void ManualControlDialog::done(int result)
{
    releaseInput();
    emit sendAction(InputEmulator::NO_INPUT);
    QDialog::done(result);
}
