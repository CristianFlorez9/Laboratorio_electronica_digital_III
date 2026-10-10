#include "systick.h"
#include "hardware/regs/m0plus.h"
#include "hardware/regs/addressmap.h"






#define SYST_CSR ((volatile uint32_t *)(PPB_BASE + M0PLUS_SYST_CSR_OFFSET))
#define SYST_RVR ((volatile uint32_t *)(PPB_BASE + M0PLUS_SYST_RVR_OFFSET))
#define SYST_CVR ((volatile uint32_t *)(PPB_BASE + M0PLUS_SYST_CVR_OFFSET))

void systick_init(uint32_t reload_value) {
    *SYST_CSR = 0x00000000;
    
    //  Cargar el valor de recarga (limitado a 24 bits mediante una máscara AND)
    // Esto asegura que valores > 16.777.215 no corrompan bits reservados.
    *SYST_RVR = reload_value & 0x00FFFFFF;
    
    *SYST_CVR = 0;
    // Bit 2 (CLKSOURCE) = 1 (Reloj del procesador)
    // Bit 1 (TICKINT) = 1 (Habilitar excepción/interrupción)
    // Bit 0 (ENABLE) = 1 (Iniciar contador)

    *SYST_CSR = 0x00000007;
}

void systick_set_state(bool enable) {
    if (enable) {
        // Enciende el bit 0 (ENABLE) sin alterar los bits de reloj e interrupción
        *SYST_CSR |= 0x00000001; 
    } else {
        // Apaga exclusivamente el bit 0 (ENABLE)
        *SYST_CSR &= ~0x00000001; 
    }
}