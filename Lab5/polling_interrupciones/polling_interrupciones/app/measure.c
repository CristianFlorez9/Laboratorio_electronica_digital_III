/**
 * @file measure.c
 * @brief Cálculo de frecuencia y duty (interfaz en measure.h).
 *
 * @details
 * Un periodo está completo cuando, tras una subida y su bajada, llega otra subida:
 * T = de subida a subida, tH = de subida a bajada. Entre reportes se suman T y tH de
 * todos los periodos completos y al reportar se promedia. Si en la ventana no hubo
 * periodos se repite la última medición válida. Si pasan más de 500 ms sin flancos se
 * informa SIN_TRANSICIONES.
 */

#include "measure.h"
#include "capture.h"
#include "UART.h"
#include "pico/time.h"
#include <stdio.h>

#define NO_TRANSITION_TIMEOUT_US 500000     /* sin flancos por más de esto: sin medición */

/* Estado del emparejamiento de flancos */
enum { ESPERO_SUBIDA, VI_SUBIDA, VI_BAJADA };
static int      estado = ESPERO_SUBIDA;
static uint32_t t_subida;                   /* instante de la última subida [us] */
static uint32_t t_alto;                     /* tiempo en alto del periodo en curso [us] */

/* Acumuladores de la ventana */
static uint64_t sum_T, sum_H;               /* [us] */
static uint32_t n_periods;

/* Última medición válida */
static bool     have_meas;
static double   last_f, last_d;             /* [Hz], [%] */

/* Control de flancos y reportes */
static uint32_t last_edge_t;                /* instante del último flanco [us] */
static uint32_t seen_overflow;
static uint32_t reports_seen, prev_tick_t, late_reports;

void measure_init(void) {
    last_edge_t = time_us_32();             /* referencia para el timeout inicial */
}

/* Un flanco: avanza el emparejamiento y acumula si se completó un periodo. */
static void handle_edge(const edge_event_t *e) {
    last_edge_t = e->t_us;

    if (e->rising) {
        if (estado == VI_BAJADA) {          /* periodo completo */
            sum_T += (uint32_t)(e->t_us - t_subida);
            sum_H += t_alto;
            n_periods++;
        }
        t_subida = e->t_us;
        estado = VI_SUBIDA;
    } else if (estado == VI_SUBIDA) {
        t_alto = e->t_us - t_subida;
        estado = VI_BAJADA;
    } else {
        estado = ESPERO_SUBIDA;             /* bajada inesperada: reiniciar */
    }
}

void measure_poll(void) {
    /* 1. Procesar los flancos pendientes (si se perdieron, descartar la ventana). */
    uint32_t ov = capture_overflow_count();
    if (ov != seen_overflow) {
        seen_overflow = ov;
        estado = ESPERO_SUBIDA;
        sum_T = sum_H = n_periods = 0;
    }
    edge_event_t e;
    while (capture_pop_edge(&e)) handle_edge(&e);

    /* 2. ¿Toca reporte? */
    uint32_t cnt, tick_t;
    capture_get_tick(&cnt, &tick_t);
    if (cnt == reports_seen) return;

    uint32_t n = cnt - 1;
    uint32_t dt_ms = reports_seen ? (tick_t - prev_tick_t + 500u) / 1000u : 0;
    late_reports += cnt - reports_seen - 1;
    reports_seen = cnt;
    prev_tick_t = tick_t;

    /* 3. Calcular. */
    bool timeout = (int32_t)(tick_t - last_edge_t) > NO_TRANSITION_TIMEOUT_US;
    if (timeout) {
        have_meas = false;
        estado = ESPERO_SUBIDA;
    } else if (n_periods > 0) {
        last_f = n_periods * 1e6 / sum_T;
        last_d = 100.0 * sum_H / sum_T;
        have_meas = true;
    }
    sum_T = sum_H = n_periods = 0;          /* nueva ventana */

    /* 4. Enviar. */
    char line[64];
    if (timeout)        snprintf(line, sizeof line, "%lu,%lu,NA,NA,SIN_TRANSICIONES\n",
                                 (unsigned long)n, (unsigned long)dt_ms);
    else if (have_meas) snprintf(line, sizeof line, "%lu,%lu,%.3f,%.1f,OK\n",
                                 (unsigned long)n, (unsigned long)dt_ms, last_f, last_d);
    else                snprintf(line, sizeof line, "%lu,%lu,NA,NA,ESPERANDO\n",
                                 (unsigned long)n, (unsigned long)dt_ms);
    uart_driver_send_string(line);
}

uint32_t measure_get_late_count(void) { return late_reports; }