/**
 * @file decoder_display.cpp
 * @brief Implementación del controlador y multiplexado para displays de 7 segmentos.
 */

#include "decoder_display.h"

// Variables del display
static int pines_seg[7];
static int pines_en[4];
static int digito_activo = 0;
static unsigned long aux_tiempo_display = 0;

// Tabla de números para cátodo/ánodo común (matriz de segmentos a-g)
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
 * @brief Desactiva todos los habilitadores y apaga todos los segmentos de forma segura.
 */
static void apagar_todo_seguro();

/**
 * @brief Escribe la combinación de pines correspondiente a un dígito (0-9) en los segmentos.
 * @param num Valor numérico a representar.
 */
static void mostrar_numero(int num);

/**
 * @brief Inicializa y configura los pines GPIO asignados a los segmentos y habilitadores.
 * @param pines_segmento Arreglo de 7 enteros con los pines asignados a los segmentos (a-g).
 * @param pines_digitos Arreglo de 4 enteros con los pines asignados a los enables/cátodos de los dígitos.
 */
void configurar_display(const int pines_segmento[7], const int pines_digitos[4]) {
  for (int i = 0; i < 7; i++) {
    pines_seg[i] = pines_segmento[i];
    pinMode(pines_seg[i], OUTPUT);
  }

  for (int i = 0; i < 4; i++) {
    pines_en[i] = pines_digitos[i];
    pinMode(pines_en[i], OUTPUT);
  }

  apagar_todo_seguro();
}

/**
 * @brief Refresca periódicamente por multiplexado los dígitos que muestran nivel, vidas y tiempo.
 * @param nivel Valor del nivel actual a mostrar en el dígito correspondiente.
 * @param vidas Cantidad de vidas restantes a mostrar en el dígito correspondiente.
 * @param tiempo Valor de tiempo (0-99) a multiplexar en las unidades y decenas.
 * @param tiempo_actual Estampa de tiempo actual del sistema en milisegundos (millis()).
 */
void actualizar_display(int nivel, int vidas, int tiempo, unsigned long tiempo_actual) {
  if (tiempo_actual - aux_tiempo_display >= 3) {
    aux_tiempo_display = tiempo_actual;

    apagar_todo_seguro();

    int numero_a_mostrar = 0;

    switch (digito_activo) {
      case 0:
        numero_a_mostrar = tiempo % 10;
        break;

      case 1:
        numero_a_mostrar = tiempo / 10;
        break;

      case 2:
        numero_a_mostrar = vidas;
        break;

      case 3:
        numero_a_mostrar = nivel;
        break;
    }

    mostrar_numero(numero_a_mostrar);
    digitalWrite(pines_en[digito_activo], LOW);

    digito_activo = (digito_activo + 1) % 4;
  }
}

/**
 * @brief Desactiva todos los habilitadores y apaga todos los segmentos de forma segura.
 */
static void apagar_todo_seguro() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(pines_en[i], HIGH);
  }

  for (int i = 0; i < 7; i++) {
    digitalWrite(pines_seg[i], HIGH);
  }
}

/**
 * @brief Escribe la combinación de pines correspondiente a un dígito (0-9) en los segmentos.
 * @param num Valor numérico a representar (será restringido entre 0 y 9).
 */
static void mostrar_numero(int num) {
  num = constrain(num, 0, 9);

  for (int i = 0; i < 7; i++) {
    digitalWrite(pines_seg[i], numeros[num][i]);
  }
}
