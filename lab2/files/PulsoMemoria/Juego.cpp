/**
 * @file Juego.cpp
 * @brief Implementación del núcleo de la máquina de estados y reglas del juego Simon Says.
 */

#include "Juego.h"
#include <Arduino.h>

static EstadoJuego estado_actual = INICIO;
static ResultadoBoton estado_botones = CORRECTO_CONTINUA;

static int secuencia_leds[9];
static int nivel_actual = 1;
static int vidas = 3;
static int contador_botones = 0;
static int tiempo_acumulado = 0;

static unsigned long tiempo_presentacion_actual = 0;
static unsigned long tiempo_ingreso = 0;
static unsigned long aux_tiempo_ingreso = 0;
static unsigned long aux_tiempo_presentacion = 0;

static unsigned long inicio_respuesta = 0;
static unsigned long tiempo_respuesta_usuario = 0;

static unsigned long tiempo_nivel_completo = 0;
static unsigned long tiempo_nivel_fallido = 0;

static const unsigned long DURACION_VICTORIA = 1000;
static const unsigned long DURACION_FALLO = 450;

/**
 * @brief Genera de forma aleatoria el siguiente LED de la secuencia.
 */
static void generar_siguiente_elemento() {
  int indice = nivel_actual - 1;

  if (indice >= 0 && indice < 9) {
    secuencia_leds[indice] = random(0,4);
  }
}

/**
 * @brief Calcula la duración total de la fase de presentación
 * y el tiempo disponible para que el usuario responda.
 */
static void calcular_tiempo_presentacion() {
  float frecuencia;

  if (nivel_actual <= 2) {
    frecuencia = 1.0;
  } else if (nivel_actual <= 4) {
    frecuencia = 1.5;
  } else if (nivel_actual <= 6) {
    frecuencia = 2.0;
  } else if (nivel_actual <= 8) {
    frecuencia = 2.5;
  } else {
    frecuencia = 3.0;
  }

  tiempo_presentacion_actual =
      (unsigned long)((nivel_actual / frecuencia) * 1000.0);

  tiempo_ingreso =
      (unsigned long)(1.25 * tiempo_presentacion_actual);

  Serial.println();
  Serial.println("========================================");
  Serial.print("NIVEL: ");
  Serial.println(nivel_actual);

  Serial.print("Tiempo total de presentacion: ");
  Serial.print(tiempo_presentacion_actual);
  Serial.println(" ms");

  Serial.print("Tiempo por elemento: ");
  Serial.print(tiempo_presentacion_actual / nivel_actual);
  Serial.println(" ms");

  Serial.print("Tiempo disponible para responder: ");
  Serial.print(tiempo_ingreso);
  Serial.println(" ms");

  Serial.println("========================================");
}

/**
 * @brief Suma el tiempo consumido durante el turno actual al acumulador global.
 */
static void acumular_tiempo(unsigned long tiempo_transcurrido) {
  unsigned long usado_ms =
      tiempo_transcurrido - aux_tiempo_ingreso;

  tiempo_acumulado += (int)(usado_ms / 1000);

  if (tiempo_acumulado > 99) {
    tiempo_acumulado = 99;
  }
}

void inicio_reset() {
  estado_actual = INICIO;
  nivel_actual = 1;
  vidas = 3;
  contador_botones = 0;
  tiempo_acumulado = 0;

  tiempo_presentacion_actual = 0;
  tiempo_ingreso = 0;

  aux_tiempo_ingreso = 0;
  aux_tiempo_presentacion = 0;

  inicio_respuesta = 0;
  tiempo_respuesta_usuario = 0;

  tiempo_nivel_completo = 0;
  tiempo_nivel_fallido = 0;
}

EstadoJuego verificacion_estado() {
  return estado_actual;
}

void cambio_estado_tiempo(unsigned long tiempo_transcurrido) {
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
        Serial.println();
        Serial.println("========================================");
        Serial.println("GAME OVER");
        Serial.println("========================================");
        acumular_tiempo(tiempo_transcurrido);


        estado_actual = FIN;
      } else {
        aux_tiempo_presentacion = tiempo_transcurrido;
        estado_actual = PRESENTACION;
      }
    }

    return;
  }

  if (estado_actual == PRESENTACION) {
    if (tiempo_transcurrido - aux_tiempo_presentacion >=
        tiempo_presentacion_actual) {

      estado_actual = INPUTS;

      aux_tiempo_ingreso = tiempo_transcurrido;
      inicio_respuesta = tiempo_transcurrido;

      Serial.println();
      Serial.print("NIVEL ");
      Serial.print(nivel_actual);
      Serial.println(" - INICIO DE RESPUESTA");

      Serial.print("El usuario tiene ");
      Serial.print(tiempo_ingreso);
      Serial.println(" ms para responder.");

      Serial.println("----------------------------------------");
    }
  } else if (estado_actual == INPUTS) {
    if (tiempo_transcurrido - aux_tiempo_ingreso >= tiempo_ingreso) {
      tiempo_respuesta_usuario =
          tiempo_transcurrido - inicio_respuesta;

      Serial.println();
      Serial.println("----------------------------------------");

      Serial.print("NIVEL ");
      Serial.print(nivel_actual);
      Serial.println(" - TIEMPO AGOTADO");

      Serial.print("Tiempo utilizado: ");
      Serial.print(tiempo_respuesta_usuario);
      Serial.println(" ms");

      Serial.print("Tiempo disponible: ");
      Serial.print(tiempo_ingreso);
      Serial.println(" ms");

      Serial.println("----------------------------------------");

      acumular_tiempo(tiempo_transcurrido);

      contador_botones = 0;
      vidas--;

      tiempo_nivel_fallido = tiempo_transcurrido;
      estado_actual = NIVEL_FALLIDO;
    }
  }
}

ResultadoBoton procesar_boton(
    int boton_presionado,
    unsigned long tiempo_transcurrido) {

  if (boton_presionado == secuencia_leds[contador_botones]) {
    contador_botones++;

    if (contador_botones == nivel_actual) {
      tiempo_respuesta_usuario =
          tiempo_transcurrido - inicio_respuesta;

      Serial.println();
      Serial.println("========================================");

      Serial.print("NIVEL ");
      Serial.print(nivel_actual);
      Serial.println(" COMPLETADO");

      Serial.print("Tiempo disponible: ");
      Serial.print(tiempo_ingreso);
      Serial.println(" ms");

      Serial.print("Tiempo del usuario: ");
      Serial.print(tiempo_respuesta_usuario);
      Serial.println(" ms");

      Serial.print("Tiempo restante: ");

      if (tiempo_respuesta_usuario < tiempo_ingreso) {
        Serial.print(
            tiempo_ingreso - tiempo_respuesta_usuario
        );
        Serial.println(" ms");
      } else {
        Serial.println("0 ms");
      }

      Serial.println("========================================");

      acumular_tiempo(tiempo_transcurrido);

      contador_botones = 0;
      estado_botones = CORRECTO_NIVEL_COMPLETO;

      if (nivel_actual == 9) {
        Serial.println();
        Serial.println("********************************");
        Serial.println("JUEGO COMPLETADO");
        Serial.println("********************************");

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
    tiempo_respuesta_usuario =
        tiempo_transcurrido - inicio_respuesta;

    Serial.println();
    Serial.println("----------------------------------------");

    Serial.print("NIVEL ");
    Serial.print(nivel_actual);
    Serial.println(" - RESPUESTA INCORRECTA");

    Serial.print("Tiempo hasta el error: ");
    Serial.print(tiempo_respuesta_usuario);
    Serial.println(" ms");

    Serial.print("Tiempo disponible: ");
    Serial.print(tiempo_ingreso);
    Serial.println(" ms");

    Serial.print("Vidas restantes: ");
    Serial.println(vidas - 1);

    Serial.println("----------------------------------------");

    acumular_tiempo(tiempo_transcurrido);

    contador_botones = 0;
    estado_botones = INCORRECTO;

    vidas--;

    tiempo_nivel_fallido = tiempo_transcurrido;
    estado_actual = NIVEL_FALLIDO;
  }

  return estado_botones;
}

void iniciar_partida(unsigned long tiempo_transcurrido) {
  inicio_reset();

  calcular_tiempo_presentacion();
  generar_siguiente_elemento();

  aux_tiempo_presentacion = tiempo_transcurrido;
  estado_actual = PRESENTACION;

  Serial.println();
  Serial.println("########################################");
  Serial.println("         NUEVA PARTIDA");
  Serial.println("########################################");
}

int obtener_porcentaje_tiempo(
    unsigned long tiempo_transcurrido) {

  if (estado_actual != INPUTS || tiempo_ingreso == 0) {
    return 0;
  }

  unsigned long tiempo_pasado =
      tiempo_transcurrido - aux_tiempo_ingreso;

  int porcentaje =
      (int)((tiempo_pasado * 100UL) / tiempo_ingreso);

  if (porcentaje > 100) {
    porcentaje = 100;
  }

  return porcentaje;
}

int obtener_nivel() {
  return nivel_actual;
}

int obtener_vidas() {
  return vidas;
}

int obtener_tiempo_acumulado() {
  return tiempo_acumulado;
}

int obtener_led_actual_presentacion(
    unsigned long tiempo_transcurrido) {

  if (estado_actual != PRESENTACION) {
    return -1;
  }

  unsigned long transcurrido =
      tiempo_transcurrido - aux_tiempo_presentacion;

  unsigned long tiempo_por_elemento =
      tiempo_presentacion_actual / nivel_actual;

  if (tiempo_por_elemento == 0) {
    return -1;
  }

  int indice =
      (int)(transcurrido / tiempo_por_elemento);

  if (indice >= nivel_actual) {
    return -1;
  }

  unsigned long dentro_del_elemento =
      transcurrido % tiempo_por_elemento;

  unsigned long tiempo_encendido =
      (tiempo_por_elemento * 70UL) / 100UL;

  if (dentro_del_elemento < tiempo_encendido) {
    return secuencia_leds[indice];
  }

  return -1;
}