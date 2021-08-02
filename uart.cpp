#include "uart.h"

void uart_init() {
    UART_DDR |= UART_TX;
    UART_DDR &= ~UART_RX;
    UBRR0     = UART_UBRR;
    UCSR0B    = (1 << RXEN0) | (1 << TXEN0);
    UCSR0C    = (1 << UCSZ01) | (1 << UCSZ00);
}

void uart_send(uint8_t data) {
    loop_until_bit_is_set(UCSR0A, UDRE0);
    UDR0 = data;
}

uint8_t uart_recv() {
    loop_until_bit_is_set(UCSR0A, RXC0);
    return UDR0;
}