/**
 * @file capture.h
 * @brief Captura de flancos con dos slices PWM y alarma de reporte.
 *
 * @details
 * Cada slice PWM cuenta flancos de su pin con TOP = 0, así que cada flanco provoca una
 * interrupción. La ISR solo guarda el instante y el tipo del flanco en un anillo; el
 * cálculo lo hace measure.c en el ciclo principal.
 *
 * El modo de entrada del PWM solo existe en el canal B (GPIO impar). La señal se conecta
 * a los dos pines a la vez, y deben ser de slices distintos.
 */

#ifndef CAPTURE_H
#define CAPTURE_H

#include <stdint.h>
#include <stdbool.h>

#define CAPTURE_RISE_GPIO  3        /**< Pin que cuenta subidas (GPIO impar). */
#define CAPTURE_FALL_GPIO  7        /**< Pin que cuenta bajadas (GPIO impar, otro slice). */
#define REPORT_PERIOD_US   100000u  /**< Periodo de reporte [us]: 10 por segundo. */
#define EDGE_RING_SIZE     1024u    /**< Capacidad del anillo [eventos]. Potencia de 2. */

/** Un flanco capturado. */
typedef struct {
    uint32_t t_us;    /**< Instante del flanco [us]. */
    bool     rising;  /**< true = subida, false = bajada. */
} edge_event_t;

/** @brief Configura los slices PWM, su interrupción y la alarma de reporte. */
void capture_init(void);

/** @brief Saca el flanco más antiguo del anillo. Devuelve false si está vacío. */
bool capture_pop_edge(edge_event_t *ev);

/** @brief Flancos perdidos por anillo lleno [eventos]. */
uint32_t capture_overflow_count(void);

/**
 * @brief Lee juntos el número de ticks de reporte y el instante del último.
 * @param[out] count Ticks desde el arranque (el primero deja count = 1).
 * @param[out] t_us  Instante del último tick [us].
 */
void capture_get_tick(uint32_t *count, uint32_t *t_us);

#endif