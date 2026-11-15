
/**
 * @file    acelerometro_MPU_6050.h
 * @brief   Driver del acelerómetro (mapa de registros, escalas, modos).
 * @details Configura el sensor, solicita muestras a i2c_driver y las publica en
 *          un buffer circular con contador de desbordamientos. Convierte
 *          cuentas a unidades físicas. Ofrece modo de bajo consumo y detección
 *          de movimiento por pin INT para el despertar.
 * @note    Contexto: init antes de iniciar la sesión y fuera de ISR; el callback
 *          de lectura corre en contexto de ISR y solo copia datos.
 * @warning No detecta pasos ni accede a registros del I2C del RP2040.
 * 
 */