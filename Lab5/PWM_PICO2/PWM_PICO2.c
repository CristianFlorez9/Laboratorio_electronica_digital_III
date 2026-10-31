#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

#define PWM_GPIO 15

int main(void)
{
    stdio_init_all();

    uint32_t fclk = clock_get_hz(clk_sys);

    gpio_set_function(PWM_GPIO, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(PWM_GPIO);

    printf("fclk = %u Hz\n", (unsigned)fclk);
    printf("Escribe: frecuencia_Hz duty_pct (ejemplo: 1000 25)\n");

    while (true) {
        uint32_t ftarget;
        uint32_t duty;

        if (scanf("%u %u", &ftarget, &duty) != 2 || ftarget == 0 || duty > 100) {
            printf("Entrada invalida\n");
            continue;
        }

        uint32_t prescaler = fclk / (ftarget * 65536) + 1;
        uint32_t wrap = fclk / (prescaler * ftarget) - 1;
        uint32_t level = (wrap + 1) * duty / 100;

        pwm_set_enabled(slice, false);
        pwm_set_clkdiv_int_frac(slice, prescaler, 0);
        pwm_set_wrap(slice, wrap);
        pwm_set_gpio_level(PWM_GPIO, level);
        pwm_set_enabled(slice, true);

        printf("prescaler=%u wrap=%u level=%u\n",
               (unsigned)prescaler, (unsigned)wrap, (unsigned)level);
    }
}