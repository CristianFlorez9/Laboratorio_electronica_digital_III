/**
 * @file UART.c
 * @brief Implementación del driver UART0 por interrupciones (interfaz en UART.h).
 *
 * @details
 * Después de uart_driver_init(), todo acceso al hardware se hace por registro desde
 * dos contextos: la ISR de la UART y el ciclo principal (uart_driver_send_string y
 * uart_driver_read_byte). La aplicación usa un solo núcleo, así que la exclusión entre
 * ambos contextos se logra con una sección crítica en la ruta de TX, y con índices de
 * un solo escritor en las rutas de RX.
 */

#include "UART.h"
#include <string.h>
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "pico/sync.h"

_Static_assert((TX_BUFFER_SIZE & (TX_BUFFER_SIZE - 1)) == 0,
               "TX_BUFFER_SIZE debe ser potencia de 2");
_Static_assert((RX_BUFFER_SIZE & (RX_BUFFER_SIZE - 1)) == 0,
               "RX_BUFFER_SIZE debe ser potencia de 2");

/** Máscara con todos los bits de UARTICR, para limpiar interrupciones pendientes al iniciar. */
#define UART_ALL_IRQ_CLEAR 0x7FFu

/**
 * Buffer circular de TX [bytes]. Lo llena uart_driver_send_string() y lo vacía
 * tx_fill_hw(), siempre dentro de la sección crítica o de la ISR.
 */
static volatile char tx_buffer[TX_BUFFER_SIZE];

/**
 * Índice de lectura de TX. Contador creciente que se enmascara al indexar. Solo lo
 * avanza tx_fill_hw(), desde la ISR o desde una sección crítica.
 */
static volatile uint32_t tx_head = 0;

/**
 * Índice de escritura de TX. Contador creciente que se enmascara al indexar. Solo lo
 * avanza uart_driver_send_string(), dentro de la sección crítica.
 */
static volatile uint32_t tx_tail = 0;

/** Líneas descartadas por falta de espacio en el buffer de TX [líneas]. */
static volatile uint32_t dropped_reports = 0;

/** Buffer circular de RX [bytes]. Lo llena solo la ISR y lo vacía solo el ciclo principal. */
static volatile uint8_t rx_buffer[RX_BUFFER_SIZE];

/** Índice de lectura de RX. Solo lo avanza uart_driver_read_byte() en el ciclo principal. */
static volatile uint32_t rx_head = 0;

/** Índice de escritura de RX. Solo lo avanza rx_drain_hw() en la ISR. */
static volatile uint32_t rx_tail = 0;

/** Veces que se atendió la interrupción por umbral de la FIFO de RX [eventos]. */
static volatile uint32_t rx_irq_level = 0;

/** Veces que se atendió la interrupción por timeout de recepción [eventos]. */
static volatile uint32_t rx_irq_timeout = 0;

/** Sección crítica que protege el buffer de TX entre el ciclo principal y la ISR. */
static critical_section_t uart_crit_sec;

/**
 * @brief Mueve a la FIFO de TX lo que quepa del buffer de TX, sin esperar.
 *
 * Consulta UARTFR (bandera TXFF) y escribe UARTDR por registro. Termina cuando el
 * buffer queda vacío o la FIFO se llena; no espera a que aparezca espacio nuevo.
 *
 * @note Contexto: ISR de la UART o sección crítica. Nunca se ejecuta concurrentemente
 *       consigo misma.
 */
static inline void tx_fill_hw(void) {
    uart_hw_t *u = uart_get_hw(UART_ID);
    while (tx_head != tx_tail && !(u->fr & UART_UARTFR_TXFF_BITS)) {
        u->dr = (uint8_t)tx_buffer[tx_head & TX_BUFFER_MASK];
        tx_head++;
    }
}

/**
 * @brief Pasa al buffer de RX todo lo que haya en la FIFO de RX en este momento.
 *
 * Lee UARTDR mientras UARTFR indique que la FIFO no está vacía. Si el buffer de RX está
 * lleno, el byte se lee igual (para vaciar la FIFO y que la interrupción se apague) y
 * se descarta. No espera datos nuevos.
 *
 * @note Contexto: ISR de la UART.
 */
static inline void rx_drain_hw(void) {
    uart_hw_t *u = uart_get_hw(UART_ID);
    while (!(u->fr & UART_UARTFR_RXFE_BITS)) {
        uint8_t byte = (uint8_t)u->dr;
        if ((rx_tail - rx_head) < RX_BUFFER_SIZE) {
            rx_buffer[rx_tail & RX_BUFFER_MASK] = byte;
            rx_tail++;
        }
    }
}

/**
 * @brief ISR única de la UART. Distingue la causa leyendo UARTMIS.
 *
 *  - RXMIS: la FIFO de RX llegó al umbral configurado en UARTIFLS.
 *  - RTMIS: hay datos en la FIFO de RX pero dejaron de llegar (timeout).
 *  - TXMIS: la FIFO de TX bajó al umbral de TX; se rellena con lo pendiente y, si ya no
 *    queda nada, se apaga la interrupción de TX en UARTIMSC.
 *
 * Las de RX se limpian en UARTICR después de vaciar la FIFO.
 *
 * @note Contexto: ISR (prioridad por defecto del SDK).
 */
static void uart_isr(void) {
    uart_hw_t *u = uart_get_hw(UART_ID);
    uint32_t mis = u->mis;

    if (mis & (UART_UARTMIS_RXMIS_BITS | UART_UARTMIS_RTMIS_BITS)) {
        if (mis & UART_UARTMIS_RXMIS_BITS) {
            rx_irq_level++;
        }
        if (mis & UART_UARTMIS_RTMIS_BITS) {
            rx_irq_timeout++;
        }
        rx_drain_hw();
        u->icr = UART_UARTICR_RXIC_BITS | UART_UARTICR_RTIC_BITS;
    }

    if (mis & UART_UARTMIS_TXMIS_BITS) {
        tx_fill_hw();
        if (tx_head == tx_tail) {
            hw_clear_bits(&u->imsc, UART_UARTIMSC_TXIM_BITS);
        }
    }
}

void uart_driver_init(void) {
    critical_section_init(&uart_crit_sec);

    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);
    uart_set_hw_flow(UART_ID, false, false);
    uart_set_fifo_enabled(UART_ID, true);

    uart_hw_t *u = uart_get_hw(UART_ID);
    u->ifls = (UART_RX_FIFO_LEVEL_SEL << UART_UARTIFLS_RXIFLSEL_LSB) |
              (UART_TX_FIFO_LEVEL_SEL << UART_UARTIFLS_TXIFLSEL_LSB);

    u->icr = UART_ALL_IRQ_CLEAR;
    u->imsc = UART_UARTIMSC_RXIM_BITS | UART_UARTIMSC_RTIM_BITS;

    int irq = (UART_ID == uart0) ? UART0_IRQ : UART1_IRQ;
    irq_set_exclusive_handler(irq, uart_isr);
    irq_set_enabled(irq, true);
}

bool uart_driver_send_string(const char *str) {
    uint32_t len = (uint32_t)strlen(str);

    critical_section_enter_blocking(&uart_crit_sec);

    uint32_t free_space = (TX_BUFFER_SIZE - 1) - (tx_tail - tx_head);
    if (len > free_space) {
        dropped_reports++;
        critical_section_exit(&uart_crit_sec);
        return false;
    }

    for (uint32_t i = 0; i < len; i++) {
        tx_buffer[(tx_tail + i) & TX_BUFFER_MASK] = str[i];
    }
    tx_tail += len;

    tx_fill_hw();
    if (tx_head != tx_tail) {
        hw_set_bits(&uart_get_hw(UART_ID)->imsc, UART_UARTIMSC_TXIM_BITS);
    }

    critical_section_exit(&uart_crit_sec);
    return true;
}

uint32_t uart_driver_get_dropped_count(void) {
    return dropped_reports;
}

bool uart_driver_read_byte(uint8_t *byte) {
    if (rx_head == rx_tail) {
        return false;
    }
    *byte = rx_buffer[rx_head & RX_BUFFER_MASK];
    rx_head++;
    return true;
}

void uart_driver_get_rx_irq_counts(uint32_t *level_irqs, uint32_t *timeout_irqs) {
    *level_irqs = rx_irq_level;
    *timeout_irqs = rx_irq_timeout;
}