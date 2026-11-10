#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

#define UART_ID uart0      /**< @brief Periférico UART utilizado. */
#define UART_TX_PIN 0      /**< @brief Pin GPIO de transmisión. */
#define UART_RX_PIN 1      /**< @brief Pin GPIO de recepción (no usado). */
#define BAUD_RATE 115200   /**< @brief Velocidad en baudios. */

/** @brief Tamaño del buffer circular de TX (debe ser potencia de 2). */
#define TX_BUFFER_SIZE 256 

/** @brief Máscara para el índice circular del buffer. */
#define TX_BUFFER_MASK (TX_BUFFER_SIZE - 1)

/** @brief Configura la UART (8N1, FIFO) y su interrupción de transmisión. */
void uart_driver_init(void);

/**
 * @brief Encola una cadena para transmitirla por interrupción (no bloqueante).
 * @param str Cadena terminada en '\0'.
 * @return true si se encoló; false si no cabía completa y se descartó.
 */
bool uart_driver_send_string(const char* str);
// Función de transmisión guiada por eventos (descarte completo si no hay espacio)

/** @brief Cantidad de cadenas descartadas por buffer lleno. */
uint32_t uart_driver_get_dropped_count(void);
// Retorna la cantidad de paquetes descartados

#endif
