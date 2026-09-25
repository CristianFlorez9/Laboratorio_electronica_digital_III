/**
 * @file Dispositivos.h
 * @brief Declaración de la interfaz de control de hardware, lectura de entradas y animaciones.
 */

#ifndef DISPOSITIVOS_H
#define DISPOSITIVOS_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Configura los pines de entrada y salida, e inicializa el hardware (GPIO + interrupciones).
 */
void inicializar_dispositivos(void);

/**
 * @brief Lee el estado (ya antirrebotado) de los 4 botones de la secuencia de juego.
 * @return Índice del botón presionado (0 a 3) o -1 si no hay pulso válido nuevo.
 */
int leer_boton_juego(void);

/**
 * @brief Evalúa la lectura del botón START gestionando el tiempo de pulsación.
 * @param tiempo_actual Estampa de tiempo actual del sistema en milisegundos.
 * @return 0 = Sin pulsación, 1 = Pulsación corta, 2 = Pulsación sostenida (>= 2 s).
 */
int leer_boton_start(uint32_t tiempo_actual);

/**
 * @brief Modifica el estado de un LED específico dentro de la secuencia.
 * @param indice Identificador del LED (0 a 3).
 * @param estado true para encender, false para apagar.
 */
void encender_led_secuencia(int indice, bool estado);

/**
 * @brief Apaga todos los LEDs del juego y el LED de pálpito.
 */
void apagar_todos_leds(void);

/**
 * @brief Controla el parpadeo del LED de pálpito con aceleración cuadrática.
 * @param porcentaje_tiempo Valor flotante entre 0.0 (inicio) y 1.0 (tiempo agotado).
 * @param tiempo_actual Estampa de tiempo actual en milisegundos.
 */
void actualizar_palpito(float porcentaje_tiempo, uint32_t tiempo_actual);

/**
 * @brief Actualiza la información desplegada en el display de 7 segmentos.
 * @param nivel Valor del nivel actual a mostrar.
 * @param vidas Cantidad de vidas restantes a mostrar.
 * @param tiempo Valor del tiempo acumulado a mostrar (0-99).
 * @param tiempo_actual Estampa de tiempo actual del sistema en milisegundos.
 */
void mostrar_en_displays(int nivel, int vidas, int tiempo, uint32_t tiempo_actual);

/**
 * @brief Animación de barrido cuando el jugador acierta toda la secuencia.
 * @param tiempo_actual Estampa de tiempo actual del sistema en milisegundos.
 */
void animacion_secuencia_correcta(uint32_t tiempo_actual);

/**
 * @brief Animación de parpadeo alternado cuando se presiona un botón incorrecto.
 * @param tiempo_actual Estampa de tiempo actual del sistema en milisegundos.
 */
void animacion_entrada_incorrecta(uint32_t tiempo_actual);

/**
 * @brief Animación de destello simultáneo cuando el tiempo expira.
 * @param tiempo_actual Estampa de tiempo actual del sistema en milisegundos.
 */
void animacion_tiempo_agotado(uint32_t tiempo_actual);

/**
 * @brief LEDs fijos encendidos para indicar la pérdida de una vida.
 * @param tiempo_actual Estampa de tiempo actual del sistema en milisegundos (no usada, reservada
 *        para mantener una firma uniforme entre todas las animaciones).
 */
void animacion_perdida_vida(uint32_t tiempo_actual);

/**
 * @brief Animación de marquesina al completar el Nivel 9 (Victoria final).
 * @param tiempo_actual Estampa de tiempo actual del sistema en milisegundos.
 */
void animacion_victoria_nivel_9(uint32_t tiempo_actual);

/**
 * @brief Animación de apagado progresivo al perder todas las vidas (Game Over).
 * @param tiempo_actual Estampa de tiempo actual del sistema en milisegundos.
 */
void animacion_game_over(uint32_t tiempo_actual);

#endif