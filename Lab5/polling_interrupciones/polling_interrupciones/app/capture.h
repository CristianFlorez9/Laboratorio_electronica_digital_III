/**
 * @file capture.h
 * @brief Captura de los flancos de la señal de entrada con slices PWM y base de tiempo de reporte.
 *
 * @details
 * Este módulo es la parte de "atención al hardware" del flujo Polling+IRQs: sus ISR solo
 * guardan información y no hacen cálculos. El cálculo lo hace measure.c en el ciclo
 * principal.
 *
 * **Conteo con PWM.** Dos slices PWM se configuran como contadores de flancos de entrada,
 * con TOP = 0, de modo que cada flanco contado provoca un wrap y, con él, una interrupción:
 *  - Slice de subida: modo PWM_DIV_B_RISING en CAPTURE_RISE_GPIO.
 *  - Slice de bajada: modo PWM_DIV_B_FALLING en CAPTURE_FALL_GPIO.
 *
 * El modo de entrada del PWM solo existe en el canal B, es decir, en GPIO impares. Como
 * cada pin pertenece a un único slice, la señal de entrada (3,3 V) debe conectarse a
 * los dos pines a la vez, y estos deben ser de slices distintos.
 *
 * **Por qué se guarda el instante de cada flanco.** Contar flancos en una ventana fija de
 * 100 ms no alcanza el 1 % de exactitud a 20 Hz (habría solo 2 flancos por ventana). Con
 * el instante de cada flanco, el periodo se mide con resolución de 1 us.
 *
 * **Anillo de eventos.** Es una cola de un productor (ISR de PWM) y un consumidor (ciclo
 * principal). Cada escritor modifica solo su índice. Si el anillo se llena, el evento se
 * descarta y se cuenta (capture_overflow_count()).
 *
 * **Base de tiempo de reporte.** Una alarma periódica de REPORT_PERIOD_US marca "toca
 * reporte" incrementando un contador de ticks y guardando su instante.
 */

#ifndef CAPTURE_H
#define CAPTURE_H

#include <stdint.h>
#include <stdbool.h>

/** Pin de entrada del contador de flancos de subida. Debe ser un GPIO impar (canal B). */
#define CAPTURE_RISE_GPIO   3

/**
 * Pin de entrada del contador de flancos de bajada. Debe ser un GPIO impar (canal B) y
 * de un slice distinto al de CAPTURE_RISE_GPIO.
 */
#define CAPTURE_FALL_GPIO   7

/** Periodo nominal de reporte [us]. Corresponde a 10 reportes por segundo. */
#define REPORT_PERIOD_US    100000u

/** Capacidad del anillo de eventos de flanco [eventos]. Debe ser potencia de 2. */
#define EDGE_RING_SIZE      1024u

/** Máscara para indexar el anillo de eventos (EDGE_RING_SIZE - 1). */
#define EDGE_RING_MASK      (EDGE_RING_SIZE - 1u)

/** Evento de flanco capturado por la ISR de PWM. */
typedef struct {
    uint32_t t_us;     /**< Marca de tiempo del temporizador de 1 MHz (32 bits, con wrap) [us]. */
    bool     rising;   /**< true = flanco de subida, false = flanco de bajada. */
} edge_event_t;

/**
 * @brief Configura los slices PWM contadores de flancos, su interrupción y la alarma periódica.
 * @note Contexto: main, antes de iniciar el ciclo principal.
 */
void capture_init(void);

/**
 * @brief Extrae el evento más antiguo del anillo.
 * @param[out] ev Destino del evento.
 * @return true si había un evento; false si el anillo estaba vacío.
 * @note Contexto: ciclo principal (único consumidor).
 */
bool capture_pop_edge(edge_event_t *ev);

/**
 * @brief Consulta cuántos eventos se perdieron por anillo lleno.
 * @return Eventos descartados desde el arranque [eventos].
 */
uint32_t capture_overflow_count(void);

/**
 * @brief Lee de forma atómica el contador de ticks de reporte y el instante del último.
 * @param[out] count Ticks acumulados desde el arranque [ticks]. El primero deja count = 1,
 *                   que corresponde al reporte n = 0.
 * @param[out] t_us  Instante del último tick [us].
 * @note Contexto: ciclo principal. Deshabilita interrupciones durante la lectura.
 */
void capture_get_tick(uint32_t *count, uint32_t *t_us);

#endif