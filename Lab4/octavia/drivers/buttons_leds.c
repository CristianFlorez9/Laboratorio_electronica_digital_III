/**
 * @file buttons_leds.c
 * @brief Driver de botones (con antirrebote) y LEDs de Octavia.
 *
 * Los flancos de los botones se capturan por interrupción de GPIO. La ISR
 * deshabilita el pin, guarda el instante del flanco y lo marca como pendiente;
 * el antirrebote y la generación de eventos se hacen en bl_actualizar(), que
 * espera BL_ANTIRREBOTE_MS desde ese instante y luego confirma el nivel leído.
 * Las funciones públicas están documentadas en buttons_leds.h.
 */

#include "buttons_leds.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/sync.h"

/** @brief Valor de indice_de_pin para los GPIO que no son botones. */
#define BL_SIN_BOTON 0xFFu

/** @brief Eventos de GPIO que se monitorean: flanco de bajada y de subida. */
#define BL_EVENTOS_GPIO (GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE)

/** @brief Tiempo de antirrebote en microsegundos. */
#define BL_ANTIRREBOTE_US (BL_ANTIRREBOTE_MS * 1000u)

/**
 * @brief GPIO de cada botón, indexados por número de botón.
 *
 * Índices 0 a 6: notas (GPIO 16 a 22 = do a si). Índices 7 a 9: octavas
 * (GPIO 4, 3 y 2 = octavas 0, 1 y 2).
 */
static const uint8_t pines_botones[BL_NUM_BOTONES] = {
    16, 17, 18, 19, 20, 21, 22,
    4, 3, 2
};

/** @brief GPIO de los LEDs de octava (GPIO 7, 6 y 5 = octavas 0, 1 y 2). */
static const uint8_t pines_leds[BL_NUM_OCTAVAS] = {7, 6, 5};

/** @brief Tabla inversa: número de GPIO a número de botón (o BL_SIN_BOTON). */
static uint8_t indice_de_pin[NUM_BANK0_GPIOS];

/** @brief Máscara con el estado ya validado de cada botón (1 = presionado). */
static volatile uint16_t estado_estable;

/** @brief Máscara de botones con un flanco detectado y antirrebote en curso. */
static volatile uint16_t pendientes;

/** @brief Máscara de eventos de pulsación aún no consumidos. */
static volatile uint16_t eventos_presionado;

/** @brief Máscara de eventos de liberación aún no consumidos. */
static volatile uint16_t eventos_liberado;

/** @brief Octava vigente (0 a BL_NUM_OCTAVAS - 1). */
static volatile uint8_t octava;

/** @brief Instante del primer flanco de cada botón pendiente (time_us_32). */
static volatile uint32_t t_flanco_us[BL_NUM_BOTONES];

/** @brief Instante del primer flanco del último cambio validado de cada botón. */
static volatile uint32_t t_evento_us[BL_NUM_BOTONES];

/**
 * @brief Lee el nivel instantáneo de un botón, sin filtrar.
 *
 * @param i Número de botón (0 a BL_NUM_BOTONES - 1).
 * @return true si el botón está presionado, según BL_BOTON_ACTIVO_BAJO.
 */
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

/** @brief Enciende solo el LED de la octava vigente y apaga los demás. */
static void leds_mostrar_octava(void)
{
    for (uint8_t i = 0; i < BL_NUM_OCTAVAS; i++) {
        bl_led_escribir(i, i == octava);
    }
}

/**
 * @brief ISR de GPIO: marca el botón como pendiente de validación.
 *
 * No valida nada: deshabilita las interrupciones del pin (para ignorar el
 * rebote), guarda el instante del flanco y agrega el botón a pendientes. La
 * validación se hace en bl_actualizar().
 *
 * @param gpio    GPIO que generó la interrupción.
 * @param eventos Eventos detectados (no se usan).
 */
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
    t_flanco_us[i] = time_us_32();
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
        t_flanco_us[i] = 0;
        t_evento_us[i] = 0;

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
        if ((uint32_t)(time_us_32() - t_flanco_us[i]) < BL_ANTIRREBOTE_US) {
            continue;
        }

        /* La ISR también escribe pendientes: la limpieza debe ser atómica. */
        uint32_t irq = save_and_disable_interrupts();
        pendientes &= (uint16_t)~(1u << i);
        restore_interrupts(irq);

        bool crudo = leer_boton_crudo(i);
        bool estable = (estado_estable >> i) & 1u;

        if (crudo != estable) {
            estado_estable ^= (uint16_t)(1u << i);
            t_evento_us[i] = t_flanco_us[i];
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

        /* Si el nivel cambió justo antes de rehabilitar, reiniciar el filtro. */
        if (leer_boton_crudo(i) != (bool)((estado_estable >> i) & 1u)) {
            gpio_set_irq_enabled(pines_botones[i], BL_EVENTOS_GPIO, false);
            t_flanco_us[i] = time_us_32();

            irq = save_and_disable_interrupts();
            pendientes |= (uint16_t)(1u << i);
            restore_interrupts(irq);
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

uint32_t bl_tiempo_evento_us(uint8_t indice)
{
    return (indice < BL_NUM_BOTONES) ? t_evento_us[indice] : 0u;
}