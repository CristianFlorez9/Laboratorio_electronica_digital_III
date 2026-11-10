/**
 * @file medidor_frecuencia_duty.c
 * @brief Medidor de frecuencia y duty cycle por interrupciones (RP2040).
 *        Mide la señal en SIGNAL_PIN y reporta por UART de forma periódica.
 */

#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "hardware/sync.h"
#include "drivers/UART.h" 
#include "drivers/medicion.h"
#include "drivers/reporte.h"
#include "hardware/irq.h" 

/**
 * @brief Inicializa reloj, drivers y prioridades de interrupción; luego duerme hasta cada IRQ.
 */
int main() {
    // Configuración del reloj del sistema a 125 MHz
    set_sys_clock_khz(125000, true);


    uart_driver_init(); 
    medicion_init();    
    reporte_init();    

    

    // MÁXIMA PRIORIDAD 
    irq_set_priority(IO_IRQ_BANK0, 0x00);

    // PRIORIDAD MÍNIMA 
    irq_set_priority(UART0_IRQ, 0xC0);

    

    while (true) {
        __wfi(); 
    }
}
