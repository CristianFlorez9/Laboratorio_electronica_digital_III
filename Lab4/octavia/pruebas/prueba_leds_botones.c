#include <stdio.h>
#include "pico/stdlib.h"
#include "buttons_leds.h"

static repeating_timer_t temporizador;

/* Se ejecuta cada 1 ms: avanza el antirrebote. */
static bool tick_1ms(repeating_timer_t *t)
{
    (void)t;
    bl_actualizar();
    return true;   // true = seguir repitiendo
}

int main(void)
{
    stdio_init_all();
    sleep_ms(2000);          // tiempo para abrir el monitor serie

    bl_iniciar();

    /* -1000 us = periodo de 1 ms medido entre inicios de llamada */
    add_repeating_timer_us(-1000, tick_1ms, NULL, &temporizador);

    printf("Listo. Pulsa botones.\n");

    while (true) {
        uint8_t pres, lib;
        bl_notas_tomar_eventos(&pres, &lib);

        static const char *nombres[7] = {"do","re","mi","fa","sol","la","si"};
        for (uint8_t n = 0; n < BL_NUM_NOTAS; n++) {
            if (pres & (1u << n)) printf("Nota %s PRESIONADA (octava %u)\n", nombres[n], bl_octava_actual());
            if (lib  & (1u << n)) printf("Nota %s liberada\n", nombres[n]);
        }
        sleep_ms(5);
    }
}