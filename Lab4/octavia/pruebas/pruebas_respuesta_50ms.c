#include "octavia.h"
#include "pico/stdlib.h"
#include "synth.h"
#include "drivers/buttons_leds.h"

uint8_t octava_vigente = 0;      // octava 3 por defecto
uint8_t nota_sonando = 255;      // 255 = Silencio

void octavia_iniciar(void){
    stdio_init_all();
    sleep_ms(2000);              // Tiempo para iniciar el monitor serial

    bl_iniciar();
    synth_init();
}

void octavia_ejecutar(void){
    bl_actualizar();
        
    uint8_t presionadas = 0;
    uint8_t liberadas = 0;
        
    // Extraemos eventos y estado actual
    bl_notas_tomar_eventos(&presionadas, &liberadas);

    uint8_t sostenidas = bl_notas_presionadas();

    // REGLA A: ¿Se presionó una tecla nueva?
    if (presionadas > 0) {

        /* Tomamos el tiempo exacto en que detectamos la pulsación */
        absolute_time_t tiempo_inicio = get_absolute_time();
        
        // Solo leemos la octava del hardware si veníamos del silencio
        if (nota_sonando == 255) {
            octava_vigente = bl_octava_actual(); 
        }
            
        // Buscamos qué nota nueva se presionó (prioridad a la más aguda)
        for (int i = 6; i >= 0; i--) {
            if ((presionadas >> i) & 1u) {
                nota_sonando = i;
                synth_tocar_nota(octava_vigente, nota_sonando);

                /* Tomamos el tiempo final */
                absolute_time_t tiempo_fin = get_absolute_time();
                /* Calculamos diferencia en microsegundos y la pasamos a milisegundos */
                int64_t latencia_us = absolute_time_diff_us(tiempo_inicio, tiempo_fin);
                printf("Tiempo de ataque: %.3f ms\n", latencia_us / 1000.0);

                break;
            }
        }
    } 
    // REGLA B: ¿Se soltó alguna tecla?
    else if (liberadas > 0) {
        
        if (sostenidas == 0) {
            // Ya no hay ningún dedo en el teclado -> Silencio
            nota_sonando = 255;
            synth_liberar_nota();
        } else {
            // LEGATO: Cuando habián varias teclas presionadas y se suelta una
            // busca en las que quedan presionadas y es la que suena

            /* Tomamos el tiempo exacto en que detectamos el inicio del legato */
            absolute_time_t tiempo_inicio = get_absolute_time();

            for (int i = 6; i >= 0; i--) {
                if ((sostenidas >> i) & 1u) {
                    nota_sonando = i;
                    synth_tocar_nota(octava_vigente, nota_sonando);

                    /* Tomamos el tiempo final */
                    absolute_time_t tiempo_fin = get_absolute_time();
                    int64_t latencia_us = absolute_time_diff_us(tiempo_inicio, tiempo_fin);
                    printf("Tiempo de legato: %.3f ms\n", latencia_us / 1000.0);

                    break; 
                }
            }
        }
    }

    // Pequeña pausa para no saturar el procesador
    sleep_ms(1);
}