#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "opl2.h"

static void uart_init() {
    DDRD  &= ~(1 << PD0);
    DDRD  |=  (1 << PD1);
    UBRR0  = 10;
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

static char uart_recv() {
    loop_until_bit_is_set(UCSR0A, RXC0);
    return UDR0;
}

static void uart_send(char data) {
    loop_until_bit_is_set(UCSR0A, UDRE0);
    UDR0 = data;
}

static int stdio_read(FILE *) {
    return uart_recv();
}

static int stdio_write(char data, FILE *) {
    uart_send(data);
    return 0;
}

static void stdio_init() {
    stdin  = fdevopen(NULL, stdio_read);
    stdout = fdevopen(stdio_write, NULL);
    stderr = fdevopen(stdio_write, NULL);
}

int main() {
    uart_init();
    stdio_init();

    /* initialize OPL2 */
    opl2_init();
    printf("* OPL2 Initialized.\r\n");

    /* reset OPL2 */
    opl2_reset();
    printf("* OPL2 Resetted.\r\n");

    DDRD |= 1 << PD7;
    for (;;) {
        getchar();
        PORTD ^= 1 << PD7;
    }
}