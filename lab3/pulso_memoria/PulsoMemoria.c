/**
 * @file PulsoMemoria.c
 * @brief Archivo principal de control y máquina de estados del juego Simon Says.
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "Dispositivos.h"
#include "Juego.h"

/**
 * @brief Punto de entrada del firmware.
 * @details Inicializa periféricos y el estado del juego, y luego entra en un ciclo principal
 *          infinito que en cada vuelta: 1) resuelve las transiciones de estado dependientes del
 *          tiempo (cambio_estado_tiempo()), 2) procesa el botón START (inicio de partida o
 *          reinicio sostenido), 3) procesa los botones de juego mientras se está en el estado
 *          INPUTS, 4) despacha la animación de LEDs correspondiente al estado actual, y
 *          5) refresca el display de 7 segmentos con el nivel, las vidas y el tiempo acumulado.
 * @return No retorna en condiciones normales (el ciclo principal es infinito); el valor de
 *         retorno 0 solo existe para cumplir con la firma estándar de main().
 */
int main(void) {
  stdio_init_all();

  inicializar_dispositivos();
  inicio_reset();

  while (true) {

    uint32_t tiempo_actual = to_ms_since_boot(get_absolute_time());

    // Control de transiciones de estado dependientes de temporización
    cambio_estado_tiempo(tiempo_actual);

    // Lectura y procesamiento del botón START
    int evento_start = leer_boton_start(tiempo_actual);
    if (evento_start == 1) {
      if (verificacion_estado() == INICIO || verificacion_estado() == FIN) {
        iniciar_partida(tiempo_actual);
      }
    } else if (evento_start == 2) {
      inicio_reset();
    }

    EstadoJuego estado = verificacion_estado();

    if (estado == INPUTS) {
      int boton_juego = leer_boton_juego();
      if (boton_juego != -1) {
        procesar_boton(boton_juego, tiempo_actual);
      }
    }

    // Manejo de señales visuales y animaciones según el estado
    switch (estado) {
      case INICIO:
        apagar_todos_leds();
        break;

      case PRESENTACION: {
        int led_activo = obtener_led_actual_presentacion(tiempo_actual);
        for (int i = 0; i < 4; i++) {
          encender_led_secuencia(i, i == led_activo);
        }
        break;
      }

      case INPUTS: {
        float porcentaje = (float)obtener_porcentaje_tiempo(tiempo_actual) / 100.0f;
        actualizar_palpito(porcentaje, tiempo_actual);
        break;
      }

      case NIVEL_COMPLETO:
        if (obtener_nivel() >= 9) {
          animacion_victoria_nivel_9(tiempo_actual);
        } else {
          animacion_secuencia_correcta(tiempo_actual);
        }
        break;

      case NIVEL_FALLIDO:
        animacion_entrada_incorrecta(tiempo_actual);
        break;

      case FIN:
        if (obtener_nivel() >= 9 && obtener_vidas() > 0) {
          animacion_victoria_nivel_9(tiempo_actual);
        } else {
          animacion_game_over(tiempo_actual);
        }
        break;

      default:
        apagar_todos_leds();
        break;
    }

    // Multiplexado de displays (Nivel, Vidas y Tiempo acumulado)
    mostrar_en_displays(
      obtener_nivel(),
      obtener_vidas(),
      obtener_tiempo_acumulado(),
      tiempo_actual
    );
  }

  return 0;
}