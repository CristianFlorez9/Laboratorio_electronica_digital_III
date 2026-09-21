/**
 * @file Juego.h
 * @brief Declaración del núcleo de la lógica, máquina de estados y control del juego Simon Says.
 * @details Este módulo gestiona las reglas del juego, la secuencia aleatoria, la validación
 *          de las entradas del usuario, el seguimiento del tiempo consumido, el nivel y las vidas.
 */

#ifndef JUEGO_H
#define JUEGO_H

#include <Arduino.h>

/**
 * @enum EstadoJuego
 * @brief Define los estados posibles de la máquina de estados del juego.
 */
enum EstadoJuego {
  INICIO,          /**< Estado de reposo o espera a que se presione Start. */
  PRESENTACION,    /**< Muestra la secuencia aleatoria de LEDs correspondiente al nivel actual. */
  INPUTS,          /**< Espera y valida la respuesta del usuario a través de los botones. */
  NIVEL_COMPLETO,  /**< Pausa de éxito tras replicar correctamente la secuencia actual. */
  NIVEL_FALLIDO,   /**< Pausa de fallo cuando el usuario se equivoca o se agota el tiempo. */
  FIN              /**< Fin de la partida por perder todas las vidas o completar el juego. */
};

/**
 * @enum ResultadoBoton
 * @brief Define el resultado obtenido tras procesar una entrada de botón durante el juego.
 */
enum ResultadoBoton {
  CORRECTO_CONTINUA,       /**< El botón fue correcto, pero faltan más pasos en la secuencia. */
  CORRECTO_NIVEL_COMPLETO, /**< El botón fue correcto y se completó exitosamente toda la secuencia del nivel. */
  INCORRECTO               /**< El botón presionado no coincidía con el esperado en la secuencia. */
};

/**
 * @brief Reinicia completamente el sistema del juego a su estado de reposo inicial.
 * @details Restablece variables de juego como nivel, vidas, tiempos acumulados y posiciona 
 *          la máquina de estados en `INICIO`.
 */
void inicio_reset();

/**
 * @brief Obtiene el estado actual en el que se encuentra la máquina de estados del juego.
 * @return EstadoJuego El estado actual (`INICIO`, `PRESENTACION`, `INPUTS`, etc.).
 */
EstadoJuego verificacion_estado();

/**
 * @brief Controla las transiciones automáticas de estado basadas en el tiempo transcurrido.
 * @details Transita entre estados como `PRESENTACION` -> `INPUTS` o `NIVEL_COMPLETO` -> `PRESENTACION`
 *          una vez que se cumplen los tiempos preestablecidos.
 * @param tiempo_transcurrido Estampa de tiempo actual del sistema en milisegundos (`millis()`).
 */
void cambio_estado_tiempo(
  unsigned long tiempo_transcurrido
);

/**
 * @brief Evalúa la pulsación de un botón recibida durante el estado de entrada de usuario (`INPUTS`).
 * @details Compara el índice del botón presionado con el elemento correspondiente de la secuencia aleatoria,
 *          gestiona aciertos, desaciertos y acumula el tiempo transcurrido en el intento.
 * @param boton_presionado Índice del botón que activó el jugador (0 a 3).
 * @param tiempo_transcurrido Estampa de tiempo actual del sistema en milisegundos (`millis()`).
 * @return ResultadoBoton Indica el resultado de la pulsación (`CORRECTO_CONTINUA`, `CORRECTO_NIVEL_COMPLETO`, o `INCORRECTO`).
 */
ResultadoBoton procesar_boton(
  int boton_presionado,
  unsigned long tiempo_transcurrido
);

/**
 * @brief Arranca una nueva partida del juego a partir de una interacción del usuario.
 * @details Prepara la primera secuencia, reinicia variables de juego y cambia el estado a `PRESENTACION`.
 * @param tiempo_transcurrido Estampa de tiempo actual del sistema en milisegundos (`millis()`).
 */
void iniciar_partida(
  unsigned long tiempo_transcurrido
);

/**
 * @brief Calcula el porcentaje de tiempo restante del límite permitido para el turno actual.
 * @details Se utiliza principalmente para ajustar la frecuencia de parpadeo del LED de pálpito.
 * @param tiempo_transcurrido Estampa de tiempo actual del sistema en milisegundos (`millis()`).
 * @return int Porcentaje de tiempo restante (entre 0 y 100).
 */
int obtener_porcentaje_tiempo(
  unsigned long tiempo_transcurrido
);

/**
 * @brief Obtiene el nivel actual en el que se encuentra el jugador.
 * @return int Número del nivel actual (por ejemplo, de 1 en adelante).
 */
int obtener_nivel();

/**
 * @brief Obtiene la cantidad de vidas restantes del jugador.
 * @return int Número de vidas actual.
 */
int obtener_vidas();

/**
 * @brief Obtiene el tiempo total acumulado que le ha tomado al jugador responder durante el juego.
 * @return int Tiempo acumulado expresado en segundos.
 */
int obtener_tiempo_acumulado();

/**
 * @brief Determina qué LED debe estar encendido durante la fase de presentación de la secuencia.
 * @details Calcula cuál paso de la secuencia debe mostrarse según el tiempo transcurrido desde el inicio del nivel.
 * @param tiempo_transcurrido Estampa de tiempo actual del sistema en milisegundos (`millis()`).
 * @return int Índice del LED a encender (0 a 3), o -1 si el tiempo corresponde a un espacio de apagado (pausa entre notas).
 */
int obtener_led_actual_presentacion(
  unsigned long tiempo_transcurrido
);

#endif
