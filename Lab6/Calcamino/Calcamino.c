/**
 * @file    Calcamino.c
 * @brief   Ciclo principal: polling + interrupciones
 * @details Inicializa periféricos antes de la sesión y fuera de ISR. En el bucle
 *          consulta el trabajo pendiente (muestras, pasos, batería, pantalla,
 *          estados) y orquesta los módulos. Cuando no hay trabajo, espera una
 *          interrupción sin dejar eventos pendientes. Reporta los contadores de
 *          desbordamiento.
 * @note    No contiene lógica de programa
 * 
 */


 /**
 * @file Calcamino.c
 * @brief Prueba sencilla de lectura del ADXL345 usando I2C_driver (polling + IRQs).
 */

#include <math.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/sync.h"
#include "I2C_driver.h"

/* Configuración de pines y registros del ADXL345 */
#define I2C_PORT        i2c0
#define I2C_SDA_PIN     4u
#define I2C_SCL_PIN     5u
#define ADXL_ADDR       0x53u

#define ADXL_REG_DEVID      0x00u
#define ADXL_REG_BW_RATE    0x2Cu
#define ADXL_REG_POWER_CTL  0x2Du
#define ADXL_REG_DATA_FMT   0x31u
#define ADXL_REG_DATAX0     0x32u
#define ADXL_G_PER_LSB      (1.0f / 256.0f) /* Modo Full-Res: 3.9 mg/LSB */

/* Buffers persistentes (nunca locales) para las transacciones asíncronas */
static uint8_t g_rx_buf[6];
static uint8_t g_devid = 0u;

/* Banderas compartidas entre las ISR y el ciclo principal */
static volatile bool g_i2c_done = false;
static volatile i2c_drv_status_t g_i2c_status = I2C_DRV_OK;
static volatile bool g_sample_tick = false;

/** @brief Callback invocado desde la ISR de I2C al concluir cada transferencia. */
static void on_i2c_done(i2c_drv_status_t status) {
    g_i2c_status = status;
    g_i2c_done   = true;
}

/** @brief ISR del timer periódico: solo notifica a main() que toca pedir una muestra. */
static bool on_sample_timer(repeating_timer_t *rt) {
    (void)rt;
    g_sample_tick = true;
    return true;
}

/**
 * @brief Espera pasiva (__wfi) usada únicamente durante la configuración previa a la sesión.
 */
static bool wait_init_txn(void) {
    while (!g_i2c_done) {
        uint32_t irq = save_and_disable_interrupts();
        if (!g_i2c_done) {
            __wfi();
        }
        restore_interrupts(irq);
    }
    g_i2c_done = false;
    return (g_i2c_status == I2C_DRV_OK);
}

/** @brief Verifica el ID del ADXL345 y configura ±4g Full-Res a 50 Hz. */
static bool adxl345_setup(void) {
    /* Comandos persistentes en flash {registro, valor} */
    static const uint8_t cmd_fmt[2] = {ADXL_REG_DATA_FMT,  0x09u}; /* Full-Res, ±4g */
    static const uint8_t cmd_odr[2] = {ADXL_REG_BW_RATE,   0x09u}; /* 50 Hz */
    static const uint8_t cmd_pwr[2] = {ADXL_REG_POWER_CTL, 0x08u}; /* Modo medición */

    /* 1. Leer DEVID (debe responder 0xE5) */
    g_i2c_done = false;
    if (i2c_drv_read_reg(I2C_PORT, ADXL_ADDR, ADXL_REG_DEVID, &g_devid, 1u, on_i2c_done) != I2C_DRV_OK) {
        return false;
    }
    if (!wait_init_txn() || (g_devid != 0xE5u)) {
        return false;
    }

    /* 2. Escribir configuración inicial */
    if (i2c_drv_write(I2C_PORT, ADXL_ADDR, cmd_fmt, 2u, on_i2c_done) != I2C_DRV_OK || !wait_init_txn()) return false;
    if (i2c_drv_write(I2C_PORT, ADXL_ADDR, cmd_odr, 2u, on_i2c_done) != I2C_DRV_OK || !wait_init_txn()) return false;
    if (i2c_drv_write(I2C_PORT, ADXL_ADDR, cmd_pwr, 2u, on_i2c_done) != I2C_DRV_OK || !wait_init_txn()) return false;

    return true;
}

int main(void) {
    stdio_init_all();
    sleep_ms(1500); /* Tiempo para abrir el monitor serial USB */

    if (!i2c_drv_init(I2C_PORT, I2C_SDA_PIN, I2C_SCL_PIN, 400000u)) {
        printf("ERROR: No se pudo inicializar I2C0.\n");
        while (true) { __wfi(); }
    }

    if (!adxl345_setup()) {
        printf("ERROR: ADXL345 no detectado (DEVID=0x%02X). Revisa CS->3V3 y SDO->GND.\n", g_devid);
        while (true) { __wfi(); }
    }

    printf("ADXL345 listo (DEVID=0x%02X). Leyendo a 10 Hz...\n", g_devid);

    /* Timer de prueba cada 100 ms (cámbialo a 20 ms para 50 Hz) */
    repeating_timer_t timer;
    add_repeating_timer_ms(100, on_sample_timer, NULL, &timer);

    while (true) {
        /* 1. Disparar lectura no bloqueante de los 6 bytes XYZ cuando toca */
        if (g_sample_tick) {
            g_sample_tick = false;
            (void)i2c_drv_read_reg(I2C_PORT, ADXL_ADDR, ADXL_REG_DATAX0,
                                   g_rx_buf, 6u, on_i2c_done);
        }

        /* 2. Procesar la muestra en main() cuando la ISR de I2C avisa que terminó */
        if (g_i2c_done) {
            g_i2c_done = false;

            if (g_i2c_status == I2C_DRV_OK) {
                /* ADXL345 entrega los bytes en Little-Endian [X0, X1, Y0, Y1, Z0, Z1] */
                int16_t rx = (int16_t)((uint16_t)g_rx_buf[0] | ((uint16_t)g_rx_buf[1] << 8));
                int16_t ry = (int16_t)((uint16_t)g_rx_buf[2] | ((uint16_t)g_rx_buf[3] << 8));
                int16_t rz = (int16_t)((uint16_t)g_rx_buf[4] | ((uint16_t)g_rx_buf[5] << 8));

                float ax = (float)rx * ADXL_G_PER_LSB;
                float ay = (float)ry * ADXL_G_PER_LSB;
                float az = (float)rz * ADXL_G_PER_LSB;
                float mag = sqrtf(ax * ax + ay * ay + az * az);

                printf("ax=%+.3fg  ay=%+.3fg  az=%+.3fg  |a|=%.3fg\n", ax, ay, az, mag);
            } else {
                i2c_drv_stats_t st;
                i2c_drv_get_stats(I2C_PORT, &st);
                printf("ERROR I2C (err=%lu, drop=%lu, max_isr=%lu us)\n",
                       (unsigned long)st.error_count,
                       (unsigned long)st.busy_drop_count,
                       (unsigned long)st.max_isr_us);
            }
        }

        /* 3. Dormir CPU hasta la siguiente interrupción sin perder eventos */
        uint32_t irq = save_and_disable_interrupts();
        if (!g_sample_tick && !g_i2c_done) {
            __wfi();
        }
        restore_interrupts(irq);
    }
}