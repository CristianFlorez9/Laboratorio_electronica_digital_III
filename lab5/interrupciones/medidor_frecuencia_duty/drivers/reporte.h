#ifndef REPORTE_H
#define REPORTE_H

#include <stdbool.h>

// Inicializa el temporizador de hardware que disparará los reportes cada 100 ms
/** @brief Arranca el temporizador periódico que envía el reporte (n, dt_ms, f_hz, duty, estado) por UART. */
void reporte_init(void);

#endif
