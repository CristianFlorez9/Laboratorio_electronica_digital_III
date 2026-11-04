#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "hardware/sync.h"
#include "drivers/UART.h" 
#include "drivers/medicion.h"
#include "drivers/reporte.h"
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/sync.h"
#include "hardware/irq.h" 

int main() {
    // 1. Configuración del reloj del sistema a 125 MHz
    set_sys_clock_khz(125000, true);

    // 2. Inicialización de los periféricos y registro de callbacks
    uart_driver_init(); // Configura pines TX/RX, FIFOs y registra la ISR de UART
    medicion_init();    // Configura pin GPIO de entrada y registra la ISR de flancos
    reporte_init();     // Configura la alarma periódica para la ISR del temporizador

    // 3. Configuración de Prioridades del NVIC (ARM Cortex-M0+)
    // En el RP2040, las prioridades son 0x00 (Máxima), 0x40 (Alta), 0x80 (Media), 0xC0 (Baja).
    // Por defecto, todas las interrupciones en el SDK inician en 0x80 (Media).

    // MÁXIMA PRIORIDAD (0x00): Captura de la señal física.
    // Esto es vital: Si llega un flanco (ej. a 10 kHz) justo cuando el Temporizador está
    // ocupado ejecutando las divisiones o el snprintf, el hardware pausará al snprintf, 
    // tomará el tiempo exacto del flanco y luego continuará el reporte.
    irq_set_priority(IO_IRQ_BANK0, 0x00);

    // PRIORIDAD MÍNIMA (0xC0): Transmisión UART.
    // La transmisión solo saca datos de la RAM hacia el cable. Es la tarea menos 
    // crítica en tiempo, por lo que nunca debe bloquear ni al Timer ni al GPIO.
    // (Asume que usas uart0. Si usas uart1, cambia a UART1_IRQ).
    irq_set_priority(UART0_IRQ, 0xC0);

    // NOTA: La interrupción del temporizador (Timer) no se modifica, quedándose
    // con su prioridad por defecto (0x80). Así logramos: GPIO > Timer > UART.

    // 4. Lazo Principal Inactivo (Cumplimiento estricto de IRQs Puras)[cite: 2, 3]
    while (true) {
        // "Wait For Interrupt": El procesador apaga sus ciclos de reloj y entra en
        // bajo consumo hasta que un periférico dispare alguna de las 3 interrupciones.
        __wfi(); 
    }
}