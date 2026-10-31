#ifndef BUTTONS_LEDS_H
#define BUTTONS_LEDS_H

/**
 * @file buttons_leds.h
 * @brief Interfaz del driver de botones (con antirrebote) y LEDs de Octavia.
 */

#include <stdint.h>
#include <stdbool.h>

/** Cantidad de botones de nota (do, re, mi, fa, sol, la, si). */
#define BL_NUM_NOTAS 7u
/** Cantidad de octavas seleccionables (botones y LEDs). */
#define BL_NUM_OCTAVAS 3u
/** Total de botones gestionados por el driver. */
#define BL_NUM_BOTONES (BL_NUM_NOTAS + BL_NUM_OCTAVAS)

/** Tiempo de antirrebote en milisegundos, medido con el temporizador del sistema. */
#ifndef BL_ANTIRREBOTE_MS
#define BL_ANTIRREBOTE_MS 30u
#endif

/** 1 si el botón conecta el pin a GND (con pull-up interno), 0 si lo conecta a 3V3. */
#ifndef BL_BOTON_ACTIVO_BAJO
#define BL_BOTON_ACTIVO_BAJO 1
#endif

/** 1 si el LED enciende con nivel alto, 0 si enciende con nivel bajo. */
#ifndef BL_LED_ACTIVO_ALTO
#define BL_LED_ACTIVO_ALTO 1
#endif

/** 1 para que los LEDs muestren automáticamente la octava seleccionada. */
#ifndef BL_LEDS_SIGUEN_OCTAVA
#define BL_LEDS_SIGUEN_OCTAVA 1
#endif

/**
 * @brief Configura GPIO, interrupciones de flanco y estado inicial.
 *
 * Toma el estado de los botones al arrancar como estable (sin eventos falsos),
 * registra la callback de GPIO y habilita IO_IRQ_BANK0. La octava inicial es 0.
 *
 * @note La interrupción de GPIO queda asociada al núcleo que llama a esta
 *       función. Llamarla una sola vez, antes de iniciar el SysTick.
 */
void bl_iniciar(void);

/**
 * @brief Valida los botones pendientes una vez cumplido el antirrebote.
 *
 * Debe llamarse de forma periódica desde el lazo principal. La ISR de GPIO
 * guarda el instante del primer flanco de cada botón; esta función confirma
 * el cambio cuando han transcurrido BL_ANTIRREBOTE_MS desde ese instante,
 * medido con time_us_32(), por lo que el antirrebote no depende del periodo
 * del lazo (solo se añade el retraso con que se vuelva a llamar). No bloquea
 * y retorna de inmediato si no hay botones pendientes.
 *
 * @note Modifica variables compartidas con la ISR de GPIO.
 */
void bl_actualizar(void);

/**
 * @brief Devuelve las notas presionadas (ya validadas).
 * @return Máscara de 7 bits: bit 0 = do, ..., bit 6 = si. 1 = presionada.
 */
uint8_t bl_notas_presionadas(void);

/**
 * @brief Lee y borra los eventos de nota acumulados desde la última llamada.
 *
 * @param[out] presionadas Notas que pasaron a presionadas (puede ser NULL).
 * @param[out] liberadas   Notas que pasaron a liberadas (puede ser NULL).
 */
void bl_notas_tomar_eventos(uint8_t *presionadas, uint8_t *liberadas);

/**
 * @brief Devuelve la octava vigente (la del último botón de octava validado).
 *
 * La aplicación debe leerla al pulsar cada nota y guardarla, para que cambiar
 * de octava no afecte a las notas ya sostenidas.
 *
 * @return Índice de octava, de 0 a BL_NUM_OCTAVAS - 1.
 */
uint8_t bl_octava_actual(void);

/**
 * @brief Enciende o apaga un LED directamente.
 *
 * @param indice Índice del LED, de 0 a BL_NUM_OCTAVAS - 1. Fuera de rango se ignora.
 * @param encendido true para encender.
 */
void bl_led_escribir(uint8_t indice, bool encendido);

/**
 * @brief Instante (time_us_32) del primer flanco del último cambio validado de un botón.
 *
 * @param indice Número de botón (0 a BL_NUM_BOTONES - 1).
 * @return Microsegundos desde el arranque, con desbordamiento a 32 bits.
 *         Devuelve 0 si el índice es inválido.
 */
uint32_t bl_tiempo_evento_us(uint8_t indice);

#endif