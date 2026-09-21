/**
 * @file PulsoMemoria.ino
 * @brief Archivo principal de control y máquina de estados del juego Simon Says.
 */

#include "Dispositivos.h"
#include "Juego.h"

void setup() {
  Serial.begin(115200);
  inicializar_dispositivos();
  inicio_reset();
}

void loop() {
  
  unsigned long tiempo_actual = millis();

  // Control de transiciones de estado dependientes de temporización
  cambio_estado_tiempo(tiempo_actual);

  // Lectura y procesamiento de entradas de botones
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

  // 4. Multiplexado de displays (Nivel, Vidas y Tiempo acumulado)
  mostrar_en_displays(
    obtener_nivel(),
    obtener_vidas(),
    obtener_tiempo_acumulado(),
    tiempo_actual
  );
}