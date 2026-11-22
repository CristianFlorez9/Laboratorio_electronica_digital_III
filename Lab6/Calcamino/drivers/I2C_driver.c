/**
 * @file I2C_driver.c
 * @brief Implementación simplificada del driver maestro I2C no bloqueante para RP2040.
 *
 * Controla las instancias i2c0 e i2c1 usando directamente las FIFO de 16 entradas
 * y las interrupciones del periférico DW_apb_i2c (TX_EMPTY, STOP_DET y TX_ABRT),
 * cumpliendo la estrategia polling + interrupciones sin esperas activas.
 */

#include "I2C_driver.h"           /* Interfaz pública, tipos de estado y prototipos */
#include <string.h>               /* Función memset() para limpiar estructuras */
#include "hardware/gpio.h"        /* Configuración de pines SDA/SCL y pull-ups */
#include "hardware/irq.h"         /* Registro y habilitación de I2C0_IRQ / I2C1_IRQ */
#include "hardware/regs/i2c.h"    /* Máscaras de bits de los registros DW_apb_i2c */
#include "hardware/structs/i2c.h" /* Definición de la estructura de registros i2c_hw_t */
#include "hardware/sync.h"        /* Secciones críticas save_and_disable_interrupts / restore */
#include "hardware/timer.h"       /* Lectura de tiempo en microsegundos time_us_32() */

/** @brief Profundidad física (en entradas) de las FIFO de TX y RX del RP2040. */
#define I2C_FIFO_DEPTH   16u

/** @brief Máximo de bytes por lectura (1 byte de registro + 15 comandos de lectura = 16). */
#define I2C_MAX_READ_LEN 15u

/** @brief Umbral de TX FIFO: TX_EMPTY salta cuando quedan 4 entradas o menos por enviar. */
#define I2C_TX_FIFO_TL   4u

/**
 * @brief Descriptor interno de estado para un controlador I2C físico (i2c0 o i2c1).
 */
typedef struct {
    i2c_hw_t *hw;          /**< Puntero a registros de hardware (NULL si no inicializado). */
    volatile bool busy;    /**< true mientras hay una transferencia activa (modificada en ISR). */
    bool is_read;          /**< true si la operación en curso es lectura; false si es escritura. */

    const uint8_t *tx_buf; /**< Puntero al buffer externo persistente de transmisión. */
    size_t tx_len;         /**< Cantidad total de bytes a transmitir en la trama actual. */
    size_t tx_sent;        /**< Contador de bytes ya cargados en la FIFO de transmisión. */

    uint8_t *rx_buf;       /**< Puntero al buffer externo de destino para lecturas. */
    uint8_t rx_len;        /**< Cantidad de bytes solicitados en la lectura actual (1..15). */

    i2c_drv_cb_t cb;       /**< Callback a invocar desde la ISR al finalizar la transacción. */
    i2c_drv_stats_t stats; /**< Métricas acumuladas de errores, rechazos y tiempo máximo ISR. */
} i2c_drv_bus_t;

/** @brief Arreglo estático con el estado de los dos buses I2C del RP2040 (i2c0 y i2c1). */
static i2c_drv_bus_t g_bus[2] = {0};




/**
 * @brief Valida la instancia I2C y retorna su estructura interna si ya fue inicializada.
 *
 * @param i2c Instancia de hardware del Pico SDK (i2c0 o i2c1).
 * @return Puntero al descriptor interno si es válido y está activo; NULL en caso contrario.
 */
static inline i2c_drv_bus_t *get_bus(i2c_inst_t *i2c) {
    if ((i2c != i2c0) && (i2c != i2c1)) {
        return NULL; /* Rechazar punteros nulos o instancias desconocidas */
    }
    i2c_drv_bus_t *b = &g_bus[i2c_hw_index(i2c)]; /* Seleccionar g_bus[0] o g_bus[1] */
    return (b->hw != NULL) ? b : NULL;            /* hw != NULL indica bus inicializado */
}

/**
 * @brief Vacía y descarta cualquier byte residual presente en la FIFO de recepción.
 *
 * @param hw Puntero a los registros de hardware del controlador I2C.
 */
static inline void flush_rx_fifo(i2c_hw_t *hw) {
    while (hw->rxflr > 0u) {
        (void)hw->data_cmd; /* Extraer y descartar cada byte pendiente en RX */
    }
}

/**
 * @brief Prepara el controlador y actualiza la dirección esclava antes de una trama.
 * @note Llamar siempre dentro de una sección crítica (interrupciones deshabilitadas).
 *
 * @param b    Puntero al estado interno del bus.
 * @param addr Dirección esclava de 7 bits del dispositivo destino.
 */
static void prepare_bus_locked(i2c_drv_bus_t *b, uint8_t addr) {
    i2c_hw_t *hw = b->hw;
    hw->intr_mask = 0u; /* Enmascarar interrupciones durante la preparación */

    /* Reprogramar el registro TAR solo si la dirección destino difiere de la actual */
    if ((hw->tar & I2C_IC_TAR_IC_TAR_BITS) != addr) {
        hw->enable = 0u;   /* Deshabilitar bloque (requisito hardware para escribir TAR) */
        hw->tar    = addr; /* Cargar nueva dirección esclava de 7 bits */
        hw->enable = 1u;   /* Rehabilitar controlador I2C */
    }

    flush_rx_fifo(hw);      /* Eliminar residuos previos de la RX FIFO */
    (void)hw->clr_tx_abrt;  /* Limpiar bandera previa de aborto (TX_ABRT) */
    (void)hw->clr_stop_det; /* Limpiar bandera previa de condición STOP */
}

/**
 * @brief Carga bytes desde @c tx_buf hacia la FIFO de transmisión hasta llenarla o terminar.
 *
 * @param b Puntero al estado interno del bus en modo escritura.
 */
static void pump_tx_fifo(i2c_drv_bus_t *b) {
    i2c_hw_t *hw = b->hw;

    while ((hw->txflr < I2C_FIFO_DEPTH) && (b->tx_sent < b->tx_len)) {
        uint32_t cmd = b->tx_buf[b->tx_sent++]; /* Tomar siguiente byte y avanzar índice */
        if (b->tx_sent == b->tx_len) {
            cmd |= I2C_IC_DATA_CMD_STOP_BITS;   /* Adjuntar STOP en el último byte */
        }
        hw->data_cmd = cmd; /* Insertar byte y flags en la TX FIFO de hardware */
    }
}

/**
 * @brief Cierra la transacción en curso, libera el bus e invoca el callback registrado.
 *
 * @param b  Puntero al estado interno del bus.
 * @param st Estado final de la operación (I2C_DRV_OK o I2C_DRV_ERROR).
 */
static void finish_txn(i2c_drv_bus_t *b, i2c_drv_status_t st) {
    b->hw->intr_mask = 0u;   /* Apagar interrupciones del periférico al quedar libre */
    i2c_drv_cb_t cb = b->cb; /* Guardar copia local del callback antes de liberar */
    b->busy = false;         /* Marcar bus libre (permite encadenar otra llamada en cb) */

    if (cb != NULL) {
        cb(st); /* Notificar resultado al módulo usuario */
    }
}





/**
 * @brief Rutina común de servicio de interrupción para i2c0 e i2c1.
 *
 * Atiende abortos de bus (TX_ABRT), recarga de escrituras largas (TX_EMPTY) y
 * finalización de tramas (STOP_DET), registrando la duración máxima de la ISR.
 *
 * @param b Puntero al estado interno del bus que generó la interrupción.
 */
static void i2c_isr_common(i2c_drv_bus_t *b) {
    uint32_t t0   = time_us_32(); /* Instante de entrada para medir latencia de ISR */
    i2c_hw_t *hw  = b->hw;
    uint32_t stat = hw->intr_stat; /* Leer causas activas de interrupción */

    if (!b->busy) {
        /* Interrupción espuria con bus inactivo: desactivar y limpiar */
        hw->intr_mask = 0u;
        (void)hw->clr_intr;
    } else if (stat & I2C_IC_INTR_STAT_R_TX_ABRT_BITS) {
        /* Prioridad máxima: aborto por NACK, pérdida de arbitraje o error de bus */
        (void)hw->clr_tx_abrt;
        (void)hw->clr_stop_det;
        flush_rx_fifo(hw);
        b->stats.error_count++;
        finish_txn(b, I2C_DRV_ERROR);
    } else {
        /* 1. Rellenar TX FIFO en escrituras mayores a 16 bytes (ej. pantalla OLED) */
        if (!b->is_read && (stat & I2C_IC_INTR_STAT_R_TX_EMPTY_BITS)) {
            pump_tx_fifo(b);
            if (b->tx_sent >= b->tx_len) {
                /* Ya entraron todos los bytes: apagar TX_EMPTY y esperar solo STOP/ABRT */
                hw->intr_mask = I2C_IC_INTR_MASK_M_STOP_DET_BITS |
                                I2C_IC_INTR_MASK_M_TX_ABRT_BITS;
            }
        }

        /* 2. Condición STOP detectada: cerrar lectura o confirmar fin de escritura */
        if (stat & I2C_IC_INTR_STAT_R_STOP_DET_BITS) {
            (void)hw->clr_stop_det;

            if (b->is_read) {
                uint8_t n = 0u;
                while ((hw->rxflr > 0u) && (n < b->rx_len)) {
                    b->rx_buf[n++] = (uint8_t)(hw->data_cmd & I2C_IC_DATA_CMD_DAT_BITS);
                }
                flush_rx_fifo(hw);

                if (n != b->rx_len) {
                    b->stats.error_count++; /* Contabilizar trama incompleta */
                }
                finish_txn(b, (n == b->rx_len) ? I2C_DRV_OK : I2C_DRV_ERROR);
            } else if ((b->tx_sent >= b->tx_len) && (hw->txflr == 0u)) {
                finish_txn(b, I2C_DRV_OK); /* Todos los bytes transmitidos con éxito */
            }
        }
    }

    /* Registrar peor tiempo de ejecución de la ISR [us] */
    uint32_t dt = time_us_32() - t0;
    if (dt > b->stats.max_isr_us) {
        b->stats.max_isr_us = dt;
    }
}

/** @brief Vector de interrupción exclusivo asignado a I2C0_IRQ. */
static void i2c0_irq_handler(void) {
    i2c_isr_common(&g_bus[0]);
}

/** @brief Vector de interrupción exclusivo asignado a I2C1_IRQ. */
static void i2c1_irq_handler(void) {
    i2c_isr_common(&g_bus[1]);
}





/**
 * @brief Inicializa un bus I2C físico (i2c0 o i2c1), sus pines GPIO y su interrupción.
 *
 * @param i2c     Instancia de hardware (i2c0 o i2c1).
 * @param sda_pin Número de GPIO para SDA.
 * @param scl_pin Número de GPIO para SCL.
 * @param baud_hz Frecuencia del reloj I2C en Hz (ej. 400000).
 * @return true si se inicializó correctamente; false si los argumentos son inválidos.
 */
bool i2c_drv_init(i2c_inst_t *i2c, uint sda_pin, uint scl_pin, uint32_t baud_hz) {
    if (((i2c != i2c0) && (i2c != i2c1)) || (baud_hz == 0u)) {
        return false;
    }

    uint idx = i2c_hw_index(i2c);
    i2c_drv_bus_t *b = &g_bus[idx];
    memset(b, 0, sizeof(*b)); /* Reiniciar estado y estadísticas en cero */

    i2c_hw_t *hw = i2c_get_hw(i2c);

    i2c_init(i2c, baud_hz);                    /* Configurar reloj y modo maestro */
    gpio_set_function(sda_pin, GPIO_FUNC_I2C); /* Asignar SDA al controlador I2C */
    gpio_set_function(scl_pin, GPIO_FUNC_I2C); /* Asignar SCL al controlador I2C */
    gpio_pull_up(sda_pin);                     /* Activar pull-up interno en SDA */
    gpio_pull_up(scl_pin);                     /* Activar pull-up interno en SCL */

    hw->intr_mask = 0u;                            /* Mantener IRQs deshabilitadas */
    hw->enable    = 0u;                            /* Pausar bloque para configurar */
    hw->con      |= I2C_IC_CON_IC_RESTART_EN_BITS; /* Habilitar Repeated START */
    hw->tx_tl     = I2C_TX_FIFO_TL;                /* Umbral TX_EMPTY en 4 entradas */
    hw->enable    = 1u;                            /* Reactivar bloque I2C */
    (void)hw->clr_intr;                            /* Limpiar banderas residuales */
    flush_rx_fifo(hw);                             /* Vaciar RX FIFO */

    uint irq_num = (idx == 0u) ? I2C0_IRQ : I2C1_IRQ;
    irq_set_exclusive_handler(irq_num, (idx == 0u) ? i2c0_irq_handler : i2c1_irq_handler);
    irq_set_enabled(irq_num, true);

    b->hw = hw; /* Publicar puntero: marca oficialmente el bus como inicializado */
    return true;
}

/**
 * @brief Consulta si el bus especificado tiene una transferencia activa.
 *
 * @param i2c Instancia de hardware (i2c0 o i2c1).
 * @return true si el bus está ocupado; false si está libre o no inicializado.
 */
bool i2c_drv_is_busy(i2c_inst_t *i2c) {
    i2c_drv_bus_t *b = get_bus(i2c);
    return (b != NULL) ? b->busy : false;
}

/**
 * @brief Inicia de forma no bloqueante una escritura I2C de longitud arbitraria.
 *
 * @param i2c    Instancia de hardware (i2c0 o i2c1).
 * @param addr   Dirección esclava de 7 bits.
 * @param tx_buf Puntero al buffer persistente de datos a transmitir.
 * @param len    Cantidad de bytes a enviar (> 0).
 * @param cb     Callback opcional al concluir (puede ser NULL).
 * @return I2C_DRV_OK si arrancó; I2C_DRV_BUSY si estaba ocupado; I2C_DRV_INVALID si hay error de argumentos.
 */
i2c_drv_status_t i2c_drv_write(i2c_inst_t *i2c, uint8_t addr,
                               const uint8_t *tx_buf, size_t len,
                               i2c_drv_cb_t cb) {
    i2c_drv_bus_t *b = get_bus(i2c);
    if ((b == NULL) || (tx_buf == NULL) || (len == 0u)) {
        return I2C_DRV_INVALID;
    }

    uint32_t irq_state = save_and_disable_interrupts(); /* Sección crítica */
    if (b->busy) {
        b->stats.busy_drop_count++; /* Registrar rechazo por bus ocupado */
        restore_interrupts(irq_state);
        return I2C_DRV_BUSY;
    }

    b->busy    = true;
    b->is_read = false;
    b->tx_buf  = tx_buf;
    b->tx_len  = len;
    b->tx_sent = 0u;
    b->cb      = cb;

    prepare_bus_locked(b, addr);
    pump_tx_fifo(b); /* Cargar hasta los primeros 16 bytes */

    uint32_t mask = I2C_IC_INTR_MASK_M_STOP_DET_BITS | I2C_IC_INTR_MASK_M_TX_ABRT_BITS;
    if (b->tx_sent < b->tx_len) {
        mask |= I2C_IC_INTR_MASK_M_TX_EMPTY_BITS; /* Habilitar recargas para tramas > 16 bytes */
    }
    b->hw->intr_mask = mask;

    restore_interrupts(irq_state);
    return I2C_DRV_OK;
}

/**
 * @brief Inicia de forma no bloqueante la lectura de 1 a 15 bytes desde un registro con Repeated START.
 *
 * @param i2c    Instancia de hardware (i2c0 o i2c1).
 * @param addr   Dirección esclava de 7 bits.
 * @param reg    Dirección del registro inicial a leer.
 * @param rx_buf Buffer persistente de destino.
 * @param len    Número de bytes a leer (1 a 15).
 * @param cb     Callback opcional al concluir (puede ser NULL).
 * @return I2C_DRV_OK si arrancó; I2C_DRV_BUSY si estaba ocupado; I2C_DRV_INVALID si hay error de argumentos.
 */
i2c_drv_status_t i2c_drv_read_reg(i2c_inst_t *i2c, uint8_t addr, uint8_t reg,
                                  uint8_t *rx_buf, uint8_t len,
                                  i2c_drv_cb_t cb) {
    i2c_drv_bus_t *b = get_bus(i2c);
    if ((b == NULL) || (rx_buf == NULL) || (len == 0u) || (len > I2C_MAX_READ_LEN)) {
        return I2C_DRV_INVALID;
    }

    uint32_t irq_state = save_and_disable_interrupts(); /* Sección crítica */
    if (b->busy) {
        b->stats.busy_drop_count++; /* Registrar rechazo por bus ocupado */
        restore_interrupts(irq_state);
        return I2C_DRV_BUSY;
    }

    b->busy    = true;
    b->is_read = true;
    b->rx_buf  = rx_buf;
    b->rx_len  = len;
    b->cb      = cb;

    prepare_bus_locked(b, addr);

    /* 1 byte de registro + hasta 15 comandos de lectura (cabe completo en la FIFO) */
    b->hw->data_cmd = reg;
    for (uint8_t i = 0u; i < len; i++) {
        uint32_t cmd = I2C_IC_DATA_CMD_CMD_BITS; /* Bit CMD=1 ordena lectura */
        if (i == 0u) {
            cmd |= I2C_IC_DATA_CMD_RESTART_BITS; /* Repeated START en el primer byte */
        }
        if (i == (len - 1u)) {
            cmd |= I2C_IC_DATA_CMD_STOP_BITS;    /* NACK + STOP en el último byte */
        }
        b->hw->data_cmd = cmd;
    }

    /* Solo esperar fin de trama (STOP_DET) o aborto (TX_ABRT) */
    b->hw->intr_mask = I2C_IC_INTR_MASK_M_STOP_DET_BITS | I2C_IC_INTR_MASK_M_TX_ABRT_BITS;

    restore_interrupts(irq_state);
    return I2C_DRV_OK;
}

/**
 * @brief Obtiene una copia coherente de las métricas de diagnóstico del bus.
 *
 * @param i2c      Instancia de hardware (i2c0 o i2c1).
 * @param[out] out Estructura destino donde se copian las estadísticas.
 */
void i2c_drv_get_stats(i2c_inst_t *i2c, i2c_drv_stats_t *out) {
    i2c_drv_bus_t *b = get_bus(i2c);
    if ((b == NULL) || (out == NULL)) {
        return;
    }

    uint32_t irq_state = save_and_disable_interrupts();
    *out = b->stats; /* Copia atómica para evitar lecturas parciales frente a la ISR */
    restore_interrupts(irq_state);
}




/*enable: Habilita (1) o pausa (0) el controlador I2C para permitir cambios de configuración */
/*con: Registro de control general del I2C (modo maestro, velocidad y Repeated START) */
/*tar: Dirección esclava de 7 bits del dispositivo destino (requiere enable = 0 para cambiarla) */

/*data_cmd: Ventanilla a las FIFOs: escribir mete comandos/datos en TX FIFO; leer extrae bytes de RX FIFO */
/*txflr: Cantidad actual de elementos pendientes por enviar en la TX FIFO (0..16) */
/*rxflr: Cantidad actual de bytes recibidos disponibles para leer en la RX FIFO (0..16) */
/*tx_tl: Umbral de TX FIFO: dispara la interrupción TX_EMPTY cuando quedan <= N bytes por enviar */

/*intr_mask: Máscara de interrupciones: habilita (1) o bloquea (0) qué eventos I2C disparan la ISR */
/*intr_stat: Estado de interrupciones activas y habilitadas: indica a la ISR qué evento ocurrió */
/*clr_intr: Lectura de limpieza (read-to-clear): borra todas las banderas de interrupción pendientes */
/*clr_stop_det: Lectura de limpieza (read-to-clear): borra la bandera de fin de trama (STOP_DET) */
/*clr_tx_abrt: Lectura de limpieza (read-to-clear): borra la bandera de error/aborto de bus (TX_ABRT) */
