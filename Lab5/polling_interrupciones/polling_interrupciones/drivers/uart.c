/**
 * @file UART.c
 * @brief Driver de transmisión de UART0 por interrupciones (interfaz en UART.h).
 *
 * @details
 * El buffer de TX es circular, con dos contadores crecientes:
 *  - tx_tail: dónde se escribe. Solo lo avanza uart_driver_send_string().
 *  - tx_head: dónde se lee. Solo lo avanza tx_fill_hw().
 *
 * tx_fill_hw() la llaman la ISR y send_string (esta con las interrupciones apagadas),
 * así que nunca se ejecuta dos veces a la vez. La interrupción de TX solo está
 * habilitada mientras queden datos en el buffer.
 */

#include "UART.h"
#include <string.h>
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/sync.h"

static volatile char     tx_buffer[TX_BUFFER_SIZE];
static volatile uint32_t tx_head = 0;
static volatile uint32_t tx_tail = 0;
static volatile uint32_t dropped = 0;       /* líneas descartadas por falta de espacio */

/* Pasa a la FIFO de TX lo que quepa del buffer, sin esperar. */
static void tx_fill_hw(void) {
    uart_hw_t *u = uart_get_hw(UART_ID);
    while (tx_head != tx_tail && !(u->fr & UART_UARTFR_TXFF_BITS)) {   /* FIFO no llena */
        u->dr = tx_buffer[tx_head % TX_BUFFER_SIZE];
        tx_head++;
    }
}

/* ISR: la FIFO de TX se vació hasta el umbral; sigue con lo pendiente. */
static void uart_isr(void) {
    tx_fill_hw();
    if (tx_head == tx_tail) {
        uart_get_hw(UART_ID)->imsc = 0;     /* nada pendiente: apagar la interrupción de TX */
    }
}

void uart_driver_init(void) {
    uart_init(UART_ID, BAUD_RATE);          /* deja 8N1 y sin control de flujo */
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    uart_set_fifo_enabled(UART_ID, true);
    uart_get_hw(UART_ID)->ifls = 0;         /* IRQ de TX cuando la FIFO baja a 1/8 (4 bytes) */

    irq_set_exclusive_handler(UART0_IRQ, uart_isr);
    irq_set_enabled(UART0_IRQ, true);
}

bool uart_driver_send_string(const char *str) {
    uint32_t len = strlen(str);
    uint32_t irq_state = save_and_disable_interrupts();   /* la ISR no entra mientras se copia */

    if (len > TX_BUFFER_SIZE - (tx_tail - tx_head)) {     /* no cabe entera: descartar */
        dropped++;
        restore_interrupts(irq_state);
        return false;
    }

    for (uint32_t i = 0; i < len; i++) {
        tx_buffer[(tx_tail + i) % TX_BUFFER_SIZE] = str[i];
    }
    tx_tail += len;

    tx_fill_hw();                                         /* a la FIFO lo que quepa ya */
    if (tx_head != tx_tail) {
        uart_get_hw(UART_ID)->imsc = UART_UARTIMSC_TXIM_BITS;   /* el resto lo envía la ISR */
    }

    restore_interrupts(irq_state);
    return true;
}

uint32_t uart_driver_get_dropped_count(void) { return dropped; }