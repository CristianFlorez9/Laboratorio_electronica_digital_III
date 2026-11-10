#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

#define PWM_GPIO 13
#define TIEMPO_PASO_MS 10000   // 10 segundos por cada PWM

// Frecuencias de la rutina (Hz)
static const uint32_t FRECUENCIAS[] = {
    27, 41, 53, 84, 96, 178, 327, 462,
    691, 815, 2164, 4821, 6092, 7583, 9375
};
#define NUM_FRECUENCIAS (sizeof(FRECUENCIAS) / sizeof(FRECUENCIAS[0]))

// Duties de la rutina (%)
static const uint32_t DUTYS[] = {10, 50, 90};
#define NUM_DUTYS (sizeof(DUTYS) / sizeof(DUTYS[0]))

static uint32_t fclk;
static uint slice;

// Configura y activa el PWM con la frecuencia y duty indicados
static void aplicar_pwm(uint32_t ftarget, uint32_t duty)
{
    uint32_t prescaler = fclk / (ftarget * 65536) + 1;
    uint32_t wrap = fclk / (prescaler * ftarget) - 1;
    uint32_t level = (wrap + 1) * duty / 100;

    pwm_set_enabled(slice, false);

    pwm_set_clkdiv_int_frac(slice, prescaler, 0);
    pwm_set_wrap(slice, wrap);
    pwm_set_gpio_level(PWM_GPIO, level);

    pwm_set_enabled(slice, true);
}

// Espera 'ms' milisegundos; retorna true si se recibió un caracter (abortar)
static bool esperar_o_abortar(uint32_t ms)
{
    absolute_time_t fin = make_timeout_time_ms(ms);
    while (absolute_time_diff_us(get_absolute_time(), fin) > 0)
    {
        int c = getchar_timeout_us(0);
        if (c != PICO_ERROR_TIMEOUT)
        {
            return true;
        }
        sleep_ms(10);
    }
    return false;
}

// Rutina: todas las frecuencias a 10%, luego a 50%, luego a 90%
static void ejecutar_rutina(void)
{
    uint32_t total = NUM_DUTYS * NUM_FRECUENCIAS;
    uint32_t paso = 0;

    printf("\n*** INICIO DE RUTINA ***\n");
    printf("%u pasos de %d s (total %u s). Envie cualquier caracter para abortar.\n",
           (unsigned)total, TIEMPO_PASO_MS / 1000,
           (unsigned)(total * TIEMPO_PASO_MS / 1000));

    for (uint32_t d = 0; d < NUM_DUTYS; d++)
    {
        printf("\n--- Duty %u%% ---\n", (unsigned)DUTYS[d]);

        for (uint32_t f = 0; f < NUM_FRECUENCIAS; f++)
        {
            paso++;
            aplicar_pwm(FRECUENCIAS[f], DUTYS[d]);

            printf("[%u/%u] Frecuencia: %u Hz | Duty: %u%%\n",
                   (unsigned)paso, (unsigned)total,
                   (unsigned)FRECUENCIAS[f], (unsigned)DUTYS[d]);

            if (esperar_o_abortar(TIEMPO_PASO_MS))
            {
                pwm_set_enabled(slice, false);
                printf("\n*** RUTINA ABORTADA ***\n");
                return;
            }
        }
    }

    pwm_set_enabled(slice, false);
    printf("\n*** RUTINA FINALIZADA ***\n");
}

int main(void)
{
    stdio_init_all();

    fclk = clock_get_hz(clk_sys);

    gpio_set_function(PWM_GPIO, GPIO_FUNC_PWM);
    slice = pwm_gpio_to_slice_num(PWM_GPIO);

    // Esperar 10 segundos para permitir conexión USB
    sleep_ms(10000);

    char entrada[50];

    while (true)
    {
        printf("\n");
        printf("====================================\n");
        printf("Ingrese: frecuencia_Hz duty_pct\n");
        printf("Ejemplo: 1000 25\n");
        printf("O escriba RUTINA para la rutina automatica\n");
        printf("====================================\n");

        // Leer una línea completa
        if (fgets(entrada, sizeof(entrada), stdin) == NULL)
        {
            continue;
        }

        // Quitar saltos de línea / retorno de carro
        entrada[strcspn(entrada, "\r\n")] = '\0';

        // Comando RUTINA (sin distinguir mayúsculas/minúsculas)
        if (strcasecmp(entrada, "RUTINA") == 0)
        {
            ejecutar_rutina();
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

        aplicar_pwm(ftarget, duty);

        printf("PWM activado: %u Hz, %u%% de duty cycle\n",
               (unsigned)ftarget, (unsigned)duty);
    }
}