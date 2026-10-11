#include "dac.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"

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