#include "synth.h"
#include "wavetable.h"
#include "notes.h"
#include "drivers/systick.h"
#include "drivers/dac.h"
#include "hardware/exception.h"
#include "hardware/clocks.h"

#define F_MUESTREO 40000u
#define NIVEL_MAXIMO 256u

volatile uint32_t acumulador_fase = 0;
volatile uint32_t incremento_fase = 0;
volatile uint32_t nivel_envolvente = 0;

void isr_systick(void);

void synth_init(void) {
    uint32_t f_cpu = clock_get_hz(clk_sys);
    uint32_t reload = (f_cpu + F_MUESTREO / 2u) / F_MUESTREO - 1u;
    float f_real = (float)f_cpu / (float)(reload + 1u);

    notes_init(f_real);
    dac_init();

    exception_set_exclusive_handler(SYSTICK_EXCEPTION, isr_systick);
    systick_init(reload);
}

void isr_systick(void) {
    acumulador_fase += incremento_fase;
    uint8_t indice = acumulador_fase >> 24;

    int32_t muestra_centrada = (int32_t)tabla_onda[indice] - 128;
    muestra_centrada = (muestra_centrada * (int32_t)nivel_envolvente) >> 8;

    dac_escribir((uint8_t)(muestra_centrada + 128));
}

void synth_tocar_nota(uint8_t octava, uint8_t nota) {
    incremento_fase = notes_get_incremento(octava, nota);
    acumulador_fase = 0;
    nivel_envolvente = NIVEL_MAXIMO;
}

void synth_liberar_nota(void) {
    nivel_envolvente = 0;
}