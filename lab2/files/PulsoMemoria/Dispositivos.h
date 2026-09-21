/**
 * @file Dispositivos.h
 * @brief Declaración de la interfaz de control de hardware, lectura de entradas y animaciones visuales.
 */

#ifndef DISPOSITIVOS_H
#define DISPOSITIVOS_H

#include <Arduino.h>

/**
 * @brief Configura los pines de entrada y salida, e inicializa el hardware.
 */
void inicializar_dispositivos();

/**
 * @brief Lee el estado de los botones de la secuencia de juego.
 * @return Índice del botón presionado (0 a 3) o -1 si ninguno está presionado.
 */
int leer_boton_juego();

/**
 * @brief Evalúa la lectura del botón START gestionando el tiempo de pulsación.
 * @param tiempo_actual Estampa de tiempo actual del sistema en milisegundos (millis()).
 * @return 0 = Sin pulsación, 1 = Pulsación corta, 2 = Pulsación sostenida (>= 2 s).
 */
int leer_boton_start(unsigned long tiempo_actual);

/**
 * @brief Modifica el estado de un LED específico dentro de la secuencia.
 * @param indice Identificador del LED (0 a 3).
 * @param estado `true` para encender, `false` para apagar.
 */
void encender_led_secuencia(int indice, bool estado);

/**
 * @brief Apaga todos los LEDs del juego y el LED de pálpito.
 */
void apagar_todos_leds();

/**
 * @brief Controla el parpadeo del LED de pálpito con aceleración cuadrática.
 * @param porcentaje_tiempo Valor flotante entre 0.0 (inicio) y 1.0 (tiempo agotado).
 * @param tiempo_actual Estampa de tiempo actual en milisegundos (millis()).
 */
void actualizar_palpito(float porcentaje_tiempo, unsigned long tiempo_actual);

/**
 * @brief Actualiza la información desplegada en el display de 7 segmentos.
 * @param nivel Valor del nivel actual.
 * @param vidas Cantidad de vidas restantes.
 * @param tiempo Tiempo acumulado.
 * @param tiempo_actual Estampa de tiempo actual en milisegundos (millis()).
 */
void mostrar_en_displays(int nivel, int vidas, int tiempo, unsigned long tiempo_actual);

/**
 * @brief Executa la animación de barrido cuando el jugador acierta toda la secuencia.
 * @param tiempo_actual Estampa de tiempo actual en milisegundos (millis()).
 */
void animacion_secuencia_correcta(unsigned long tiempo_actual);

/**
 * @brief Executa la animación de parpadeo alternado cuando se presiona un botón incorrecto.
 * @param tiempo_actual Estampa de tiempo actual en milisegundos (millis()).
 */
void animacion_entrada_incorrecta(unsigned long tiempo_actual);

/**
 * @brief Executa la animación de destello simultáneo cuando el tiempo expira.
 * @param tiempo_actual Estampa de tiempo actual en milisegundos (millis()).
 */
void animacion_tiempo_agotado(unsigned long tiempo_actual);

/**
 * @brief Enciende todos los LEDs de forma fija para indicar la pérdida de una vida.
 * @param tiempo_actual Estampa de tiempo actual en milisegundos (millis()).
 */
void animacion_perdida_vida(unsigned long tiempo_actual);

/**
 * @brief Executa la animación de marquesina al completar el Nivel 9 (Victoria final).
 * @param tiempo_actual Estampa de tiempo actual en milisegundos (millis()).
 */
void animacion_victoria_nivel_9(unsigned long tiempo_actual);

/**
 * @brief Executa la animación de apagado progresivo al perder todas las vidas (Game Over).
 * @param tiempo_actual Estampa de tiempo actual en milisegundos (millis()).
 */
void animacion_game_over(unsigned long tiempo_actual);

#endif