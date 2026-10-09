/**
 * @file Juego.c
 * @brief Implementación del núcleo de la máquina de estados y reglas del juego Simon Says.
 * @details Puerto a C / Raspberry Pi Pico SDK: random() se sustituye por get_rand_32()
 *          (pico/rand.h) y Serial.print/println por printf.
 */

#include "Juego.h"
#include "medicion.h"
#include <stdio.h>
#include <inttypes.h>
#include "pico/rand.h"

/** @brief Estado actual de la máquina de estados del juego. */
static EstadoJuego estado_actual = INICIO;

/** @brief Último resultado producido al procesar una pulsación de botón. */
static ResultadoBoton estado_botones = CORRECTO_CONTINUA;

/** @brief Secuencia aleatoria de LEDs de la partida (índices 0-3), hasta 9 elementos (nivel máximo). */
static int secuencia_leds[9];

/** @brief Nivel actual de la partida (1 a 9). */
static int nivel_actual = 1;

/** @brief Vidas restantes del jugador (0 a 3). */
static int vidas = 3;

/** @brief Cantidad de botones ya acertados correctamente en la secuencia del nivel actual. */
static int contador_botones = 0;

/**
 * @brief Tiempo total acumulado de respuesta, en segundos, listo para mostrar en el display.
 * @details Se deriva de #tiempo_acumulado_ms en cada llamada a acumular_tiempo(), redondeando
 *          al segundo más cercano y topado en 99.
 */
static int tiempo_acumulado = 0;

/**
 * @brief Acumulador auxiliar del tiempo de respuesta, en milisegundos exactos.
 * @details Guarda la suma exacta de todos los tiempos de respuesta del jugador, SIN truncar
 *          en cada nivel. #tiempo_acumulado (en segundos) se recalcula desde este total
 *          completo en cada llamada a acumular_tiempo(), de modo que el truncamiento/redondeo
 *          por división entera ocurre una sola vez sobre el acumulado global y no una vez por
 *          cada nivel, evitando que se pierdan milisegundos ronda a ronda.
 */
static uint32_t tiempo_acumulado_ms = 0;

/** @brief Duración total (ms) de la fase de presentación de la secuencia del nivel actual. */
static uint32_t tiempo_presentacion_actual = 0;

/** @brief Tiempo total (ms) disponible para que el usuario responda la secuencia del nivel actual. */
static uint32_t tiempo_ingreso = 0;

/** @brief Estampa de tiempo (ms) en la que comenzó la ventana de entrada del usuario (estado INPUTS). */
static uint32_t aux_tiempo_ingreso = 0;

/** @brief Estampa de tiempo (ms) en la que comenzó la fase de presentación de la secuencia actual. */
static uint32_t aux_tiempo_presentacion = 0;

/** @brief Estampa de tiempo (ms) en la que el usuario recibió el turno para empezar a responder. */
static uint32_t inicio_respuesta = 0;

/** @brief Tiempo (ms) que tardó el usuario en su última respuesta (correcta, incorrecta o agotada). */
static uint32_t tiempo_respuesta_usuario = 0;

/** @brief Estampa de tiempo (ms) en la que se entró al estado NIVEL_COMPLETO (inicio de la pausa de éxito). */
static uint32_t tiempo_nivel_completo = 0;

/** @brief Estampa de tiempo (ms) en la que se entró al estado NIVEL_FALLIDO (inicio de la pausa de fallo). */
static uint32_t tiempo_nivel_fallido = 0;

/** @brief Duración (ms) de la pausa de éxito antes de avanzar al siguiente nivel. */
static const uint32_t DURACION_VICTORIA = 1000;

/** @brief Duración (ms) de la pausa de fallo antes de reintentar el nivel o terminar la partida. */
static const uint32_t DURACION_FALLO = 450;

// --- Medición de tiempos de ejecución (ver medicion.h) ---

/** @brief Duración (us) de la última llamada a generar_siguiente_elemento(). */
static uint32_t tiempo_generar_siguiente_us = 0;

/** @brief Estadísticas de cambio_estado_tiempo() en llamadas donde NO hubo transición de estado. */
static MedEstadistica med_cambio_estado = MED_ESTADISTICA_INICIAL;

/**
 * @brief Genera de forma aleatoria el siguiente LED de la secuencia.
 * @details Escribe en secuencia_leds[nivel_actual - 1] un índice de LED (0-3) obtenido con
 *          get_rand_32(). No hace nada si nivel_actual queda fuera del rango del arreglo.
 *          Mide su duración en #tiempo_generar_siguiente_us sin imprimir: el reporte lo hace
 *          reportar_generar_siguiente() desde fuera, para que el printf no infle el tiempo de
 *          las funciones que la llaman.
 */
static void generar_siguiente_elemento(void) {
  uint32_t t0 = time_us_32();

  int indice = nivel_actual - 1;

  if (indice >= 0 && indice < 9) {
    secuencia_leds[indice] = (int)(get_rand_32() % 4);
  }

  tiempo_generar_siguiente_us = time_us_32() - t0;
}

/**
 * @brief Imprime la duración de la última llamada a generar_siguiente_elemento().
 */
static void reportar_generar_siguiente(void) {
  MED_PRINTF("[TIEMPO] generar_siguiente_elemento (nivel %d): %" PRIu32 " us\n",
             nivel_actual, tiempo_generar_siguiente_us);
}

/**
 * @brief Calcula la duración total de la fase de presentación
 * y el tiempo disponible para que el usuario responda.
 * @details La frecuencia de presentación (elementos/segundo) aumenta por tramos de nivel;
 *          el tiempo de respuesta permitido es 1.25 veces el tiempo de presentación.
 *          Además imprime por consola el resumen del nivel que comienza.
 */
static void calcular_tiempo_presentacion(void) {
  float frecuencia;

  if (nivel_actual <= 2) {
    frecuencia = 1.0f;
  } else if (nivel_actual <= 4) {
    frecuencia = 1.5f;
  } else if (nivel_actual <= 6) {
    frecuencia = 2.0f;
  } else if (nivel_actual <= 8) {
    frecuencia = 2.5f;
  } else {
    frecuencia = 3.0f;
  }

  tiempo_presentacion_actual =
      (uint32_t)((nivel_actual / frecuencia) * 1000.0f);

  tiempo_ingreso =
      (uint32_t)(1.25f * tiempo_presentacion_actual);

  printf("\n");
  printf("========================================\n");
  printf("NIVEL: %d\n", nivel_actual);

  printf("Tiempo total de presentacion: %" PRIu32 " ms\n", tiempo_presentacion_actual);
  printf("Tiempo por elemento: %" PRIu32 " ms\n", tiempo_presentacion_actual / nivel_actual);
  printf("Tiempo disponible para responder: %" PRIu32 " ms\n", tiempo_ingreso);

  printf("========================================\n");
}

/**
 * @brief Suma el tiempo consumido durante el turno actual al acumulador global.
 * @details Se acumula primero en milisegundos exactos (#tiempo_acumulado_ms) y solo al final
 *          se deriva #tiempo_acumulado (segundos) desde ese total, redondeando al segundo más
 *          cercano (sumando 500 ms antes de dividir entre 1000, para no truncar siempre hacia
 *          abajo). Así el redondeo ocurre una sola vez sobre el acumulado, no una vez por cada
 *          nivel, evitando que se pierdan milisegundos en cada ronda. El resultado se topa en
 *          99 para no desbordar el display de 7 segmentos.
 * @param tiempo_transcurrido Estampa de tiempo actual del sistema en milisegundos, tomada en el
 *        instante en que terminó el turno (acierto, error o tiempo agotado).
 */
static void acumular_tiempo(uint32_t tiempo_transcurrido) {
  uint32_t usado_ms = tiempo_transcurrido - aux_tiempo_ingreso;

  tiempo_acumulado_ms += usado_ms;

  /* Redondeo al segundo mas cercano (no truncar hacia abajo):
   * sumar 500 ms antes de dividir entre 1000. Ej: 9952 ms -> 10 s. */
  tiempo_acumulado = (int)((tiempo_acumulado_ms + 500) / 1000);

  if (tiempo_acumulado > 99) {
    tiempo_acumulado = 99;
  }
}

void inicio_reset(void) {
  estado_actual = INICIO;
  nivel_actual = 1;
  vidas = 3;
  contador_botones = 0;
  tiempo_acumulado = 0;
  tiempo_acumulado_ms = 0;

  tiempo_presentacion_actual = 0;
  tiempo_ingreso = 0;

  aux_tiempo_ingreso = 0;
  aux_tiempo_presentacion = 0;

  inicio_respuesta = 0;
  tiempo_respuesta_usuario = 0;

  tiempo_nivel_completo = 0;
  tiempo_nivel_fallido = 0;
}

EstadoJuego verificacion_estado(void) {
  return estado_actual;
}

/**
 * @brief Nombre legible de un estado, solo para los mensajes de medición.
 * @param e Estado a nombrar.
 * @return Cadena constante con el nombre del estado.
 */
static const char *nombre_estado(EstadoJuego e) {
  switch (e) {
    case INICIO:         return "INICIO";
    case PRESENTACION:   return "PRESENTACION";
    case INPUTS:         return "INPUTS";
    case NIVEL_COMPLETO: return "NIVEL_COMPLETO";
    case NIVEL_FALLIDO:  return "NIVEL_FALLIDO";
    case FIN:            return "FIN";
  }

  return "?";
}

/**
 * @brief Cuerpo original de cambio_estado_tiempo(): resuelve las transiciones que dependen del tiempo.
 * @param tiempo_transcurrido Estampa de tiempo actual del sistema en milisegundos.
 */
static void evaluar_transiciones(uint32_t tiempo_transcurrido) {
  if (estado_actual == NIVEL_COMPLETO) {
    if (tiempo_transcurrido - tiempo_nivel_completo >= DURACION_VICTORIA) {
      nivel_actual++;

      calcular_tiempo_presentacion();
      generar_siguiente_elemento();

      aux_tiempo_presentacion = tiempo_transcurrido;
      estado_actual = PRESENTACION;
    }

    return;
  }

  if (estado_actual == NIVEL_FALLIDO) {
    if (tiempo_transcurrido - tiempo_nivel_fallido >= DURACION_FALLO) {
      if (vidas <= 0) {
        printf("\n");
        printf("========================================\n");
        printf("GAME OVER\n");
        printf("========================================\n");

        estado_actual = FIN;
      } else {
        aux_tiempo_presentacion = tiempo_transcurrido;
        estado_actual = PRESENTACION;
      }
    }

    return;
  }

  if (estado_actual == PRESENTACION) {
    if (tiempo_transcurrido - aux_tiempo_presentacion >= tiempo_presentacion_actual) {
      estado_actual = INPUTS;

      aux_tiempo_ingreso = tiempo_transcurrido;
      inicio_respuesta = tiempo_transcurrido;

      printf("\n");
      printf("NIVEL %d - INICIO DE RESPUESTA\n", nivel_actual);
      printf("El usuario tiene %" PRIu32 " ms para responder.\n", tiempo_ingreso);
      printf("----------------------------------------\n");
    }
  } else if (estado_actual == INPUTS) {
    if (tiempo_transcurrido - aux_tiempo_ingreso >= tiempo_ingreso) {
      tiempo_respuesta_usuario = tiempo_transcurrido - inicio_respuesta;

      printf("\n");
      printf("----------------------------------------\n");
      printf("NIVEL %d - TIEMPO AGOTADO\n", nivel_actual);
      printf("Tiempo utilizado: %" PRIu32 " ms\n", tiempo_respuesta_usuario);
      printf("Tiempo disponible: %" PRIu32 " ms\n", tiempo_ingreso);
      printf("----------------------------------------\n");

      acumular_tiempo(tiempo_transcurrido);

      contador_botones = 0;
      vidas--;

      tiempo_nivel_fallido = tiempo_transcurrido;
      estado_actual = NIVEL_FALLIDO;
    }
  }
}

void cambio_estado_tiempo(uint32_t tiempo_transcurrido) {
  EstadoJuego antes = estado_actual;

  uint32_t t0 = time_us_32();
  evaluar_transiciones(tiempo_transcurrido);
  uint32_t dt = time_us_32() - t0;

  if (estado_actual == antes) {
    med_registrar(&med_cambio_estado, dt); // llamada sin transición: solo se acumula
    return;
  }

  // Hubo transición: se reporta (fuera del intervalo medido) cuánto tardó esa llamada.
  MED_PRINTF("[TIEMPO] cambio_estado_tiempo %s -> %s: %" PRIu32 " us\n",
             nombre_estado(antes), nombre_estado(estado_actual), dt);
  med_reportar("cambio_estado_tiempo (llamadas previas sin transicion)", &med_cambio_estado);

  if (antes == NIVEL_COMPLETO) {
    reportar_generar_siguiente(); // esa transición fue la que generó el nuevo elemento
  }
}

ResultadoBoton procesar_boton(int boton_presionado, uint32_t tiempo_transcurrido) {
  if (boton_presionado == secuencia_leds[contador_botones]) {
    contador_botones++;

    if (contador_botones == nivel_actual) {
      tiempo_respuesta_usuario = tiempo_transcurrido - inicio_respuesta;

      printf("\n");
      printf("========================================\n");
      printf("NIVEL %d COMPLETADO\n", nivel_actual);
      printf("Tiempo disponible: %" PRIu32 " ms\n", tiempo_ingreso);
      printf("Tiempo del usuario: %" PRIu32 " ms\n", tiempo_respuesta_usuario);

      if (tiempo_respuesta_usuario < tiempo_ingreso) {
        printf("Tiempo restante: %" PRIu32 " ms\n", tiempo_ingreso - tiempo_respuesta_usuario);
      } else {
        printf("Tiempo restante: 0 ms\n");
      }

      printf("========================================\n");

      acumular_tiempo(tiempo_transcurrido);

      contador_botones = 0;
      estado_botones = CORRECTO_NIVEL_COMPLETO;

      if (nivel_actual == 9) {
        printf("\n");
        printf("********************************\n");
        printf("JUEGO COMPLETADO\n");
        printf("********************************\n");

        estado_actual = FIN;
      } else {
        tiempo_nivel_completo = tiempo_transcurrido;
        estado_actual = NIVEL_COMPLETO;
      }
    } else {
      estado_botones = CORRECTO_CONTINUA;
      estado_actual = INPUTS;
    }
  } else {
    tiempo_respuesta_usuario = tiempo_transcurrido - inicio_respuesta;

    printf("\n");
    printf("----------------------------------------\n");
    printf("NIVEL %d - RESPUESTA INCORRECTA\n", nivel_actual);
    printf("Tiempo hasta el error: %" PRIu32 " ms\n", tiempo_respuesta_usuario);
    printf("Tiempo disponible: %" PRIu32 " ms\n", tiempo_ingreso);
    printf("Vidas restantes: %d\n", vidas - 1);
    printf("----------------------------------------\n");

    acumular_tiempo(tiempo_transcurrido);

    contador_botones = 0;
    estado_botones = INCORRECTO;

    vidas--;

    tiempo_nivel_fallido = tiempo_transcurrido;
    estado_actual = NIVEL_FALLIDO;
  }

  return estado_botones;
}

void iniciar_partida(uint32_t tiempo_transcurrido) {
  inicio_reset();

  calcular_tiempo_presentacion();
  generar_siguiente_elemento();
  reportar_generar_siguiente();

  aux_tiempo_presentacion = tiempo_transcurrido;
  estado_actual = PRESENTACION;

  printf("\n");
  printf("########################################\n");
  printf("         NUEVA PARTIDA\n");
  printf("########################################\n");
}

int obtener_porcentaje_tiempo(uint32_t tiempo_transcurrido) {
  if (estado_actual != INPUTS || tiempo_ingreso == 0) {
    return 0;
  }

  uint32_t tiempo_pasado = tiempo_transcurrido - aux_tiempo_ingreso;

  int porcentaje = (int)((tiempo_pasado * 100UL) / tiempo_ingreso);

  if (porcentaje > 100) {
    porcentaje = 100;
  }

  return porcentaje;
}

int obtener_nivel(void) {
  return nivel_actual;
}

int obtener_vidas(void) {
  return vidas;
}

int obtener_tiempo_acumulado(void) {
  return tiempo_acumulado;
}

int obtener_led_actual_presentacion(uint32_t tiempo_transcurrido) {
  if (estado_actual != PRESENTACION) {
    return -1;
  }

  uint32_t transcurrido = tiempo_transcurrido - aux_tiempo_presentacion;
  uint32_t tiempo_por_elemento = tiempo_presentacion_actual / nivel_actual;

  if (tiempo_por_elemento == 0) {
    return -1;
  }

  int indice = (int)(transcurrido / tiempo_por_elemento);

  if (indice >= nivel_actual) {
    return -1;
  }

  uint32_t dentro_del_elemento = transcurrido % tiempo_por_elemento;
  uint32_t tiempo_encendido = (tiempo_por_elemento * 70UL) / 100UL;

  if (dentro_del_elemento < tiempo_encendido) {
    return secuencia_leds[indice];
  }

  return -1;
}
