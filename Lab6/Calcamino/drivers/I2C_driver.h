/**
 * @file I2C_driver.h
 * @brief Driver maestro I2C no bloqueante (polling + IRQs) para i2c0 e i2c1 en RP2040.
 */

#ifndef I2C_DRIVER_H
#define I2C_DRIVER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "hardware/i2c.h"

/** @brief Códigos de estado retornados por las operaciones del driver I2C. */
typedef enum {
    I2C_DRV_OK = 0,  /**< Operación aceptada o finalizada correctamente. */
    I2C_DRV_BUSY,    /**< Bus ocupado atendiendo otra transferencia. */
    I2C_DRV_INVALID, /**< Parámetro inválido, puntero nulo o bus no inicializado. */
    I2C_DRV_ERROR    /**< Error de hardware en el bus (NACK / TX_ABRT / lectura incompleta). */
} i2c_drv_status_t;

/** @brief Métricas de diagnóstico e instrumentación por bus I2C. */
typedef struct {
    uint32_t error_count;     /**< Errores de bus acumulados (TX_ABRT o lectura incompleta). */
    uint32_t busy_drop_count; /**< Solicitudes rechazadas porque el bus estaba ocupado. */
    uint32_t max_isr_us;      /**< Peor tiempo de ejecución medido en la ISR [us]. */
} i2c_drv_stats_t;

/**
 * @brief Callback invocado desde la ISR al finalizar una transferencia.
 * @warning Se ejecuta en contexto de interrupción: mantenerlo estrictamente acotado
 *          (levantar banderas o guardar muestras), sin cálculos pesados ni esperas activas.
 *
 * @param status Resultado final de la transacción (I2C_DRV_OK o I2C_DRV_ERROR).
 */
typedef void (*i2c_drv_cb_t)(i2c_drv_status_t status);

/**
 * @brief Inicializa un bus I2C físico (i2c0 o i2c1), sus pines GPIO y su interrupción.

 * @param i2c     Instancia de hardware (i2c0 o i2c1).
 * @param sda_pin Número de GPIO para la línea SDA.
 * @param scl_pin Número de GPIO para la línea SCL.
 * @param baud_hz Frecuencia del reloj I2C en Hz (ej. 400000).
 * @return true si el bus se inicializó correctamente; false si algún parámetro es inválido.
 */
bool i2c_drv_init(i2c_inst_t *i2c, uint sda_pin, uint scl_pin, uint32_t baud_hz);

/**
 * @brief Consulta si el bus especificado tiene una transacción en curso.
 * @note Útil para verificar inactividad del bus antes de encadenar envíos o entrar en bajo consumo.
 *
 * @param i2c Instancia de hardware (i2c0 o i2c1).
 * @return true si el bus está ocupado; false si está libre o no inicializado.
 */
bool i2c_drv_is_busy(i2c_inst_t *i2c);

/**
 * @brief Inicia de forma no bloqueante una escritura I2C de longitud arbitraria.
 * @note Sirve tanto para configurar registros ({reg, val}) como para enviar
 *       framebuffers de pantalla ({0x40, pixel_0, ...}). El buffer @p tx_buf
 *       debe permanecer válido en memoria hasta que se invoque @p cb.
 *
 * @param i2c    Instancia de hardware (i2c0 o i2c1).
 * @param addr   Dirección esclava de 7 bits.
 * @param tx_buf Puntero al buffer persistente de datos a transmitir.
 * @param len    Cantidad de bytes a enviar (> 0).
 * @param cb     Callback opcional invocado al terminar (puede ser NULL).
 * @return I2C_DRV_OK si se lanzó; I2C_DRV_BUSY si estaba ocupado; I2C_DRV_INVALID si hay error de argumentos.
 */
i2c_drv_status_t i2c_drv_write(i2c_inst_t *i2c, uint8_t addr,
                               const uint8_t *tx_buf, size_t len,
                               i2c_drv_cb_t cb);

/**
 * @brief Inicia de forma no bloqueante la lectura de 1 a 15 bytes desde un registro con Repeated START.
 * @note El buffer @p rx_buf debe permanecer válido en memoria hasta que se invoque @p cb.
 *
 * @param i2c    Instancia de hardware (i2c0 o i2c1).
 * @param addr   Dirección esclava de 7 bits.
 * @param reg    Dirección del primer registro interno a leer.
 * @param rx_buf Buffer persistente donde la ISR depositará los bytes leídos.
 * @param len    Cantidad de bytes a leer (rango válido: 1 a 15).
 * @param cb     Callback opcional invocado al terminar (puede ser NULL).
 * @return I2C_DRV_OK si se lanzó; I2C_DRV_BUSY si estaba ocupado; I2C_DRV_INVALID si hay error de argumentos.
 */
i2c_drv_status_t i2c_drv_read_reg(i2c_inst_t *i2c, uint8_t addr, uint8_t reg,
                                  uint8_t *rx_buf, uint8_t len,
                                  i2c_drv_cb_t cb);

/**
 * @brief Obtiene una copia atómica de las estadísticas de diagnóstico del bus.
 *
 * @param i2c      Instancia de hardware (i2c0 o i2c1).
 * @param[out] out Puntero a la estructura donde se copiarán las métricas.
 */
void i2c_drv_get_stats(i2c_inst_t *i2c, i2c_drv_stats_t *out);

#endif 