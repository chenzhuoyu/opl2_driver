#include <avr/pgmspace.h>
#include <util/delay.h>

#include "opl2.h"

#define SET_A0()    P_A0  |= A0
#define SET_WR()    P_WR  |= WR
#define SET_RST()   P_RST |= RST

#define CLR_A0()    P_A0  &= ~A0
#define CLR_WR()    P_WR  &= ~WR
#define CLR_RST()   P_RST &= ~RST

static PROGMEM const uint8_t OpRegTab[NumChannels * NumOperators] = {
    0x00, 0x01, 0x02, 0x08, 0x09, 0x0a, 0x10, 0x11, 0x12,   // initializers for operator 1
    0x03, 0x04, 0x05, 0x0b, 0x0c, 0x0d, 0x13, 0x14, 0x15    // initializers for operator 2
};

static uint8_t  IR[7];
static uint8_t  CR[NumChannels][3];
static uint16_t OR[NumChannels][NumOperators][5];

static inline int idx_ir(ChipRegister r) {
    switch (r) {
        case ChipRegister::NOR : return 0;
        case ChipRegister::TST : return 1;
        case ChipRegister::TM1 : return 2;
        case ChipRegister::TM2 : return 3;
        case ChipRegister::IRQ : return 4;
        case ChipRegister::CSM : return 5;
        case ChipRegister::RTM : return 6;
        default                : __builtin_unreachable();
    }
}

static inline int idx_cr(ChannelRegister r) {
    switch (r) {
        case ChannelRegister::FNR : return 0;
        case ChannelRegister::KBF : return 1;
        case ChannelRegister::FBC : return 2;
        default                   : __builtin_unreachable();
    }
}

static inline int idx_or(OperatorRegister r) {
    switch (r) {
        case OperatorRegister::AVR : return 0;
        case OperatorRegister::KSL : return 1;
        case OperatorRegister::ADR : return 2;
        case OperatorRegister::SRR : return 3;
        case OperatorRegister::WSR : return 4;
        default                    : __builtin_unreachable();
    }
}

static void SET_U8(uint8_t v) {
    P_D2_7 = (v >> 2);
    P_D0_1 = (P_D0_1 & ~D0_1) | (v & D0_1);
}

void opl2_init() {
    D_A0   |= A0;
    D_WR   |= WR;
    D_RST  |= RST;
    D_D0_1 |= D0_1;
    D_D2_7 |= D2_7;
    CLR_A0();
    SET_WR();
    SET_RST();
}

void opl2_reset() {
    CLR_RST();
    _delay_ms(1);
    SET_RST();

    /* initialize chip registers */
    opl2_write(ChipRegister::NOR, 0x00);
    opl2_write(ChipRegister::CSM, 0x40);
    opl2_write(ChipRegister::RTM, 0x00);

    /* initialize channel & operator registers */
    for (auto i = Channel::CH0; i <= Channel::CH8; i = (Channel)((uint8_t)i + 1)) {
        opl2_write(ChannelRegister::FNR, i, 0x00);
        opl2_write(ChannelRegister::KBF, i, 0x00);
        opl2_write(ChannelRegister::FBC, i, 0x00);

        /* initialize operator registers */
        for (auto j = Operator::Op0; j <= Operator::Op1; j = (Operator)((uint8_t)j + 1)) {
            opl2_write(OperatorRegister::AVR, i, j, 0x00);
            opl2_write(OperatorRegister::KSL, i, j, 0x3f);
            opl2_write(OperatorRegister::ADR, i, j, 0x00);
            opl2_write(OperatorRegister::SRR, i, j, 0x00);
            opl2_write(OperatorRegister::WSR, i, j, 0x00);
        }
    }
}

void opl2_write(uint8_t reg, uint8_t val) {
    CLR_A0();
    SET_U8(reg);
    CLR_WR();
    _delay_us(10);
    SET_WR();
    _delay_us(10);
    SET_A0();
    SET_U8(val);
    CLR_WR();
    _delay_us(10);
    SET_WR();
    _delay_us(10);
}

void opl2_write(ChipRegister reg, uint8_t val) {
    IR[idx_ir(reg)] = val;
    opl2_write((uint8_t)reg, val);
}

void opl2_write(ChannelRegister reg, Channel ch, uint8_t val) {
    CR[(int)ch][idx_cr(reg)] = val;
    opl2_write((uint8_t)reg + (uint8_t)ch, val);
}

void opl2_write(OperatorRegister reg, Channel ch, Operator op, uint8_t val) {
    OR[(int)ch][(int)op][idx_or(reg)] = val;
    opl2_write((uint8_t)reg + pgm_read_byte(&OpRegTab[(int)op * NumChannels + (int)ch]), val);
}