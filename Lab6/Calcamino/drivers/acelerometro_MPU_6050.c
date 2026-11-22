#include "acelerometro_MPU_6050.h"
#include <stddef.h>

// --- Buffers y estado interno del driver ---
// Se declaran estáticos para que persistan en memoria y no bloqueen a otras funciones
static uint8_t tx_buf[2];
static uint8_t rx_buf[6]; // 6 bytes para X_H, X_L, Y_H, Y_L, Z_H, Z_L
static i2c_inst_t *mpu_bus = NULL;
static bool mpu_is_ready = false;

// --- Máquina de estados para la inicialización ---
typedef struct {
    uint8_t reg;
    uint8_t val;
} mpu_cfg_step_t;

// Secuencia de configuración del MPU
static const mpu_cfg_step_t mpu_init_sequence[] = {
    {PWR_MGMT,  0x00}, // Despertar (Registro 0x6B = 0x00)
    {PWR_STBY,  0x07}, // Giroscopio en standby
    {ACCEL_CFG, 0x01}, // Filtro paso alto a 5 Hz
    {0x1F,      15},   // Umbral de movimiento
    {0x20,      2},    // Duración mínima
    {IRQ_CFG,   0x20}, // Latch activo
    {IRQ_EN,    0x40}, // Habilitar MOT_EN
    {PWR_STBY,  0x47}, // Frec. despertar = 5 Hz
    {PWR_MGMT,  0x28}  // Modo ciclo bajo consumo
};

static uint8_t init_step = 0;

// Callback que se ejecuta cada vez que el I2C termina de enviar 1 registro
static void mpu_init_callback(i2c_drv_status_t status) {
    if (status != I2C_DRV_OK) {
        // Manejo de error básico: detener inicialización
        return;
    }

    init_step++;
    
    // ¿Quedan pasos en la secuencia?
    if (init_step < (sizeof(mpu_init_sequence) / sizeof(mpu_init_sequence[0]))) {
        tx_buf[0] = mpu_init_sequence[init_step].reg;
        tx_buf[1] = mpu_init_sequence[init_step].val;
        
        // Enviar siguiente paso y encadenar este mismo callback
        i2c_drv_write(mpu_bus, MPU_ADDR, tx_buf, 2, mpu_init_callback);
    } else {
        // La secuencia terminó exitosamente
        mpu_is_ready = true;
    }
}

// --- Implementación de la API Pública ---

void mpu_init_non_blocking(i2c_inst_t *i2c) {
    mpu_bus = i2c;
    init_step = 0;
    mpu_is_ready = false;

    // Nota: La configuración del GPIO (INT_PIN) se removió de aquí porque 
    // en esta arquitectura corresponde al main() gestionar las interrupciones externas.

    // Arrancar el primer paso de la secuencia
    tx_buf[0] = mpu_init_sequence[0].reg;
    tx_buf[1] = mpu_init_sequence[0].val;
    
    i2c_drv_write(mpu_bus, MPU_ADDR, tx_buf, 2, mpu_init_callback);
}

bool mpu_is_init_complete(void) {
    return mpu_is_ready;
}

void mpu_request_data_non_blocking(i2c_drv_cb_t cb_done) {
    if (mpu_bus != NULL) {
        // Solicitar lectura de 6 bytes desde el registro base de aceleración (0x3B)
        i2c_drv_read_reg(mpu_bus, MPU_ADDR, ACCEL_XOUT_H, rx_buf, 6, cb_done);
    }
}

void mpu_get_last_data(uint8_t *out_buffer) {
    // Copiar los datos del buffer interno al buffer del usuario
    for (int i = 0; i < 6; i++) {
        out_buffer[i] = rx_buf[i];
    }
}
