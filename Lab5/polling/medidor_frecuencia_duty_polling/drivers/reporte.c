#include "reporte.h"
#include "medicion.h"
#include "hardware/uart.h"
#include "hardware/timer.h"
#include "hardware/gpio.h"
#include <stdio.h>        

static uint32_t n_report = 0;            // Número de reporte (n)
static uint64_t last_report_time_us = 0; // Para calcular el delta t (dt) (Faltaba el ;)

void reporte_init(void){
    // Inicializar el UART del SDK
    uart_init(UART_ID, BAUD_RATE);
    
    // Mpaeo de pines del uart.
    gpio_set_function(0, GPIO_FUNC_UART); // TX
    gpio_set_function(1, GPIO_FUNC_UART); // RX
}

void reporte_actualizar(void){
    uint64_t now_us = time_us_64();

    // LÓGICA POLLING: Comprobar si ya pasaron 100ms (100,000 microsegundos)
    if (last_report_time_us != 0 && (now_us - last_report_time_us) < 100000) {
        return;
    }

    uint32_t dt_ms = (last_report_time_us == 0) ? 0 : (uint32_t)((now_us - last_report_time_us) / 1000);
    last_report_time_us = now_us;

    uint64_t T_local_us, t_H_local_us, last_edge_local_us;
    
    medicion_obtener_datos(&T_local_us, &t_H_local_us, &last_edge_local_us);

    char buffer[64];

    if(T_local_us == 0){
        snprintf(buffer, sizeof(buffer), "%lu,%lu,NA,NA,ESPERANDO\n", n_report, dt_ms);
    }
    // Validar si no se ha completado la medición del primer periodo
    else if((now_us - last_edge_local_us) > 500000){
        snprintf(buffer, sizeof(buffer), "%lu,%lu,NA,NA,SIN_TRANSICIONES\n", n_report, dt_ms);
    }
    // Si los datos son frescos y válidos, aplicar matemática de punto flotante
    else{
        // T_local_us está en microsegundos, por lo que f = 1'000.000 / T
        float f_hz = 1000000.0f / (float)T_local_us;
        float duty = 100.0f * ((float)t_H_local_us / (float)T_local_us);

        // Corregido %1f por %.1f para que imprima solo 1 decimal
        snprintf(buffer, sizeof(buffer), "%lu,%lu,%.3f,%.1f,OK\n", n_report, dt_ms, f_hz, duty);
    }
    
    // Enviar el string usando la función del SDK correctamente (sin declarar los tipos de datos)
    uart_puts(UART_ID, buffer);
    n_report++;
}