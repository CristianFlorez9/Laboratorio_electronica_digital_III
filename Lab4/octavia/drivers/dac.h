#ifndef DAC_H_
#define DAC_H_

#include <stdint.h>

/** Resolucion del DAC R-2R en bits. */
#define DAC_BITS 8u
/** GPIO del bit menos significativo; los bits siguientes ocupan GPIO consecutivos. */
#ifndef DAC_PIN_BASE
#define DAC_PIN_BASE 8u
#endif
/** Valor central (salida en reposo, sin componente continua despues del acople). */
#define DAC_CENTRO 128u

/**
 * @brief Configura los GPIO del DAC como salidas y pone la salida en el valor central.
 * @note Llamar una sola vez antes de dac_escribir().
 */
void dac_init(void);

/**
 * @brief Envia una muestra al DAC en una sola escritura atomica de los 8 pines.
 * @param muestra Valor 0..255 (0 = 0 V, 255 = Vref * 255/256).
 * @note Es segura desde una ISR: no bloquea y no usa estructuras compartidas.
 */
void dac_escribir(uint8_t muestra);

#endif /* DAC_H_ */