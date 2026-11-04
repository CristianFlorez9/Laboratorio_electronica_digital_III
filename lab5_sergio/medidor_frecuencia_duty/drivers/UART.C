#include "drivers/UART.h" 
#include <string.h>
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "hardware/gpio.h" 

static volatile char tx_buffer[TX_BUFFER_SIZE];

// Contadores absolutos
static volatile uint32_t head = 0; 
static volatile uint32_t tail = 0;
static volatile uint32_t dropped_reports = 0;

static void on_uart_tx_isr(void) {
    while (uart_is_writable(UART_ID)) {
        if (head != tail) { // Hay datos en el búfer circular
            // Extraer byte aplicando la máscara para dar la vuelta al arreglo circular
            char c = tx_buffer[head & TX_BUFFER_MASK];
            uart_putc_raw(UART_ID, c); // Envío directo al hardware
            head++; // Avanzar contador de lectura
        } else {
            // El búfer de software se vació por completo. 
            // Apagamos la interrupción de transmisión.
            uart_set_irq_enables(UART_ID, false, false);
            break;
        }
    }
}

void uart_driver_init(void) {
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    
    // Configuración estricta de formato: 8 bits de datos, sin paridad, 1 bit de parada[cite: 2]
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);
    uart_set_hw_flow(UART_ID, false, false); 
    uart_set_fifo_enabled(UART_ID, true);    

    // Configurar el manejador de la interrupción en el NVIC
    int uart_irq_num = (UART_ID == uart0) ? UART0_IRQ : UART1_IRQ;
    irq_set_exclusive_handler(uart_irq_num, on_uart_tx_isr);
    irq_set_enabled(uart_irq_num, true);
    
    uart_set_irq_enables(UART_ID, false, false);
}

bool uart_driver_send_string(const char* str) {
    uint32_t len = strlen(str);
    
    uint32_t current_head = head;
    uint32_t used_space = tail - current_head;
    uint32_t free_space = (TX_BUFFER_SIZE - 1) - used_space;

    if (len > free_space) {
        dropped_reports++;
        return false; 
    }

    for (uint32_t i = 0; i < len; i++) {
        tx_buffer[(tail + i) & TX_BUFFER_MASK] = str[i];
    }
    
    tail += len;

    // 1. Encendemos la máscara de la interrupción[cite: 3]
    uart_set_irq_enables(UART_ID, false, true);
    
    // 2. FORZAMOS el primer disparo de la interrupción en el procesador.
    // Esto soluciona la inactividad del hardware PL011 al arrancar con FIFO vacío.
    int uart_irq_num = (UART_ID == uart0) ? UART0_IRQ : UART1_IRQ;
    irq_set_pending(uart_irq_num);
    
    return true;
}

uint32_t uart_driver_get_dropped_count(void) {
    return dropped_reports;
}