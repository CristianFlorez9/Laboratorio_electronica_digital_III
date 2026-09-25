/**
 * @file decoder_display.c
 * @brief Implementación del controlador y multiplexado para displays de 7 segmentos.
 * @details Los buses de segmentos y de habilitadores se escriben con gpio_put_masked(), de modo
 *          que cada refresco del display actualiza varios pines en una sola operación atómica en
 *          vez de recorrerlos uno a uno con digitalWrite.
 */

#include "decoder_display.h"
#include "hardware/gpio.h"
#include <stdbool.h>

/** @brief Pines GPIO asignados a cada segmento (a-g), en el mismo orden que la tabla #numeros. */
static uint8_t pines_seg[7];

/** @brief Pines GPIO habilitadores de cada uno de los 4 dígitos del display multiplexado. */
static uint8_t pines_en[4];

/** @brief Máscara con todos los bits de #pines_seg activos, para escrituras atómicas del bus de segmentos. */
static uint32_t mascara_seg = 0;

/** @brief Máscara con todos los bits de #pines_en activos, para apagar todos los dígitos de una sola vez. */
static uint32_t mascara_en  = 0;

/** @brief Índice (0-3) del dígito que le corresponde ser refrescado en el próximo ciclo de multiplexado. */
static int digito_activo = 0;

/** @brief Estampa de tiempo (ms) del último refresco de dígito, usada para temporizar el multiplexado. */
static uint32_t aux_tiempo_display = 0;

/**
 * @brief Tabla de patrones de segmentos (a-g) para los dígitos 0-9, en lógica de cátodo/ánodo común.
 * @details Cada fila es un dígito; cada columna i corresponde al segmento asociado a pines_seg[i].
 */
static const bool numeros[10][7] = {
  {0, 0, 0, 0, 0, 0, 1}, // 0
  {1, 0, 0, 1, 1, 1, 1}, // 1
  {0, 0, 1, 0, 0, 1, 0}, // 2
  {0, 0, 0, 0, 1, 1, 0}, // 3
  {1, 0, 0, 1, 1, 0, 0}, // 4
  {0, 1, 0, 0, 1, 0, 0}, // 5
  {0, 1, 0, 0, 0, 0, 0}, // 6
  {0, 0, 0, 1, 1, 1, 1}, // 7
  {0, 0, 0, 0, 0, 0, 0}, // 8
  {0, 0, 0, 0, 1, 0, 0}  // 9
};

/**
 * @brief Pone en HIGH todos los segmentos y todos los habilitadores (apagado seguro), en dos
 *        escrituras enmascaradas en vez de un for por pin.
 */
static void apagar_todo_seguro(void) {
  gpio_put_masked(mascara_seg, mascara_seg);
  gpio_put_masked(mascara_en, mascara_en);
}

/**
 * @brief Escribe en el bus de segmentos, en una sola operación, el patrón del dígito (0-9).
 * @param num Dígito a mostrar; se satura al rango [0, 9] si llega fuera de esos límites.
 */
static void mostrar_numero(int num) {
  if (num < 0) num = 0;
  if (num > 9) num = 9;

  uint32_t valor = 0;

  for (int i = 0; i < 7; i++) {
    if (numeros[num][i]) {
      valor |= (1u << pines_seg[i]);
    }
  }

  gpio_put_masked(mascara_seg, valor);
}

void configurar_display(const uint8_t pines_segmento[7], const uint8_t pines_digitos[4]) {
  mascara_seg = 0;
  mascara_en  = 0;

  for (int i = 0; i < 7; i++) {
    pines_seg[i] = pines_segmento[i];
    gpio_init(pines_seg[i]);
    gpio_set_dir(pines_seg[i], GPIO_OUT);
    mascara_seg |= (1u << pines_seg[i]);
  }

  for (int i = 0; i < 4; i++) {
    pines_en[i] = pines_digitos[i];
    gpio_init(pines_en[i]);
    gpio_set_dir(pines_en[i], GPIO_OUT);
    mascara_en |= (1u << pines_en[i]);
  }

  apagar_todo_seguro();
}

void actualizar_display(int nivel, int vidas, int tiempo, uint32_t tiempo_actual) {
  if (tiempo_actual - aux_tiempo_display >= 3) {
    aux_tiempo_display = tiempo_actual;

    apagar_todo_seguro();

    int numero_a_mostrar = 0;

    switch (digito_activo) {
      case 0: numero_a_mostrar = tiempo % 10; break;
      case 1: numero_a_mostrar = tiempo / 10; break;
      case 2: numero_a_mostrar = vidas;       break;
      case 3: numero_a_mostrar = nivel;       break;
    }

    mostrar_numero(numero_a_mostrar);

    uint32_t bit_en = 1u << pines_en[digito_activo];
    gpio_put_masked(bit_en, 0); // habilita (LOW) solo el dígito activo

    digito_activo = (digito_activo + 1) % 4;
  }
}