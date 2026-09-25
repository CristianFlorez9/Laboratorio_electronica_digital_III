/**
 * @file Juego.h
 * @brief Declaración del núcleo de la lógica, máquina de estados y control del juego Simon Says.
 * @details Este módulo gestiona las reglas del juego, la secuencia aleatoria, la validación
 *          de las entradas del usuario, el seguimiento del tiempo consumido, el nivel y las vidas.
 *
 * Puerto a C / Raspberry Pi Pico SDK: sin dependencias de Arduino.h. Los tiempos se manejan
 * en milisegundos con uint32_t (equivalente al unsigned long / millis() del original).
 */

#ifndef JUEGO_H
#define JUEGO_H

#include <stdint.h>

/**
 * @brief Estados posibles de la máquina de estados del juego.
 */
typedef enum {
  INICIO,          /**< Estado de reposo o espera a que se presione Start. */
  PRESENTACION,    /**< Muestra la secuencia aleatoria de LEDs correspondiente al nivel actual. */
  INPUTS,          /**< Espera y valida la respuesta del usuario a través de los botones. */
  NIVEL_COMPLETO,  /**< Pausa de éxito tras replicar correctamente la secuencia actual. */
  NIVEL_FALLIDO,   /**< Pausa de fallo cuando el usuario se equivoca o se agota el tiempo. */
  FIN              /**< Fin de la partida por perder todas las vidas o completar el juego. */
} EstadoJuego;

/**
 * @brief Resultado de procesar una entrada de botón durante el juego.
 */
typedef enum {
  CORRECTO_CONTINUA,       /**< El botón fue correcto, pero faltan más pasos en la secuencia. */
  CORRECTO_NIVEL_COMPLETO, /**< El botón fue correcto y se completó toda la secuencia del nivel. */
  INCORRECTO               /**< El botón presionado no coincidía con el esperado en la secuencia. */
} ResultadoBoton;

/**
 * @brief Reinicia completamente el sistema del juego a su estado de reposo inicial.
 * @details Restablece nivel, vidas, contadores y todos los acumuladores de tiempo
 *          (incluyendo el auxiliar en milisegundos) a sus valores por defecto.
 */
void inicio_reset(void);

/**
 * @brief Obtiene el estado actual en el que se encuentra la máquina de estados del juego.
 * @return Estado actual (ver #EstadoJuego).
 */
EstadoJuego verificacion_estado(void);

/**
 * @brief Controla las transiciones automáticas de estado basadas en el tiempo transcurrido.
 * @details Debe llamarse en cada vuelta del ciclo principal. Resuelve las transiciones que
 *          dependen únicamente del paso del tiempo: fin de la presentación de la secuencia,
 *          expiración del tiempo de respuesta del usuario, y las pausas tras completar o
 *          fallar un nivel.
 * @param tiempo_transcurrido Estampa de tiempo actual del sistema en milisegundos.
 */
void cambio_estado_tiempo(uint32_t tiempo_transcurrido);

/**
 * @brief Evalúa la pulsación de un botón recibida durante el estado de entrada de usuario (INPUTS).
 * @param boton_presionado Índice del botón que activó el jugador (0 a 3).
 * @param tiempo_transcurrido Estampa de tiempo actual del sistema en milisegundos.
 * @return Resultado de la evaluación (ver #ResultadoBoton).
 */
ResultadoBoton procesar_boton(int boton_presionado, uint32_t tiempo_transcurrido);

/**
 * @brief Arranca una nueva partida del juego a partir de una interacción del usuario.
 * @param tiempo_transcurrido Estampa de tiempo actual del sistema en milisegundos.
 */
void iniciar_partida(uint32_t tiempo_transcurrido);

/**
 * @brief Calcula el porcentaje de tiempo consumido del límite permitido para el turno actual.
 * @param tiempo_transcurrido Estampa de tiempo actual del sistema en milisegundos.
 * @return Porcentaje de tiempo consumido (0 a 100).
 */
int obtener_porcentaje_tiempo(uint32_t tiempo_transcurrido);

/**
 * @brief Nivel actual del jugador.
 * @return Número de nivel (1 a 9).
 */
int obtener_nivel(void);

/**
 * @brief Vidas restantes del jugador.
 * @return Cantidad de vidas (0 a 3).
 */
int obtener_vidas(void);

/**
 * @brief Tiempo total acumulado de respuesta, en segundos.
 * @details El valor se calcula redondeando al segundo más cercano el acumulado exacto en
 *          milisegundos (no se trunca hacia abajo), y queda topado en 99 para no desbordar
 *          el display de 7 segmentos.
 * @return Segundos acumulados (0 a 99).
 */
int obtener_tiempo_acumulado(void);

/**
 * @brief Determina qué LED debe estar encendido durante la fase de presentación de la secuencia.
 * @param tiempo_transcurrido Estampa de tiempo actual del sistema en milisegundos.
 * @return Índice del LED a encender (0 a 3), o -1 si corresponde a un espacio de apagado.
 */
int obtener_led_actual_presentacion(uint32_t tiempo_transcurrido);

#endif