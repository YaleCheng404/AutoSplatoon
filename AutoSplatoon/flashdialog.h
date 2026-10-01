#pragma once
#include <QDialog>
#include <QProcess>
#include <QTemporaryDir>
#include <QTimer>
class QPlainTextEdit;
class QPushButton;
class QProgressBar;

QString bundledEsptool();
QByteArray verifiedFirmware();

class FlashDialog : public QDialog {
    Q_OBJECT
public:
    explicit FlashDialog(const QString& port, QWidget* parent = nullptr, const QString& tool = bundledEsptool());
    ~FlashDialog() override;
signals:
    void flashed();
protected:
    void reject() override;
private:
    void begin();
    void appendOutput();
    void finish(bool success, const QString& message);
    QProcess process;
    QTemporaryDir temporary;
    QTimer timeout;
    QString port, firmwarePath, tool;
    bool probing = false;
    bool active = false;
    QPlainTextEdit* log;
    QPushButton* start;
    QPushButton* cancel;
    QProgressBar* progress;
};
