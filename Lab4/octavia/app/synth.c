/**
 * @file synth.c
 * @brief Implementación del sintetizador de Octavia.
 *
 * En cada interrupción del SysTick se avanza el acumulador de fase, se lee la
 * muestra de la tabla de ondas, se escala con la envolvente y se escribe al
 * DAC. La frecuencia de la nota la fija el incremento de fase.
 */

#include "synth.h"
#include "wavetable.h"
#include "notes.h"
#include "drivers/systick.h"
#include "drivers/dac.h"
#include "hardware/exception.h"
#include "hardware/clocks.h"

/** @brief Frecuencia de muestreo objetivo del DAC, en Hz. */
#define F_MUESTREO 40000u

/** @brief Nivel de envolvente a plena amplitud (escala de 0 a 256). */
#define NIVEL_MAXIMO 256u

/**
 * @brief Acumulador de fase de 32 bits.
 *
 * Los 8 bits más altos indexan la tabla de ondas. Se actualiza en la ISR del
 * SysTick y se reinicia en synth_tocar_nota().
 */
volatile uint32_t acumulador_fase = 0;

/**
 * @brief Incremento de fase por muestra.
 *
 * Determina la frecuencia de la nota: f = incremento * F_MUESTREO / 2^32.
 */
volatile uint32_t incremento_fase = 0;

/**
 * @brief Nivel de la envolvente (0 = silencio, NIVEL_MAXIMO = amplitud plena).
 */
volatile uint32_t nivel_envolvente = 0;

/**
 * @brief Rutina de servicio del SysTick: genera una muestra de audio.
 *
 * Avanza el acumulador de fase, lee la muestra de la tabla de ondas (256
 * entradas, 8 bits sin signo), la centra en cero, la escala por la
 * envolvente, la vuelve a centrar en 128 y la escribe al DAC.
 *
 * @note Se ejecuta a F_MUESTREO. Debe ser breve y sin llamadas bloqueantes.
 */
void isr_systick(void);

/**
 * @brief Inicializa el sintetizador.
 *
 * Calcula el valor de recarga del SysTick con redondeo al entero más cercano:
 * reload = f_cpu / F_MUESTREO - 1, y la frecuencia de muestreo real
 * f_real = f_cpu / (reload + 1), que se usa para calcular los incrementos de
 * fase de las notas y reducir el error de afinación. Luego inicializa el DAC,
 * registra la ISR y arranca el SysTick con el driver propio.
 *
 * @pre El reloj clk_sys ya está configurado.
 * @post El SysTick interrumpe a la frecuencia de muestreo y la salida queda
 *       en silencio.
 */
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

/**
 * @brief Inicia la reproducción de una nota.
 *
 * @param octava Índice de octava (0 a 2; 0 = octava 3, 1 = octava 4,
 *               2 = octava 5). Se satura a 2.
 * @param nota   Índice de nota natural (0 = do ... 6 = si). Se satura a 6.
 *
 * @note Reinicia la fase y fija la envolvente al máximo. Escribe variables
 *       compartidas con la ISR sin sección crítica.
 */
void synth_tocar_nota(uint8_t octava, uint8_t nota) {
    incremento_fase = notes_get_incremento(octava, nota);
    acumulador_fase = 0;
    nivel_envolvente = NIVEL_MAXIMO;
}

/**
 * @brief Libera la nota que suena llevando la envolvente a cero.
 *
 * @note La salida del DAC queda en el nivel central (128).
 */
void synth_liberar_nota(void) {
    nivel_envolvente = 0;
}