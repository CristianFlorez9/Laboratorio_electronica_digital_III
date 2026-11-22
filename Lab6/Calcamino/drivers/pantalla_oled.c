#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "pantalla_oled.h"
#include "drivers/font.h"

static i2c_inst_t *oled_bus = NULL;

// Buffer de transmisión único. 
// tx_buf[0] será siempre 0x40 (byte de control para datos).
// tx_buf[1] al tx_buf[1024] funcionarán como nuestro framebuffer directo.
static uint8_t oled_tx_buf[OLED_BUF_LEN + 1]; 

// Buffer para la secuencia de inicialización (debe ser static para el bus asíncrono)
static uint8_t init_sequence[] = {
    0x00,             // Control byte: Stream de comandos
    0xAE,             // Display OFF
    0x20, 0x00,       // Direccionamiento horizontal
    0x81, 0x7F,       // Contraste
    0xA1,             // Segment remap (espejo horizontal)
    0xC8,             // COM scan direction normal/invertido
    0xA6,             // Display normal (no invertir)
    0xA8, 0x3F,       // Multiplex 1/64
    0xD3, 0x00,       // Display offset 0
    0x40,             // Start line 0
    0x8D, 0x14,       // Charge pump ON
    0xAF              // Display ON
};

// Comandos para setear la ventana de dibujo en cada frame
static uint8_t window_cmd[] = {
    0x00,             // Control byte: Stream de comandos
    0x21, 0x00, 0x7F, // Column address (0 a 127)
    0x22, 0x00, 0x07  // Page address (0 a 7)
};

void oled_init(i2c_inst_t *bus){
    oled_bus = bus;
    
    // Configuramos el byte inicial de nuestro framebuffer estático
    oled_tx_buf[0] = 0x40; 
    oled_clear(); // Limpiamos el resto del buffer

    // Enviamos la inicialización en UNA SOLA transferencia asíncrona
    i2c_drv_write(oled_bus, OLED_ADDR, init_sequence, sizeof(init_sequence), false);
}

void oled_show(void){
    if(!oled_bus) return;

    // 1. Configuramos las direcciones (Columnas y Páginas).
    i2c_drv_write(oled_bus, OLED_ADDR, window_cmd, sizeof(window_cmd), false);
    
    // 2. Enviamos todo el framebuffer de inmediato. 
    // (Asumimos que el i2c_drv_write tiene una pequeña cola para aceptar estas 2 peticiones seguidas).
    i2c_drv_write(oled_bus, OLED_ADDR, oled_tx_buf, sizeof(oled_tx_buf), false);
}

void oled_clear(void){
    // Limpiamos desde la posición 1 (dejamos el byte de control 0x40 intacto)
    memset(&oled_tx_buf[1], 0x00, OLED_BUF_LEN);
}

void oled_pixel(int x, int y, bool on){
    if(x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) return;
    
    // Desplazamos el índice +1 porque el byte 0 es de control
    size_t idx = 1 + (size_t)(y / 8) * OLED_WIDTH + (size_t)x;
    uint8_t mask = (uint8_t)(1u << (y % 8));
    
    if(on) oled_tx_buf[idx] |= mask;
    else   oled_tx_buf[idx] &= ~mask;
}

void oled_escribir_texto(int x, int y, const char *texto) {
    int cursor_x = x; // Guardamos por dónde vamos dibujando horizontalmente
    
    // Recorremos la cadena de texto hasta encontrar el final '\0'
    while (*texto) {
        char c = *texto;
        
        // Solo dibujamos si es un carácter ASCII imprimible (del 32 al 127)
        if (c >= 32 && c <= 127) {
            int indice_letra = c - 32; // Ajustamos al índice de nuestro arreglo
            
            // Cada letra tiene 5 columnas de ancho
            for (int col = 0; col < 5; col++) {
                uint8_t columna_pixels = font_5x7[indice_letra][col];
                
                // Y dibujamos los 8 bits (filas) de esa columna
                for (int fila = 0; fila < 8; fila++) {
                    // Verificamos si el bit de esta fila es un '1'
                    bool pixel_encendido = (columna_pixels & (1 << fila)) != 0;
                    
                    // Llamamos a tu función para pintar el punto
                    oled_pixel(cursor_x + col, y + fila, pixel_encendido);
                }
            }
        }
        
        // Movemos el cursor 6 píxeles a la derecha para la siguiente letra 
        // (5 píxeles de ancho de la letra + 1 píxel en blanco de separación)
        cursor_x += 6;
        
        // Pasamos al siguiente carácter en el texto
        texto++;
    }
}
