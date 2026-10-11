#ifndef SYSTICK_H_
#define SYSTICK_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Configura y arranca el SysTick con interrupcion periodica.
 * @param reload_value Ciclos de reloj - 1 entre interrupciones (1..0xFFFFFF).
 *        f_tick = f_cpu / (reload_value + 1). Ejemplo: 125 MHz, 28160 Hz -> 4438.
 * @note El manejador es isr_systick(), que debe definir la aplicacion.
 */
void systick_init(uint32_t reload_value);

/**
 * @brief Cambia el periodo sin detener el contador.
 * @param reload_value Nuevo valor de recarga (24 bits). Se aplica al proximo ciclo.
 */
void systick_set_reload(uint32_t reload_value);

/** @brief Enciende o apaga el contador sin tocar el resto de la configuracion. */
void systick_set_state(bool enable);

#endif /* SYSTICK_H_ */