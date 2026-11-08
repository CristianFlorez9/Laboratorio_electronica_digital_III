#include "medicion.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/timer.h"

static uint slice_freq;
static uint slice_duty;

// Variables para los resultados
static uint64_t T_us = 0;             
static uint64_t t_H_us = 0;           
static uint64_t last_edge_time = 0;

// Variables de control de tiempo para la ventana de medición
static uint64_t last_measure_time_us = 0;

void medicion_init(void) {
    // 1. Obtener los números de slice para cada pin
    slice_freq = pwm_gpio_to_slice_num(PIN_FREQ_B);
    slice_duty = pwm_gpio_to_slice_num(PIN_DUTY_B);

    // 2. Configurar el pin para medir FRECUENCIA (Contar flancos de subida)
    gpio_set_function(PIN_FREQ_B, GPIO_FUNC_PWM);
    pwm_config cfg_freq = pwm_get_default_config();
    // Modo contador: incrementa 1 con cada flanco de subida en el pin B
    pwm_config_set_clkdiv_mode(&cfg_freq, PWM_DIV_B_RISING); 
    pwm_init(slice_freq, &cfg_freq, false);

    // 3. Configurar el pin para medir DUTY CYCLE (Medir tiempo en alto)
    gpio_set_function(PIN_DUTY_B, GPIO_FUNC_PWM);
    pwm_config cfg_duty = pwm_get_default_config();
    // Modo high: cuenta ciclos de reloj del sistema MIENTRAS el pin B esté en ALTO
    pwm_config_set_clkdiv_mode(&cfg_duty, PWM_DIV_B_HIGH);
    // Hacemos que cuente a 1 MHz (1 tick = 1 microsegundo) para cálculos fáciles.
    // Reloj por defecto de Pico es 125 MHz. 125M / 125 = 1 MHz.
    pwm_config_set_clkdiv(&cfg_duty, 125.0f); 
    pwm_init(slice_duty, &cfg_duty, false);

    // 4. Inicializar contadores a 0 y arrancar ambos slices al mismo tiempo
    pwm_set_counter(slice_freq, 0);
    pwm_set_counter(slice_duty, 0);
    
    last_measure_time_us = time_us_64();
    pwm_set_mask_enabled((1u << slice_freq) | (1u << slice_duty));
}

void medicion_actualizar(void) {
    uint64_t now_us = time_us_64();
    uint64_t delta_t = now_us - last_measure_time_us;

    // Solo actualizamos si ha pasado una ventana de tiempo razonable (ej. 100ms)
    // Esto evita dividir por ceros o tener lecturas ruidosas instantáneas.
    if (delta_t < 100000) {
        return; 
    }

    // 1. Leer los contadores de hardware
    uint16_t flancos_contados = pwm_get_counter(slice_freq);
    uint16_t ticks_en_alto = pwm_get_counter(slice_duty);

    // 2. Reiniciar los contadores inmediatamente para la siguiente ventana
    pwm_set_counter(slice_freq, 0);
    pwm_set_counter(slice_duty, 0);
    last_measure_time_us = now_us;

    // 3. Procesar matemáticamente los datos obtenidos en esta ventana
    if (flancos_contados > 0) {
        // Periodo = Tiempo total transcurrido / Número de flancos ocurridos
        T_us = delta_t / flancos_contados;
        
        // Tiempo en alto = Ticks totales en alto / Número de flancos (pulsos) ocurridos
        // Como configuramos el divisor a 125.0, cada tick equivale a 1 microsegundo.
        t_H_us = (uint64_t)ticks_en_alto / flancos_contados;
        
        // El último flanco lo aproximamos al momento de lectura (ya que el hardware cuenta, no hace timestamp)
        last_edge_time = now_us;
    } else {
        // Si no hubo flancos, mantenemos T_us en 0 para que reporte_actualizar() sepa que no hay señal
        T_us = 0; 
        t_H_us = 0;
    }
}

void medicion_obtener_datos(uint64_t* periodo_us, uint64_t* tiempo_alto_us, uint64_t* ultimo_flanco_us) {
    *periodo_us = T_us;
    *tiempo_alto_us = t_H_us;
    *ultimo_flanco_us = last_edge_time;
}