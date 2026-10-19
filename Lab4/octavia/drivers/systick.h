#ifndef SYSTICK_H_
#define SYSTICK_H_

/**
 * @file systick.h
 * @brief Interfaz del driver propio del temporizador SysTick (RP2040, Cortex-M0+).
 *
 * Accede directamente a los registros SYST_CSR, SYST_RVR, SYST_CVR y SCB_SHPR3.
 * No usa funciones del SDK para configurar ni operar el temporizador; solo se
 * usan sus cabeceras de direcciones y offsets de registros.
 *
 * El SysTick es un contador descendente de 24 bits que, al llegar a cero, se
 * recarga con el valor de SYST_RVR y genera la excepción SysTick. En Octavia
 * marca el instante de cada muestra del DAC.
 */

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Configura y arranca el SysTick con interrupción periódica.
 *
 * Apaga el contador, fija la prioridad de la excepción (SYSTICK_PRIORIDAD),
 * carga el valor de recarga, limpia el contador y lo enciende usando el reloj
 * del procesador con la interrupción habilitada.
 *
 * @param reload_value Ciclos de reloj - 1 entre interrupciones (1..0xFFFFFF).
 *        f_tick = f_cpu / (reload_value + 1). Ejemplo: 125 MHz, 28160 Hz -> 4438.
 *        Los bits por encima del 24 se descartan sin aviso; con 0 el contador
 *        no genera interrupciones periódicas.
 *
 * @pre El manejador de la excepción SysTick ya debe estar registrado en la
 *      tabla de vectores, para que la primera interrupción no llegue a un
 *      manejador por defecto. En Octavia lo hace synth_init() antes de
 *      llamar a esta función.
 * @post El SysTick queda en marcha e interrumpiendo periódicamente.
 * @note Tiene efectos secundarios: modifica la prioridad de la excepción
 *       SysTick (byte alto de SCB_SHPR3) y deja el contador en cero.
 */
void systick_init(uint32_t reload_value);

/**
 * @brief Enciende o apaga el contador sin tocar el resto de la configuración.
 *
 * @param enable true para encender, false para apagar.
 *
 * @note Al apagar, el contador se congela con su valor actual y se reanuda
 *       desde él al encender. Leer SYST_CSR (como hace esta función) borra el
 *       indicador COUNTFLAG, de modo que no debe usarse para consultarlo.
 */
void systick_set_state(bool enable);

#endif /* SYSTICK_H_ */
