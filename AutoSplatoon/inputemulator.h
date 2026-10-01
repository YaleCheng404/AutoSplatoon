#pragma once
#include <QtGlobal>
#include <cstdint>

class InputEmulator {
public:
    //ActualSwitchDPADValues;
    static constexpr uint8_t A_DPAD_CENTER = 0x08;
    static constexpr uint8_t A_DPAD_U = 0x00;
    static constexpr uint8_t A_DPAD_U_R = 0x01;
    static constexpr uint8_t A_DPAD_R = 0x02;
    static constexpr uint8_t A_DPAD_D_R = 0x03;
    static constexpr uint8_t A_DPAD_D = 0x04;
    static constexpr uint8_t A_DPAD_D_L = 0x05;
    static constexpr uint8_t A_DPAD_L = 0x06;
    static constexpr uint8_t A_DPAD_U_L = 0x07;

    //EnumDIRValues;
    static constexpr uint8_t DIR_CENTER = 0x00;
    static constexpr uint8_t DIR_U = 0x01;
    static constexpr uint8_t DIR_R = 0x02;
    static constexpr uint8_t DIR_D = 0x04;
    static constexpr uint8_t DIR_L = 0x08;
    static constexpr uint8_t DIR_U_R = DIR_U + DIR_R;
    static constexpr uint8_t DIR_D_R = DIR_D + DIR_R;
    static constexpr uint8_t DIR_U_L = DIR_U + DIR_L;
    static constexpr uint8_t DIR_D_L = DIR_D + DIR_L;

    static constexpr quint64 BTN_NONE = 0x0000000000000000;
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
    static constexpr quint64 BTN_RCLICK = 0x0000000000000800;
    static constexpr quint64 BTN_HOME = 0x0000000000001000;
    static constexpr quint64 BTN_CAPTURE = 0x0000000000002000;
    static constexpr quint64 BTN_SL = 0x0000000000004000;
    static constexpr quint64 BTN_SR = 0x0000000000008000;

    static constexpr quint64 DPAD_CENTER = 0x0000000000000000;
    static constexpr quint64 DPAD_U = 0x0000000000010000;
    static constexpr quint64 DPAD_R = 0x0000000000020000;
    static constexpr quint64 DPAD_D = 0x0000000000040000;
    static constexpr quint64 DPAD_L = 0x0000000000080000;
    static constexpr quint64 DPAD_U_R = DPAD_U + DPAD_R;
    static constexpr quint64 DPAD_D_R = DPAD_D + DPAD_R;
    static constexpr quint64 DPAD_U_L = DPAD_U + DPAD_L;
    static constexpr quint64 DPAD_D_L = DPAD_D + DPAD_L;

    static constexpr quint64 LSTICK_CENTER = 0x0000000000000000;
    static constexpr quint64 LSTICK_R = 0x00000000FF000000; //0(000);
    static constexpr quint64 LSTICK_U_R = 0x0000002DFF000000; //45(02D);
    static constexpr quint64 LSTICK_U = 0x0000005AFF000000; //90(05A);
    static constexpr quint64 LSTICK_U_L = 0x00000087FF000000; //135(087);
    static constexpr quint64 LSTICK_L = 0x000000B4FF000000; //180(0B4);
    static constexpr quint64 LSTICK_D_L = 0x000000E1FF000000; //225(0E1);
    static constexpr quint64 LSTICK_D = 0x0000010EFF000000; //270(10E);
    static constexpr quint64 LSTICK_D_R = 0x0000013BFF000000; //315(13B);

    static constexpr quint64 RSTICK_CENTER = 0x0000000000000000;
    static constexpr quint64 RSTICK_R = 0x000FF00000000000; //0(000);
    static constexpr quint64 RSTICK_U_R = 0x02DFF00000000000; //45(02D);
    static constexpr quint64 RSTICK_U = 0x05AFF00000000000; //90(05A);
    static constexpr quint64 RSTICK_U_L = 0x087FF00000000000; //135(087);
    static constexpr quint64 RSTICK_L = 0x0B4FF00000000000; //180(0B4);
    static constexpr quint64 RSTICK_D_L = 0x0E1FF00000000000; //225(0E1);
    static constexpr quint64 RSTICK_D = 0x10EFF00000000000; //270(10E);
    static constexpr quint64 RSTICK_D_R = 0x13BFF00000000000; //315(13B);

    static constexpr quint64 NO_INPUT = BTN_NONE + DPAD_CENTER + LSTICK_CENTER + RSTICK_CENTER;

};
