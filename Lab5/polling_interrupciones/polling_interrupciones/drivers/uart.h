/**
 * @file UART.h
 * @brief Driver propio de transmisión por UART0, por interrupciones.
 *
 * @details
 * Enlace a 115200 bits/s, 8N1, sin control de flujo, con las FIFOs habilitadas. Solo
 * transmite (la guía no pide recepción).
 *
 * El texto se copia a un buffer circular. Lo que cabe pasa de inmediato a la FIFO de
 * hardware y el resto lo transfiere la ISR cuando la FIFO se vacía; nunca se espera a
 * que haya espacio. Si una línea no cabe entera en el buffer, se descarta completa y se
 * cuenta: no se transmiten líneas truncadas ni entrelazadas.
 */

#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

#define UART_ID         uart0   /**< Instancia de UART usada. */
#define UART_TX_PIN     0       /**< GPIO de transmisión. */
#define BAUD_RATE       115200  /**< Velocidad [bits/s]. */
#define TX_BUFFER_SIZE  256u    /**< Tamaño del buffer de TX [bytes]. Potencia de 2. */

/** @brief Configura UART0, el pin TX y la interrupción. Llamar desde main, antes del ciclo. */
void uart_driver_init(void);

/**
 * @brief Encola una línea para enviarla por interrupciones, sin esperar.
 * @param[in] str Cadena terminada en NUL (se copia).
 * @return true si quedó encolada completa; false si no cabía (se descarta y se cuenta).
 */
bool uart_driver_send_string(const char *str);

/** @brief Líneas descartadas por falta de espacio desde el arranque [líneas]. */
uint32_t uart_driver_get_dropped_count(void);

#endif