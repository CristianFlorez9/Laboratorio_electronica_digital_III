/**
 * @file octavia.c
 * @brief Lógica de aplicación del instrumento Octavia.
 *
 * Traduce los eventos del driver de botones (notas y octava) en órdenes para
 * el sintetizador. El instrumento es monofónico: suena una sola nota a la vez,
 * con prioridad a la más aguda.
 */

#include "octavia.h"
#include "pico/stdlib.h"
#include "synth.h"
#include "drivers/buttons_leds.h"

/** @brief Valor de nota_sonando que indica silencio (ninguna nota activa). */
#define SIN_NOTA 255u

/** @brief Cantidad de notas naturales (do, re, mi, fa, sol, la, si). */
#define NUM_NOTAS 7

/**
 * @brief Índice de la nota que suena actualmente (0 = do ... 6 = si).
 *
 * Vale SIN_NOTA cuando el instrumento está en silencio.
 */
static uint8_t nota_sonando = SIN_NOTA;

/**
 * @brief Octava capturada para cada nota en el instante de su pulsación.
 *
 * Se indexa por nota (0 a 6). Cada valor es el índice de octava devuelto por
 * bl_octava_actual() cuando la nota fue presionada, y se conserva mientras la
 * nota siga presionada.
 */
static uint8_t octava_de_nota[NUM_NOTAS];

/**
 * @brief Inicializa el instrumento.
 *
 * Configura stdio, el driver de botones y LEDs (con sus interrupciones de
 * GPIO) y el sintetizador, que a su vez configura el DAC y el SysTick.
 *
 * @pre Se llama una sola vez, antes de octavia_ejecutar().
 * @post El instrumento queda en silencio con las interrupciones habilitadas.
 */
void octavia_iniciar(void){
    stdio_init_all();
    bl_iniciar();
    synth_init();
}

/**
 * @brief Busca la nota más aguda dentro de una máscara de notas.
 *
 * @param mascara Máscara de bits donde el bit i (0 a 6) indica que la nota i
 *                está activa.
 * @return Índice de la nota más aguda activa (0 a 6), o -1 si la máscara no
 *         tiene ninguna nota activa.
 */
static int8_t nota_mas_aguda(uint8_t mascara)
{
    for (int8_t i = NUM_NOTAS - 1; i >= 0; i--) {
        if ((mascara >> i) & 1u) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Ejecuta una iteración de la lógica del instrumento.
 *
 * Pasos de cada iteración:
 * -# Actualiza el antirrebote del driver de botones.
 * -# Toma los eventos de pulsación y liberación de las notas.
 * -# Captura la octava vigente para cada nota recién presionada.
 * -# Procesa las liberaciones: si no queda ninguna nota presionada, libera el
 *    sonido; si se soltó la nota que sonaba y quedan otras, retoma la más
 *    aguda de las restantes con su propia octava (legato).
 * -# Procesa las pulsaciones nuevas: toca la más aguda de las recién
 *    presionadas con la octava capturada.
 *
 * Las liberaciones se procesan antes que las pulsaciones para que una
 * liberación coincidente con una pulsación en la misma iteración no se ignore.
 *
 * @pre octavia_iniciar() ya fue llamada.
 * @note Bloquea 1 ms al final de cada iteración (sleep_ms). Debe llamarse
 *       periódicamente desde el lazo principal.
 * @note Consume los eventos del driver mediante bl_notas_tomar_eventos(), por
 *       lo que solo debe llamarse desde un único contexto.
 */
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