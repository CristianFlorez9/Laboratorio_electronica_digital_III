/**
 * @file    step_detector.h
 * @brief   Algoritmo de detección de marcha y conteo de pasos.
 * @details Entrada: muestras de aceleración triaxial. Salida: conteo de pasos,
 *          estado de marcha confirmada, cadencia en pasos por minuto y marca de
 *          tiempo del último paso. Filtra movimientos aislados no rítmicos y
 *          suma retroactivamente los pasos iniciales al confirmar una marcha.
 * @note    Contexto: solo main (nunca en ISR). Sin dependencias de hardware.
 */

#ifndef STEP_DETECTOR_H
#define STEP_DETECTOR_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Reinicia todos los contadores y estados internos del detector de pasos.
 */
void step_detector_init(void);

/**
 * @brief Procesa una nueva muestra triaxial del acelerómetro en contexto main.
 * @param accel_x Aceleración cruda en eje X.
 * @param accel_y Aceleración cruda en eje Y.
 * @param accel_z Aceleración cruda en eje Z.
 */
void step_detector_update(int16_t accel_x, int16_t accel_y, int16_t accel_z);

/**
 * @brief Retorna el conteo total de pasos confirmados en la sesión.
 * @return Número acumulado de pasos.
 */
uint32_t step_count(void);

/**
 * @brief Indica si el usuario se encuentra actualmente en marcha confirmada.
 * @return true si hay caminata activa; false en reposo o movimiento aislado.
 */
bool is_walking(void);

/**
 * @brief Retorna la cadencia actual de caminata.
 * @return Cadencia en pasos por minuto (SPM), o 0 si no está caminando.
 */
int cadence(void);

/**
 * @brief Retorna el instante [ms] en que ocurrió el último paso válido.
 * @return Tiempo desde el arranque en milisegundos (0 si aún no hubo pasos).
 */
uint32_t step_last_time_ms(void);

#endif /* STEP_DETECTOR_H */