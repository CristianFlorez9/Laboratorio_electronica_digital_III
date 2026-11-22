/**
 * @file    acelerometro_MPU_6050.h
 * @brief   Driver del acelerómetro adaptado para I2C no bloqueante.
 * @details Configura el sensor mediante una máquina de estados I2C y solicita 
 *          muestras sin bloquear la CPU. Soporta modo de bajo consumo y 
 *          despertar mediante el pin INT.
 */

#ifndef ACELEROMETRO_MPU_6050_H
#define ACELEROMETRO_MPU_6050_H

#include <stdint.h>
#include <stdbool.h>
#include "drivers/I2C_driver.h"

// --- Configuraciones de Hardware ---
#define MPU_ADDR 0x68
#define INT_PIN  6

// --- Mapa de Registros Principales ---
#define ACCEL_XOUT_H 0x3B // Registro base para leer datos de aceleración
#define IRQ_CFG      0x37
#define IRQ_EN       0x38
#define PWR_MGMT     0x6B
#define PWR_STBY     0x6C
#define ACCEL_CFG    0x1C


/**
 * @brief Inicia la secuencia de configuración del MPU6050 sin bloquear la CPU.
 * @param i2c Instancia del bus I2C (ej. i2c0).
 */
void mpu_init_non_blocking(i2c_inst_t *i2c);

/**
 * @brief Consulta si la secuencia de inicialización ya terminó de enviarse.
 * @return true si el MPU está listo para operar; false si sigue configurándose.
 */
bool mpu_is_init_complete(void);

/**
 * @brief Solicita la lectura de datos por I2C. Retorna inmediatamente.
 * 
 * @param cb_done Callback de tu driver I2C que se invocará desde la ISR 
 *                cuando la lectura finalice (ej. mpu_read_done_callback en tu main).
 */
void mpu_request_data_non_blocking(i2c_drv_cb_t cb_done);

/**
 * @brief Obtiene de forma segura los últimos datos que llegaron por el bus I2C.
 * @note  Debe llamarse desde el loop principal después de que `cb_done` avise que el dato llegó.
 * 
 * @param out_buffer Puntero al buffer donde se copiarán los bytes leídos.
 */
void mpu_get_last_data(uint8_t *out_buffer);

#endif // ACELEROMETRO_MPU_6050_H
