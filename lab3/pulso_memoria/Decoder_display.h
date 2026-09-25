/**
 * @file decoder_display.h
 * @brief Interfaz del controlador y multiplexado de displays de 7 segmentos.
 */

#ifndef DECODER_DISPLAY_H
#define DECODER_DISPLAY_H

#include <stdint.h>

/**
 * @brief Inicializa los pines GPIO asignados a los segmentos y dígitos del display.
 * @param pines_segmento Arreglo de 7 pines para los segmentos (a-g).
 * @param pines_digitos Arreglo de 4 pines para los habilitadores de cada dígito.
 */
void configurar_display(const uint8_t pines_segmento[7], const uint8_t pines_digitos[4]);

/**
 * @brief Realiza el multiplexado de los displays para actualizar nivel, vidas y tiempo.
 * @param nivel Valor del nivel actual (0-9).
 * @param vidas Cantidad de vidas restantes (0-9).
 * @param tiempo Valor del tiempo acumulado (0-99).
 * @param tiempo_actual Estampa de tiempo actual del sistema en milisegundos.
 */
void actualizar_display(int nivel, int vidas, int tiempo, uint32_t tiempo_actual);

#endif