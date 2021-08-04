#ifndef __UART_H__
#define __UART_H__

#include <avr/io.h>
#include <stdint.h>

#define UART_RX     (1 << PD0)
#define UART_TX     (1 << PD1)
#define UART_DDR    DDRD
#define UART_UBRR   0

void    uart_init();
char    uart_has_data();
uint8_t uart_read_byte();

void uart_write_byte(uint8_t data);
void uart_write_word(uint16_t data);

#endif