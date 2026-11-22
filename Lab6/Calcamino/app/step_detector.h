/**
 * @file    step_detector.h
 * @brief   Algoritmo de detección de marcha y conteo de pasos.
 * @details Entrada: muestras de aceleración. Salida: eventos de paso, estado
 *          caminando/no caminando y cadencia. Distingue marcha lenta, habitual
 *          y rápida de otros movimientos.
 * @note    Contexto: solo main (nunca en ISR). Sin dependencias de hardware.
 */

#ifndef STEP_DETECTOR_H
#define STEP_DETECTOR_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

// Inicializa las variables del detector
void step_detector_init(void);

// Actualiza el estado (se debe llamar desde el main cuando hay nuevos datos)
void step_detector_update(int16_t accel_x, int16_t accel_y, int16_t accel_z);

// Retorna el conteo total de pasos
uint32_t step_count(void);

// Retorna true si está caminando actualmente
bool is_walking(void);

// Retorna la cadencia actual en pasos por minuto
int cadence(void);

#endif /* STEP_DETECTOR_H */
