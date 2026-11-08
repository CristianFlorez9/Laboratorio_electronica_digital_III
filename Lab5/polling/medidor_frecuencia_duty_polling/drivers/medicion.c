#include "medicion.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"

// Variables para cálculos
static uint64_t T_us = 0;             // Periodo en microsegundos
static uint64_t t_H_us = 0;           // Tiempo en alto en microsegundos
static uint64_t last_edge_time = 0;   // Marca temporal del último cambio

static uint64_t t_rise_prev = 0;
static bool first_edge = true;

// Variable crucial para el polling: recordar cómo estaba el pin la última vez que lo leímos
static bool estado_previo_pin = false;

void medicion_init(void){
    gpio_init(SIGNAL_PIN);
    gpio_set_dir(SIGNAL_PIN, GPIO_IN);
    gpio_pull_down(SIGNAL_PIN);
    
    // Leemos el estado inicial del pin
    estado_previo_pin = gpio_get(SIGNAL_PIN);
}

void medicion_actualizar(void){
    // 1. Leer el estado actual del pin
    bool estado_actual_pin = gpio_get(SIGNAL_PIN);
    uint64_t now = time_us_64();

    // 2. Detectar flancos comparando el estado actual con el anterior
    if (estado_actual_pin == true && estado_previo_pin == false) {
        // --- FLANCO DE SUBIDA (pasó de 0 a 1) ---
        last_edge_time = now;
        
        if (!first_edge) {
            T_us = now - t_rise_prev;
        }
        t_rise_prev = now;
        first_edge = false;
        
    } 
    else if (estado_actual_pin == false && estado_previo_pin == true) {
        // --- FLANCO DE BAJADA (pasó de 1 a 0) ---
        last_edge_time = now;
        
        if (!first_edge) {
            t_H_us = now - t_rise_prev;
        }
    }

    // 3. Actualizar el estado previo para la próxima lectura
    estado_previo_pin = estado_actual_pin;
}

void medicion_obtener_datos(uint64_t* periodo_us, uint64_t* tiempo_alto_us, uint64_t* ultimo_flanco_us){
    // Corregidos los errores de tipeo de las variables
    *periodo_us = T_us;
    *tiempo_alto_us = t_H_us;
    *ultimo_flanco_us = last_edge_time;
}