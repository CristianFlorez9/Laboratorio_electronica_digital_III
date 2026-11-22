/**
 * @file    interfaz_grafica.h
 * @brief   Interfaz de usuario: pantallas de sesión y de configuración.
 * @details Compone en el framebuffer los pasos (entero), las kcal (con unidad)
 *          y el porcentaje de batería, de una misma sesión. Gestiona la
 *          configuración de parámetros personales y el inicio o reinicio de
 *          sesión sin modificar el código fuente.
 * @note    Lanza el envío asíncrono y no espera su finalización.
 * 
 */

#ifndef INTERFAZ_GRAFICA_H
#define INTERFAZ_GRAFICA_H

#include <stdint.h>
#include <stdbool.h>

// interfaz de configuración de parámetros personales (kcal, peso, altura, edad, sexo)
void interfaz_cnfg(uint32_t kcal, uint32_t peso, uint32_t altura, uint32_t bateria);


// interfaz de sesión: pasos, estado, cadencia, kcal, batería
void interfaz_sesion(uint32_t pasos, float estado, int cadence, uint32_t kcal, uint32_t bateria);


#endif /* INTERFAZ_GRAFICA_H */
