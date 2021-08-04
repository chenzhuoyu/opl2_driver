#include "uart.h"

void uart_init() {
    UART_DDR |= UART_TX;
    UART_DDR &= ~UART_RX;
    UBRR0     = UART_UBRR;
    UCSR0B    = (1 << RXEN0) | (1 << TXEN0);
    UCSR0C    = (1 << UCSZ01) | (1 << UCSZ00);
}

char uart_has_data() {
    return (UCSR0A & (1 << RXC0)) != 0;
}

uint8_t uart_read_byte() {
    while (!uart_has_data());
    return UDR0;
}

void uart_write_byte(uint8_t data) {
    loop_until_bit_is_set(UCSR0A, UDRE0);
    UDR0 = data;
}

void uart_write_word(uint16_t data) {
    uart_write_byte((uint8_t)data);
    uart_write_byte((uint8_t)(data >> 8));
}