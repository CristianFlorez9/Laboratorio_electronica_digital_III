/**
 * @file    adc_driver.h
 * @brief   Medición continua del voltaje de batería con el ADC mediante Timer + IRQ.
 * @details Muestreo periódico disparado por temporizador a través de un divisor
 *          resistivo. La ISR guarda la lectura cruda en un buffer circular y levanta
 *          un flag de muestra nueva; el promediado se realiza en main.
 * @warning No convierte voltaje a porcentaje (responsabilidad de modelo_bateria).
 */

#ifndef ADC_DRIVER_H
#define ADC_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "pico/types.h"

/** @brief Voltaje de referencia del ADC del RP2040 en milivoltios. */
#define ADC_VREF_MV             3300u

/** @brief Resolución máxima del ADC (12 bits = 4096 cuentas). */
#define ADC_MAX_CUENTAS         4096u

/** @brief Factor multiplicador del divisor resistivo (ej. 2/1 para R1 = R2 = 100k). */
#ifndef ADC_DIVISOR_NUM
#define ADC_DIVISOR_NUM         2u
#endif
#ifndef ADC_DIVISOR_DEN
#define ADC_DIVISOR_DEN         1u
#endif

/** @brief Cantidad de muestras promediadas en main antes de entregar nuevo voltaje. */
#ifndef ADC_MUESTRAS_PROMEDIO
#define ADC_MUESTRAS_PROMEDIO   16u
#endif

/**
 * @brief Inicializa el canal ADC, su FIFO con interrupción y el timer de muestreo.
 * @param gpio_pin   Pin GPIO analógico (26, 27 o 28 en RP2040).
 * @param periodo_ms Periodo de muestreo en milisegundos (ej. 50 ms).
 * @return true si la configuración fue exitosa; false si el GPIO es inválido.
 */
bool adc_driver_init(uint gpio_pin, uint32_t periodo_ms);

/**
 * @brief Pausa o reanuda el temporizador de disparo del ADC (útil en bajo consumo).
 * @param pausar true para detener muestreos; false para reanudar.
 */
void adc_driver_pausar(bool pausar);

/**
 * @brief Indica si hay muestras crudas pendientes en el buffer circular.
 * @return true si la ISR depositó al menos una lectura.
 */
bool adc_driver_hay_muestra(void);

/**
 * @brief Procesa en contexto main las muestras pendientes y calcula el promedio en mV.
 * @param[out] mv_out Puntero donde se escribe el voltaje promediado de la batería [mV].
 * @return true cuando se completó un bloque de promediado y @p mv_out es válido.
 */
bool adc_driver_procesar(uint16_t *mv_out);

/**
 * @brief Consulta cuántas muestras se perdieron por saturación del buffer circular.
 * @return Número acumulado de desbordamientos.
 */
uint32_t adc_driver_overflows(void);

#endif /* ADC_DRIVER_H */