/**
 * @file    acelerometro_MPU_6050.h
 * @brief   Driver del acelerómetro MPU6050 adaptado para I2C no bloqueante.
 * @details Configura el sensor mediante una máquina de estados I2C y solicita
 *          muestras sin bloquear la CPU. Soporta modo continuo a 50 Hz y modo
 *          de bajo consumo Wake-on-Motion (WOM) con despertar por pin INT.
 */

#ifndef ACELEROMETRO_MPU_6050_H
#define ACELEROMETRO_MPU_6050_H

#include <stdint.h>
#include <stdbool.h>
#include "drivers/I2C_driver.h"

/* Configuraciones de hardware */
#define MPU_ADDR        0x68u
#define INT_PIN         4u

/* Mapa de registros principales */
#define SMPLRT_DIV      0x19u
#define ACCEL_CFG       0x1Cu
#define MOT_THR         0x1Fu
#define MOT_DUR         0x20u
#define IRQ_CFG         0x37u
#define IRQ_EN          0x38u
#define INT_STATUS      0x3Au
#define ACCEL_XOUT_H    0x3Bu
#define PWR_MGMT        0x6Bu
#define PWR_STBY        0x6Cu

/**
 * @brief Inicia la secuencia de configuración activa del MPU6050 (50 Hz) sin bloquear.
 * @param i2c Instancia del bus I2C (ej. i2c1).
 */
void mpu_init_non_blocking(i2c_inst_t *i2c);

/**
 * @brief Consulta si la secuencia de inicialización ya terminó de enviarse.
 * @return true si el MPU6050 está listo para operar; false si sigue configurándose.
 */
bool mpu_is_init_complete(void);

/**
 * @brief Conmuta de forma no bloqueante entre modo normal (50 Hz) y modo
 *        Wake-on-Motion de bajo consumo (5 Hz + interrupción en INT_PIN).
 * @param enable  true para activar WOM (bajo consumo); false para volver a 50 Hz.
 * @param cb_done Callback invocado al finalizar la secuencia I2C (puede ser NULL).
 * @return true si la secuencia arrancó; false si el bus estaba ocupado o no inicializado.
 */
bool mpu_set_wom_mode(bool enable, i2c_drv_cb_t cb_done);

/**
 * @brief Lee el registro INT_STATUS (0x3A) para limpiar el latch del pin INT.
 * @param cb_done Callback invocado al finalizar la lectura (puede ser NULL).
 * @return true si se lanzó la lectura; false si el bus estaba ocupado.
 */
bool mpu_clear_irq_non_blocking(i2c_drv_cb_t cb_done);

/**
 * @brief Solicita la lectura de 6 bytes de aceleración (XYZ) por I2C.
 * @param cb_done Callback invocado desde la ISR al finalizar la lectura.
 */
void mpu_request_data_non_blocking(i2c_drv_cb_t cb_done);

/**
 * @brief Copia los últimos 6 bytes de aceleración leídos hacia @p out_buffer.
 * @param[out] out_buffer Arreglo de al menos 6 bytes donde se depositan los datos.
 */
void mpu_get_last_data(uint8_t *out_buffer);

#endif /* ACELEROMETRO_MPU_6050_H */