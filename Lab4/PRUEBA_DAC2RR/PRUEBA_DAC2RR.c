#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "hardware/pio_instructions.h"
#include <math.h>

// ---------------- Configuración ----------------
#define FIRST_GPIO      8            // GPIO 8..15 -> R-2R
#define NUM_BITS        8
#define LUT_SIZE        256          // debe ser 256 (anillo DMA de 8 bits)
#define NOTE_MS         1000         // duración de cada nota
#define REVERSE_BITS    0            // 1 si GPIO8 es el MSB
#define DMA_COUNT       0x0FFFFFFFu

static uint8_t sine_lut[LUT_SIZE] __attribute__((aligned(256)));

// ---------------- Tabla de frecuencias (Hz) ----------------
static const char *note_name[7] = {"Do", "Re", "Mi", "Fa", "Sol", "La", "Si"};

static const double freq_table[3][7] = {
    // Do      Re      Mi      Fa      Sol     La      Si
    {130.81, 146.83, 164.81, 174.61, 196.00, 220.00, 246.94},   // octava 3
    {261.63, 293.66, 329.63, 349.23, 392.00, 440.00, 493.88},   // octava 4
    {523.25, 587.33, 659.26, 698.46, 783.99, 880.00, 987.77},   // octava 5
};

static PIO pio = pio0;
static uint sm;
static int dma_ch;

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
        sine_lut[i] = v;
    }
}

static void init_pio(void) {
    uint16_t instr = pio_encode_out(pio_pins, NUM_BITS);   // out pins, 8
    struct pio_program prog = { .instructions = &instr, .length = 1, .origin = -1 };
    uint offset = pio_add_program(pio, &prog);

    sm = pio_claim_unused_sm(pio, true);

    for (int i = FIRST_GPIO; i < FIRST_GPIO + NUM_BITS; i++)
        pio_gpio_init(pio, i);
    pio_sm_set_consecutive_pindirs(pio, sm, FIRST_GPIO, NUM_BITS, true);

    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_out_pins(&c, FIRST_GPIO, NUM_BITS);
    sm_config_set_out_shift(&c, true, true, NUM_BITS);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
    sm_config_set_wrap(&c, offset, offset);
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

static void init_dma(void) {
    dma_ch = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(dma_ch);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_ring(&c, false, 8);               // anillo de 256 bytes
    channel_config_set_dreq(&c, pio_get_dreq(pio, sm, true));
    dma_channel_configure(dma_ch, &c, &pio->txf[sm], sine_lut, DMA_COUNT, true);
}

// Fija la frecuencia con el divisor fraccionario. Devuelve la frecuencia real.
static double set_note(double f) {
    double sys = (double)clock_get_hz(clk_sys);
    double div = sys / (f * LUT_SIZE);

    uint32_t div_int  = (uint32_t)div;
    uint32_t div_frac = (uint32_t)((div - div_int) * 256.0);
    pio_sm_set_clkdiv_int_frac(pio, sm, (uint16_t)div_int, (uint8_t)div_frac);

    double div_real = div_int + div_frac / 256.0;
    return sys / (div_real * LUT_SIZE);
}

int main(void) {
    stdio_init_all();

    init_sine_lut();
    init_pio();
    set_note(freq_table[0][0]);
    init_dma();

    sleep_ms(2000);   // tiempo para abrir el monitor serie
    printf("sys clk = %lu Hz\n", (unsigned long)clock_get_hz(clk_sys));

    absolute_time_t next = get_absolute_time();

    while (1) {
        for (int n = 0; n < 7; n++) {                // Do ... Si (por fuera)
            for (int oct = 0; oct < 3; oct++) {      // octavas 3, 4, 5 (por dentro)
                double target = freq_table[oct][n];
                double real   = set_note(target);

                printf("%s%d: objetivo %.2f Hz, real %.4f Hz\n",
                       note_name[n], oct + 3, target, real);

                // Rearmar el DMA si terminó su cuenta (muy raro: ~17 min)
                if (!dma_channel_is_busy(dma_ch))
                    dma_channel_set_trans_count(dma_ch, DMA_COUNT, true);

                next = delayed_by_ms(next, NOTE_MS);
                sleep_until(next);
            }
        }
    }
}