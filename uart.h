#ifndef __UART_H__
#define __UART_H__

#include <avr/io.h>
#include <stdint.h>

#define UART_RX     (1 << PD0)
#define UART_TX     (1 << PD1)
#define UART_DDR    DDRD
#define UART_BAUD   115200
#define UART_UBRR   (uint16_t)((F_CPU / (16.0 * UART_BAUD)) - 0.5)

void    uart_init();
void    uart_send(uint8_t data);
uint8_t uart_recv();

#endif