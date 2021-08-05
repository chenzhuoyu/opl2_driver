#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "opl2.h"
#include "uart.h"

#define ERR_OK      0xe0
#define ERR_INVAL   0xe1
#define ERR_OVERFL  0xe2

#define CMD_RESET   0xc0
#define CMD_WRITE   0xc1
#define CMD_QUERY   0xc2

#define OPL_SLEEP   0xd0
#define OPL_WRITE   0xd1
#define OPL_CLOSE   0xd2

#define T_INIT      6       // 1ms, at F_CPU=16MHz
#define Q_SIZE      1024

#define decr_nz(v)  if (v) { v--; }

static uint16_t _len;
static uint16_t _ridx;
static uint16_t _widx;
static uint8_t  _queue[Q_SIZE];

/* increases every millisecond */
static volatile uint16_t _tick  = 0;
static volatile uint16_t _sleep = 0;

ISR(TIMER0_OVF_vect) {
    _tick++;
    TCNT0 = T_INIT;
    decr_nz(_sleep);
}

static void q_clear() {
    _len  = 0;
    _ridx = 0;
    _widx = 0;
}

static void q_append(uint8_t v) {
    _len++;
    _queue[_widx++] = v;
    _widx %= Q_SIZE;
}

static uint8_t q_remove() {
    uint16_t i = _ridx++;
    uint8_t  v = _queue[i];

    /* adjust the reader index after read */
    _len--;
    _ridx %= Q_SIZE;
    return v;
}

static uint8_t q_receive(uint16_t nb) {
    for (;;) {
        uint8_t  cmd;
        uint16_t len;

        /* check for receiving end */
        if (nb == 0) {
            return ERR_OK;
        }

        /* find the command length */
        switch ((cmd = uart_read_byte())) {
            case OPL_SLEEP : len = 3; break;
            case OPL_CLOSE : len = 1; break;
            case OPL_WRITE : len = 3; break;
            default        : return ERR_INVAL;
        }

        /* must have n - 1 bytes remaining */
        if (nb < len - 1) {
            return ERR_INVAL;
        }

        /* write the command */
        nb -= len;
        q_append(cmd);

        /* receive the bytes */
        while (--len) {
            q_append(uart_read_byte());
        }
    }
}

static void tick_init() {
    TCNT0   = T_INIT;
    TCCR0B |= (1 << CS01) | (1 << CS00);
    TIMSK0 |= (1 << TOIE0);
}

static void opl2_close() {
    q_clear();
    opl2_reset();
    uart_write_byte(ERR_OK);
}

static void poll_events() {
    if (uart_has_data()) {
        uint8_t  cmd = uart_read_byte();
        uint16_t len = uart_read_byte();

        /* check for buffer space */
        if (len > Q_SIZE - _len) {
            uart_write_byte(ERR_OVERFL);
            return;
        }

        /* command dispatch */
        switch (cmd) {
            default: {
                uart_write_byte(ERR_INVAL);
                break;
            }

            /* CMD_RESET clears the buffer and resets OPL2 immediately */
            case CMD_RESET: {
                opl2_close();
                break;
            }

            /* CMD_WRITE writes data to the ring buffer */
            case CMD_WRITE: {
                uart_write_byte(q_receive(len));
                break;
            }

            /* CMD_QUERY queries the current buffer size and level */
            case CMD_QUERY: {
                uart_write_byte(ERR_OK);
                uart_write_word(Q_SIZE);
                uart_write_word((uint16_t)_len);
                break;
            }
        }
    }
}

static void handle_sleep() {
    uint8_t lsb = q_remove();
    _sleep = lsb | (q_remove() << 8);
}

static void handle_write() {
    uint8_t reg = q_remove();
    opl2_write(reg, q_remove());
}

static void process_status() {
    if (_len == 0) {
        PORTD |= (1 << PD7);
    } else {
        PORTD &= ~(1 << PD7);
    }
}

static void process_commands() {
    if (_len != 0 && _sleep == 0) {
        switch (q_remove()) {
            case OPL_SLEEP: handle_sleep(); break;
            case OPL_WRITE: handle_write(); break;
            case OPL_CLOSE: opl2_close();   break;
        }
    }
}

int main() {
    q_clear();
    tick_init();
    uart_init();
    opl2_init();
    opl2_reset();
    sei();

    /* buffer under-run indicator */
    DDRD  |= (1 << PD7);
    PORTD |= (1 << PD7);

    /* main event loop */
    for (;;) {
        poll_events();
        process_status();
        process_commands();
    }
}