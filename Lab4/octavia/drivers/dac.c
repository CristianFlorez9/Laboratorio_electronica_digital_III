/**
 * @file dac.c
 * @brief Implementación del driver del DAC R-2R de 8 bits.
 *
 * Los 8 GPIO del DAC se manejan con operaciones enmascaradas sobre el banco
 * de GPIO, de modo que todos los bits de la muestra cambian en una sola
 * escritura. Las funciones públicas están documentadas en dac.h.
 */

#include "dac.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"

/** @brief Máscara de los GPIO del DAC dentro del banco de GPIO (bits DAC_PIN_BASE en adelante). */
#define DAC_MASCARA (((1u << DAC_BITS) - 1u) << DAC_PIN_BASE)

void dac_init(void)
{
    gpio_init_mask(DAC_MASCARA);
    gpio_set_dir_out_masked(DAC_MASCARA);
    gpio_put_masked(DAC_MASCARA, (uint32_t)DAC_CENTRO << DAC_PIN_BASE);
}

void dac_escribir(uint8_t muestra)
{
    gpio_put_masked(DAC_MASCARA, (uint32_t)muestra << DAC_PIN_BASE);
}