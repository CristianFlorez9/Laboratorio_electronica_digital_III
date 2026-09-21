#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"

#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

#define DEFAULT_FREQUENCY 2.0f

int pico_led_init(void) {
#if defined(PICO_DEFAULT_LED_PIN)
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    return PICO_OK;
#elif defined(CYW43_WL_GPIO_LED_PIN)
    return cyw43_arch_init();
#endif
}

void pico_set_led(bool led_on) {
#if defined(PICO_DEFAULT_LED_PIN)
    gpio_put(PICO_DEFAULT_LED_PIN, led_on);
#elif defined(CYW43_WL_GPIO_LED_PIN)
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
#endif
}

int main() {
    stdio_init_all();

    int rc = pico_led_init();
    hard_assert(rc == PICO_OK);

    float frecuencia = DEFAULT_FREQUENCY;
    bool estado_led = false;
    
    // Almacena el tiempo del último cambio de estado en microsegundos
    uint64_t ultimo_cambio = time_us_64();

    char buffer[32];
    int indice_buffer = 0;

    printf("\n=================================\n");
    printf("Control de frecuencia del LED\n");
    printf("=================================\n");
    printf("Frecuencia actual: %.2f Hz\n", frecuencia);
    printf("Ingrese nueva frecuencia en Hz:\n");

    while (true) {
        // 1. LECTURA SERIAL NO BLOQUEANTE
        int c = getchar_timeout_us(0); // Revisa al instante si hay un caracter
        if (c != PICO_ERROR_TIMEOUT) {
            if (c == '\n' || c == '\r') {
                if (indice_buffer > 0) {
                    buffer[indice_buffer] = '\0'; // Cerrar la cadena de texto
                    float nueva_frec = atof(buffer);
                    
                    if (nueva_frec > 0) {
                        frecuencia = nueva_frec;
                        printf("\nNueva frecuencia: %.2f Hz\n", frecuencia);
                    } else {
                        printf("\nLa frecuencia debe ser mayor a 0.\n");
                    }
                    printf("Ingrese otra frecuencia:\n");
                    indice_buffer = 0; // Limpiar buffer
                }
            } else if (indice_buffer < (sizeof(buffer) - 1)) {
                buffer[indice_buffer++] = (char)c; // Ir guardando los dígitos
            }
        }

        // 2. PARPADEO DEL LED NO BLOQUEANTE (Usa el tiempo actual)
        uint64_t tiempo_actual = time_us_64();
        uint64_t semi_periodo_us = (uint64_t)(500000.0f / frecuencia);

        if (tiempo_actual - ultimo_cambio >= semi_periodo_us) {
            estado_led = !estado_led;
            pico_set_led(estado_led);
            ultimo_cambio = tiempo_actual;
        }
    }
}