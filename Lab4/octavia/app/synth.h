#ifndef SYNTH_H_
#define SYNTH_H_

/**
 * @file synth.h
 * @brief Interfaz del sintetizador de Octavia.
 *
 * Genera la nota por síntesis de tabla de ondas con acumulador de fase de
 * 32 bits. La actualización de cada muestra del DAC la temporiza el SysTick
 * (driver propio) a una frecuencia de muestreo fija.
 */

#include <stdint.h>

/**
 * @brief Inicializa el sintetizador.
 *
 * Calcula el valor de recarga del SysTick para la frecuencia de muestreo
 * objetivo, genera la tabla de incrementos de fase de las notas con la
 * frecuencia de muestreo real obtenida, inicializa el DAC, registra la ISR
 * del SysTick y arranca el temporizador.
 *
 * @pre El reloj del sistema ya está configurado (clk_sys).
 * @post El SysTick queda en marcha e interrumpiendo a la frecuencia de
 *       muestreo; la salida permanece en silencio hasta llamar a
 *       synth_tocar_nota().
 * @note Debe llamarse una sola vez, antes de synth_tocar_nota().
 */
void synth_init(void);

/**
 * @brief Inicia la reproducción de una nota.
 *
 * Fija el incremento de fase de la nota, reinicia la fase de la onda y
 * activa la envolvente al nivel máximo.
 *
 * @param octava Índice de octava (0 a 2). Con la tabla actual, 0 corresponde
 *               a la octava 3, 1 a la octava 4 y 2 a la octava 5. Los valores
 *               mayores se saturan a 2.
 * @param nota   Índice de nota natural (0 = do, 1 = re, 2 = mi, 3 = fa,
 *               4 = sol, 5 = la, 6 = si). Los valores mayores se saturan a 6.
 *
 * @note Reiniciar la fase puede producir un chasquido si la señal no está en
 *       cero en ese instante.
 * @note Modifica variables compartidas con la ISR del SysTick sin sección
 *       crítica.
 */
void synth_tocar_nota(uint8_t octava, uint8_t nota);

/**
 * @brief Libera la nota que suena.
 *
 * Lleva la envolvente a cero, de modo que la salida del DAC queda en el nivel
 * central (silencio). No detiene el SysTick, que sigue actualizando el DAC.
 *
 * @note La liberación es inmediata, sin rampa de decaimiento.
 */
void synth_liberar_nota(void);

#endif /* SYNTH_H_ */