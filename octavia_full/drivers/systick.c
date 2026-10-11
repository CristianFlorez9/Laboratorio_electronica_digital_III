#include "systick.h"
#include "hardware/regs/m0plus.h"
#include "hardware/regs/addressmap.h"

#define SYST_CSR  ((volatile uint32_t *)(PPB_BASE + M0PLUS_SYST_CSR_OFFSET))
#define SYST_RVR  ((volatile uint32_t *)(PPB_BASE + M0PLUS_SYST_RVR_OFFSET))
#define SYST_CVR  ((volatile uint32_t *)(PPB_BASE + M0PLUS_SYST_CVR_OFFSET))
#define SCB_SHPR3 ((volatile uint32_t *)(PPB_BASE + M0PLUS_SHPR3_OFFSET))

#define SYST_CSR_ENABLE    (1u << 0)
#define SYST_CSR_TICKINT   (1u << 1)
#define SYST_CSR_CLKSOURCE (1u << 2)
#define SYST_MASCARA_24    0x00FFFFFFu

/* Prioridad del SysTick (el RP2040 solo implementa los 2 bits altos).
 * 0x80 = igual que las IRQ de GPIO por defecto, asi no se interrumpen entre si. */
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