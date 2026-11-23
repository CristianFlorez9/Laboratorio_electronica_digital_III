/**
 * @file    acelerometro_MPU_6050.c
 * @brief   Implementación no bloqueante del driver MPU6050 sobre I2C_driver.
 */

#include "acelerometro_MPU_6050.h"
#include <stddef.h>

typedef struct {
    uint8_t reg;
    uint8_t val;
} mpu_cfg_step_t;

static uint8_t tx_buf[2];
static uint8_t rx_buf[6];
static uint8_t irq_status_buf = 0u;
static i2c_inst_t *mpu_bus = NULL;
static bool mpu_is_ready = false;

/* Secuencia de arranque en modo continuo a 50 Hz (1000 Hz / (1 + 19)) */
static const mpu_cfg_step_t mpu_init_sequence[] = {
    {PWR_MGMT,   0x00u}, /* Despertar sensor */
    {PWR_STBY,   0x07u}, /* Giroscopio en standby para ahorrar energía */
    {SMPLRT_DIV, 19u  }, /* Tasa de muestreo = 50 Hz */
    {ACCEL_CFG,  0x01u}, /* ±2g, filtro paso alto a 5 Hz para detección de movimiento */
    {MOT_THR,    20u  }, /* Umbral de movimiento WOM */
    {MOT_DUR,    3u   }, /* Duración mínima de movimiento */
    {IRQ_CFG,    0x20u}, /* Pin INT activo alto con latch hasta lectura */
    {IRQ_EN,     0x00u}  /* Interrupción WOM deshabilitada durante muestreo activo */
};

/* Secuencia para entrar en Wake-on-Motion (bajo consumo a 5 Hz) */
static const mpu_cfg_step_t mpu_wom_enable_seq[] = {
    {IRQ_EN,   0x40u}, /* Habilitar MOT_EN */
    {PWR_STBY, 0x47u}, /* Frecuencia de despertar = 5 Hz, giroscopio en standby */
    {PWR_MGMT, 0x28u}  /* Modo ciclo de bajo consumo (CYCLE=1, TEMP_DIS=1) */
};

/* Secuencia para salir de Wake-on-Motion y volver a muestreo continuo a 50 Hz */
static const mpu_cfg_step_t mpu_wom_disable_seq[] = {
    {PWR_MGMT, 0x00u}, /* Salir de modo ciclo */
    {PWR_STBY, 0x07u}, /* Mantener giroscopio en standby */
    {IRQ_EN,   0x00u}  /* Deshabilitar interrupción WOM */
};

static const mpu_cfg_step_t *active_seq = NULL;
static uint8_t active_seq_len = 0u;
static uint8_t seq_step = 0u;
static i2c_drv_cb_t user_seq_cb = NULL;

static void mpu_seq_callback(i2c_drv_status_t status) {
    if (status != I2C_DRV_OK) {
        if (user_seq_cb != NULL) {
            user_seq_cb(status);
        }
        return;
    }

    seq_step++;

    if (seq_step < active_seq_len) {
        tx_buf[0] = active_seq[seq_step].reg;
        tx_buf[1] = active_seq[seq_step].val;
        (void)i2c_drv_write(mpu_bus, MPU_ADDR, tx_buf, 2u, mpu_seq_callback);
    } else {
        if (active_seq == mpu_init_sequence) {
            mpu_is_ready = true;
        }
        if (user_seq_cb != NULL) {
            user_seq_cb(I2C_DRV_OK);
        }
    }
}

static bool start_sequence(const mpu_cfg_step_t *seq, uint8_t len, i2c_drv_cb_t cb) {
    if ((mpu_bus == NULL) || (seq == NULL) || (len == 0u)) {
        return false;
    }

    active_seq     = seq;
    active_seq_len = len;
    seq_step       = 0u;
    user_seq_cb    = cb;

    tx_buf[0] = active_seq[0].reg;
    tx_buf[1] = active_seq[0].val;

    return (i2c_drv_write(mpu_bus, MPU_ADDR, tx_buf, 2u, mpu_seq_callback) == I2C_DRV_OK);
}

void mpu_init_non_blocking(i2c_inst_t *i2c) {
    mpu_bus      = i2c;
    mpu_is_ready = false;
    (void)start_sequence(mpu_init_sequence,
                         (uint8_t)(sizeof(mpu_init_sequence) / sizeof(mpu_init_sequence[0])),
                         NULL);
}

bool mpu_is_init_complete(void) {
    return mpu_is_ready;
}

bool mpu_set_wom_mode(bool enable, i2c_drv_cb_t cb_done) {
    if (enable) {
        return start_sequence(mpu_wom_enable_seq,
                              (uint8_t)(sizeof(mpu_wom_enable_seq) / sizeof(mpu_wom_enable_seq[0])),
                              cb_done);
    }
    return start_sequence(mpu_wom_disable_seq,
                          (uint8_t)(sizeof(mpu_wom_disable_seq) / sizeof(mpu_wom_disable_seq[0])),
                          cb_done);
}

bool mpu_clear_irq_non_blocking(i2c_drv_cb_t cb_done) {
    if (mpu_bus == NULL) {
        return false;
    }
    return (i2c_drv_read_reg(mpu_bus, MPU_ADDR, INT_STATUS,
                             &irq_status_buf, 1u, cb_done) == I2C_DRV_OK);
}

void mpu_request_data_non_blocking(i2c_drv_cb_t cb_done) {
    if (mpu_bus != NULL) {
        (void)i2c_drv_read_reg(mpu_bus, MPU_ADDR, ACCEL_XOUT_H, rx_buf, 6u, cb_done);
    }
}

void mpu_get_last_data(uint8_t *out_buffer) {
    if (out_buffer == NULL) {
        return;
    }
    for (int i = 0; i < 6; i++) {
        out_buffer[i] = rx_buf[i];
    }
}