/**
 * @file measure.c
 * @brief Implementación de la medición de frecuencia y duty (interfaz en measure.h).
 *
 * @details
 * **Emparejamiento de flancos.** Un periodo está completo cuando, después de una subida
 * y su bajada, llega la siguiente subida. Entonces T es la diferencia entre las dos
 * subidas y tH es el tiempo entre la primera subida y la bajada. Una bajada sin subida
 * previa, o una secuencia inconsistente, reinicia el emparejamiento.
 *
 * **Ventana.** Entre dos reportes se acumulan T y tH de todos los periodos completos.
 * Al reportar se calculan los promedios y la ventana se reinicia. Si en una ventana no
 * se completó ningún periodo, se repite la última medición válida mientras sigan
 * llegando transiciones.
 *
 * **Sin transiciones.** Si desde el último flanco (o desde el arranque, si no ha habido
 * ninguno) pasan más de 500 ms al momento de un reporte, se invalida la medición y se
 * informa SIN_TRANSICIONES. Como se evalúa en cada tick de 100 ms, la invalidación
 * aparece entre 500 y 600 ms después del último flanco, en el siguiente reporte
 * programado.
 *
 * **Representación numérica.** Aritmética entera: la frecuencia se guarda en mHz y el
 * ciclo de trabajo en décimas de porcentaje, ambos con redondeo, y los acumuladores son
 * de 64 bits para no desbordar.
 */

#include "measure.h"
#include "capture.h"
#include "UART.h"
#include "pico/time.h"
#include <stdio.h>
#include <stdbool.h>

/** Tiempo sin transiciones tras el cual se invalida la medición [us]. */
#define NO_TRANSITION_TIMEOUT_US 500000

/** Hay una subida pendiente de emparejar con su bajada. */
static bool have_rise;

/** Hay una bajada ya emparejada con la subida en curso. */
static bool have_fall;

/** Instante de la última subida [us]. */
static uint32_t last_rise_t;

/** Tiempo en alto del periodo en curso, medido de subida a bajada [us]. */
static uint32_t tH_us;

/** Suma de los periodos completos de la ventana actual [us]. */
static uint64_t sum_T;

/** Suma de los tiempos en alto de la ventana actual [us]. */
static uint64_t sum_H;

/** Número de periodos completos acumulados en la ventana actual [periodos]. */
static uint32_t n_periods;

/** Hay una última medición válida para reportar. */
static bool have_meas;

/** Última frecuencia válida [mHz]. */
static uint32_t last_f_mhz;

/** Último ciclo de trabajo válido [décimas de %]. */
static uint32_t last_d_dec;

/** Instante de arranque de la medición [us]. */
static uint32_t boot_t;

/** Instante del último flanco recibido [us]. Válido solo si any_edge es true. */
static uint32_t last_edge_t;

/** Ya se recibió al menos un flanco. */
static bool any_edge;

/** Última cuenta de eventos perdidos vista, para detectar nuevas pérdidas [eventos]. */
static uint32_t seen_overflow;

/** Ticks de reporte ya atendidos [ticks]. */
static uint32_t reports_seen;

/** Instante del tick del reporte anterior [us]. Válido solo si have_prev_tick es true. */
static uint32_t prev_tick_t;

/** Ya hay un tick anterior contra el cual calcular dt. */
static bool have_prev_tick;

/** Reportes programados que no se generaron porque el ciclo llegó tarde [reportes]. */
static uint32_t late_reports;

/** Descarta el emparejamiento de flancos en curso. */
static void reset_pairing(void) { have_rise = have_fall = false; }

/** Reinicia los acumuladores de la ventana. */
static void reset_window(void)  { sum_T = sum_H = 0; n_periods = 0; }

void measure_init(void) {
    boot_t = time_us_32();
    any_edge = false;
    have_meas = false;
    reset_pairing();
    reset_window();
}

/**
 * @brief Procesa un flanco: actualiza el emparejamiento y acumula periodos completos.
 * @param[in] e Evento de flanco con su instante [us] y su tipo.
 * @note Contexto: ciclo principal.
 */
static void handle_edge(const edge_event_t *e) {
    last_edge_t = e->t_us;
    any_edge = true;

    if (e->rising) {
        if (have_rise && have_fall) {
            sum_T += (uint32_t)(e->t_us - last_rise_t);
            sum_H += tH_us;
            n_periods++;
        }
        last_rise_t = e->t_us;
        have_rise = true;
        have_fall = false;
    } else {
        if (have_rise && !have_fall) {
            tH_us = e->t_us - last_rise_t;
            have_fall = true;
        } else {
            reset_pairing();
        }
    }
}

/**
 * @brief Vacía el anillo de flancos. Si hubo pérdidas, descarta la ventana en curso.
 * @note Contexto: ciclo principal.
 */
static void drain_edges(void) {
    uint32_t ov = capture_overflow_count();
    if (ov != seen_overflow) {
        seen_overflow = ov;
        reset_pairing();
        reset_window();
    }
    edge_event_t e;
    while (capture_pop_edge(&e)) handle_edge(&e);
}

void measure_poll(void) {
    drain_edges();

    uint32_t cnt, tick_t;
    capture_get_tick(&cnt, &tick_t);
    if (cnt == reports_seen) return;

    uint32_t n = cnt - 1;
    late_reports += (cnt - reports_seen - 1);
    reports_seen = cnt;

    uint32_t dt_ms = 0;
    if (have_prev_tick && n != 0) dt_ms = (tick_t - prev_tick_t + 500u) / 1000u;
    prev_tick_t = tick_t;
    have_prev_tick = true;

    uint32_t ref = any_edge ? last_edge_t : boot_t;
    bool timeout = (int32_t)(tick_t - ref) > NO_TRANSITION_TIMEOUT_US;

    char line[64];
    int len;

    if (timeout) {
        have_meas = false;
        reset_pairing();
        reset_window();
        len = snprintf(line, sizeof line, "%lu,%lu,NA,NA,SIN_TRANSICIONES\n",
                       (unsigned long)n, (unsigned long)dt_ms);
    } else {
        if (n_periods > 0) {
            last_f_mhz = (uint32_t)(((uint64_t)n_periods * 1000000000ull + sum_T / 2) / sum_T);
            last_d_dec = (uint32_t)((sum_H * 1000ull + sum_T / 2) / sum_T);
            have_meas  = true;
            reset_window();
        }
        if (have_meas) {
            len = snprintf(line, sizeof line, "%lu,%lu,%lu.%03lu,%lu.%lu,OK\n",
                           (unsigned long)n, (unsigned long)dt_ms,
                           (unsigned long)(last_f_mhz / 1000), (unsigned long)(last_f_mhz % 1000),
                           (unsigned long)(last_d_dec / 10),   (unsigned long)(last_d_dec % 10));
        } else {
            len = snprintf(line, sizeof line, "%lu,%lu,NA,NA,ESPERANDO\n",
                           (unsigned long)n, (unsigned long)dt_ms);
        }
    }

    if (len > 0 && len < (int)sizeof line) {
        uart_driver_send_string(line);
    }
}

uint32_t measure_get_late_count(void) { return late_reports; }