#include "drivers/UART.h" 
#include <string.h>
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "hardware/gpio.h" 

static volatile char tx_buffer[TX_BUFFER_SIZE];

// Contadores absolutos
static volatile uint32_t head = 0; //Posición de lectura. Marca dónde está el espacio ocupado más antiguo
static volatile uint32_t tail = 0; //Posición de escritura. Marca dónde está el siguiente espacio vacío
static volatile uint32_t dropped_reports = 0;


static void on_uart_tx_isr(void) {
    while (uart_is_writable(UART_ID)) {
        if (head != tail) { 
    
            char c = tx_buffer[head & TX_BUFFER_MASK];
            uart_putc_raw(UART_ID, c);
            head++; 
        } else {
    
            uart_set_irq_enables(UART_ID, false, false);
            break;
        }
    }
}

void uart_driver_init(void) {
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART); //Define la función del pin como UART
    
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE); //8N1
    uart_set_hw_flow(UART_ID, false, false); //Deshabilita el control de pines RTS y CTS
    uart_set_fifo_enabled(UART_ID, true);// Activa los búferes de memoria internos

    int uart_irq_num = (UART_ID == uart0) ? UART0_IRQ : UART1_IRQ;
    irq_set_exclusive_handler(uart_irq_num, on_uart_tx_isr); //Interrupción
    irq_set_enabled(uart_irq_num, true); //Habilita la interrupción
    
    uart_set_irq_enables(UART_ID, false, false);//Apagamos las interrupciones
}




bool uart_driver_send_string(const char* str) {
    uint32_t len = strlen(str); //Calcula cuántos caracteres tiene la cadena a enviar.
    
    uint32_t current_head = head;
    uint32_t used_space = tail - current_head;
    uint32_t free_space = (TX_BUFFER_SIZE - 1) - used_space; //Espacio disponible en el buffer

    if (len > free_space) {//Si la cadena es mayor al espacio libre se rechaza
        dropped_reports++;
        return false; 
    }

    for (uint32_t i = 0; i < len; i++) {
        tx_buffer[(tail + i) & TX_BUFFER_MASK] = str[i];
    }
    
    tail += len;


    uart_set_irq_enables(UART_ID, false, true);
    

    int uart_irq_num = (UART_ID == uart0) ? UART0_IRQ : UART1_IRQ;
    irq_set_pending(uart_irq_num);
    
    return true;
}

uint32_t uart_driver_get_dropped_count(void) {
    return dropped_reports;
}