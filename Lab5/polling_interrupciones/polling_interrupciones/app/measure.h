/**
 * @file measure.h
 * @brief Cálculo de frecuencia y duty y armado del reporte por UART.
 *
 * @details
 * Procesamiento diferido del flujo Polling+IRQs: corre en el ciclo principal con los
 * flancos y ticks que dejan las ISR de capture.c. Entre dos reportes promedia todos los
 * periodos completos: f = N / suma(T) y D = 100 * suma(tH) / suma(T).
 *
 * Reporte, una línea ASCII terminada en LF: `n,dt,f,D,estado`, por ejemplo
 * `17,100,1000.000,25.0,OK`.
 *  - n: número de reporte (desde 0, avanza con cada tick aunque no se envíe).
 *  - dt: ms desde el reporte anterior (0 en el primero).
 *  - f en Hz con tres decimales y D en % con uno, o NA.
 *  - estado: OK, ESPERANDO (sin periodo completo aún) o SIN_TRANSICIONES (> 500 ms).
 */

#ifndef MEASURE_H
#define MEASURE_H

#include <stdint.h>

/** @brief Inicializa la medición. Llamar antes de capture_init(). */
void measure_init(void);

/** @brief Un paso del ciclo principal: procesa flancos y, si toca, envía un reporte. No bloquea. */
void measure_poll(void);

/** @brief Reportes programados que no se generaron por llegar tarde el ciclo [reportes]. */
uint32_t measure_get_late_count(void);

#endif