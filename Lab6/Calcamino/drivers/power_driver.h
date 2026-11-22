/**
 * @file    power_driver.h
 * @brief   Gestión de energía del RP2040 (SLEEP con clock-gating y despertar).
 * @details Configura fuentes de despertar, reduce frecuencia de reloj, apaga PLLs y
 *          periféricos no usados durante el reposo, y ejecuta espera WFI sin carrera.
 * @warning No decide cuándo dormir (responsabilidad de state_machine).
 */

#ifndef POWER_DRIVER_H
#define POWER_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Configura las máscaras de clock-gating de reposo del RP2040.
 */
void power_init(void);

/**
 * @brief Espera una interrupción sin condición de carrera hasta que haya trabajo
 *        o venza el tiempo máximo indicado.
 * @param hay_trabajo Puntero a bandera volátil que indica eventos pendientes en main.
 * @param timeout_ms  Tiempo máximo de espera [ms] (0 = esperar indefinidamente hasta IRQ).
 */
void power_esperar_evento(volatile const bool *hay_trabajo, uint32_t timeout_ms);

/**
 * @brief Reduce consumo del MCU para estado BAJO_CONSUMO: baja clk_sys a 12 MHz (XOSC),
 *        apaga PLLs y deshabilita relojes de periféricos durante WFI.
 */
void power_entrar_bajo_consumo(void);

/**
 * @brief Restaura PLL_SYS a 125 MHz y relojes normales tras salir de BAJO_CONSUMO.
 */
void power_salir_bajo_consumo(void);

/**
 * @brief Consulta si el microcontrolador está actualmente en perfil de bajo consumo.
 * @return true si el perfil de bajo consumo está activo.
 */
bool power_en_bajo_consumo(void);

#endif /* POWER_DRIVER_H */