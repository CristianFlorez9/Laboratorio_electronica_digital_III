/**
 * @file    Calcamino.c
 * @brief   Ciclo principal de CalCamino bajo arquitectura polling + interrupciones.
 * @details Inicializa periféricos y modelos antes de la sesión (fuera de ISR).
 *          En el bucle principal consulta de forma no bloqueante las banderas
 *          levantadas por las ISR (Timer 50 Hz, I2C, ADC y GPIO), ejecuta
 *          la detección de marcha, cálculo calórico, estimación de batería,
 *          máquina de estados, refresco asíncrono de OLED y entrada sin carrera
 *          a bajo consumo (__wfi). Reporta contadores de desbordamiento y diagnóstico.
 * @note    No contiene esperas activas (busy-wait) ni RTOS.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"

#include "drivers/I2C_driver.h"
#include "drivers/acelerometro_MPU_6050.h"
#include "drivers/pantalla_oled.h"
#include "drivers/adc_driver.h"
#include "drivers/power_driver.h"

#include "app/state_machine.h"
#include "app/step_detector.h"
#include "app/modelo_calorias.h"
#include "app/modelo_bateria.h"
#include "app/interfaz_grafica.h"

/* Mapa de pines de hardware (RP2040) */
#define I2C_OLED_BUS        i2c0
#define I2C_OLED_SDA_PIN    0u       /* GP0, pin físico 1 */
#define I2C_OLED_SCL_PIN    1u       /* GP1, pin físico 2 */

#define I2C_MPU_BUS         i2c1
#define I2C_MPU_SDA_PIN     2u       /* GP2, pin físico 4 */
#define I2C_MPU_SCL_PIN     3u       /* GP3, pin físico 5 */

#define MPU_INT_PIN         INT_PIN  /* GP4 definido en acelerometro_MPU_6050.h */
#define BAT_ADC_PIN         26u      /* GP26 (ADC0), pin físico 31, a través de divisor resistivo */

#define BTN_ENTER_PIN       15u      /* Abrir CONFIG, confirmar campo, iniciar sesión */
#define BTN_UP_PIN          22u      /* Subir el valor en CONFIG */
#define BTN_DOWN_PIN        18u      /* Bajar el valor en CONFIG */

/* Constantes de temporización */
#define ACCEL_PERIODO_MS    20u      /* Muestreo acelerómetro: 50 Hz */
#define ADC_PERIODO_MS      50u      /* Muestreo batería: 20 Hz */
#define OLED_PERIODO_MS     200u     /* Refresco visual: 5 Hz */
#define DIAG_PERIODO_MS     5000u    /* Reporte de diagnóstico: 0.2 Hz */
#define DEBOUNCE_BTN_MS     40u      /* Tiempo antirrebote por pulsador [ms] */

/** @brief Estados internos para conmutación no bloqueante Wake-on-Motion del MPU6050. */
typedef enum {
    WOM_IDLE = 0,       /**< Sensor en modo activo continuo a 50 Hz. */
    WOM_ENABLING,       /**< Enviando secuencia I2C para activar WOM (5 Hz). */
    WOM_ENABLED,        /**< Sensor en modo WOM; MCU listo para bajo consumo. */
    WOM_CLEARING_IRQ,   /**< Pendiente lectura de INT_STATUS tras despertar. */
    WOM_DISABLING,      /**< Leyendo INT_STATUS para limpiar latch del pin INT. */
    WOM_WAIT_ACTIVE     /**< Enviando secuencia I2C para restaurar 50 Hz. */
} wom_ctrl_state_t;

/** @brief Estado de un pulsador: la ISR mide cuánto duró cada pulsación. */
typedef struct {
    bool              abajo;         /**< true mientras el botón está presionado (solo ISR). */
    uint32_t          inicio_ms;     /**< Instante en que se presionó (solo ISR). */
    uint32_t          ultimo_ms;     /**< Instante del último borde aceptado (solo ISR). */
    volatile uint32_t duracion_ms;   /**< Duración de la última pulsación completada. */
    volatile bool     evento;        /**< true cuando hay una pulsación completada sin atender. */
} boton_t;

/* Buffers persistentes para transacciones I2C asíncronas */
static uint8_t g_mpu_raw[6];

/* Banderas compartidas ISR <-> ciclo principal */
static volatile bool g_tick_accel      = false;
static volatile bool g_mpu_data_listo  = false;
static volatile bool g_mpu_error       = false;
static volatile bool g_i2c_cfg_done    = false;
static volatile bool g_irq_movimiento  = false;
static volatile bool g_alarma_sm       = false;

static boton_t g_btn_enter = {0};
static boton_t g_btn_up    = {0};
static boton_t g_btn_down  = {0};

/** @brief ISR del timer periódico (50 Hz): solicita captura del acelerómetro. */
static bool on_accel_timer_isr(repeating_timer_t *rt) {
    (void)rt;
    g_tick_accel = true;
    return true;
}

/** @brief Callback de fin de lectura I2C de 6 bytes del MPU6050. */
static void on_mpu_read_done_isr(i2c_drv_status_t status) {
    if (status == I2C_DRV_OK) {
        g_mpu_data_listo = true;
    } else {
        g_mpu_error = true;
    }
}

/** @brief Callback de fin de secuencia de configuración I2C del MPU6050. */
static void on_mpu_cfg_done_isr(i2c_drv_status_t status) {
    (void)status;
    g_i2c_cfg_done = true;
}

/** @brief Alarma de un solo disparo para despertar antes de vencer timeout de estado. */
static int64_t on_wakeup_alarm_isr(alarm_id_t id, void *user_data) {
    (void)id;
    (void)user_data;
    g_alarma_sm = true;
    return 0;
}

/**
 * @brief Procesa un borde de un pulsador activo en bajo (contexto ISR, ejecución acotada).
 * @param b      Estado del pulsador.
 * @param gpio   Pin del pulsador, para leer su nivel actual.
 * @param ahora  Instante actual [ms].
 */
static void procesar_borde_boton(boton_t *b, uint gpio, uint32_t ahora) {
    if ((ahora - b->ultimo_ms) < DEBOUNCE_BTN_MS) {
        return;
    }
    b->ultimo_ms = ahora;

    bool presionado = !gpio_get(gpio);

    if (presionado && !b->abajo) {
        b->abajo     = true;
        b->inicio_ms = ahora;
    } else if (!presionado && b->abajo) {
        b->abajo       = false;
        b->duracion_ms = ahora - b->inicio_ms;
        b->evento      = true;
    }
}

/** @brief ISR unificada de pines GPIO: pin INT del MPU6050 y botones de usuario. */
static void on_gpio_irq_callback(uint gpio, uint32_t events) {
    (void)events;
    uint32_t ahora = to_ms_since_boot(get_absolute_time());

    if (gpio == MPU_INT_PIN) {
        g_irq_movimiento = true;
    } else if (gpio == BTN_ENTER_PIN) {
        procesar_borde_boton(&g_btn_enter, gpio, ahora);
    } else if (gpio == BTN_UP_PIN) {
        procesar_borde_boton(&g_btn_up, gpio, ahora);
    } else if (gpio == BTN_DOWN_PIN) {
        procesar_borde_boton(&g_btn_down, gpio, ahora);
    }
}

/** @brief Configura entradas digitales con pull-up/pull-down e interrupciones GPIO. */
static void configurar_gpio_irqs(void) {
    const uint btns[] = { BTN_ENTER_PIN, BTN_UP_PIN, BTN_DOWN_PIN };

    for (size_t i = 0u; i < 3u; i++) {
        gpio_init(btns[i]);
        gpio_set_dir(btns[i], GPIO_IN);
        gpio_pull_up(btns[i]);
        gpio_set_irq_enabled_with_callback(btns[i],
                                           GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE,
                                           true, &on_gpio_irq_callback);
    }

    gpio_init(MPU_INT_PIN);
    gpio_set_dir(MPU_INT_PIN, GPIO_IN);
    gpio_pull_down(MPU_INT_PIN);
    gpio_set_irq_enabled(MPU_INT_PIN, GPIO_IRQ_EDGE_RISE, true);
}

/**
 * @brief Toma de forma atómica una pulsación completada de un pulsador.
 * @param b            Estado del pulsador.
 * @param[out] dur_ms  Duración de la pulsación [ms].
 * @return true si había una pulsación pendiente (y queda marcada como atendida).
 */
static bool tomar_pulsacion(boton_t *b, uint32_t *dur_ms) {
    bool hay;
    uint32_t irq = save_and_disable_interrupts();
    hay = b->evento;
    if (hay) {
        *dur_ms   = b->duracion_ms;
        b->evento = false;
    }
    restore_interrupts(irq);
    return hay;
}

/** @brief Verifica si existe cualquier evento pendiente producido por las ISR. */
static inline bool hay_trabajo_pendiente(void) {
    return g_tick_accel || g_mpu_data_listo || g_mpu_error ||
           g_i2c_cfg_done || g_irq_movimiento ||
           g_btn_enter.evento || g_btn_up.evento || g_btn_down.evento ||
           adc_driver_hay_muestra();
}

/**
 * @brief Sube o baja en una unidad el campo seleccionado, con vuelta al otro extremo.
 * @param p      Parámetros personales a modificar.
 * @param campo  0: masa [kg], 1: estatura [cm].
 * @param subir  true para incrementar, false para decrementar.
 */
static void ajustar_valor(modelo_cal_params_t *p, uint8_t campo, bool subir) {
    if (campo == 0u) {
        if (subir) {
            p->masa_kg = (p->masa_kg >= MODELO_CAL_MASA_MAX_KG)
                         ? MODELO_CAL_MASA_MIN_KG
                         : (uint16_t)(p->masa_kg + 1u);
        } else {
            p->masa_kg = (p->masa_kg <= MODELO_CAL_MASA_MIN_KG)
                         ? MODELO_CAL_MASA_MAX_KG
                         : (uint16_t)(p->masa_kg - 1u);
        }
    } else {
        if (subir) {
            p->estatura_cm = (p->estatura_cm >= MODELO_CAL_ESTATURA_MAX_CM)
                             ? MODELO_CAL_ESTATURA_MIN_CM
                             : (uint16_t)(p->estatura_cm + 1u);
        } else {
            p->estatura_cm = (p->estatura_cm <= MODELO_CAL_ESTATURA_MIN_CM)
                             ? MODELO_CAL_ESTATURA_MAX_CM
                             : (uint16_t)(p->estatura_cm - 1u);
        }
    }
}

int main(void) {
    stdio_init_all();

    /* Inicialización de hardware y drivers antes de la sesión (fuera de ISR) */
    power_init();
    i2c_drv_init(I2C_MPU_BUS, I2C_MPU_SDA_PIN, I2C_MPU_SCL_PIN, 400000u);
    i2c_drv_init(I2C_OLED_BUS, I2C_OLED_SDA_PIN, I2C_OLED_SCL_PIN, 400000u);
    configurar_gpio_irqs();

    mpu_init_non_blocking(I2C_MPU_BUS);
    while (!mpu_is_init_complete()) {
        uint32_t irq = save_and_disable_interrupts();
        if (!mpu_is_init_complete()) {
            __wfi();
        }
        restore_interrupts(irq);
    }

    oled_init(I2C_OLED_BUS);
    adc_driver_init(BAT_ADC_PIN, ADC_PERIODO_MS);

    /* Inicialización de módulos de aplicación */
    sm_t sm;
    sm_init(&sm, to_ms_since_boot(get_absolute_time()));

    bateria_t bat;
    bateria_init(&bat);
    uint8_t bat_pct = 100u;

    modelo_cal_params_t cal_params = {
        .masa_kg     = 70u,
        .estatura_cm = 170u,
        .tipo        = MODELO_CAL_TOTAL
    };
    modelo_cal_t cal;
    modelo_cal_init(&cal, &cal_params);

    step_detector_init();

    repeating_timer_t accel_timer;
    bool accel_timer_activo = add_repeating_timer_ms((int32_t)ACCEL_PERIODO_MS,
                                                     on_accel_timer_isr, NULL, &accel_timer);

    wom_ctrl_state_t wom_state   = WOM_IDLE;
    bool estaba_caminando        = false;
    uint8_t campo_config         = 0u; /* 0: masa_kg, 1: estatura_cm */
    uint32_t ultimo_refresh_ms   = 0u;
    uint32_t ultimo_paso_cal_ms  = 0u;
    uint32_t ultimo_diag_ms      = 0u;

    /* Bucle único de control (polling sobre flags de ISR + espera WFI) */
    while (true) {
        uint32_t ahora_ms  = to_ms_since_boot(get_absolute_time());
        sm_estado_t estado = sm_obtener_estado(&sm);

        /* A. Despertar por movimiento desde BAJO_CONSUMO */
        if (g_irq_movimiento) {
            g_irq_movimiento = false;
            if (estado == SM_ESTADO_BAJO_CONSUMO) {
                power_salir_bajo_consumo();
                adc_driver_pausar(false);
                if (!accel_timer_activo) {
                    accel_timer_activo = add_repeating_timer_ms((int32_t)ACCEL_PERIODO_MS,
                                                                on_accel_timer_isr, NULL, &accel_timer);
                }
                sm_evento(&sm, SM_EV_MOVIMIENTO, ahora_ms);
                wom_state = WOM_CLEARING_IRQ;
            }
        }

        /* B. Máquina de transición no bloqueante de salida de WOM */
        if ((wom_state == WOM_CLEARING_IRQ) && !i2c_drv_is_busy(I2C_MPU_BUS)) {
            g_i2c_cfg_done = false;
            if (mpu_clear_irq_non_blocking(on_mpu_cfg_done_isr)) {
                wom_state = WOM_DISABLING;
            }
        } else if ((wom_state == WOM_DISABLING) && g_i2c_cfg_done && !i2c_drv_is_busy(I2C_MPU_BUS)) {
            g_i2c_cfg_done = false;
            if (mpu_set_wom_mode(false, on_mpu_cfg_done_isr)) {
                wom_state = WOM_WAIT_ACTIVE;
            }
        } else if ((wom_state == WOM_WAIT_ACTIVE) && g_i2c_cfg_done) {
            g_i2c_cfg_done = false;
            wom_state = WOM_IDLE;
        }

        /* C. Atención de los tres pulsadores de usuario */
        uint32_t dur_ms = 0u;

        if (tomar_pulsacion(&g_btn_enter, &dur_ms)) {
            if (estado == SM_ESTADO_CONFIG) {
                if (campo_config == 0u) {
                    campo_config = 1u;
                } else {
                    campo_config = 0u;
                    modelo_cal_init(&cal, &cal_params);
                    sm_evento(&sm, SM_EV_BOTON_ACEPTAR, ahora_ms);
                }
            } else if ((estado == SM_ESTADO_INACTIVO) ||
                       (estado == SM_ESTADO_SESION_ACTIVA)) {
                campo_config = 0u;
                sm_evento(&sm, SM_EV_BOTON_CONFIG, ahora_ms);
            }
        }

        if (tomar_pulsacion(&g_btn_up, &dur_ms)) {
            if (estado == SM_ESTADO_CONFIG) {
                ajustar_valor(&cal_params, campo_config, true);
            }
        }

        if (tomar_pulsacion(&g_btn_down, &dur_ms)) {
            if (estado == SM_ESTADO_CONFIG) {
                ajustar_valor(&cal_params, campo_config, false);
            }
        }

        /* D. Disparo de lectura I2C no bloqueante del acelerómetro (50 Hz) */
        if (g_tick_accel) {
            g_tick_accel = false;
            if ((estado != SM_ESTADO_BAJO_CONSUMO) &&
                (wom_state == WOM_IDLE) &&
                !i2c_drv_is_busy(I2C_MPU_BUS)) {
                mpu_request_data_non_blocking(on_mpu_read_done_isr);
            }
        }

        /* E. Procesamiento diferido de muestra del acelerómetro */
        if (g_mpu_data_listo) {
            g_mpu_data_listo = false;
            mpu_get_last_data(g_mpu_raw);

            /* MPU6050 entrega 16 bits en Big-Endian */
            int16_t ax = (int16_t)(((uint16_t)g_mpu_raw[0] << 8) | g_mpu_raw[1]);
            int16_t ay = (int16_t)(((uint16_t)g_mpu_raw[2] << 8) | g_mpu_raw[3]);
            int16_t az = (int16_t)(((uint16_t)g_mpu_raw[4] << 8) | g_mpu_raw[5]);

            uint32_t pasos_prev = step_count();
            step_detector_update(ax, ay, az);
            bool caminando     = is_walking();
            uint32_t pasos_act = step_count();

            if (caminando && !estaba_caminando) {
                sm_evento(&sm, SM_EV_MARCHA_CONFIRMADA, ahora_ms);
                ultimo_paso_cal_ms = ahora_ms;
            } else if (!caminando && estaba_caminando) {
                uint32_t t_ultimo = step_last_time_ms();
                sm_evento(&sm, SM_EV_MARCHA_FINALIZADA, (t_ultimo > 0u) ? t_ultimo : ahora_ms);
            }
            estaba_caminando = caminando;

            /* Acumular calorías únicamente durante intervalos de caminata confirmada */
            if (caminando && (pasos_act > pasos_prev)) {
                uint32_t dt_ms = (ahora_ms > ultimo_paso_cal_ms)
                                 ? (ahora_ms - ultimo_paso_cal_ms)
                                 : ACCEL_PERIODO_MS;
                (void)modelo_cal_agregar(&cal, (float)cadence(), dt_ms);
                ultimo_paso_cal_ms = ahora_ms;
            }
        }

        if (g_mpu_error) {
            g_mpu_error = false;
        }

        /* F. Procesamiento diferido de muestras ADC de batería */
        uint16_t mv_bat = 0u;
        if (adc_driver_procesar(&mv_bat)) {
            bat_pct = bateria_actualizar(&bat, mv_bat);
        }

        /* G. Actualización de temporizadores de máquina de estados y acciones */
        sm_actualizar(&sm, ahora_ms);
        uint32_t acciones = sm_tomar_acciones(&sm);

        if (acciones & SM_ACCION_REINICIAR_SESION) {
            step_detector_init();
            modelo_cal_init(&cal, &cal_params);
            estaba_caminando   = false;
            ultimo_paso_cal_ms = ahora_ms;
        }
        if (acciones & SM_ACCION_PANTALLA_OFF) {
            oled_set_power(false);
        }
        if (acciones & SM_ACCION_PANTALLA_ON) {
            oled_set_power(true);
        }

        estado = sm_obtener_estado(&sm);

        /* H. Entrada secuencial no bloqueante a BAJO_CONSUMO */
        if ((estado == SM_ESTADO_BAJO_CONSUMO) && !power_en_bajo_consumo()) {
            if ((wom_state == WOM_IDLE) && !oled_is_busy() && !i2c_drv_is_busy(I2C_MPU_BUS)) {
                g_i2c_cfg_done = false;
                if (mpu_set_wom_mode(true, on_mpu_cfg_done_isr)) {
                    wom_state = WOM_ENABLING;
                }
            } else if ((wom_state == WOM_ENABLING) && g_i2c_cfg_done) {
                g_i2c_cfg_done = false;
                wom_state = WOM_ENABLED;

                if (accel_timer_activo) {
                    cancel_repeating_timer(&accel_timer);
                    accel_timer_activo = false;
                }
                adc_driver_pausar(true);
                power_entrar_bajo_consumo();
            }
        } else if ((estado != SM_ESTADO_BAJO_CONSUMO) && (wom_state == WOM_ENABLED)) {
            wom_state = WOM_IDLE;
        }

        /* I. Refresco asíncrono de pantalla OLED (5 Hz), en su propio bus */
        if ((estado != SM_ESTADO_BAJO_CONSUMO) &&
            !oled_is_busy() &&
            ((ahora_ms - ultimo_refresh_ms) >= OLED_PERIODO_MS)) {

            ultimo_refresh_ms = ahora_ms;
            uint32_t kcal_enteras = modelo_cal_milikcal(&cal) / 1000u;

            if (estado == SM_ESTADO_CONFIG) {
                interfaz_cnfg(kcal_enteras, cal_params.masa_kg,
                              cal_params.estatura_cm, bat_pct, campo_config);
            } else {
                interfaz_sesion(step_count(), (float)estado, cadence(),
                                kcal_enteras, bat_pct);
            }
        }

        /* J. Reporte periódico de diagnóstico y desbordamientos */
        if ((estado != SM_ESTADO_BAJO_CONSUMO) &&
            ((ahora_ms - ultimo_diag_ms) >= DIAG_PERIODO_MS)) {
            ultimo_diag_ms = ahora_ms;
            i2c_drv_stats_t mpu_st;
            i2c_drv_stats_t oled_st;
            i2c_drv_get_stats(I2C_MPU_BUS, &mpu_st);
            i2c_drv_get_stats(I2C_OLED_BUS, &oled_st);

            printf("[DIAG] Est=%s | Pasos=%lu | Bat=%u%% | MPU err=%lu drop=%lu isr=%luus | OLED err=%lu drop=%lu isr=%luus | ADC_ovf=%lu\n",
                   sm_nombre_estado(estado),
                   (unsigned long)step_count(),
                   bat_pct,
                   (unsigned long)mpu_st.error_count,
                   (unsigned long)mpu_st.busy_drop_count,
                   (unsigned long)mpu_st.max_isr_us,
                   (unsigned long)oled_st.error_count,
                   (unsigned long)oled_st.busy_drop_count,
                   (unsigned long)oled_st.max_isr_us,
                   (unsigned long)adc_driver_overflows());
        }

        /* K. Transición atómica a espera WFI sin perder interrupciones */
        if (!hay_trabajo_pendiente()) {
            uint32_t ms_vencimiento = 0u;
            bool tiene_timer_sm = sm_ms_hasta_vencimiento(&sm, ahora_ms, &ms_vencimiento);
            alarm_id_t alarm_id = 0;
            g_alarma_sm = false;

            if (tiene_timer_sm && (ms_vencimiento > 0u)) {
                alarm_id = add_alarm_in_ms(ms_vencimiento, on_wakeup_alarm_isr, NULL, true);
            }

            uint32_t irq_state = save_and_disable_interrupts();
            if (!hay_trabajo_pendiente() && !g_alarma_sm) {
                __wfi();
            }
            restore_interrupts(irq_state);

            if (alarm_id > 0) {
                cancel_alarm(alarm_id);
            }
        }
    }
}