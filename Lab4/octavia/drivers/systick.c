/**
 * @file systick.c
 * @brief Implementación del driver propio del SysTick por acceso a registros.
 *
 * Registros usados (todos en el bloque PPB del Cortex-M0+):
 * - SYST_CSR: control y estado. Bit 0 ENABLE, bit 1 TICKINT (habilita la
 *   excepción), bit 2 CLKSOURCE (1 = reloj del procesador).
 * - SYST_RVR: valor de recarga de 24 bits.
 * - SYST_CVR: valor actual del contador; cualquier escritura lo pone a cero.
 * - SCB_SHPR3: prioridad de las excepciones del sistema; el byte alto
 *   (bits 31:24) corresponde al SysTick.
 *
 * Las funciones públicas están documentadas en systick.h.
 */

#include "systick.h"
#include "hardware/regs/m0plus.h"
#include "hardware/regs/addressmap.h"

/** @brief Registro de control y estado del SysTick. */
#define SYST_CSR  ((volatile uint32_t *)(PPB_BASE + M0PLUS_SYST_CSR_OFFSET))

/** @brief Registro de valor de recarga del SysTick (24 bits). */
#define SYST_RVR  ((volatile uint32_t *)(PPB_BASE + M0PLUS_SYST_RVR_OFFSET))

/** @brief Registro del valor actual del contador del SysTick. */
#define SYST_CVR  ((volatile uint32_t *)(PPB_BASE + M0PLUS_SYST_CVR_OFFSET))

/** @brief Registro de prioridad de las excepciones del sistema (SysTick en bits 31:24). */
#define SCB_SHPR3 ((volatile uint32_t *)(PPB_BASE + M0PLUS_SHPR3_OFFSET))

/** @brief Bit ENABLE de SYST_CSR: enciende el contador. */
#define SYST_CSR_ENABLE    (1u << 0)

/** @brief Bit TICKINT de SYST_CSR: genera la excepción al llegar a cero. */
#define SYST_CSR_TICKINT   (1u << 1)

/** @brief Bit CLKSOURCE de SYST_CSR: 1 = reloj del procesador. */
#define SYST_CSR_CLKSOURCE (1u << 2)

/** @brief Máscara de los 24 bits válidos de SYST_RVR. */
#define SYST_MASCARA_24    0x00FFFFFFu

/**
 * @brief Prioridad del SysTick (el RP2040 solo implementa los 2 bits altos).
 *
 * 0x80 = igual que las IRQ de GPIO por defecto, así no se interrumpen entre sí.
 * Se puede redefinir en tiempo de compilación.
 */
#ifndef SYSTICK_PRIORIDAD
#define SYSTICK_PRIORIDAD 0x80u
#endif

void systick_init(uint32_t reload_value)
{
    *SYST_CSR = 0;                                    /* apagar mientras se configura */

    /* Prioridad del SysTick: byte alto de SHPR3 (bits 31:24). */
    *SCB_SHPR3 = (*SCB_SHPR3 & 0x00FFFFFFu) | ((uint32_t)SYSTICK_PRIORIDAD << 24);

    *SYST_RVR = reload_value & SYST_MASCARA_24;
    *SYST_CVR = 0;                                    /* cualquier escritura limpia el contador */

    *SYST_CSR = SYST_CSR_CLKSOURCE | SYST_CSR_TICKINT | SYST_CSR_ENABLE;
}

void systick_set_reload(uint32_t reload_value)
{
    *SYST_RVR = reload_value & SYST_MASCARA_24;
}

void systick_set_state(bool enable)
{
    if (enable) {
        *SYST_CSR |= SYST_CSR_ENABLE;
    } else {
        *SYST_CSR &= ~SYST_CSR_ENABLE;
    }
}