/**
 * @file reporte.c
 * @brief Implementación del procesamiento y transmisión de reportes mediante UART.
 *
 * Implementa la adquisición de los resultados del módulo de medición,
 * el cálculo de frecuencia y ciclo de trabajo, la validación del estado
 * de la señal y la transmisión periódica de los datos mediante UART.
 *
 * La generación de reportes utiliza una estrategia de polling,
 * estableciendo un intervalo mínimo entre transmisiones definido por
 * REPORTE_INTERVALO_MS.
 */

#include "reporte.h"
#include "medicion.h"
#include "hardware/uart.h"
#include "hardware/timer.h"
#include "hardware/gpio.h"
#include <stdio.h>

/** Intervalo mínimo entre reportes, en milisegundos.
 *  Se puede sobrescribir antes de incluir este archivo (o desde CMake
 *  con -DREPORTE_INTERVALO_MS=200).
 */
#ifndef REPORTE_INTERVALO_MS
#define REPORTE_INTERVALO_MS 100
#endif

/** Tiempo máximo sin flancos antes de declarar SIN_TRANSICIONES, en ms. */
#ifndef REPORTE_TIMEOUT_SIN_FLANCOS_MS
#define REPORTE_TIMEOUT_SIN_FLANCOS_MS 500
#endif

/* Conversión a microsegundos (la comparación se hace con time_us_64()). */
#define REPORTE_INTERVALO_US          ((uint64_t)(REPORTE_INTERVALO_MS) * 1000ULL)
#define REPORTE_TIMEOUT_SIN_FLANCOS_US ((uint64_t)(REPORTE_TIMEOUT_SIN_FLANCOS_MS) * 1000ULL)

/** Contador consecutivo de reportes transmitidos.
 *  Se incrementa después de cada transmisión UART y permite
 *  identificar cada registro generado por el módulo.
 */
static uint32_t n_report = 0;

/** Instante de transmisión del último reporte.
 *  Se utiliza como referencia para calcular el intervalo de tiempo
 *  transcurrido entre reportes consecutivos.
 */
static uint64_t last_report_time_us = 0;

void reporte_init(void){
    /* Inicializar el UART del SDK */
    uart_init(UART_ID, BAUD_RATE);

    /* Mapeo de pines del uart. */
    gpio_set_function(0, GPIO_FUNC_UART); /* TX */
    gpio_set_function(1, GPIO_FUNC_UART); /* RX */
}

void reporte_actualizar(void){
    uint64_t now_us = time_us_64();

    /* LÓGICA POLLING: Comprobar si ya pasó el intervalo configurado */
    if (last_report_time_us != 0 && (now_us - last_report_time_us) < REPORTE_INTERVALO_US) {
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
    /* Validar si dejaron de llegar flancos */
    else if((now_us - last_edge_local_us) > REPORTE_TIMEOUT_SIN_FLANCOS_US){
        snprintf(buffer, sizeof(buffer), "%lu,%lu,NA,NA,SIN_TRANSICIONES\n", n_report, dt_ms);
    }
    /* Si los datos son frescos y válidos, aplicar matemática de punto flotante */
    else{
        /* T_local_us está en microsegundos, por lo que f = 1'000.000 / T */
        float f_hz = 1000000.0f / (float)T_local_us;
        float duty = 100.0f * ((float)t_H_local_us / (float)T_local_us);

        snprintf(buffer, sizeof(buffer), "%lu,%lu,%.3f,%.1f,OK\n", n_report, dt_ms, f_hz, duty);
    }

    /* Enviar el string usando la función del SDK */
    uart_puts(UART_ID, buffer);
    n_report++;
}
