/**
 * @file capture.c
 * @brief Captura de flancos con slices PWM (interfaz en capture.h).
 *
 * @details
 * Dos ISR que solo guardan datos: la de PWM (una por flanco) y la de la alarma (una por
 * reporte). El ciclo principal los lee.
 */

#include "capture.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
#include "pico/time.h"

/* Anillo de flancos: w_idx lo avanza la ISR, r_idx el ciclo principal. */
static volatile edge_event_t ring[EDGE_RING_SIZE];
static volatile uint32_t w_idx, r_idx;
static volatile uint32_t overflow;          /* flancos perdidos por anillo lleno */

/* Ticks de reporte: los escribe la ISR de la alarma. */
static volatile uint32_t tick_count, tick_t_us;

static uint rise_slice, fall_slice;
static repeating_timer_t report_timer;

/* ISR de PWM: una vez por flanco. Guarda el instante y el tipo, sin calcular. */
static void pwm_isr(void) {
    uint32_t t = time_us_32();
    uint32_t pending = pwm_get_irq_status_mask();   /* ambos slices comparten la IRQ */

    for (int i = 0; i < 2; i++) {
        uint slice = i ? fall_slice : rise_slice;
        if (!(pending & (1u << slice))) continue;
        pwm_clear_irq(slice);

        if (w_idx - r_idx >= EDGE_RING_SIZE) {      /* anillo lleno: descartar y contar */
            overflow++;
        } else {
            ring[w_idx % EDGE_RING_SIZE].t_us   = t;
            ring[w_idx % EDGE_RING_SIZE].rising = (i == 0);
            w_idx++;
        }
    }
}

/* ISR de la alarma: marca "toca reporte". */
static bool tick_isr(repeating_timer_t *rt) {
    (void)rt;
    tick_t_us = time_us_32();
    tick_count++;
    return true;
}

/* Slice que cuenta flancos del pin B; TOP = 0 para un wrap (y una IRQ) por flanco. */
static uint counter_init(uint gpio, enum pwm_clkdiv_mode mode) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(gpio);

    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv_mode(&cfg, mode);
    pwm_config_set_wrap(&cfg, 0);
    pwm_init(slice, &cfg, false);               /* queda detenido */

    pwm_clear_irq(slice);
    pwm_set_irq_enabled(slice, true);
    return slice;
}

void capture_init(void) {
    rise_slice = counter_init(CAPTURE_RISE_GPIO, PWM_DIV_B_RISING);
    fall_slice = counter_init(CAPTURE_FALL_GPIO, PWM_DIV_B_FALLING);

    irq_set_exclusive_handler(PWM_IRQ_WRAP, pwm_isr);
    irq_set_priority(PWM_IRQ_WRAP, 0x00);       /* la más alta, para no perder flancos */
    irq_set_enabled(PWM_IRQ_WRAP, true);
    pwm_set_mask_enabled((1u << rise_slice) | (1u << fall_slice));

    /* Retardo negativo: periodo fijo entre inicios, sin deriva. */
    add_repeating_timer_us(-(int32_t)REPORT_PERIOD_US, tick_isr, NULL, &report_timer);
}

bool capture_pop_edge(edge_event_t *ev) {
    if (r_idx == w_idx) return false;
    ev->t_us   = ring[r_idx % EDGE_RING_SIZE].t_us;
    ev->rising = ring[r_idx % EDGE_RING_SIZE].rising;
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