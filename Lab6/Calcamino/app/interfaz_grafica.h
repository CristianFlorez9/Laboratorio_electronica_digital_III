/**
 * @file    interfaz_grafica.h
 * @brief   Interfaz de usuario: pantallas de sesión y de configuración.
 * @details Compone en el framebuffer los pasos (entero), las kcal (con unidad)
 *          y el porcentaje de batería de una misma sesión. Gestiona la
 *          visualización de configuración de parámetros personales sin modificar
 *          el código fuente.
 * @note    Lanza el envío asíncrono mediante oled_show() y no espera su finalización.
 */

#ifndef INTERFAZ_GRAFICA_H
#define INTERFAZ_GRAFICA_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Renderiza y envía la pantalla de configuración de parámetros personales.
 * @param kcal    Gasto calórico acumulado actual [kcal].
 * @param peso    Masa corporal configurada [kg].
 * @param altura  Estatura configurada [cm].
 * @param bateria Porcentaje de batería restante [0..100].
 * @param campo   Campo en edición: 0 = peso, 1 = estatura.
 */
void interfaz_cnfg(uint32_t kcal, uint32_t peso, uint32_t altura, uint32_t bateria, uint8_t campo);

/**
 * @brief Renderiza y envía la pantalla principal de sesión de caminata.
 * @param pasos   Número acumulado de pasos en la sesión.
 * @param estado  Identificador numérico del estado actual de la máquina.
 * @param cadence Cadencia actual en pasos por minuto [SPM].
 * @param kcal    Gasto calórico estimado acumulado [kcal].
 * @param bateria Porcentaje de batería restante [0..100].
 */
void interfaz_sesion(uint32_t pasos, float estado, int cadence, uint32_t kcal, uint32_t bateria);

#endif /* INTERFAZ_GRAFICA_H */