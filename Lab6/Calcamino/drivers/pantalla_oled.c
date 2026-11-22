/**
 * @file    pantalla_oled.c
 * @brief   Implementación no bloqueante del driver SSD1306 (128x64) por I2C.
 */

#include "pantalla_oled.h"
#include "drivers/font.h"
#include <string.h>

static i2c_inst_t *oled_bus = NULL;
static volatile bool oled_busy = false;

/* tx_buf[0] = 0x40 (control de datos); tx_buf[1..1024] = framebuffer */
static uint8_t oled_tx_buf[OLED_BUF_LEN + 1u];

static const uint8_t init_sequence[] = {
    0x00,             /* Control byte: Stream de comandos */
    0xAE,             /* Display OFF */
    0x20, 0x00,       /* Direccionamiento horizontal */
    0x81, 0x7F,       /* Contraste */
    0xA1,             /* Segment remap (espejo horizontal) */
    0xC8,             /* COM scan direction */
    0xA6,             /* Display normal (no invertido) */
    0xA8, 0x3F,       /* Multiplex 1/64 */
    0xD3, 0x00,       /* Display offset 0 */
    0x40,             /* Start line 0 */
    0x8D, 0x14,       /* Charge pump ON */
    0xAF              /* Display ON */
};

static const uint8_t window_cmd[] = {
    0x00,             /* Control byte: Stream de comandos */
    0x21, 0x00, 0x7F, /* Column address (0 a 127) */
    0x22, 0x00, 0x07  /* Page address (0 a 7) */
};

static const uint8_t cmd_disp_off[] = { 0x00, 0xAE };
static const uint8_t cmd_disp_on[]  = { 0x00, 0xAF };

/** @brief Callback final: libera el estado ocupado de la pantalla. */
static void oled_done_cb(i2c_drv_status_t status) {
    (void)status;
    oled_busy = false;
}

/** @brief Callback intermedio: tras fijar ventana, lanza el envío del framebuffer. */
static void oled_window_cb(i2c_drv_status_t status) {
    if (status == I2C_DRV_OK) {
        if (i2c_drv_write(oled_bus, OLED_ADDR, oled_tx_buf,
                          sizeof(oled_tx_buf), oled_done_cb) == I2C_DRV_OK) {
            return;
        }
    }
    oled_busy = false;
}

void oled_init(i2c_inst_t *bus) {
    oled_bus = bus;
    oled_tx_buf[0] = 0x40u;
    oled_clear();

    if (oled_bus == NULL) {
        return;
    }

    oled_busy = true;
    if (i2c_drv_write(oled_bus, OLED_ADDR, init_sequence,
                      sizeof(init_sequence), oled_done_cb) != I2C_DRV_OK) {
        oled_busy = false;
    }
}

bool oled_is_busy(void) {
    return oled_busy;
}

void oled_set_power(bool on) {
    if ((oled_bus == NULL) || oled_busy) {
        return;
    }

    oled_busy = true;
    const uint8_t *cmd = on ? cmd_disp_on : cmd_disp_off;
    if (i2c_drv_write(oled_bus, OLED_ADDR, cmd, 2u, oled_done_cb) != I2C_DRV_OK) {
        oled_busy = false;
    }
}

void oled_show(void) {
    if ((oled_bus == NULL) || oled_busy) {
        return;
    }

    oled_busy = true;
    if (i2c_drv_write(oled_bus, OLED_ADDR, window_cmd,
                      sizeof(window_cmd), oled_window_cb) != I2C_DRV_OK) {
        oled_busy = false;
    }
}

void oled_clear(void) {
    memset(&oled_tx_buf[1], 0x00, OLED_BUF_LEN);
}

void oled_pixel(int x, int y, bool on) {
    if ((x < 0) || (x >= OLED_WIDTH) || (y < 0) || (y >= OLED_HEIGHT)) {
        return;
    }

    size_t idx = 1u + ((size_t)(y / 8) * OLED_WIDTH) + (size_t)x;
    uint8_t mask = (uint8_t)(1u << (y % 8));

    if (on) {
        oled_tx_buf[idx] |= mask;
    } else {
        oled_tx_buf[idx] &= (uint8_t)~mask;
    }
}

void oled_escribir_texto(int x, int y, const char *texto) {
    int cursor_x = x;

    if (texto == NULL) {
        return;
    }

    while (*texto != '\0') {
        unsigned char c = (unsigned char)(*texto);

        if (c <= 127u) {
            int indice_letra = (int)c;

            for (int col = 0; col < 5; col++) {
                uint8_t columna_pixels = font_5x7[indice_letra][col];

                for (int fila = 0; fila < 8; fila++) {
                    bool pixel_encendido = (columna_pixels & (1u << fila)) != 0u;
                    oled_pixel(cursor_x + col, y + fila, pixel_encendido);
                }
            }
        }

        cursor_x += 6;
        texto++;
    }
}