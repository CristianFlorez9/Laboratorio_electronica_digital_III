/**
 * @file main.c
 * @brief Programa principal e integración de los módulos de medición y reporte.
 *
 * Inicializa los módulos de medición y comunicación UART, y ejecuta
 * continuamente sus funciones de actualización mediante una estrategia
 * de polling.
 *
 * El ciclo principal ejecuta las siguientes operaciones:
 * - Actualización de las mediciones mediante los contadores hardware PWM.
 * - Procesamiento y transmisión periódica de los resultados mediante UART.
 *
 * @note La función de medición se ejecuta continuamente para procesar
 *       los contadores y evitar retrasos innecesarios en su lectura.
 * @note La transmisión de reportes se encuentra limitada internamente
 *       a intervalos mínimos de 100 ms.
 */

#include "medicion.h"
#include "reporte.h"

int main() {
    /* Inicialización */
    medicion_init();
    reporte_init();

    while (1) {
        /* Ejecutar a máxima velocidad para no perder flancos */
        medicion_actualizar(); 
        
        /** Ejecutar a máxima velocidad (la función internamente 
        * decide si ignora el llamado o transmite porque ya pasaron 100ms) */
        reporte_actualizar();  
    }
}