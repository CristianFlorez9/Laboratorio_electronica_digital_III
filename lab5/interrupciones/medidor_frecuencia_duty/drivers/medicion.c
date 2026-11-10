#include "medicion.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/sync.h"



static volatile uint64_t T_us = 0;           // Periodo en microsegundos 
static volatile uint64_t t_H_us = 0;         // Tiempo en estado alto en microsegundos 
static volatile uint64_t last_edge_time = 0; // Marca temporal de la última transición


/** @brief Instante del flanco de subida anterior [µs]. */
static volatile uint64_t t_rise_prev = 0;
/** @brief true hasta recibir el primer flanco de subida (evita medidas inválidas). */
static volatile bool first_edge = true;




/**
 * @brief ISR de GPIO: en subida calcula el periodo; en bajada calcula el tiempo en alto.
 * @param gpio   Pin que generó la interrupción.
 * @param events Máscara de eventos (flanco de subida/bajada).
 */
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



// Configura el pin de entrada y la interrupción por ambos flancos.
void medicion_init(void) {
    gpio_init(SIGNAL_PIN);
    gpio_set_dir(SIGNAL_PIN, GPIO_IN);
    

    gpio_pull_down(SIGNAL_PIN);

    gpio_set_irq_enabled_with_callback(SIGNAL_PIN, 
                                       GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, 
                                       true, 
                                       &gpio_callback);
}





// Lectura atómica: se deshabilitan las IRQ para que la ISR no modifique los datos a mitad de copia.
void medicion_obtener_datos(uint64_t* period_us, uint64_t* time_high_us, uint64_t* last_flank_us) {
    uint32_t previous_state = save_and_disable_interrupts();

    *period_us = T_us;
    *time_high_us = t_H_us;
    *last_flank_us = last_edge_time;
    
    restore_interrupts(previous_state);
}
