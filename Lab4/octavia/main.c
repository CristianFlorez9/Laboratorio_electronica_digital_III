#include <math.h>
#include "pico/stdlib.h"
#include "systick.h"
#include "dac.h"

#define N_MUESTRAS 64u

static uint8_t tabla[N_MUESTRAS];     /* en el proyecto final: const, en Flash */
static volatile uint32_t fase = 0;

/* f_tick = 440 Hz * 64 = 28160 Hz  ->  reload = 125e6 / 28160 - 1 = 4438 */
void isr_systick(void)
{
    dac_escribir(tabla[fase]);
    fase = (fase + 1u) & (N_MUESTRAS - 1u);   /* da la vuelta sin division */
}

int main(void)
{
    for (uint32_t k = 0; k < N_MUESTRAS; k++) {
        tabla[k] = (uint8_t)(128.0f + 127.0f * sinf(2.0f * 3.14159265f * k / N_MUESTRAS));
    }

    dac_init();
    systick_init(4438);

    while (true) {
        tight_loop_contents();
    }
}