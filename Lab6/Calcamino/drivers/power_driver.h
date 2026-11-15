/**
 * @file    power_driver.h
 * @brief   Gestión de energía del RP2040 (SLEEP/DORMANT y despertar).
 * @details Configura fuentes de despertar, reduce o apaga relojes y periféricos
 *          no usados, y entra al estado de ahorro con secuencia sin carrera:
 *          deshabilitar IRQ, revisar flags pendientes y ejecutar WFI.
 *          Restaura relojes al despertar si se usa DORMANT.
 * @warning No decide cuándo dormir (responsabilidad de state_machine).
 * 
 */