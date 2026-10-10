#include "buttons_leds.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/sync.h"

#define BL_SIN_BOTON 0xFFu
#define BL_EVENTOS_GPIO (GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE)

/* Botones: GPIO 16-22 son do..si; GPIO 4, 3, 2 son las octavas 0, 1, 2. */
static const uint8_t pines_botones[BL_NUM_BOTONES] = {
    16, 17, 18, 19, 20, 21, 22,
    4, 3, 2
};

/* LEDs: GPIO 7, 6, 5 indican las octavas 0, 1, 2. */
static const uint8_t pines_leds[BL_NUM_OCTAVAS] = {7, 6, 5};

static uint8_t indice_de_pin[NUM_BANK0_GPIOS];

static volatile uint16_t estado_estable;
static volatile uint16_t pendientes;
static volatile uint16_t eventos_presionado;
static volatile uint16_t eventos_liberado;
static volatile uint8_t octava;
static uint8_t contador[BL_NUM_BOTONES];

static bool leer_boton_crudo(uint8_t i)
{
    bool nivel = gpio_get(pines_botones[i]);
#if BL_BOTON_ACTIVO_BAJO
    return !nivel;
#else
    return nivel;
#endif
}

void bl_led_escribir(uint8_t indice, bool encendido)
{
    if (indice >= BL_NUM_OCTAVAS) {
        return;
    }
#if BL_LED_ACTIVO_ALTO
    gpio_put(pines_leds[indice], encendido);
#else
    gpio_put(pines_leds[indice], !encendido);
#endif
}

static void leds_mostrar_octava(void)
{
    for (uint8_t i = 0; i < BL_NUM_OCTAVAS; i++) {
        bl_led_escribir(i, i == octava);
    }
}

/* ISR de GPIO: no valida nada, solo bloquea el pin y lo deja pendiente. */
static void gpio_isr(uint gpio, uint32_t eventos)
{
    (void)eventos;
    if (gpio >= NUM_BANK0_GPIOS) {
        return;
    }
    uint8_t i = indice_de_pin[gpio];
    if (i == BL_SIN_BOTON) {
        return;
    }
    gpio_set_irq_enabled(gpio, BL_EVENTOS_GPIO, false);
    contador[i] = 0;
    pendientes |= (uint16_t)(1u << i);
}

void bl_iniciar(void)
{
    for (uint8_t p = 0; p < NUM_BANK0_GPIOS; p++) {
        indice_de_pin[p] = BL_SIN_BOTON;
    }

    for (uint8_t i = 0; i < BL_NUM_OCTAVAS; i++) {
        gpio_init(pines_leds[i]);
        gpio_set_dir(pines_leds[i], GPIO_OUT);
        bl_led_escribir(i, false);
    }

    estado_estable = 0;
    pendientes = 0;
    eventos_presionado = 0;
    eventos_liberado = 0;
    octava = 0;

    for (uint8_t i = 0; i < BL_NUM_BOTONES; i++) {
        indice_de_pin[pines_botones[i]] = i;
        contador[i] = 0;

        gpio_init(pines_botones[i]);
        gpio_set_dir(pines_botones[i], GPIO_IN);
#if BL_BOTON_ACTIVO_BAJO
        gpio_pull_up(pines_botones[i]);
#else
        gpio_pull_down(pines_botones[i]);
#endif
    }

    sleep_us(100);

    for (uint8_t i = 0; i < BL_NUM_BOTONES; i++) {
        if (leer_boton_crudo(i)) {
            estado_estable |= (uint16_t)(1u << i);
        }
    }

#if BL_LEDS_SIGUEN_OCTAVA
    leds_mostrar_octava();
#endif

    gpio_set_irq_callback(&gpio_isr);
    for (uint8_t i = 0; i < BL_NUM_BOTONES; i++) {
        gpio_acknowledge_irq(pines_botones[i], BL_EVENTOS_GPIO);
        gpio_set_irq_enabled(pines_botones[i], BL_EVENTOS_GPIO, true);
    }
    irq_set_enabled(IO_IRQ_BANK0, true);
}

void bl_actualizar(void)
{
    uint16_t pend = pendientes;
    if (pend == 0) {
        return;
    }

    for (uint8_t i = 0; i < BL_NUM_BOTONES; i++) {
        if (!((pend >> i) & 1u)) {
            continue;
        }
        if (++contador[i] < BL_TICKS_ANTIRREBOTE) {
            continue;
        }

        contador[i] = 0;
        pendientes &= (uint16_t)~(1u << i);

        bool crudo = leer_boton_crudo(i);
        bool estable = (estado_estable >> i) & 1u;

        if (crudo != estable) {
            estado_estable ^= (uint16_t)(1u << i);
            if (crudo) {
                eventos_presionado |= (uint16_t)(1u << i);
                if (i >= BL_NUM_NOTAS) {
                    octava = (uint8_t)(i - BL_NUM_NOTAS);
#if BL_LEDS_SIGUEN_OCTAVA
                    leds_mostrar_octava();
#endif
                }
            } else {
                eventos_liberado |= (uint16_t)(1u << i);
            }
        }

        /* Borra el flanco latente del rebote y rehabilita el pin. */
        gpio_acknowledge_irq(pines_botones[i], BL_EVENTOS_GPIO);
        gpio_set_irq_enabled(pines_botones[i], BL_EVENTOS_GPIO, true);

        /* Si el nivel cambio justo antes de rehabilitar, reiniciar el filtro. */
        if (leer_boton_crudo(i) != (bool)((estado_estable >> i) & 1u)) {
            gpio_set_irq_enabled(pines_botones[i], BL_EVENTOS_GPIO, false);
            contador[i] = 0;
            pendientes |= (uint16_t)(1u << i);
        }
    }
}

uint8_t bl_notas_presionadas(void)
{
    return (uint8_t)(estado_estable & ((1u << BL_NUM_NOTAS) - 1u));
}

void bl_notas_tomar_eventos(uint8_t *presionadas, uint8_t *liberadas)
{
    const uint16_t mascara = (1u << BL_NUM_NOTAS) - 1u;

    uint32_t irq = save_and_disable_interrupts();
    uint16_t p = eventos_presionado & mascara;
    uint16_t l = eventos_liberado & mascara;
    eventos_presionado &= (uint16_t)~mascara;
    eventos_liberado &= (uint16_t)~mascara;
    restore_interrupts(irq);

    if (presionadas) {
        *presionadas = (uint8_t)p;
    }
    if (liberadas) {
        *liberadas = (uint8_t)l;
    }
}

uint8_t bl_octava_actual(void)
{
    return octava;
}