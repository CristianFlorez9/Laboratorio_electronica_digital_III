#ifndef DAC_H_
#define DAC_H_

/**
 * @file dac.h
 * @brief Interfaz del driver del DAC R-2R de 8 bits de Octavia.
 *
 * El DAC es externo y de interfaz paralela: cada bit de la muestra se
 * conecta a un GPIO consecutivo de la Raspberry Pi Pico, comenzando en
 * DAC_PIN_BASE.
 */

#include <stdint.h>

/** Resolución del DAC R-2R en bits. */
#define DAC_BITS 8u

/**
 * GPIO del bit menos significativo; los bits siguientes ocupan GPIO
 * consecutivos (por defecto GPIO 8 a 15).
 */
#ifndef DAC_PIN_BASE
#define DAC_PIN_BASE 8u
#endif

/** Valor central (salida en reposo, sin componente continua después del acople). */
#define DAC_CENTRO 128u

/**
 * @brief Configura los GPIO del DAC como salidas y pone la salida en el valor central.
 * @note Llamar una sola vez antes de dac_escribir().
 */
void dac_init(void);

/**
 * @brief Envía una muestra al DAC en una sola escritura atómica de los 8 pines.
 * @param muestra Valor 0..255 (0 = 0 V, 255 = Vref * 255/256).
 * @note Es segura desde una ISR: no bloquea y no usa estructuras compartidas.
 */
void dac_escribir(uint8_t muestra);

#endif /* DAC_H_ */