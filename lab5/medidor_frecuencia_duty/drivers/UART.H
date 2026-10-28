#ifndef UART_DRIVER_H
#define UART_DRIVER_H



#define UART_ID uart0
#define UART_TX_PIN 0
#define UART_RX_PIN 1 
#define BAUD_RATE 115200

#define TX_BUFFER_SIZE 256 
#define TX_BUFFER_MASK (TX_BUFFER_SIZE - 1)



void uart_driver_init(void);


bool uart_driver_send_string(const char* str);


uint32_t uart_driver_get_dropped_count(void);

#endif