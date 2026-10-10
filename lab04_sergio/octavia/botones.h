/**
 * @file botones.h
 * @brief Definiciones de pines, máscaras de bits y prototipos para el control de los pulsadores de entrada.
 * @author Sergio Alarcón
 * @date 2026
 */

#ifndef BOTONES_H 
#define BOTONES_H
#include "pico/stdlib.h"
#include <stdint.h>

/** @name Definición de Pines GPIO para los Pulsadores */
/** @{ */
#define BOTON_0 14 /**< Pin GPIO asignado al botón principal de inicio y reinicio. */
#define BOTON_1 21/**< Pin GPIO asignado al primer botón de juego. */
#define BOTON_2 20/**< Pin GPIO asignado al segundo botón de juego. */
#define BOTON_3 19/**< Pin GPIO asignado al tercero botón de juego. */
#define BOTON_4 18/**< Pin GPIO asignado al cuarto botón de juego. */
/** @} */

/** @name Máscaras de Bits para las Entradas */
/** @{ */
#define MASK_BOTON_0 (1u << BOTON_0) /**< Máscara de bits para el botón principal. */
#define MASK_BOTON_1 (1u << BOTON_1)/**< Máscara de bits para el botón de juego 1. */
#define MASK_BOTON_2 (1u << BOTON_2)/**< Máscara de bits para el botón de juego 2. */
#define MASK_BOTON_3 (1u << BOTON_3)/**< Máscara de bits para el botón de juego 3. */
#define MASK_BOTON_4 (1u << BOTON_4)/**< Máscara de bits para el botón de juego 4. */
/** Máscara combinada que agrupa todos los pines de los pulsadores para operaciones conjuntas. */
#define ALL_BOTONES_MASK (MASK_BOTON_0 | MASK_BOTON_1 | MASK_BOTON_2 | MASK_BOTON_3 | MASK_BOTON_4)
/** @} */


void botones_init(void);
/**
 * @brief Inicializa los pines GPIO correspondientes a los pulsadores.
 * @details Configura los pines como entradas digitales habilitando sus resistencias internas de \em pull-up.
 */


bool boton_read(uint32_t boton_pin);
/**
 * @brief Lee el estado lógico actual de un pulsador específico.
 * @param boton_pin Pin GPIO del botón que se desea consultar (ej. BOTON_1).
 * @return \c true si el botón está suelto (nivel alto por pull-up), \c false si está presionado (nivel bajo).
 */


uint32_t botones_read_all_masked(void);
/**
 * @brief Lee de manera simultánea el estado de todos los pines de los pulsadores.
 * @return Un valor de 32 bits con el estado lógico de la máscara de botones.
 */
#endif