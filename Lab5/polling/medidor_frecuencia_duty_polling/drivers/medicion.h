/**
 * @file medicion.h
 * @brief Interfaz del módulo de medición de período y tiempo en alto de una señal PWM.
 *
 * Este módulo permite medir el período de una señal y su tiempo en alto
 * mediante los contadores hardware PWM del RP2040.
 *
 * Para realizar las mediciones se utilizan dos slices PWM independientes:
 * - Un contador configurado para detectar flancos de subida y determinar
 *   la cantidad de períodos dentro de una ventana de medición.
 * - Un contador configurado para acumular el tiempo durante el cual la
 *   señal permanece en estado alto.
 *
 * Los resultados se procesan periódicamente y se expresan en microsegundos.
 *
 * @note Los pines utilizados deben ser impares (canal B) y pertenecer
 *       a slices PWM diferentes.
 * @note La señal debe conectarse físicamente a ambos pines de medición.
 */

#ifndef MEDICION_H
#define MEDICION_H

#include <stdint.h>
#include <stdbool.h>

/** PIN_FREQ_B Pin utilizado para contar los flancos de subida de la señal.*/

/** PIN_DUTY_BPin utilizado para medir el tiempo durante el cual la señal permanece en estado alto. */

#define PIN_FREQ_B  3  /**  Slice 0, Canal B */
#define PIN_DUTY_B  7  /**  Slice 1, Canal B */

/**
 * @brief Inicializa el módulo de medición.
 *
 * Configura los slices PWM asociados a los pines de medición:
 * - El primer slice se configura como contador de flancos de subida.
 * - El segundo slice se configura para contar mientras la señal está en alto.
 *
 * Ambos contadores se inicializan en cero y se habilitan simultáneamente.
 *
 * @note El contador de tiempo en alto utiliza un divisor de reloj de 250,
 *       obteniendo una resolución de 2 us por tick.
 */
void medicion_init(void);

/**
 * @brief Actualiza los resultados de las mediciones.
 *
 * Lee los contadores hardware PWM, calcula el período promedio y el
 * tiempo promedio en alto de la señal durante la ventana de medición.
 *
 * Los contadores se reinician después de cada lectura para comenzar
 * una nueva ventana de medición.
 *
 * @note La actualización se realiza únicamente cuando han transcurrido
 *       al menos 100 ms desde la última ventana de medición.
 * @note Si no se detectan flancos de subida, los resultados de período
 *       y tiempo en alto se establecen en cero.
 */
void medicion_actualizar(void);

/**
 * @brief Obtiene los resultados de la última medición procesada.
 *
 * Entrega los valores calculados de período, tiempo en alto y el instante
 * en el que se detectó el último flanco de subida.
 *
 * @param[out] periodo_us Puntero donde se almacena el período promedio, expresado en microsegundos.
 * @param[out] tiempo_alto_us Puntero donde se almacena el tiempo promedio en alto, expresado en microsegundos.
 * @param[out] ultimo_flanco_us Puntero donde se almacena el instante de la última detección de flanco, expresado
 *                              en microsegundos desde el inicio del sistema.
 *
 * @note Los valores corresponden a la última ventana de medición procesada.
 */
void medicion_obtener_datos(uint64_t* periodo_us, uint64_t* tiempo_alto_us, uint64_t* ultimo_flanco_us);

#endif