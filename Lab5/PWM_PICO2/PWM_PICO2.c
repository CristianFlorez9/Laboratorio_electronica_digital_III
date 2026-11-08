#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

#define PWM_GPIO 13

int main(void)
{
    stdio_init_all();

    uint32_t fclk = clock_get_hz(clk_sys);

    gpio_set_function(PWM_GPIO, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(PWM_GPIO);

    // Esperar 10 segundos para permitir conexión USB
    sleep_ms(10000);

    char entrada[50];

    while (true)
    {
        printf("\n");
        printf("====================================\n");
        printf("Ingrese: frecuencia_Hz duty_pct\n");
        printf("Ejemplo: 1000 25\n");
        printf("====================================\n");

        // Leer una línea completa
        if (fgets(entrada, sizeof(entrada), stdin) == NULL)
        {
            continue;
        }

        uint32_t ftarget;
        uint32_t duty;

        // Convertir la entrada
        if (sscanf(entrada, "%u %u", &ftarget, &duty) != 2)
        {
            printf("Entrada invalida.\n");
            continue;
        }

        // Validar valores
        if (ftarget == 0 || duty > 100)
        {
            printf("Entrada invalida.\n");
            printf("La frecuencia debe ser > 0 y el duty debe estar entre 0 y 100.\n");
            continue;
        }

        // Calcular parámetros del PWM
        uint32_t prescaler = fclk / (ftarget * 65536) + 1;
        uint32_t wrap = fclk / (prescaler * ftarget) - 1;
        uint32_t level = (wrap + 1) * duty / 100;

        // Configurar PWM
        pwm_set_enabled(slice, false);

        pwm_set_clkdiv_int_frac(slice, prescaler, 0);
        pwm_set_wrap(slice, wrap);
        pwm_set_gpio_level(PWM_GPIO, level);

        // Activar PWM
        pwm_set_enabled(slice, true);

        printf("PWM activado: %u%% de duty cycle\n", (unsigned)duty);
    }
}