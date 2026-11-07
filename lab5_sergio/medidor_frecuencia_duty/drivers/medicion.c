#include "medicion.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/sync.h"



static volatile uint64_t T_us = 0;           // Periodo en microsegundos 
static volatile uint64_t t_H_us = 0;         // Tiempo en estado alto en microsegundos 
static volatile uint64_t last_edge_time = 0; // Marca temporal de la última transición


static volatile uint64_t t_rise_prev = 0;
static volatile bool first_edge = true;




static void gpio_callback(uint gpio, uint32_t events) {

    uint64_t now = time_us_64(); 
    

    last_edge_time = now; 

    if (gpio == SIGNAL_PIN) {
        if (events & GPIO_IRQ_EDGE_RISE) {
            if (!first_edge) {
                T_us = now - t_rise_prev; 
            }
            t_rise_prev = now;
            first_edge = false; 
        } 
        else if (events & GPIO_IRQ_EDGE_FALL) {
        
            if (!first_edge) {
                t_H_us = now - t_rise_prev; 
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
    uint32_t estado_previo = save_and_disable_interrupts();

    *periodo_us = T_us;
    *tiempo_alto_us = t_H_us;
    *ultimo_flanco_us = last_edge_time;
    
    restore_interrupts(estado_previo);
}