#include "octavia.h"
#include "pico/stdlib.h"
#include "synth.h"
#include "drivers/buttons_leds.h"

#define SIN_NOTA 255u
#define NUM_NOTAS 7

static uint8_t nota_sonando = SIN_NOTA;
static uint8_t octava_de_nota[NUM_NOTAS];

void octavia_iniciar(void){
    stdio_init_all();
    bl_iniciar();
    synth_init();
}

static int8_t nota_mas_aguda(uint8_t mascara)
{
    for (int8_t i = NUM_NOTAS - 1; i >= 0; i--) {
        if ((mascara >> i) & 1u) {
            return i;
        }
    }
    return -1;
}

void octavia_ejecutar(void){
    bl_actualizar();

    uint8_t presionadas = 0;
    uint8_t liberadas = 0;
    bl_notas_tomar_eventos(&presionadas, &liberadas);
    uint8_t sostenidas = bl_notas_presionadas();

    /* Cada nota captura la octava vigente en el instante de su pulsación */
    for (uint8_t i = 0; i < NUM_NOTAS; i++) {
        if ((presionadas >> i) & 1u) {
            octava_de_nota[i] = bl_octava_actual();
        }
    }

    /* Primero las liberaciones */
    if (liberadas) {
        if (sostenidas == 0) {
            nota_sonando = SIN_NOTA;
            synth_liberar_nota();
        } else if (nota_sonando != SIN_NOTA && ((liberadas >> nota_sonando) & 1u)) {
            int8_t n = nota_mas_aguda(sostenidas);
            if (n >= 0) {
                nota_sonando = (uint8_t)n;
                synth_tocar_nota(octava_de_nota[n], nota_sonando);
            }
        }
    }

    /* Después las pulsaciones nuevas */
    if (presionadas) {
        int8_t n = nota_mas_aguda(presionadas);
        if (n >= 0) {
            nota_sonando = (uint8_t)n;
            synth_tocar_nota(octava_de_nota[n], nota_sonando);
        }
    }

    sleep_ms(1);
}