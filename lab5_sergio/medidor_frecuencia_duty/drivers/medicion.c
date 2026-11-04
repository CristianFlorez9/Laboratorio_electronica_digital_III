#include "medicion.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/sync.h"



static volatile uint64_t T_us = 0;           // Periodo en microsegundos (T)
static volatile uint64_t t_H_us = 0;         // Tiempo en estado alto en microsegundos (tH)
static volatile uint64_t last_edge_time = 0; // Marca temporal de la última transición


static volatile uint64_t t_rise_prev = 0;    
static volatile bool first_edge = true;      




static void gpio_callback(uint gpio, uint32_t events) {
    // Tomar la marca de tiempo exacta del hardware apenas ocurre el flanco
    uint64_t now = time_us_64(); 
    
    // Actualizar siempre el momento del último flanco para el timeout de 500 ms
    last_edge_time = now; 

    if (gpio == SIGNAL_PIN) {
        if (events & GPIO_IRQ_EDGE_RISE) {
            // FLANCO DE SUBIDA: Termina un ciclo y empieza uno nuevo
            if (!first_edge) {
                T_us = now - t_rise_prev; // Calculamos el periodo completo
            }
            t_rise_prev = now; // Guardamos esta subida como referencia para el futuro
            first_edge = false; // Descartamos la primera medición basura
        } 
        else if (events & GPIO_IRQ_EDGE_FALL) {
            // FLANCO DE BAJADA: Termina el tiempo en estado alto
            if (!first_edge) {
                t_H_us = now - t_rise_prev; // Calculamos el ancho del pulso alto
            }
        }
    }
}

void medicion_init(void) {
    gpio_init(SIGNAL_PIN);
    gpio_set_dir(SIGNAL_PIN, GPIO_IN);
    

    gpio_pull_down(SIGNAL_PIN);

    gpio_set_irq_enabled_with_callback(SIGNAL_PIN, 
                                       GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, 
                                       true, 
                                       &gpio_callback);
}

void medicion_obtener_datos(uint64_t* periodo_us, uint64_t* tiempo_alto_us, uint64_t* ultimo_flanco_us) {
    // SECCIÓN CRÍTICA: Apagamos momentáneamente TODAS las interrupciones del procesador.
    // Esto garantiza que el GPIO no nos interrumpa a mitad de copiar estos 3 números de 64 bits.
    uint32_t estado_previo = save_and_disable_interrupts();
    
    // Realizamos una copia rápida y segura a los punteros proporcionados
    *periodo_us = T_us;
    *tiempo_alto_us = t_H_us;
    *ultimo_flanco_us = last_edge_time;
    
    // Rehabilitamos las interrupciones inmediatamente para no afectar la medición
    restore_interrupts(estado_previo);
}