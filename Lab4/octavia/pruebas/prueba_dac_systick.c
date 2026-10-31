#include <stdio.h>
#include <math.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/structs/scb.h"
#include "hardware/sync.h"
#include "systick.h"
#include "dac.h"

#define N_MUESTRAS 64u
#define VECTORES_TOTAL 48u
#define VECTOR_SYSTICK 15u

static uint8_t tabla[N_MUESTRAS];
static volatile uint32_t fase = 0;
static volatile uint64_t muestras_totales = 0;
static uint32_t vectores_ram[VECTORES_TOTAL] __attribute__((aligned(256)));

static void isr_systick_test(void)
{
    dac_escribir(tabla[fase]);
    fase = (fase + 1u) & (N_MUESTRAS - 1u);
    muestras_totales++;
}

static void instalar_isr_systick(void (*isr)(void))
{
    memcpy(vectores_ram, (const void *)scb_hw->vtor, sizeof(vectores_ram));
    vectores_ram[VECTOR_SYSTICK] = (uint32_t)isr;
    scb_hw->vtor = (uint32_t)vectores_ram;
    __dsb();
    __isb();
}

int main(void)
{
    stdio_init_all();

    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }

    printf("\nGenerador de senal DAC - Pico SDK\n");
    fflush(stdout);

    for (uint32_t k = 0; k < N_MUESTRAS; k++) {
        tabla[k] = (uint8_t)(128.0f + 127.0f * sinf(2.0f * 3.14159265f * k / N_MUESTRAS));
    }

    dac_init();
    instalar_isr_systick(isr_systick_test);
    systick_init(4438);

    printf("[OK] Generando senal a 440 Hz...\n");
    fflush(stdout);

    uint32_t ultimo_tiempo_ms = to_ms_since_boot(get_absolute_time());

    while (true) {
        uint32_t tiempo_actual_ms = to_ms_since_boot(get_absolute_time());

        if (tiempo_actual_ms - ultimo_tiempo_ms >= 1000) {
            ultimo_tiempo_ms = tiempo_actual_ms;

            uint32_t status_irq = save_and_disable_interrupts();
            uint64_t muestras_copia = muestras_totales;
            uint32_t fase_copia = fase;
            restore_interrupts(status_irq);

            printf("[STATUS] Uptime: %lu s | Muestras: %llu | Ciclos: %lu | Fase: %lu\n",
                   (unsigned long)(tiempo_actual_ms / 1000),
                   (unsigned long long)muestras_copia,
                   (unsigned long)(muestras_copia / N_MUESTRAS),
                   (unsigned long)fase_copia);
            fflush(stdout);
        }
    }
}