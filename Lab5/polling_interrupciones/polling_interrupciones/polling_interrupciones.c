/**
 * @file main.c
 * @brief Punto de entrada de la versión Polling+IRQs de la Práctica 5.
 */

/**
 * @mainpage Práctica 5 - Medición de frecuencia y duty (versión Polling+IRQs)
 *
 * @section desc Descripción
 * Instrumento para la Raspberry Pi Pico (RP2040) que mide la frecuencia y el ciclo de
 * trabajo de una señal rectangular y reporta ambas magnitudes al computador por UART, a
 * 10 reportes por segundo. Usa un solo núcleo, sin DMA ni PIO.
 *
 * @section flow Organización Polling+IRQs
 * Las ISR atienden el hardware y solo conservan información. El ciclo principal consulta
 * ese trabajo pendiente, calcula y prepara los reportes. El servicio de transmisión UART
 * se hace por interrupciones.
 *
 * | Contexto                      | Módulo               | Qué hace                                              |
 * |-------------------------------|----------------------|-------------------------------------------------------|
 * | ISR de wrap de PWM (prio. 0)  | capture.c            | Guarda instante y tipo de cada flanco en un anillo    |
 * | ISR de alarma periódica       | capture.c            | Marca "toca reporte" cada 100 ms                      |
 * | ISR de la UART0               | UART.c               | Umbral y timeout de RX, y servicio de TX              |
 * | Ciclo principal               | main.c, measure.c    | Drena flancos, calcula, formatea y entrega al driver  |
 *
 * @section pins Pines
 * | Pin    | Función                                                       |
 * |--------|---------------------------------------------------------------|
 * | GPIO 3 | Entrada de la señal: slice PWM 1, canal B, cuenta subidas     |
 * | GPIO 7 | Entrada de la señal: slice PWM 3, canal B, cuenta bajadas     |
 * | GPIO 0 | TX de la UART0 hacia el adaptador USB a UART                  |
 * | GPIO 1 | RX de la UART0 desde el adaptador USB a UART                  |
 *
 * La señal de entrada (3,3 V, con tierra común) se conecta a los GPIO 3 y 7 a la vez.
 * La UART opera a 115200 bits/s, 8N1, sin control de flujo.
 *
 * @section report Reporte por UART
 * Línea ASCII de hasta 64 bytes, terminada en LF: `n,dt,f,D,estado`, con f en Hz (tres
 * decimales) y D en % (un decimal). Estados: OK, ESPERANDO y SIN_TRANSICIONES. Los
 * detalles están en measure.h.
 *
 * @section counters Conteo de reportes omitidos
 * Tres contadores, que se consultan al finalizar la prueba:
 *  - uart_driver_get_dropped_count(): líneas descartadas por falta de espacio en TX.
 *  - measure_get_late_count(): reportes que no se generaron por llegar tarde el ciclo.
 *  - capture_overflow_count(): flancos perdidos por anillo lleno.
 *
 * Además, uart_driver_get_rx_irq_counts() cuenta las veces que se atendió el umbral y el
 * timeout de recepción.
 *
 * @section build Compilación
 * Con el SDK oficial de Raspberry Pi Pico, enlazando las bibliotecas
 * `pico_stdlib hardware_pwm hardware_uart hardware_irq hardware_timer hardware_sync`.
 * No debe habilitarse stdio sobre UART0, porque esa UART es la del driver propio.
 * La documentación se genera con `doxygen Doxyfile`.
 */

#include "pico/stdlib.h"
#include "UART.h"
#include "capture.h"
#include "measure.h"

/**
 * @brief Inicializa los módulos y ejecuta el ciclo principal.
 *
 * El orden de inicialización es: driver UART, medición y captura. La captura va al final
 * porque es la que arranca las interrupciones de flancos y la alarma de reporte.
 *
 * El ciclo ejecuta, sin bloquear, un paso de la medición.
 *
 * @return No retorna.
 */
int main(void) {
    uart_driver_init();
    measure_init();
    capture_init();

    while (true) {
        measure_poll();
    }
}