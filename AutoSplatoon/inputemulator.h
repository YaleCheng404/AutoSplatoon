#pragma once
#include <QtGlobal>
#include <cstdint>

class InputEmulator {
public:
    // Switch HID direction values.
    static constexpr uint8_t A_DPAD_CENTER = 0x08;
    static constexpr uint8_t A_DPAD_U = 0x00;
    static constexpr uint8_t A_DPAD_U_R = 0x01;
    static constexpr uint8_t A_DPAD_R = 0x02;
    static constexpr uint8_t A_DPAD_D_R = 0x03;
    static constexpr uint8_t A_DPAD_D = 0x04;
    static constexpr uint8_t A_DPAD_D_L = 0x05;
    static constexpr uint8_t A_DPAD_L = 0x06;
    static constexpr uint8_t A_DPAD_U_L = 0x07;

    // Direction bits in an action.
    static constexpr uint8_t DIR_U = 0x01;
    static constexpr uint8_t DIR_R = 0x02;
    static constexpr uint8_t DIR_D = 0x04;
    static constexpr uint8_t DIR_L = 0x08;
    static constexpr uint8_t DIR_U_R = DIR_U + DIR_R;
    static constexpr uint8_t DIR_D_R = DIR_D + DIR_R;
    static constexpr uint8_t DIR_U_L = DIR_U + DIR_L;
    static constexpr uint8_t DIR_D_L = DIR_D + DIR_L;

    static constexpr quint64 BTN_Y = 0x0000000000000001;
    static constexpr quint64 BTN_B = 0x0000000000000002;
    static constexpr quint64 BTN_A = 0x0000000000000004;
    static constexpr quint64 BTN_X = 0x0000000000000008;
    static constexpr quint64 BTN_L = 0x0000000000000010;
    static constexpr quint64 BTN_R = 0x0000000000000020;
    static constexpr quint64 BTN_ZL = 0x0000000000000040;
    static constexpr quint64 BTN_ZR = 0x0000000000000080;
    static constexpr quint64 BTN_MINUS = 0x0000000000000100;
    static constexpr quint64 BTN_PLUS = 0x0000000000000200;
    static constexpr quint64 BTN_LCLICK = 0x0000000000000400;
    static constexpr quint64 BTN_HOME = 0x0000000000001000;
    static constexpr quint64 BTN_CAPTURE = 0x0000000000002000;

    static constexpr quint64 DPAD_U = 0x0000000000010000;
    static constexpr quint64 DPAD_R = 0x0000000000020000;
    static constexpr quint64 DPAD_D = 0x0000000000040000;
    static constexpr quint64 DPAD_L = 0x0000000000080000;

    static constexpr quint64 NO_INPUT = 0;
};
