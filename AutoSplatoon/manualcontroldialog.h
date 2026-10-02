#pragma once
#include <QDialog>

class QPushButton;

class ManualControlDialog : public QDialog {
    Q_OBJECT
public:
    explicit ManualControlDialog(QWidget* parent = nullptr);
    void done(int result) override;
signals:
    void sendAction(quint64 action);
protected:
    bool event(QEvent* event) override;
private:
    void releaseInput();
    QPushButton* held = nullptr;
};
