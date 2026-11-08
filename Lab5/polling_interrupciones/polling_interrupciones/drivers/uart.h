/**
 * @file UART.h
 * @brief Driver propio de UART0: transmisión y recepción por interrupciones.
 *
 * @details
 * Enlace serie con el computador a 115200 bits/s, 8 bits de datos, sin paridad,
 * 1 bit de parada y sin control de flujo, con las FIFOs de hardware habilitadas.
 *
 * **Uso del SDK.** Solo en la inicialización: baudios, formato, función de los pines,
 * habilitación de las FIFOs e instalación del manejador. Durante la operación el driver
 * accede al hardware directamente por registros (UARTFR, UARTDR, UARTIFLS, UARTIMSC,
 * UARTMIS y UARTICR). No usa funciones de envío o recepción del SDK ni espera
 * activamente a que aparezca espacio o datos.
 *
 * **Interrupciones.** Hay una sola ISR que distingue la causa leyendo UARTMIS:
 *  - Umbral de RX (RXIM): la FIFO de RX alcanzó el nivel UART_RX_FIFO_LEVEL_SEL.
 *  - Timeout de RX (RTIM): hay datos en la FIFO de RX y dejaron de llegar.
 *  - TX (TXIM): la FIFO de TX bajó al nivel UART_TX_FIFO_LEVEL_SEL o por debajo.
 *    Solo está habilitada mientras haya datos pendientes en el buffer de TX.
 *
 * Los umbrales se configuran escribiendo el registro UARTIFLS.
 *
 * **Propiedad de los buffers.**
 *  - Buffer de TX: lo llena uart_driver_send_string() y lo vacía la ISR (o la propia
 *    función al cebar la FIFO). Ambos accesos son exclusivos entre sí: la función
 *    trabaja dentro de una sección crítica.
 *  - Buffer de RX: lo llena únicamente la ISR y lo vacía únicamente el ciclo principal
 *    con uart_driver_read_byte().
 *
 * **Manejo de errores.** Si una línea completa no cabe en el buffer de TX, no se copia
 * nada: se descarta antes de empezar, se cuenta y la función devuelve false. Nunca se
 * transmiten líneas truncadas ni entrelazadas. Si el buffer de RX está lleno, la ISR
 * lee el byte de la FIFO (para vaciarla) y lo descarta.
 */

#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

/** Instancia de UART usada para el enlace con el computador. */
#define UART_ID uart0

/** Pin GPIO de transmisión (TX) de la UART. */
#define UART_TX_PIN 0

/** Pin GPIO de recepción (RX) de la UART. */
#define UART_RX_PIN 1

/** Velocidad del enlace serie [bits/s]. */
#define BAUD_RATE 115200

/** Tamaño del buffer circular de TX [bytes]. Debe ser potencia de 2. */
#define TX_BUFFER_SIZE 256

/** Máscara para indexar el buffer de TX (TX_BUFFER_SIZE - 1). */
#define TX_BUFFER_MASK (TX_BUFFER_SIZE - 1)

/** Tamaño del buffer circular de RX [bytes]. Debe ser potencia de 2. */
#define RX_BUFFER_SIZE 128

/** Máscara para indexar el buffer de RX (RX_BUFFER_SIZE - 1). */
#define RX_BUFFER_MASK (RX_BUFFER_SIZE - 1)

/**
 * Valor del campo RXIFLSEL del registro UARTIFLS: nivel de la FIFO de RX (32 bytes) en
 * el que se dispara la interrupción por umbral.
 * Codificación: 0 = 1/8, 1 = 1/4, 2 = 1/2, 3 = 3/4, 4 = 7/8 (28 bytes, 87,5 %).
 */
#define UART_RX_FIFO_LEVEL_SEL 4u

/**
 * Valor del campo TXIFLSEL del registro UARTIFLS: la interrupción de TX se dispara
 * cuando la FIFO de TX baja a este nivel o por debajo. Misma codificación que
 * UART_RX_FIFO_LEVEL_SEL (0 = 1/8, o sea 4 bytes ocupados o menos).
 */
#define UART_TX_FIFO_LEVEL_SEL 0u

/**
 * @brief Configura UART0, los umbrales de las FIFOs y las interrupciones de recepción.
 *
 * Deja habilitadas las interrupciones de umbral y de timeout de RX. La de TX se
 * habilita sola cuando hay datos por enviar.
 *
 * @note Contexto: main, antes de iniciar el ciclo principal.
 */
void uart_driver_init(void);

/**
 * @brief Encola una línea para transmitirla por interrupciones, sin esperar.
 *
 * Copia la línea al buffer de TX, transfiere a la FIFO de hardware lo que quepa y, si
 * queda algo pendiente, habilita la interrupción de TX para que la ISR continúe.
 *
 * @param[in] str Cadena terminada en NUL. Se copia; el llamador conserva su propiedad.
 * @return true si la línea completa quedó encolada; false si no cabía entera (no se
 *         guarda nada y se cuenta como descartada).
 *
 * @note Contexto: ciclo principal. Usa una sección crítica corta.
 */
bool uart_driver_send_string(const char *str);

/**
 * @brief Consulta cuántas líneas se descartaron por falta de espacio en el buffer de TX.
 * @return Líneas descartadas desde el arranque [líneas].
 */
uint32_t uart_driver_get_dropped_count(void);

/**
 * @brief Extrae un byte del buffer de RX.
 * @param[out] byte Byte recibido.
 * @return true si había un byte disponible; false si el buffer está vacío.
 *
 * @note Contexto: ciclo principal (único consumidor).
 */
bool uart_driver_read_byte(uint8_t *byte);

/**
 * @brief Consulta cuántas veces se atendió cada interrupción de recepción.
 *
 * Sirve como evidencia de que se usan ambas: el umbral de la FIFO y el timeout.
 *
 * @param[out] level_irqs   Veces que se atendió el umbral de la FIFO (RXIM) [eventos].
 * @param[out] timeout_irqs Veces que se atendió el timeout de recepción (RTIM) [eventos].
 */
void uart_driver_get_rx_irq_counts(uint32_t *level_irqs, uint32_t *timeout_irqs);

#endif