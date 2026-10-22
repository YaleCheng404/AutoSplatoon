#pragma once
#include <QObject>
#include <QSerialPort>
#include <QTimer>
#include <QElapsedTimer>

QByteArray controllerPacket(quint64 action);

// UARTSwitchCon Chocolate handshake / 9-byte CRC8 packets.
// Transport fixes adapted from zhougz520/AutoSplatoon.
class SerialController : public QObject {
    Q_OBJECT
public:
    explicit SerialController(QObject* parent = nullptr);
    ~SerialController() override;
    bool isReady() const { return ready; }
    bool isConnecting() const { return connecting; }
    bool hasHidReports() const { return hidActive; }
    void open(const QString& name);
    void close();
    void sendAction(quint64 action);
signals:
    void connectingChanged(bool connecting);
    void connectionChanged(bool connected);
    void hidActivityChanged(bool active);
    void message(const QString& text);
    void failed(const QString& text);
private:
    friend class CoreTests;
    void receive();
    void handleResponse(const QByteArray& bytes);
    void checkHidActivity(qint64 now);
    void fail(const QString& reason);
    void write(const QByteArray& bytes);
    void setConnecting(bool value);
    bool setResetLine(bool asserted);
    void beginHandshake();
    enum class StartupPhase { Idle, ResetHeld, BootWait };
    QSerialPort port;
    QTimer syncTimeout, replyTimer, hidTimer, startupTimer;
    QElapsedTimer clock;
    qint64 lastHidReport = -1;
    QByteArray response;
    int stage = 0;
    bool ready = false;
    bool hidActive = false;
    bool connecting = false, changingControlLines = false, closing = false;
    bool resetUnavailable = false;
    StartupPhase startupPhase = StartupPhase::Idle;
};
