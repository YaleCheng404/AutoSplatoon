#include "manualcontroldialog.h"
#include "inputemulator.h"
#include <QEvent>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QVBoxLayout>

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
    auto* hint = new QLabel(tr("在 Switch 的“更改握法 / 顺序”中按住 L + R 配对。\n鼠标按住生效，松开或切换窗口即释放。L3 为左摇杆按压。"));
    hint->setWordWrap(true);
    layout->addWidget(hint);
    auto* shoulders = new QGridLayout;
    auto* controls = new QGridLayout;
    auto* system = new QGridLayout;
    using I = InputEmulator;
    auto add = [&](QGridLayout* grid, int row, int column, const QString& text,
                   const QString& name, quint64 action, const QString& icon = QString()) {
        auto* button = new QPushButton(text);
        button->setObjectName("manual_" + name);
        button->setAccessibleName(name == "LClick" ? tr("左摇杆按压") : text);
        button->setToolTip(button->accessibleName() + tr(" · 按住生效，松开释放"));
        button->setAutoDefault(false);
        button->setAutoRepeat(false);
        button->setFocusPolicy(Qt::NoFocus);
        button->setMinimumSize(64, 56);
        if (!icon.isEmpty()) {
            button->setIcon(QIcon(":/icons/" + icon + ".png"));
            button->setIconSize(QSize(24, 24));
        }
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
    add(controls, 0, 1, tr("上"), "Up", I::DPAD_U, "Switch-Control-Up");
    add(controls, 1, 0, tr("左"), "Left", I::DPAD_L, "Switch-Control-Left");
    add(controls, 1, 1, "L3", "LClick", I::BTN_LCLICK, "Switch-Control-Press");
    add(controls, 1, 2, tr("右"), "Right", I::DPAD_R, "Switch-Control-Right");
    add(controls, 2, 1, tr("下"), "Down", I::DPAD_D, "Switch-Control-Down");
    controls->setColumnMinimumWidth(3, 24);
    add(controls, 0, 5, "X", "X", I::BTN_X, "Switch-Button-Top");
    add(controls, 1, 4, "Y", "Y", I::BTN_Y, "Switch-Button-Left");
    add(controls, 1, 6, "A", "A", I::BTN_A, "Switch-Button-Right");
    add(controls, 2, 5, "B", "B", I::BTN_B, "Switch-Button-Bottom");
    add(system, 0, 0, "−", "Minus", I::BTN_MINUS);
    add(system, 0, 1, "+", "Plus", I::BTN_PLUS);
    add(system, 0, 2, tr("截图"), "Capture", I::BTN_CAPTURE);
    add(system, 0, 3, "Home", "Home", I::BTN_HOME);
    layout->addLayout(shoulders);
    layout->addSpacing(12);
    layout->addLayout(controls);
    layout->addSpacing(12);
    layout->addLayout(system);
    layout->addStretch();
    scroll->setWidget(panel);
    root->addWidget(scroll);
    resize(QSize(600, 440).boundedTo(screen()->availableGeometry().size() - QSize(32, 48)));
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
