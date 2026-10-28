#include "uart_driver.h"
#include <string.h>
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "pico/sync.h"


static volatile char tx_buffer[TX_BUFFER_SIZE];

//contadores absolutos
static volatile uint32_t head = 0; 
static volatile uint32_t tail = 0;
static volatile uint32_t dropped_reports = 0;

static critical_section_t uart_crit_sec;


static void on_uart_tx_isr(void) {
 
    while (uart_is_tx_fifo_full(UART_ID) == false) {
        if (head != tail) { // Hay datos en el búfer circular
            // Extraer byte aplicando la máscara para dar la vuelta al arreglo circular
            char c = tx_buffer[head & TX_BUFFER_MASK];
            uart_putc_raw(UART_ID, c); // Envío directo a registro sin bloqueo
            head++; // Avanzar contador de lectura
        } else {
            // El búfer de software se vació por completo. 
            // Apagamos la interrupción de transmisión para evitar un bucle infinito.
            uart_set_irq_enables(UART_ID, false, false);
            break;
        }
    }
}


void uart_driver_init(void) {

    critical_section_init(&uart_crit_sec);

    // Inicializar periférico UART al baud rate definido
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    
    // Configuración estricta de formato: 8 bits de datos, sin paridad, 1 bit de parada
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);
    uart_set_hw_flow(UART_ID, false, false);
    uart_set_fifo_enabled(UART_ID, true);    // Activar FIFOs de hardware

    // Configurar el manejador de la interrupción en el NVIC del ARM Cortex
    int uart_irq_num = (UART_ID == uart0) ? UART0_IRQ : UART1_IRQ;
    irq_set_exclusive_handler(uart_irq_num, on_uart_tx_isr);
    irq_set_enabled(uart_irq_num, true);

    
    uart_set_irq_enables(UART_ID, false, false);
}


bool uart_driver_send_string(const char* str) {
    uint32_t len = strlen(str);
    
    // Bloquear interrupciones momentáneamente para evaluar estado del búfer de forma segura
    critical_section_enter_blocking(&uart_crit_sec);
    
    // Calcular el espacio libre. 
    // "-1" asegura que tail nunca alcance a head por dar una vuelta completa (condición de lleno)
    uint32_t used_space = tail - head;
    uint32_t free_space = (TX_BUFFER_SIZE - 1) - used_space;

    // Política de descarte completo: si no cabe entero, se omite para evitar tramas corruptas[cite: 2]
    if (len > free_space) {
        dropped_reports++;
        critical_section_exit(&uart_crit_sec); // Liberar sección crítica antes de salir
        return false; 
    }

    // Copiar la cadena al búfer circular byte a byte
    for (uint32_t i = 0; i < len; i++) {
        tx_buffer[(tail + i) & TX_BUFFER_MASK] = str[i];
    }
    
    // Avanzar el índice de escritura de forma atómica
    tail += len;

    // Encender la interrupción de transmisión. Esto es el "gatillo". 
    // Si la FIFO de hardware tiene espacio (casi siempre), la ISR saltará inmediatamente tras salir de la sección crítica.
    uart_set_irq_enables(UART_ID, false, true);
    
    critical_section_exit(&uart_crit_sec);
    return true;
}


uint32_t uart_driver_get_dropped_count(void) {
    // Variable de 32 bits en procesador de 32 bits se lee en 1 ciclo, 
    // pero si fuera necesario reiniciar el conteo, requeriría sección crítica.
    return dropped_reports;
}