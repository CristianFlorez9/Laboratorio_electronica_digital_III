/**
 * @file Dispositivos.cpp
 * @brief Implementación del control de hardware, periféricos y animaciones visuales de la ASM.
 */

#include "Dispositivos.h"
#include "decoder_display.h"
#include "hardware/gpio.h"

// Pines de periféricos
static const int PINES_LEDS[4] = {15, 14, 13, 12};
static const int PINES_BOTONES[4] = {17, 18, 19, 20};
static const int PIN_LED_PALPITO = 16;
static const int PIN_BOTON_START = 26;

// Display 7 segmentos
static const int PINES_SEGMENTOS[7] = {27, 2, 3, 4, 5, 6, 7};
static const int PINES_ENABLES[4]   = {8, 9, 10, 11};

// Configuración de antirrebote
static const unsigned long TIEMPO_ANTIRREBOTE = 50;

// Variables para antirrebote de botones del juego
static bool estado_estable_botones[4]      = {HIGH, HIGH, HIGH, HIGH};
static bool lectura_anterior_botones[4]    = {HIGH, HIGH, HIGH, HIGH};
static unsigned long tiempo_cambio_botones[4] = {0, 0, 0, 0};

// Variables para antirrebote del botón START
static bool estado_estable_start    = HIGH;
static bool lectura_anterior_start  = HIGH;
static unsigned long tiempo_cambio_start = 0;

/**
 * @brief Inicializa los pines GPIO y su estado por defecto.
 */
void inicializar_dispositivos() {
  for (int i = 0; i < 4; i++) {
    pinMode(PINES_LEDS[i], OUTPUT);
    digitalWrite(PINES_LEDS[i], LOW);

    pinMode(PINES_BOTONES[i], INPUT_PULLUP);
    gpio_set_input_hysteresis_enabled(PINES_BOTONES[i], true);

    estado_estable_botones[i]   = HIGH;
    lectura_anterior_botones[i] = HIGH;
    tiempo_cambio_botones[i]    = 0;
  }

  pinMode(PIN_LED_PALPITO, OUTPUT);
  digitalWrite(PIN_LED_PALPITO, LOW);

  pinMode(PIN_BOTON_START, INPUT_PULLUP);
  gpio_set_input_hysteresis_enabled(PIN_BOTON_START, true);

  estado_estable_start    = HIGH;
  lectura_anterior_start  = HIGH;
  tiempo_cambio_start     = 0;

  configurar_display(PINES_SEGMENTOS, PINES_ENABLES);
}

/**
 * @brief Realiza la lectura antirrebote de los 4 botones de juego.
 * @return Índice del botón presionado (0-3) o -1 si no hay pulso válido.
 */
int leer_boton_juego() {
  unsigned long tiempo_actual = millis();

  for (int i = 0; i < 4; i++) {
    bool lectura_actual = digitalRead(PINES_BOTONES[i]);

    if (lectura_actual != lectura_anterior_botones[i]) {
      tiempo_cambio_botones[i]    = tiempo_actual;
      lectura_anterior_botones[i] = lectura_actual;
    }

    if (tiempo_actual - tiempo_cambio_botones[i] >= TIEMPO_ANTIRREBOTE) {
      if (estado_estable_botones[i] != lectura_actual) {
        estado_estable_botones[i] = lectura_actual;

        if (estado_estable_botones[i] == LOW) {
          return i;
        }
      }
    }
  }

  return -1;
}

/**
 * @brief Evalúa la pulsación del botón START discriminando entre pulso corto y largo.
 * @param tiempo_actual Estampa de tiempo actual del sistema en milisegundos (millis()).
 * @return Evento registrado (0 = Ninguno, 1 = Corto, 2 = Sostenido >= 2s).
 */
int leer_boton_start(unsigned long tiempo_actual) {
  bool lectura_actual = digitalRead(PIN_BOTON_START);

  if (lectura_actual != lectura_anterior_start) {
    tiempo_cambio_start = tiempo_actual;
    lectura_anterior_start = lectura_actual;
  }

  if (tiempo_actual - tiempo_cambio_start >= TIEMPO_ANTIRREBOTE) {
    if (estado_estable_start != lectura_actual) {
      estado_estable_start = lectura_actual;
    }
  }

  static unsigned long tiempo_presionado = 0;
  static bool presionando = false;
  static bool evento_2seg_generado = false;

  if (estado_estable_start == LOW) {

    if (!presionando) {
      presionando = true;
      evento_2seg_generado = false;
      tiempo_presionado = tiempo_actual;

    } else if (!evento_2seg_generado &&
               (tiempo_actual - tiempo_presionado >= 2000)) {

      evento_2seg_generado = true;

      unsigned long tiempo_presionado_real =
          tiempo_actual - tiempo_presionado;

      Serial.println();
      Serial.println("START PRESIONADO");
      Serial.print("Tiempo medido: ");
      Serial.print(tiempo_presionado_real);
      Serial.println(" ms");

      Serial.print("Tiempo medido: ");
      Serial.print(tiempo_presionado_real / 1000.0, 3);
      Serial.println(" segundos");

      return 2;
    }

  } else {

    if (presionando) {
      presionando = false;

      unsigned long tiempo_presionado_real =
          tiempo_actual - tiempo_presionado;

      if (!evento_2seg_generado &&
          tiempo_presionado_real < 2000) {

        Serial.println();
        Serial.println("START PRESIONADO");

        Serial.print("Tiempo medido: ");
        Serial.print(tiempo_presionado_real);
        Serial.println(" ms");

        Serial.print("Tiempo medido: ");
        Serial.print(tiempo_presionado_real / 1000.0, 3);
        Serial.println(" segundos");

        return 1;
      }
    }
  }

  return 0;
}

/**
 * @brief Enciende o apaga un LED individual de la secuencia.
 * @param indice Número de LED (0-3).
 * @param estado Estado lógico a escribir (`true` = HIGH, `false` = LOW).
 */
void encender_led_secuencia(int indice, bool estado) {
  if (indice >= 0 && indice < 4) {
    digitalWrite(PINES_LEDS[indice], estado ? HIGH : LOW);
  }
}

/**
 * @brief Apaga todos los LEDs principales y el indicador de pálpito.
 */
void apagar_todos_leds() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(PINES_LEDS[i], LOW);
  }
  digitalWrite(PIN_LED_PALPITO, LOW);
}

/**
 * @brief Modula el parpadeo del LED de pálpito con aceleración cuadrática según el tiempo consumido.
 * @param porcentaje_tiempo Valor en rango 0.0 (inicio) a 1.0 (tiempo agotado).
 * @param tiempo_actual Estampa de tiempo actual en milisegundos (millis()).
 */
void actualizar_palpito(float porcentaje_tiempo, unsigned long tiempo_actual) {
  if (porcentaje_tiempo <= 0.0f) {
    digitalWrite(PIN_LED_PALPITO, LOW);
    return;
  }

  if (porcentaje_tiempo > 1.0f) {
    porcentaje_tiempo = 1.0f;
  }

  float factor = (1.0f - porcentaje_tiempo);
  int intervalo = 40 + (int)(560.0f * (factor * factor));

  static unsigned long ultima_conmutacion = 0;
  static bool estado_led = false;

  if (tiempo_actual - ultima_conmutacion >= (unsigned long)intervalo) {
    ultima_conmutacion = tiempo_actual;
    estado_led = !estado_led;
    digitalWrite(PIN_LED_PALPITO, estado_led ? HIGH : LOW);
  }
}

/**
 * @brief Llama a la rutina de multiplexado de los displays de 7 segmentos.
 * @param nivel Nivel actual.
 * @param vidas Vidas restantes.
 * @param tiempo Tiempo acumulado.
 * @param tiempo_actual Estampa de tiempo actual en milisegundos (millis()).
 */
void mostrar_en_displays(int nivel, int vidas, int tiempo, unsigned long tiempo_actual) {
  actualizar_display(nivel, vidas, tiempo, tiempo_actual);
}

/**
 * @brief Ejecuta un parpadeo síncrono de todos los LEDs 3 veces (Mala pulsación).
 * @param tiempo_actual Estampa de tiempo actual en milisegundos (millis()).
 */
void animacion_entrada_incorrecta(unsigned long tiempo_actual) {
  // 75 ms encendido / 75 ms apagado (150 ms por ciclo).
  // 6 alternancias equivalen a 3 parpadeos completos.
  int ciclo = (tiempo_actual / 75) % 6; 
  bool estado = (ciclo % 2 == 0);

  for (int i = 0; i < 4; i++) {
    digitalWrite(PINES_LEDS[i], estado ? HIGH : LOW);
  }
}

/**
 * @brief Enciende todos los LEDs de forma continua (Nivel completado).
 * @param tiempo_actual Estampa de tiempo actual en milisegundos (millis()).
 */
void animacion_secuencia_correcta(unsigned long tiempo_actual) {
  (void)tiempo_actual; // Se ignora para evitar advertencias de compilación

  for (int i = 0; i < 4; i++) {
    digitalWrite(PINES_LEDS[i], HIGH);
  }
}

/**
 * @brief Ejecuta un destello síncrono general en los 4 LEDs (Agotamiento del Tiempo).
 * @param tiempo_actual Estampa de tiempo actual en milisegundos (millis()).
 */
void animacion_tiempo_agotado(unsigned long tiempo_actual) {
  bool estado = (tiempo_actual / 100) % 2 == 0;
  for (int i = 0; i < 4; i++) {
    digitalWrite(PINES_LEDS[i], estado ? HIGH : LOW);
  }
}

/**
 * @brief Mantiene encendidos todos los LEDs de forma fija (Pérdida de Vida).
 * @param tiempo_actual Estampa de tiempo actual en milisegundos (millis()).
 */
void animacion_perdida_vida(unsigned long tiempo_actual) {
  for (int i = 0; i < 4; i++) {
    digitalWrite(PINES_LEDS[i], HIGH);
  }
}

/**
 * @brief Ejecuta una marquesina vaivén en los LEDs al ganar el juego (Nivel 9 Completado).
 * @param tiempo_actual Estampa de tiempo actual en milisegundos (millis()).
 */
void animacion_victoria_nivel_9(unsigned long tiempo_actual) {
  int secuencia[6] = {0, 1, 2, 3, 2, 1};
  int paso = (tiempo_actual / 100) % 6;
  for (int i = 0; i < 4; i++) {
    digitalWrite(PINES_LEDS[i], (i == secuencia[paso]) ? HIGH : LOW);
  }
}

/**
 * @brief Ejecuta el apagado descendente de LEDs al agotar todas las vidas (Game Over).
 * @param tiempo_actual Estampa de tiempo actual en milisegundos (millis()).
 */
void animacion_game_over(unsigned long tiempo_actual) {
  int leds_activos = 4 - ((tiempo_actual / 200) % 5);
  for (int i = 0; i < 4; i++) {
    digitalWrite(PINES_LEDS[i], (i < leds_activos) ? HIGH : LOW);
  }
}