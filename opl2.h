#ifndef __OPL2_H__
#define __OPL2_H__

#include <stdio.h>
#include <avr/io.h>

#define A0      (1 << PD4)
#define WR      (1 << PB2)
#define RST     (1 << PD3)
#define D0_1    0x03
#define D2_7    0x3f

#define D_A0    DDRD
#define D_WR    DDRB
#define D_RST   DDRD
#define D_D0_1  DDRB
#define D_D2_7  DDRC

#define P_A0    PORTD
#define P_WR    PORTB
#define P_RST   PORTD
#define P_D0_1  PORTB
#define P_D2_7  PORTC

enum OPL2 {
    NumChannels   = 9,
    NumOperators  = 2,
    NumOctaves    = 7,
    NumNotes      = 12,
    NumDrumSounds = 5,
};

enum class Channel : uint8_t {
    CH0 = 0,
    CH1 = 1,
    CH2 = 2,
    CH3 = 3,
    CH4 = 4,
    CH5 = 5,
    CH6 = 6,
    CH7 = 7,
    CH8 = 8,
};

enum class Operator : uint8_t {
    Op0       = 0,
    Op1       = 1,
    Modulator = Op0,
    Carrier   = Op1,
};

enum class SynthMode : uint8_t {
    FM = 0,
    AM = 1,
};

enum class DrumSound : uint8_t {
    Bass   = 0,
    Snare  = 1,
    Tom    = 2,
    Cymbol = 3,
    HiHat  = 4,
};

enum class DrumSoundMask : uint8_t {
    Bass   = 0x10,
    Snare  = 0x08,
    Tom    = 0x04,
    Cymbol = 0x02,
    HiHat  = 0x01,
};

enum class Note : uint8_t {
    C  = 0,
    CS = 1,
    D  = 2,
    DS = 3,
    E  = 4,
    F  = 5,
    FS = 6,
    G  = 7,
    GS = 8,
    A  = 9,
    AS = 10,
    B  = 11,
};

enum class ChipRegister : uint8_t {
    NOR = 0x00,     // NOP Register
    TST = 0x01,     // Test Register
    TM1 = 0x02,     // Timer-1
    TM2 = 0x03,     // Timer-2
    IRQ = 0x04,     // IRQ Reset & Control of Timer-1,2
    CSM = 0x08,     // CSM Speech Synthesis & Note Select
    RTM = 0xbd,     // Depth (AM / VIB) & Rhythm (Bass, Snare, Tom, Tom & Cymbol, HiHat)
};

enum class ChannelRegister : uint8_t {
    FNR = 0xa0,     // F-Number (L)
    KBF = 0xb0,     // KON & Block & F-Number (H)
    FBC = 0xc0,     // Feedback & Connection
};

enum class OperatorRegister : uint8_t {
    AVR = 0x20,     // AM & VIB & EG-Type & KSR & Multiple
    KSL = 0x40,     // KSL & Total Level
    ADR = 0x60,     // Attack Rate & Decay Rate
    SRR = 0x80,     // Sustain Rate & Release Rate
    WSR = 0xe0,     // Wave Select
};

void opl2_init();
void opl2_reset();
void opl2_write(uint8_t reg, uint8_t val);
void opl2_write(ChipRegister reg, uint8_t val);
void opl2_write(ChannelRegister reg, Channel ch, uint8_t val);
void opl2_write(OperatorRegister reg, Channel ch, Operator op, uint8_t val);

#endif