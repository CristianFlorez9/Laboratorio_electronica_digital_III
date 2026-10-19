#ifndef OCTAVIA_H_
#define OCTAVIA_H_

/**
 * @file octavia.h
 * @brief Interfaz de la capa de aplicación del instrumento Octavia.
 *
 * Coordina el driver de botones y LEDs (buttons_leds) con el sintetizador
 * (synth). Interpreta las pulsaciones de las siete notas naturales y de los
 * botones de octava, y ordena al sintetizador iniciar o liberar el sonido.
 *
 * Reglas de comportamiento:
 * - Cada nota conserva la octava vigente en el instante de su pulsación,
 *   hasta que se suelta el control.
 * - Cambiar de octava solo afecta a las notas presionadas posteriormente.
 * - Con varias notas presionadas (monofónico) suena la más aguda; al soltar
 *   la que suena, se retoma la más aguda de las que siguen presionadas
 *   (legato), con su propia octava.
 */

#include <stdint.h>
#include <stdio.h>

/**
 * @brief Inicializa el instrumento.
 *
 * Configura la comunicación estándar (stdio), el driver de botones y LEDs
 * (con sus interrupciones de GPIO) y el sintetizador, que a su vez configura
 * el DAC y el temporizador SysTick.
 *
 * @pre Debe llamarse una sola vez, antes de octavia_ejecutar().
 * @post Las interrupciones de botones y la de SysTick quedan habilitadas; el
 *       instrumento inicia en silencio.
 * @note Tiene efectos secundarios sobre los GPIO de botones, LEDs y DAC, y
 *       sobre el registro SysTick y su prioridad de excepción.
 */
void octavia_iniciar(void);

/**
 * @brief Ejecuta una iteración de la lógica del instrumento.
 *
 * Actualiza el antirrebote de los botones, toma los eventos de pulsación y
 * liberación de las notas, captura la octava vigente para cada nota recién
 * presionada y ordena al sintetizador tocar o liberar la nota
 * correspondiente. Las liberaciones se procesan antes que las pulsaciones
 * nuevas.
 *
 * @pre octavia_iniciar() ya fue llamada.
 * @note No es bloqueante, salvo por una pausa de 1 ms al final de cada
 *       iteración. Debe llamarse de forma periódica desde el lazo principal.
 * @note Es la única función que consume los eventos del driver de botones
 *       (bl_notas_tomar_eventos()), por lo que no debe llamarse desde otro
 *       contexto concurrente.
 */
void octavia_ejecutar(void);

#endif /* OCTAVIA_H_ */