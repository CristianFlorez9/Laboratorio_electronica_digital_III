/**
 * @file reporte.h
 * @brief Interfaz del módulo de procesamiento y transmisión de datos mediante UART.
 *
 * Este módulo obtiene los resultados del módulo de medición, procesa
 * los parámetros temporales de la señal y transmite periódicamente
 * la información hacia una consola mediante comunicación UART.
 *
 * Los datos transmitidos incluyen:
 * - Número consecutivo del reporte.
 * - Intervalo de tiempo transcurrido desde el reporte anterior.
 * - Frecuencia de la señal en Hz.
 * - Ciclo de trabajo (duty cycle) en porcentaje.
 * - Estado de la medición.
 *
 * La actualización se realiza mediante una estrategia de polling,
 * verificando periódicamente si se ha cumplido el intervalo de reporte.
 *
 * @note La transmisión utiliza UART0 con una velocidad de 115200 baudios.
 * @note Los datos se transmiten en formato CSV, separados por comas.
 */

#ifndef REPORTE_H
#define REPORTE_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/uart.h"

/** @brief Periférico UART utilizado para la transmisión de datos.
 *  Se utiliza UART0 del RP2040.
 */

/**
 * @brief Velocidad de transmisión de la comunicación UART.
 * Configurada a 115200 baudios.
 */

#define UART_ID uart0
#define BAUD_RATE 115200

/**
 * @brief Inicializa la comunicación UART.
 *
 * Configura el periférico UART seleccionado con la velocidad de
 * transmisión establecida y asigna los pines GPIO correspondientes
 * a las señales TX y RX.
 *
 * @note Se utiliza GPIO 0 para transmisión (TX) y GPIO 1 para
 *       recepción (RX).
 */
void reporte_init(void);

/**
 * @brief Procesa y transmite periódicamente los resultados de medición.
 *
 * Obtiene los datos procesados por el módulo de medición, calcula
 * la frecuencia y el ciclo de trabajo de la señal y genera un
 * reporte en formato CSV.
 *
 * Antes de transmitir, verifica la validez y actualidad de los datos
 * para determinar el estado de la medición.
 *
 * La transmisión se realiza mediante UART utilizando una estrategia
 * de polling, sin bloquear la ejecución durante el intervalo entre
 * reportes.
 * 
 * @note El intervalo mínimo entre reportes es de 100 ms.
 * @note Los estados posibles son ESPERANDO, SIN_TRANSICIONES y OK.
 */
void reporte_actualizar(void);

#endif