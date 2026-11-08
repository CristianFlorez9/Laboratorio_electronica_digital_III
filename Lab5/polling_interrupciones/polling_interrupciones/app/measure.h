/**
 * @file measure.h
 * @brief Cálculo de frecuencia y ciclo de trabajo y armado del reporte por UART.
 *
 * @details
 * Es la parte de "procesamiento diferido" del flujo Polling+IRQs: se ejecuta solo en el
 * ciclo principal, consumiendo los flancos y los ticks que dejan las ISR de capture.c.
 *
 * **Magnitudes.** Con T el periodo y tH el tiempo en alto:
 *  - f = 1 / T, en Hz.
 *  - D = 100 * tH / T, en %.
 *
 * Se promedian todos los periodos completos que llegan entre dos reportes:
 * f = N / suma(T) y D = 100 * suma(tH) / suma(T), con N el número de periodos.
 *
 * **Reporte.** Una línea ASCII por tick, terminada en LF, con el formato
 * `n,dt,f,D,estado`:
 *  - n: número de reporte (entero sin signo de 32 bits, empieza en 0 y avanza con cada
 *    tick programado, aunque la línea no se envíe).
 *  - dt: milisegundos desde el reporte anterior; 0 en el primero.
 *  - f: frecuencia en Hz con tres decimales, o NA.
 *  - D: ciclo de trabajo en % con un decimal, o NA.
 *  - estado: OK, ESPERANDO (aún no hay un periodo completo) o SIN_TRANSICIONES (pasaron
 *    más de 500 ms sin flancos).
 *
 * Ejemplo: `17,100,1000.000,25.0,OK`.
 *
 * **Manejo de errores.**
 *  - Si el anillo de flancos se desbordó, se descarta la ventana en curso.
 *  - Si el ciclo principal llega tarde y se saltan ticks, n avanza igual y los reportes
 *    omitidos se cuentan (measure_get_late_count()).
 *  - Si el driver UART no tiene espacio, descarta la línea completa y la cuenta
 *    (uart_driver_get_dropped_count()).
 */

#ifndef MEASURE_H
#define MEASURE_H

#include <stdint.h>

/**
 * @brief Inicializa el estado de la medición y toma el instante de arranque.
 * @note Contexto: main, antes de capture_init().
 */
void measure_init(void);

/**
 * @brief Un paso del ciclo principal: vacía los flancos pendientes y, si toca, calcula y
 *        entrega un reporte al driver UART. No bloquea.
 * @note Contexto: ciclo principal.
 */
void measure_poll(void);

/**
 * @brief Consulta cuántos reportes programados no se generaron por llegar tarde el ciclo principal.
 * @return Reportes omitidos desde el arranque [reportes].
 */
uint32_t measure_get_late_count(void);

#endif