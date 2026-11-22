#include "adc_driver.h"
#include "pico/time.h"
#include "hardware/adc.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "hardware/sync.h"

#define ADC_BUFFER_SIZE 32

static volatile uint16_t adc_buffer[ADC_BUFFER_SIZE];
static volatile uint8_t head = 0;
static volatile uint8_t tail = 0;
static volatile uint32_t overflow_count = 0;
static volatile bool timer_activo = false;

static struct repeating_timer timer_adc;

static bool adc_timer_callback(struct repeating_timer *t) {
    (void)t;
    if (!timer_activo) return true;

    uint16_t raw = adc_read();
    uint8_t next_head = (head + 1) % ADC_BUFFER_SIZE;

    if (next_head == tail) {
        overflow_count++;
    } else {
        adc_buffer[head] = raw;
        head = next_head;
    }
    return true;
}

bool adc_driver_init(uint gpio_pin, uint32_t periodo_ms) {
    if (gpio_pin < 26 || gpio_pin > 29) return false;

    adc_init();
    adc_gpio_init(gpio_pin);
    adc_select_input(gpio_pin - 26);

    head = 0;
    tail = 0;
    overflow_count = 0;
    timer_activo = true;

    add_repeating_timer_ms(-(int32_t)periodo_ms, adc_timer_callback, NULL, &timer_adc);
    return true;
}

void adc_driver_pausar(bool pausar) {
    timer_activo = !pausar;
}

bool adc_driver_hay_muestra(void) {
    return head != tail;
}

bool adc_driver_procesar(uint16_t *mv_out) {
    static uint32_t acumulador = 0;
    static uint8_t muestras_tomadas = 0;

    uint32_t irq_status = save_and_disable_interrupts();
    while (head != tail && muestras_tomadas < ADC_MUESTRAS_PROMEDIO) {
        acumulador += adc_buffer[tail];
        tail = (tail + 1) % ADC_BUFFER_SIZE;
        muestras_tomadas++;
    }
    restore_interrupts(irq_status);

    if (muestras_tomadas >= ADC_MUESTRAS_PROMEDIO) {
        uint32_t promedio_raw = acumulador / ADC_MUESTRAS_PROMEDIO;

        // Convertir crudo a mV, aplicando el divisor resistivo
        uint32_t mv_pin = (promedio_raw * ADC_VREF_MV) / ADC_MAX_CUENTAS;
        *mv_out = (uint16_t)((mv_pin * ADC_DIVISOR_NUM) / ADC_DIVISOR_DEN);

        acumulador = 0;
        muestras_tomadas = 0;
        return true;
    }

    return false;
}

uint32_t adc_driver_overflows(void) {
    return overflow_count;
}