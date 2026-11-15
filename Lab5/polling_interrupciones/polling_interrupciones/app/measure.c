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

/* Un flanco: avanza el emparejamiento y acumula si se completó un periodo.
 * Secuencia esperada: subida -> bajada -> subida. Con esas tres se conocen
 * T (subida a subida) y tH (subida a bajada), y el periodo se da por completo. */
static void handle_edge(const edge_event_t *e) {
    /* Anota la hora de este flanco; measure_poll la usa para detectar el timeout */
    last_edge_t = e->t_us;

    /* Llegó una subida */
    if (e->rising) {
        /* Si el estado era VI_BAJADA, ya se vio subida y bajada: esta subida cierra el periodo */
        if (estado == VI_BAJADA) {
            /* T = subida nueva - subida anterior. t_subida aún guarda la anterior,
             * por eso esta línea va antes de actualizarla. El cast a uint32_t mantiene
             * la resta correcta aunque el reloj de 32 bits dé la vuelta */
            sum_T += (uint32_t)(e->t_us - t_subida);
            /* Suma el tiempo en alto guardado al ver la bajada de este mismo periodo */
            sum_H += t_alto;
            /* Un periodo completo más en la ventana actual */
            n_periods++;
        }
        /* Esta subida es el inicio del siguiente periodo */
        t_subida = e->t_us;
        /* Ahora se espera la bajada */
        estado = VI_SUBIDA;
    /* Llegó una bajada y se esperaba (había una subida previa) */
    } else if (estado == VI_SUBIDA) {
        /* tH = bajada - subida. Solo se guarda: no se suma hasta que el periodo se cierre */
        t_alto = e->t_us - t_subida;
        /* Ahora se espera la siguiente subida, que cierra el periodo */
        estado = VI_BAJADA;
    /* Llegó una bajada que no se esperaba (sin subida previa, o dos bajadas seguidas) */
    } else {
        /* Se descarta y se espera una subida limpia para volver a tener referencia */
        estado = ESPERO_SUBIDA;
    }
}

/* Función del ciclo principal: procesa flancos y, si toca, calcula y envía el reporte.
 * Los flancos los deja pwm_isr en el anillo y el aviso de reporte lo deja tick_isr;
 * aquí solo se leen, la ISR nunca llama a esta función. */
void measure_poll(void) {
    /* 1. Procesar los flancos pendientes (si se perdieron, descartar la ventana). */

    /* Lee cuántos flancos ha perdido el anillo por estar lleno */
    uint32_t ov = capture_overflow_count();
    /* Si cambió desde la última revisión, se perdieron flancos */
    if (ov != seen_overflow) {
        /* Recuerda el nuevo valor para no volver a reaccionar al mismo overflow */
        seen_overflow = ov;
        /* Sin todos los flancos el emparejamiento no es confiable: se reinicia */
        estado = ESPERO_SUBIDA;
        /* Y se descarta lo acumulado en la ventana, que quedó incompleto */
        sum_T = sum_H = n_periods = 0;
    }
    /* Evento temporal donde se copia cada flanco sacado del anillo */
    edge_event_t e;
    /* Vacía el anillo: cada flanco pasa por la máquina de estados, del más viejo al más nuevo */
    while (capture_pop_edge(&e)) handle_edge(&e);

    /* 2. ¿Toca reporte? */

    /* Lee el contador de ticks y la hora del último tick (lectura atómica) */
    uint32_t cnt, tick_t;
    capture_get_tick(&cnt, &tick_t);
    /* Si el contador no cambió desde el último reporte, no toca: se sale y se vuelve a llamar luego */
    if (cnt == reports_seen) return;

    /* Número de reporte, empezando en 0 */
    uint32_t n = cnt - 1;
    /* Tiempo real entre este tick y el anterior en ms (+500 redondea al ms más cercano).
     * En el primer reporte no hay tick anterior, así que vale 0 */
    uint32_t dt_ms = reports_seen ? (tick_t - prev_tick_t + 500u) / 1000u : 0;
    /* Si el contador subió más de 1, se saltaron reportes (el main fue lento): se cuentan como atrasados */
    late_reports += cnt - reports_seen - 1;
    /* Marca estos ticks como ya atendidos */
    reports_seen = cnt;
    /* Guarda la hora de este tick para calcular dt_ms en el siguiente reporte */
    prev_tick_t = tick_t;

    /* 3. Calcular. */

    /* Timeout: desde el último flanco hasta el tick pasaron más de 500 ms, no hay señal.
     * El cast a int32_t mantiene la comparación correcta si el reloj dio la vuelta */
    bool timeout = (int32_t)(tick_t - last_edge_t) > NO_TRANSITION_TIMEOUT_US;
    if (timeout) {
        /* Sin señal: la última medición ya no sirve para repetirse */
        have_meas = false;
        /* Se reinicia el emparejamiento para empezar limpio cuando vuelva la señal */
        estado = ESPERO_SUBIDA;
    /* Hay señal y se completó al menos un periodo en la ventana */
    } else if (n_periods > 0) {
        /* Frecuencia promedio en Hz: n periodos en sum_T µs; el 1e6 convierte µs a Hz */
        last_f = n_periods * 1e6 / sum_T;
        /* Duty promedio en %: tiempo total en alto sobre tiempo total de los periodos */
        last_d = 100.0 * sum_H / sum_T;
        /* Ya hay una medición válida que se puede repetir en ventanas futuras */
        have_meas = true;
    }
    /* Si hay señal pero no se completó ningún periodo, last_f y last_d no cambian
     * y se repite la última medición válida */

    /* Nueva ventana: se vacían los acumuladores para el siguiente reporte */
    sum_T = sum_H = n_periods = 0;

    /* 4. Enviar. */

    /* Buffer donde se arma la línea de texto del reporte */
    char line[64];
    /* Caso timeout: formato "n,dt_ms,NA,NA,SIN_TRANSICIONES" */
    if (timeout)        snprintf(line, sizeof line, "%lu,%lu,NA,NA,SIN_TRANSICIONES\n",
                                 (unsigned long)n, (unsigned long)dt_ms);
    /* Caso medición válida: formato "n,dt_ms,frecuencia(3 decimales),duty(1 decimal),OK" */
    else if (have_meas) snprintf(line, sizeof line, "%lu,%lu,%.3f,%.1f,OK\n",
                                 (unsigned long)n, (unsigned long)dt_ms, last_f, last_d);
    /* Caso sin medición todavía y sin timeout: formato "n,dt_ms,NA,NA,ESPERANDO" */
    else                snprintf(line, sizeof line, "%lu,%lu,NA,NA,ESPERANDO\n",
                                 (unsigned long)n, (unsigned long)dt_ms);
    /* Envía la línea por UART */
    uart_driver_send_string(line);
}

/* Devuelve cuántos reportes se atrasaron (el main tardó en atender el tick), útil para depurar */
uint32_t measure_get_late_count(void) { return late_reports; }