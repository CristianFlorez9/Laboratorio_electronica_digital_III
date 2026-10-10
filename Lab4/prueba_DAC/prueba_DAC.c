#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "hardware/exception.h"
#include "hardware/structs/sio.h"

// ---------------- Configuración ----------------
#define FIRST_GPIO       8            // GPIO 8..15 -> R-2R
#define NUM_BITS         8
#define GPIO_MASK        (((1u << NUM_BITS) - 1) << FIRST_GPIO)

#define DEBUG_PIN        16           // Debe medir SAMPLE_RATE_HZ / 2 = 25 kHz
#define DEBUG_MASK       (1u << DEBUG_PIN)

#define SYS_CLK_HZ       125000000U
#define SAMPLE_RATE_HZ   50000U
#define LUT_SIZE         256
#define TARGET_FREQ_HZ   440.0f

// 1 = bits invertidos (GPIO8 = MSB, GPIO15 = LSB)
#define REVERSE_BITS     1

// ---------------- Registros SysTick ----------------
#define PPB_BASE         0xE000E000U
#define SYST_CSR         (*(volatile uint32_t *)(PPB_BASE + 0x010U))
#define SYST_RVR         (*(volatile uint32_t *)(PPB_BASE + 0x014U))
#define SYST_CVR         (*(volatile uint32_t *)(PPB_BASE + 0x018U))

#define SYST_CSR_ENABLE    (1U << 0)
#define SYST_CSR_TICKINT   (1U << 1)
#define SYST_CSR_CLKSOURCE (1U << 2)

// ---------------- Variables ----------------
static uint32_t sine_lut[LUT_SIZE];      // ya desplazado a la posición de los GPIO
static volatile uint32_t phase_accumulator = 0;
static volatile uint32_t phase_increment  = 0;

static uint8_t reverse_bits(uint8_t b) {
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

static void init_sine_lut(void) {
    for (int i = 0; i < LUT_SIZE; i++) {
        float value = (sinf((2.0f * (float)M_PI * i) / (float)LUT_SIZE) + 1.0f) * 127.5f;
        uint8_t v = (uint8_t)roundf(value);
#if REVERSE_BITS
        v = reverse_bits(v);
#endif
        sine_lut[i] = (uint32_t)v << FIRST_GPIO;
    }
}

static void set_frequency(float target_freq_hz) {
    if (target_freq_hz <= 0.0f) {
        phase_increment = 0;
    } else {
        phase_increment = (uint32_t)(((double)target_freq_hz * 4294967296.0) / (double)SAMPLE_RATE_HZ);
    }
}

// ISR de SysTick: se ejecuta a SAMPLE_RATE_HZ
void __not_in_flash_func(systick_isr)(void) {
    phase_accumulator += phase_increment;
    gpio_put_masked(GPIO_MASK, sine_lut[phase_accumulator >> 24]);
    sio_hw->gpio_togl = DEBUG_MASK;               // pin de depuración
}

static void init_systick(void) {
    uint32_t clk = clock_get_hz(clk_sys);
    SYST_CSR = 0;
    SYST_RVR = (clk / SAMPLE_RATE_HZ) - 1;
    SYST_CVR = 0;
    SYST_CSR = SYST_CSR_ENABLE | SYST_CSR_TICKINT | SYST_CSR_CLKSOURCE;
}

int main(void) {
    set_sys_clock_hz(SYS_CLK_HZ, true);
    stdio_init_all();

    // Pines del DAC
    for (int i = FIRST_GPIO; i < FIRST_GPIO + NUM_BITS; i++) {
        gpio_init(i);
        gpio_set_dir(i, GPIO_OUT);
        gpio_put(i, 0);
    }

    // Pin de depuración
    gpio_init(DEBUG_PIN);
    gpio_set_dir(DEBUG_PIN, GPIO_OUT);
    gpio_put(DEBUG_PIN, 0);

    init_sine_lut();
    set_frequency(TARGET_FREQ_HZ);

    exception_set_exclusive_handler(SYSTICK_EXCEPTION, systick_isr);
    init_systick();

    sleep_ms(2000);   // tiempo para abrir el monitor serie
    printf("sys clk      = %lu Hz\n", (unsigned long)clock_get_hz(clk_sys));
    printf("SYST_RVR     = %lu\n", (unsigned long)SYST_RVR);
    printf("phase_inc    = %lu\n", (unsigned long)phase_increment);
    printf("Esperado: seno de %.1f Hz, pin %d a %u Hz\n",
           TARGET_FREQ_HZ, DEBUG_PIN, SAMPLE_RATE_HZ / 2);

    while (1) {
        tight_loop_contents();
    }
}