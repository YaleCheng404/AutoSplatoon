#include "serialconnection.h"
#include "inputemulator.h"

QByteArray controllerPacket(quint64 action)
{
    using I = InputEmulator;
    const quint8 direction = (action >> 16) & 0xff;
    quint8 dpad = I::A_DPAD_CENTER;
    switch (direction) {
    case I::DIR_U: dpad = I::A_DPAD_U; break;
    case I::DIR_R: dpad = I::A_DPAD_R; break;
    case I::DIR_D: dpad = I::A_DPAD_D; break;
    case I::DIR_L: dpad = I::A_DPAD_L; break;
    case I::DIR_U_R: dpad = I::A_DPAD_U_R; break;
    case I::DIR_D_R: dpad = I::A_DPAD_D_R; break;
    case I::DIR_D_L: dpad = I::A_DPAD_D_L; break;
    case I::DIR_U_L: dpad = I::A_DPAD_U_L; break;
    }
    QByteArray packet(9, char(0));
    packet[0] = char((action >> 8) & 0xff);
    packet[1] = char(action & 0xff);
    packet[2] = char(dpad);
    for (int i = 3; i <= 6; ++i) packet[i] = char(0x80);
    quint8 crc = 0;
    for (int i = 0; i < 8; ++i) {
        crc ^= quint8(packet[i]);
        for (int bit = 0; bit < 8; ++bit)
            crc = crc & 0x80 ? quint8((crc << 1) ^ 0x07) : quint8(crc << 1);
    }
    packet[8] = char(crc);
    return packet;
}
SerialController::SerialController(QObject* parent) : QObject(parent)
{
    clock.start();
    syncTimeout.setSingleShot(true);
    replyTimer.setSingleShot(true);
    hidTimer.setInterval(250);
    connect(&port, &QSerialPort::readyRead, this, &SerialController::receive);
    connect(&port, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError error) {
        if (error != QSerialPort::NoError && port.isOpen()) fail(port.errorString());
    });
    connect(&syncTimeout, &QTimer::timeout, this, [this] { fail(tr("连接超时，请检查固件和开发板。")); });
    connect(&hidTimer, &QTimer::timeout, this, [this] {
        checkHidActivity(clock.elapsed());
    });
    connect(&replyTimer, &QTimer::timeout, this, [this] {
        if (response.isEmpty() || !port.isOpen()) return;
        QByteArray bytes = response;
        response.clear();
        if (stage == 0 && bytes.contains(char(0xff))) {
            stage = 1; write(QByteArray(1, char(0xff)));
        } else if (stage == 1 && bytes.contains(char(0xff))) {
            stage = 2; write(QByteArray(1, char(0x44)));
        } else if (stage == 2 && bytes.contains(char(0xee))) {
            stage = 3; write(QByteArray(1, char(0xee)));
        } else if (stage == 3 && bytes.contains(char(0x03))) {
            ready = true; syncTimeout.stop(); hidTimer.start();
            emit connectionChanged(true);
            emit message(tr("串口已连接 · 请在 Switch 上配对，可使用手动控制。"));
            handleResponse(bytes); // HID reports can already follow the controller-type byte.
            sendAction(InputEmulator::NO_INPUT);
        }
    });
}
void SerialController::open(const QString& name)
{
    close();
    port.setPortName(name);
    port.setBaudRate(19200);
    if (!port.open(QIODevice::ReadWrite)) { fail(port.errorString()); return; }
    emit message(tr("正在连接 ESP32…"));
    syncTimeout.start(5000);
    write(QByteArray(10, char(0xff)));
}
void SerialController::close()
{
    const bool wasReady = ready;
    ready = false;
    if (wasReady && port.isOpen()) {
        port.write(controllerPacket(InputEmulator::NO_INPUT));
        port.flush();
    }
    syncTimeout.stop(); replyTimer.stop(); hidTimer.stop();
    port.close(); response.clear(); stage = 0; lastHidReport = -1;
    if (hidActive) { hidActive = false; emit hidActivityChanged(false); }
    emit connectionChanged(false);
}
bool SerialController::write(const QByteArray& bytes)
{
    if (!port.isOpen()) return false;
    const qint64 written = port.write(bytes);
    if (written != bytes.size()) { fail(tr("串口写入失败：") + port.errorString()); return false; }
    if (port.bytesToWrite() > 256) { fail(tr("串口积压，绘图已停止。")); return false; }
    return true;
}
void SerialController::sendAction(quint64 action)
{
    if (ready) write(controllerPacket(action));
}
void SerialController::receive()
{
    handleResponse(port.readAll());
}
void SerialController::handleResponse(const QByteArray& bytes)
{
    if (ready) {
        for (char byte : bytes) {
            // UARTSwitchCon 1.2 emits 0x90 periodically from send_buttons(),
            // only while Bluetooth HID is connected; it is NOT a per-input ACK.
            if (quint8(byte) == 0x90) {
                lastHidReport = clock.elapsed();
                if (!hidActive) { hidActive = true; emit hidActivityChanged(true); }
            }
            else if (quint8(byte) == 0x92) { fail(tr("设备拒绝控制包，绘图已停止。")); return; }
        }
    } else {
        response.append(bytes);
        if (response.size() > 1024) response = response.right(1024);
        // Already-paired firmware streams 0x90 every ~15 ms. Restarting this
        // timer on every read would prevent the final handshake from completing.
        if (!replyTimer.isActive()) replyTimer.start(75);
    }
}
void SerialController::checkHidActivity(qint64 now)
{
    // Before the first HID report, silence is normal: manual L+R is needed to pair.
    if (ready && hidActive && now - lastHidReport > 5000) {
        hidActive = false;
        emit hidActivityChanged(false);
        emit message(tr("Switch 手柄通信中断，绘图已停止；串口保留，请重新配对。"));
    }
}
void SerialController::fail(const QString& reason)
{
    close();
    emit failed(reason);
}
