#include <stdio.h>
#include "octavia.h"
#include "pico/stdlib.h"
#include "synth.h"
#include "drivers/buttons_leds.h"

uint8_t octava_vigente = 0;
uint8_t nota_sonando = 255;      // 255 = silencio

void octavia_iniciar(void){
    stdio_init_all();
    sleep_ms(2000);              // tiempo para abrir el monitor serie

    bl_iniciar();
    synth_init();
}

/* Devuelve el instante de flanco más reciente entre las notas de la máscara. */
static uint32_t flanco_mas_reciente_us(uint8_t mascara)
{
    uint32_t mejor = 0;
    bool hay = false;

    for (uint8_t i = 0; i < BL_NUM_NOTAS; i++) {
        if (!((mascara >> i) & 1u)) {
            continue;
        }
        uint32_t t = bl_tiempo_evento_us(i);
        if (!hay || (int32_t)(t - mejor) > 0) {
            mejor = t;
            hay = true;
        }
    }
    return mejor;
}

static void imprimir_latencia(const char *etiqueta, uint32_t t_flanco,
                              uint32_t t_deteccion, uint32_t t_fin)
{
    printf("%s | flanco->deteccion: %.3f ms | deteccion->sonido: %.3f ms | total: %.3f ms\n",
           etiqueta,
           (uint32_t)(t_deteccion - t_flanco) / 1000.0,
           (uint32_t)(t_fin - t_deteccion) / 1000.0,
           (uint32_t)(t_fin - t_flanco) / 1000.0);
}

void octavia_ejecutar(void){
    bl_actualizar();

    uint8_t presionadas = 0;
    uint8_t liberadas = 0;
    bl_notas_tomar_eventos(&presionadas, &liberadas);

    uint8_t sostenidas = bl_notas_presionadas();

    // Regla A: se presionó una tecla nueva
    if (presionadas > 0) {
        uint32_t t_deteccion = time_us_32();
        uint32_t t_flanco = flanco_mas_reciente_us(presionadas);

        // Solo se lee la octava del hardware si veníamos del silencio
        if (nota_sonando == 255) {
            octava_vigente = bl_octava_actual();
        }

        // Prioridad a la nota más aguda
        for (int i = 6; i >= 0; i--) {
            if ((presionadas >> i) & 1u) {
                nota_sonando = i;
                synth_tocar_nota(octava_vigente, nota_sonando);
                imprimir_latencia("Ataque", t_flanco, t_deteccion, time_us_32());
                break;
            }
        }
    }
    // Regla B: se soltó alguna tecla
    else if (liberadas > 0) {
        uint32_t t_deteccion = time_us_32();
        uint32_t t_flanco = flanco_mas_reciente_us(liberadas);

        if (sostenidas == 0) {
            nota_sonando = 255;
            synth_liberar_nota();
        } else {
            // Legato: suena la más aguda de las que siguen presionadas
            for (int i = 6; i >= 0; i--) {
                if ((sostenidas >> i) & 1u) {
                    nota_sonando = i;
                    synth_tocar_nota(octava_vigente, nota_sonando);
                    imprimir_latencia("Legato", t_flanco, t_deteccion, time_us_32());
                    break;
                }
            }
        }
    }
}