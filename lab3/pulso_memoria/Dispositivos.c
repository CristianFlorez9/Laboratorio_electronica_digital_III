/**
 * @file Dispositivos.c
 * @brief Implementación del control de hardware, periféricos y animaciones visuales.
 * @details Arquitectura híbrida sondeo (polling) + interrupciones:
 *
 *          - Interrupciones GPIO (flanco de subida y de bajada) capturan el instante exacto en
 *            que cambia el estado eléctrico de cada botón. Esto sustituye la lectura continua
 *            por sondeo que hacía el código Arduino original en cada vuelta del loop().
 *          - El sondeo, invocado desde el ciclo principal, sigue siendo necesario para dos
 *            cosas que una interrupción por sí sola no resuelve: (a) aplicar la ventana de
 *            antirrebote, esperando a que el pin quede eléctricamente estable durante 50 ms
 *            antes de aceptar el cambio como válido, y (b) medir cuánto tiempo lleva presionado
 *            el botón START mientras permanece presionado, ya que mientras no cambia de estado
 *            no se genera ninguna interrupción nueva y solo el sondeo periódico puede detectar
 *            que ya pasaron los 2 segundos.
 *          - El control de los 4 LEDs de la secuencia usa gpio_put_masked() para escribir varios
 *            pines a la vez en una sola operación atómica, en vez de recorrerlos uno por uno con
 *            digitalWrite (el mismo criterio se aplica a los buses del display de 7 segmentos en
 *            decoder_display.c).
 */

#include "Dispositivos.h"
#include "decoder_display.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <inttypes.h>

/** @brief Pines GPIO de los 4 LEDs que muestran la secuencia a memorizar. */
static const uint8_t PINES_LEDS[4]    = {15, 14, 13, 12};

/** @brief Pines GPIO de los 4 botones con los que el jugador reproduce la secuencia. */
static const uint8_t PINES_BOTONES[4] = {17, 18, 19, 20};

/** @brief Pin GPIO del LED de "pálpito" que indica visualmente el tiempo restante. */
static const uint8_t PIN_LED_PALPITO  = 16;

/** @brief Pin GPIO del botón START (inicio de partida / reinicio sostenido). */
static const uint8_t PIN_BOTON_START  = 26;

/** @brief Pines GPIO de los 7 segmentos (a-g) del display, pasados a decoder_display. */
static const uint8_t PINES_SEGMENTOS[7] = {27, 2, 3, 4, 5, 6, 7};

/** @brief Pines GPIO habilitadores de los 4 dígitos del display, pasados a decoder_display. */
static const uint8_t PINES_ENABLES[4]   = {8, 9, 10, 11};

/** @brief Ventana de antirrebote (ms) aplicada tanto a los botones de juego como al START. */
static const uint32_t TIEMPO_ANTIRREBOTE = 50;

/** @brief Máscara con los bits de #PINES_LEDS, calculada una sola vez en inicializar_dispositivos(). */
static uint32_t mascara_leds = 0;

// --- Estado compartido entre la ISR de GPIO y el sondeo del ciclo principal ---

/** @brief Última lectura eléctrica ya validada (post-antirrebote) de cada botón de juego. */
static bool estado_estable_botones[4]           = {true, true, true, true};

/** @brief Bandera por botón: true si la ISR detectó un flanco pendiente de resolver por sondeo. */
static volatile bool bandera_boton_juego[4]     = {false, false, false, false};

/** @brief Estampa de tiempo (ms) del último flanco detectado por la ISR en cada botón de juego. */
static volatile uint32_t tiempo_flanco_boton[4] = {0, 0, 0, 0};

/** @brief Bandera: true si la ISR detectó un flanco en el botón START pendiente de resolver. */
static volatile bool bandera_start           = false;

/** @brief Estampa de tiempo (ms) del último flanco detectado por la ISR en el botón START. */
static volatile uint32_t tiempo_flanco_start = 0;

/**
 * @brief Rutina de interrupción GPIO (callback único compartido por todos los pines).
 * @details Solo registra el instante del flanco y levanta una bandera; toda la lógica de
 *          antirrebote y de decisión se resuelve por sondeo en leer_boton_juego() /
 *          leer_boton_start(), nunca dentro de la ISR.
 * @param gpio Número de pin GPIO que generó la interrupción.
 * @param eventos Máscara de eventos de la interrupción (no usada; el flanco se determina por
 *        sondeo posterior comparando contra el último estado estable conocido).
 */
static void gpio_callback(uint gpio, uint32_t eventos) {
  (void)eventos;
  uint32_t ahora = to_ms_since_boot(get_absolute_time());

  for (int i = 0; i < 4; i++) {
    if (gpio == PINES_BOTONES[i]) {
      tiempo_flanco_boton[i] = ahora;
      bandera_boton_juego[i] = true;
      return;
    }
  }

  if (gpio == PIN_BOTON_START) {
    tiempo_flanco_start = ahora;
    bandera_start = true;
  }
}

/**
 * @brief Aplica un patrón de 4 bits a los LEDs de la secuencia en una sola escritura enmascarada.
 * @param patron Bit i (0-3) determina el estado del LED conectado a PINES_LEDS[i].
 */
static void aplicar_patron_leds(uint8_t patron) {
  uint32_t valor = 0;

  for (int i = 0; i < 4; i++) {
    if (patron & (1u << i)) {
      valor |= (1u << PINES_LEDS[i]);
    }
  }

  gpio_put_masked(mascara_leds, valor);
}

void inicializar_dispositivos(void) {
  mascara_leds = 0;

  for (int i = 0; i < 4; i++) {
    gpio_init(PINES_LEDS[i]);
    gpio_set_dir(PINES_LEDS[i], GPIO_OUT);
    mascara_leds |= (1u << PINES_LEDS[i]);

    gpio_init(PINES_BOTONES[i]);
    gpio_set_dir(PINES_BOTONES[i], GPIO_IN);
    gpio_pull_up(PINES_BOTONES[i]);
    gpio_set_input_hysteresis_enabled(PINES_BOTONES[i], true);

    estado_estable_botones[i] = true;
    bandera_boton_juego[i]    = false;
    tiempo_flanco_boton[i]    = 0;
  }

  aplicar_patron_leds(0x00);

  gpio_init(PIN_LED_PALPITO);
  gpio_set_dir(PIN_LED_PALPITO, GPIO_OUT);
  gpio_put(PIN_LED_PALPITO, 0);

  gpio_init(PIN_BOTON_START);
  gpio_set_dir(PIN_BOTON_START, GPIO_IN);
  gpio_pull_up(PIN_BOTON_START);
  gpio_set_input_hysteresis_enabled(PIN_BOTON_START, true);

  configurar_display(PINES_SEGMENTOS, PINES_ENABLES);

  // El SDK solo admite un callback global de IRQ de GPIO: se registra en el primer botón
  // y luego se habilita el mismo evento para el resto de pines.
  gpio_set_irq_enabled_with_callback(
      PINES_BOTONES[0],
      GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE,
      true,
      &gpio_callback);

  for (int i = 1; i < 4; i++) {
    gpio_set_irq_enabled(PINES_BOTONES[i], GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);
  }

  gpio_set_irq_enabled(PIN_BOTON_START, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);
}

int leer_boton_juego(void) {
  uint32_t tiempo_actual = to_ms_since_boot(get_absolute_time());

  for (int i = 0; i < 4; i++) {
    if (!bandera_boton_juego[i]) {
      continue; // ninguna interrupción pendiente de resolver en este pin
    }

    uint32_t estado_irq = save_and_disable_interrupts();
    uint32_t tiempo_flanco = tiempo_flanco_boton[i];
    restore_interrupts(estado_irq);

    if (tiempo_actual - tiempo_flanco < TIEMPO_ANTIRREBOTE) {
      continue; // aún dentro de la ventana de antirrebote; se reevalúa en la próxima vuelta
    }

    bool lectura_actual = gpio_get(PINES_BOTONES[i]);

    estado_irq = save_and_disable_interrupts();
    bandera_boton_juego[i] = false; // evento resuelto
    restore_interrupts(estado_irq);

    if (estado_estable_botones[i] != lectura_actual) {
      estado_estable_botones[i] = lectura_actual;

      if (!lectura_actual) { // LOW = presionado (entrada con pull-up)
        return i;
      }
    }
  }

  return -1;
}

int leer_boton_start(uint32_t tiempo_actual) {
  static bool estado_estable_start  = true;
  static bool presionando           = false;
  static bool evento_2seg_generado  = false;
  static uint32_t tiempo_presionado = 0;

  if (bandera_start) {
    uint32_t estado_irq = save_and_disable_interrupts();
    uint32_t tiempo_flanco = tiempo_flanco_start;
    restore_interrupts(estado_irq);

    if (tiempo_actual - tiempo_flanco >= TIEMPO_ANTIRREBOTE) {
      bool lectura_actual = gpio_get(PIN_BOTON_START);

      estado_irq = save_and_disable_interrupts();
      bandera_start = false;
      restore_interrupts(estado_irq);

      if (estado_estable_start != lectura_actual) {
        estado_estable_start = lectura_actual;
      }
    }
  }

  if (!estado_estable_start) { // LOW = presionado

    if (!presionando) {
      presionando = true;
      evento_2seg_generado = false;
      tiempo_presionado = tiempo_actual;

    } else if (!evento_2seg_generado &&
               (tiempo_actual - tiempo_presionado >= 2000)) {

      evento_2seg_generado = true;

      uint32_t tiempo_presionado_real = tiempo_actual - tiempo_presionado;

      printf("\n");
      printf("START PRESIONADO\n");
      printf("Tiempo medido: %" PRIu32 " ms\n", tiempo_presionado_real);
      printf("Tiempo medido: %.3f segundos\n", tiempo_presionado_real / 1000.0f);

      return 2;
    }

  } else {

    if (presionando) {
      presionando = false;

      uint32_t tiempo_presionado_real = tiempo_actual - tiempo_presionado;

      if (!evento_2seg_generado && tiempo_presionado_real < 2000) {

        printf("\n");
        printf("START PRESIONADO\n");
        printf("Tiempo medido: %" PRIu32 " ms\n", tiempo_presionado_real);
        printf("Tiempo medido: %.3f segundos\n", tiempo_presionado_real / 1000.0f);

        return 1;
      }
    }
  }

  return 0;
}

void encender_led_secuencia(int indice, bool estado) {
  if (indice >= 0 && indice < 4) {
    uint32_t bit = 1u << PINES_LEDS[indice];
    gpio_put_masked(bit, estado ? bit : 0);
  }
}

void apagar_todos_leds(void) {
  aplicar_patron_leds(0x00);
  gpio_put(PIN_LED_PALPITO, 0);
}

void actualizar_palpito(float porcentaje_tiempo, uint32_t tiempo_actual) {
  if (porcentaje_tiempo <= 0.0f) {
    gpio_put(PIN_LED_PALPITO, 0);
    return;
  }

  if (porcentaje_tiempo > 1.0f) {
    porcentaje_tiempo = 1.0f;
  }

  float factor = (1.0f - porcentaje_tiempo);
  int intervalo = 40 + (int)(560.0f * (factor * factor));

  static uint32_t ultima_conmutacion = 0;
  static bool estado_led = false;

  if (tiempo_actual - ultima_conmutacion >= (uint32_t)intervalo) {
    ultima_conmutacion = tiempo_actual;
    estado_led = !estado_led;
    gpio_put(PIN_LED_PALPITO, estado_led);
  }
}

void mostrar_en_displays(int nivel, int vidas, int tiempo, uint32_t tiempo_actual) {
  actualizar_display(nivel, vidas, tiempo, tiempo_actual);
}

void animacion_entrada_incorrecta(uint32_t tiempo_actual) {
  int ciclo = (tiempo_actual / 75) % 6;
  bool estado = (ciclo % 2 == 0);
  aplicar_patron_leds(estado ? 0x0F : 0x00);
}

void animacion_secuencia_correcta(uint32_t tiempo_actual) {
  (void)tiempo_actual;
  aplicar_patron_leds(0x0F);
}

void animacion_tiempo_agotado(uint32_t tiempo_actual) {
  bool estado = (tiempo_actual / 100) % 2 == 0;
  aplicar_patron_leds(estado ? 0x0F : 0x00);
}

void animacion_perdida_vida(uint32_t tiempo_actual) {
  (void)tiempo_actual;
  aplicar_patron_leds(0x0F);
}

void animacion_victoria_nivel_9(uint32_t tiempo_actual) {
  static const int secuencia[6] = {0, 1, 2, 3, 2, 1};
  int paso = (tiempo_actual / 100) % 6;
  aplicar_patron_leds((uint8_t)(1u << secuencia[paso]));
}

void animacion_game_over(uint32_t tiempo_actual) {
  int leds_activos = 4 - ((tiempo_actual / 200) % 5);
  uint8_t patron = 0;

  for (int i = 0; i < 4; i++) {
    if (i < leds_activos) {
      patron |= (uint8_t)(1u << i);
    }
  }

  aplicar_patron_leds(patron);
}