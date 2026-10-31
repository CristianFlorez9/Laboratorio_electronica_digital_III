#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

#define UART_ID uart0
#define UART_TX_PIN 0
#define UART_RX_PIN 1 
#define BAUD_RATE 115200

#define TX_BUFFER_SIZE 256 
#define TX_BUFFER_MASK (TX_BUFFER_SIZE - 1)



void uart_driver_init(void);
//Inicia el UART

bool uart_driver_send_string(const char* str);
//Función de transmisión. obliga a quien la llame a verificar 
//si el texto se guardó con éxito en el buffer
// o si fue rechazado por falta de espacio.


uint32_t uart_driver_get_dropped_count(void);
//Dice cuantos paquetes se perdieron por desbordamiento de memoria
//Si el microcontrolador genera datos más rápido
// de lo que los cables pueden transmitirlos físicamente


#endif