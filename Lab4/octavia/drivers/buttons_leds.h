#ifndef BUTTONS_LEDS_H
#define BUTTONS_LEDS_H

#include <stdint.h>
#include <stdbool.h>

/** Cantidad de botones de nota (do, re, mi, fa, sol, la, si). */
#define BL_NUM_NOTAS 7u
/** Cantidad de octavas seleccionables (botones y LEDs). */
#define BL_NUM_OCTAVAS 3u
/** Total de botones gestionados por el driver. */
#define BL_NUM_BOTONES (BL_NUM_NOTAS + BL_NUM_OCTAVAS)

/** Ticks de bl_actualizar() que un boton pendiente debe esperar antes de validarse. */
#ifndef BL_TICKS_ANTIRREBOTE
#define BL_TICKS_ANTIRREBOTE 5u

#endif

/** 1 si el boton conecta el pin a GND (con pull-up interno), 0 si lo conecta a 3V3. */
#ifndef BL_BOTON_ACTIVO_BAJO
#define BL_BOTON_ACTIVO_BAJO 1
#endif

/** 1 si el LED enciende con nivel alto, 0 si enciende con nivel bajo. */
#ifndef BL_LED_ACTIVO_ALTO
#define BL_LED_ACTIVO_ALTO 1
#endif

/** 1 para que los LEDs muestren automaticamente la octava seleccionada. */
#ifndef BL_LEDS_SIGUEN_OCTAVA
#define BL_LEDS_SIGUEN_OCTAVA 1
#endif

/**
 * @brief Configura GPIO, interrupciones de flanco y estado inicial.
 *
 * Toma el estado de los botones al arrancar como estable (sin eventos falsos),
 * registra la callback de GPIO y habilita IO_IRQ_BANK0. La octava inicial es 0.
 *
 * @note La interrupcion de GPIO queda asociada al nucleo que llama a esta
 *       funcion. Llamarla una sola vez, antes de iniciar el SysTick.
 */
void bl_iniciar(void);

/**
 * @brief Avanza el antirrebote de los botones pendientes.
 *
 * Debe llamarse con periodo fijo (por ejemplo cada 1 ms desde la ISR del
 * SysTick), en el mismo nucleo y con la misma prioridad NVIC que la
 * interrupcion de GPIO. No bloquea y retorna de inmediato si no hay botones
 * pendientes.
 */
void bl_actualizar(void);

/**
 * @brief Devuelve las notas presionadas (ya validadas).
 * @return Mascara de 7 bits: bit 0 = do, ..., bit 6 = si. 1 = presionada.
 */
uint8_t bl_notas_presionadas(void);

/**
 * @brief Lee y borra los eventos de nota acumulados desde la ultima llamada.
 *
 * @param[out] presionadas Notas que pasaron a presionadas (puede ser NULL).
 * @param[out] liberadas   Notas que pasaron a liberadas (puede ser NULL).
 */
void bl_notas_tomar_eventos(uint8_t *presionadas, uint8_t *liberadas);

/**
 * @brief Devuelve la octava vigente (la del ultimo boton de octava validado).
 *
 * La aplicacion debe leerla al pulsar cada nota y guardarla, para que cambiar
 * de octava no afecte a las notas ya sostenidas.
 *
 * @return Indice de octava, de 0 a BL_NUM_OCTAVAS - 1.
 */
uint8_t bl_octava_actual(void);

/**
 * @brief Enciende o apaga un LED directamente.
 *
 * @param indice Indice del LED, de 0 a BL_NUM_OCTAVAS - 1. Fuera de rango se ignora.
 * @param encendido true para encender.
 */
void bl_led_escribir(uint8_t indice, bool encendido);

#endif