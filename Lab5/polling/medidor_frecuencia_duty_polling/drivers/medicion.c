/**
 * @file medicion.c
 * @brief Implementación del módulo de medición de período y tiempo en alto.
 *
 * Implementa la adquisición de parámetros temporales de una señal PWM
 * mediante los periféricos hardware PWM del RP2040.
 *
 * Se utilizan dos contadores independientes que operan simultáneamente:
 * uno para contabilizar los flancos de subida y otro para acumular
 * los ciclos de reloj durante el estado alto de la señal.
 *
 * A partir de los valores obtenidos en una ventana de medición se
 * calculan el período promedio y el tiempo promedio en alto.
 */

#include "medicion.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/timer.h"

/* Slice asociado a PIN_FREQ_B.*/
static uint slice_freq;

/* Slice asociado a PIN_DUTY_B.*/
static uint slice_duty;


/* Período promedio de la señal medido en microsegundos. */
static uint64_t T_us = 0;

/* Tiempo promedio en alto de la señal medido en microsegundos. */
static uint64_t t_H_us = 0;

/* Instante de detección del último flanco de subida. */
static uint64_t last_edge_time = 0;

/* Instante de inicio de la última ventana de medición. */
static uint64_t last_measure_time_us = 0;

void medicion_init(void) {
    /* Obtener los números de slice para cada pin */
    slice_freq = pwm_gpio_to_slice_num(PIN_FREQ_B);
    slice_duty = pwm_gpio_to_slice_num(PIN_DUTY_B);

    /* Configuración del pin para medir FRECUENCIA (Contar flancos de subida) */
    gpio_set_function(PIN_FREQ_B, GPIO_FUNC_PWM);
    pwm_config cfg_freq = pwm_get_default_config();
    pwm_config_set_clkdiv_mode(&cfg_freq, PWM_DIV_B_RISING); /* Incrementa en 1 cada que hay flanco en el canal B*/
    pwm_init(slice_freq, &cfg_freq, false);

    /* Configuración del pin para medir DUTY CYCLE (Medir tiempo en alto) */
    gpio_set_function(PIN_DUTY_B, GPIO_FUNC_PWM);
    pwm_config cfg_duty = pwm_get_default_config();
    pwm_config_set_clkdiv_mode(&cfg_duty, PWM_DIV_B_HIGH); /* CUenta ciclos mientras el pin B esta en high */
    /* Divisor de 250.0f. El reloj queda a 500 kHz (1 tick = 2us) para evitar desbordamiento */ 
    pwm_config_set_clkdiv(&cfg_duty, 250.0f); 
    pwm_init(slice_duty, &cfg_duty, false);

    pwm_set_counter(slice_freq, 0);
    pwm_set_counter(slice_duty, 0);
    
    last_measure_time_us = time_us_64();
    pwm_set_mask_enabled((1u << slice_freq) | (1u << slice_duty));
}

void medicion_actualizar(void) {
    uint64_t now_us = time_us_64();
    uint64_t delta_t = now_us - last_measure_time_us;

    /* Solo actualizamos si ha pasado una ventana de tiempo razonable */
    if (delta_t < 100000) {
        return; 
    }

    /* Leer los contadores de hardware */
    uint16_t flancos_contados = pwm_get_counter(slice_freq);
    uint16_t ticks_en_alto = pwm_get_counter(slice_duty);

    /* Reiniciar los contadores inmediatamente para la siguiente ventana */
    pwm_set_counter(slice_freq, 0);
    pwm_set_counter(slice_duty, 0);
    last_measure_time_us = now_us;

    /* Procesar matemáticamente los datos obtenidos en esta ventana */
    if (flancos_contados > 0) {
        T_us = delta_t / flancos_contados;
        
        /* Multiplicamos ticks_en_alto * 2, cada tick vale 2us */
        t_H_us = ((uint64_t)ticks_en_alto * 2) / flancos_contados;
        
        last_edge_time = now_us;
    } else {
        /* Si no hay flancos, mantenemos T_us en 0 para que reporte_actualizar() sepa que no hay señal */
        T_us = 0; 
        t_H_us = 0;
    }
}

void medicion_obtener_datos(uint64_t* periodo_us, uint64_t* tiempo_alto_us, uint64_t* ultimo_flanco_us) {
    *periodo_us = T_us;
    *tiempo_alto_us = t_H_us;
    *ultimo_flanco_us = last_edge_time;
}