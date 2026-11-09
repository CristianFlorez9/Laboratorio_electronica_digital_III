#include "reporte.h"
#include "medicion.h"
#include "UART.h"
#include "hardware/timer.h"
#include "pico/stdlib.h"
#include <stdio.h> 


static struct repeating_timer timer_100ms;


static uint32_t n_report = 0;          
static uint64_t last_report_time_us = 0;


static bool timer_100ms_isr_callback(struct repeating_timer *t) {
    uint64_t now_us = time_us_64();

    uint32_t dt_ms = (last_report_time_us == 0) ? 0 : (uint32_t)((now_us - last_report_time_us) / 1000);
    last_report_time_us = now_us;


    uint64_t T_local_us, t_H_local_us, last_edge_local_us;
    medicion_obtener_datos(&T_local_us, &t_H_local_us, &last_edge_local_us);

    char buffer[64];

    if (T_local_us == 0) {
        snprintf(buffer, sizeof(buffer), "%lu,%lu,NA,NA,ESPERANDO\n", n_report, dt_ms);
    } 

    else if ((now_us - last_edge_local_us) > 500000) {
        snprintf(buffer, sizeof(buffer), "%lu,%lu,NA,NA,SIN_TRANSICIONES\n", n_report, dt_ms);
    } 

    else {

        float f_hz = 1000000.0f / (float)T_local_us;
        float duty = 100.0f * ((float)t_H_local_us / (float)T_local_us);
        

        snprintf(buffer, sizeof(buffer), "%lu,%lu,%.3f,%.1f,OK\n", n_report, dt_ms, f_hz, duty);
    }
    uart_driver_send_string(buffer);
    n_report++; 

    return true;
}




void reporte_init(void) {
    // El valor negativo garantiza que el periodo sea de 100 ms exactos 
    // entre cada inicio de interrupción, compensando el tiempo de ejecución del callback.
    add_repeating_timer_ms(-60, timer_100ms_isr_callback, NULL, &timer_100ms);
}