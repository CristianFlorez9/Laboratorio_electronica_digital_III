/**
 * @file capture.c
 * @brief Implementación de la captura de flancos con slices PWM (interfaz en capture.h).
 *
 * @details
 * Hay dos ISR: la de wrap de PWM (una por flanco) y la de la alarma periódica (una por
 * reporte). Ambas solo escriben datos; el ciclo principal los lee. La aplicación usa un
 * solo núcleo.
 *
 * Si subida y bajada quedaran pendientes a la vez (frecuencias muy altas), se publican
 * en el orden subida y luego bajada sin poder saber el orden real. measure.c descarta
 * los pares inconsistentes, y a esas frecuencias el comportamiento se documenta como
 * límite del intervalo de operación.
 */

#include "capture.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/irq.h"
#include "hardware/timer.h"
#include "hardware/sync.h"
#include "pico/time.h"

#ifdef PWM_DEFAULT_IRQ_NUM
/** Número de IRQ compartido por el wrap de todos los slices PWM. */
#define CAPTURE_PWM_IRQ PWM_DEFAULT_IRQ_NUM()
#else
/** Número de IRQ compartido por el wrap de todos los slices PWM. */
#define CAPTURE_PWM_IRQ PWM_IRQ_WRAP
#endif

_Static_assert((EDGE_RING_SIZE & (EDGE_RING_SIZE - 1)) == 0, "EDGE_RING_SIZE potencia de 2");
_Static_assert((CAPTURE_RISE_GPIO & 1) == 1, "el modo de entrada PWM solo existe en el canal B (GPIO impar)");
_Static_assert((CAPTURE_FALL_GPIO & 1) == 1, "el modo de entrada PWM solo existe en el canal B (GPIO impar)");
_Static_assert(((CAPTURE_RISE_GPIO >> 1) & 7) != ((CAPTURE_FALL_GPIO >> 1) & 7),
               "los pines de subida y bajada deben ser de slices distintos");

/** Anillo de eventos de flanco. Lo escribe la ISR de PWM y lo lee el ciclo principal. */
static volatile edge_event_t ring[EDGE_RING_SIZE];

/** Índice de escritura del anillo (contador creciente). Solo lo avanza la ISR de PWM. */
static volatile uint32_t w_idx = 0;

/** Índice de lectura del anillo (contador creciente). Solo lo avanza el ciclo principal. */
static volatile uint32_t r_idx = 0;

/** Eventos descartados por anillo lleno [eventos]. Solo lo incrementa la ISR de PWM. */
static volatile uint32_t overflow = 0;

/** Ticks de reporte transcurridos [ticks]. Solo lo incrementa la ISR de la alarma. */
static volatile uint32_t tick_count = 0;

/** Instante del último tick de reporte [us]. Solo lo escribe la ISR de la alarma. */
static volatile uint32_t tick_t_us = 0;

/** Número del slice PWM que cuenta flancos de subida. Se asigna en capture_init(). */
static uint rise_slice;

/** Número del slice PWM que cuenta flancos de bajada. Se asigna en capture_init(). */
static uint fall_slice;

/** Estado de la alarma periódica de reporte. */
static repeating_timer_t report_timer;

/**
 * @brief Publica un flanco en el anillo.
 * @param[in] t      Instante del flanco [us].
 * @param[in] rising true si es de subida, false si es de bajada.
 * @note Contexto: ISR de PWM. Si el anillo está lleno no espera: descarta y cuenta.
 */
static inline void push_edge(uint32_t t, bool rising) {
    if ((w_idx - r_idx) >= EDGE_RING_SIZE) {
        overflow++;
        return;
    }
    volatile edge_event_t *e = &ring[w_idx & EDGE_RING_MASK];
    e->t_us   = t;
    e->rising = rising;
    w_idx++;
}

/**
 * @brief ISR de wrap de PWM: una invocación por flanco contado.
 *
 * Toma la marca de tiempo lo más pronto posible, consulta cuál de los dos slices se
 * disparó (comparten una sola línea de IRQ), limpia su bandera y publica el flanco. No
 * hace cálculos.
 *
 * @note Contexto: ISR con la máxima prioridad (0x00).
 */
static void pwm_edge_isr(void) {
    uint32_t t = timer_hw->timerawl;
    uint32_t pending = pwm_get_irq_status_mask();

    if (pending & (1u << rise_slice)) {
        pwm_clear_irq(rise_slice);
        push_edge(t, true);
    }
    if (pending & (1u << fall_slice)) {
        pwm_clear_irq(fall_slice);
        push_edge(t, false);
    }
}

/**
 * @brief Callback de la alarma periódica: marca "toca reporte".
 * @param[in] rt Temporizador que invoca el callback (no se usa).
 * @return true para que la alarma siga repitiéndose.
 * @note Contexto: ISR de la alarma (prioridad por defecto del SDK).
 */
static bool report_tick_isr(repeating_timer_t *rt) {
    (void)rt;
    tick_t_us = time_us_32();
    tick_count++;
    return true;
}

/**
 * @brief Deja un slice PWM contando flancos del pin B indicado, con un wrap por flanco.
 *
 * Configura el pin con función PWM y sin resistencias de pull, el modo de entrada del
 * slice, divisor 1 y TOP = 0, y habilita la interrupción de wrap del slice. El slice
 * queda detenido; se arranca después con pwm_set_mask_enabled().
 *
 * @param[in] gpio Pin de entrada (GPIO impar, canal B).
 * @param[in] mode PWM_DIV_B_RISING o PWM_DIV_B_FALLING.
 * @return Número del slice configurado.
 */
static uint edge_counter_init(uint gpio, enum pwm_clkdiv_mode mode) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    gpio_disable_pulls(gpio);

    uint slice = pwm_gpio_to_slice_num(gpio);
    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv_mode(&cfg, mode);
    pwm_config_set_clkdiv(&cfg, 1.0f);
    pwm_config_set_wrap(&cfg, 0);
    pwm_init(slice, &cfg, false);
    pwm_set_counter(slice, 0);

    pwm_clear_irq(slice);
    pwm_set_irq_enabled(slice, true);
    return slice;
}

void capture_init(void) {
    rise_slice = edge_counter_init(CAPTURE_RISE_GPIO, PWM_DIV_B_RISING);
    fall_slice = edge_counter_init(CAPTURE_FALL_GPIO, PWM_DIV_B_FALLING);

    irq_set_exclusive_handler(CAPTURE_PWM_IRQ, pwm_edge_isr);
    irq_set_priority(CAPTURE_PWM_IRQ, 0x00);
    irq_set_enabled(CAPTURE_PWM_IRQ, true);

    pwm_set_mask_enabled((1u << rise_slice) | (1u << fall_slice));

    /* Retardo negativo: periodo fijo entre inicios, sin deriva acumulada. */
    add_repeating_timer_us(-(int32_t)REPORT_PERIOD_US, report_tick_isr, NULL, &report_timer);
}

bool capture_pop_edge(edge_event_t *ev) {
    if (r_idx == w_idx) return false;
    volatile edge_event_t *e = &ring[r_idx & EDGE_RING_MASK];
    ev->t_us   = e->t_us;
    ev->rising = e->rising;
    r_idx++;
    return true;
}

uint32_t capture_overflow_count(void) { return overflow; }

void capture_get_tick(uint32_t *count, uint32_t *t_us) {
    uint32_t s = save_and_disable_interrupts();
    *count = tick_count;
    *t_us  = tick_t_us;
    restore_interrupts(s);
}