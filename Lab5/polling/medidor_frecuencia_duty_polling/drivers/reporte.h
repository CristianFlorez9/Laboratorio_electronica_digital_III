#ifndef REPORTE_H
#define REPORTE_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/uart.h"

#define UART_ID uart0
#define BAUD_RATE 115200

// Configura el UART y los pines
void reporte_init(void);

// Se debe llamar constantemente en el while(1) del main (POLLING)
void reporte_actualizar(void);

#endif