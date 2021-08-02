#include <avr/io.h>
#include <util/delay.h>

#include "opl2.h"
#include "uart.h"

#define ERR_OK      0xe0
#define ERR_INVAL   0xe1
#define CMD_RESET   0xc1
#define CMD_WRITE   0xc2
#define CMD_SLEEP   0xc3

static inline void delay_dyn(uint16_t ms) {
    while (ms >= 1000) { _delay_ms(1000); ms -= 1000; }
    while (ms >=  100) { _delay_ms( 100); ms -=  100; }
    while (ms >=   10) { _delay_ms(  10); ms -=   10; }
    while (ms >=    1) { _delay_ms(   1); ms -=    1; }
}

int main() {
    uart_init();
    opl2_init();
    opl2_reset();

    /* command loop */
    for (;;) {
        switch (uart_recv()) {
            default: {
                uart_send(ERR_INVAL);
                break;
            }

            /* reset OPL2 */
            case CMD_RESET: {
                opl2_reset();
                uart_send(ERR_OK);
                break;
            }

            /* write OPL2 register */
            case CMD_WRITE: {
                uint8_t reg = uart_recv();
                uint8_t val = uart_recv();

                /* write the register value */
                opl2_write(reg, val);
                uart_send(ERR_OK);
                break;
            }

            /* sleep for some time */
            case CMD_SLEEP: {
                uint16_t vl = uart_recv();
                uint16_t vh = uart_recv();

                /* wait for some time */
                delay_dyn(vl | (vh << 8));
                uart_send(ERR_OK);
                break;
            }
        }
    }
}