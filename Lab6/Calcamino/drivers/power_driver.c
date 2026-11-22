/**
 * @file    power_driver.c
 * @brief   Implementación de gestión energética para RP2040.
 */

#include "power_driver.h"
#include "pico/time.h"
#include "hardware/sync.h"
#include "hardware/clocks.h"
#include "hardware/pll.h"
#include "hardware/regs/clocks.h"
#include "hardware/structs/clocks.h"

/* Periféricos del RP2040 no utilizados en CalCamino (apagados durante WFI).
 * I2C0 (OLED) e I2C1 (MPU6050) se mantienen activos en la espera normal. */
#define UNUSED_SLEEP_EN0_MASK ( \
    CLOCKS_SLEEP_EN0_CLK_SYS_DMA_BITS   | \
    CLOCKS_SLEEP_EN0_CLK_SYS_JTAG_BITS  | \
    CLOCKS_SLEEP_EN0_CLK_SYS_PIO0_BITS  | \
    CLOCKS_SLEEP_EN0_CLK_SYS_PIO1_BITS  | \
    CLOCKS_SLEEP_EN0_CLK_SYS_PWM_BITS   | \
    CLOCKS_SLEEP_EN0_CLK_RTC_RTC_BITS   | \
    CLOCKS_SLEEP_EN0_CLK_SYS_RTC_BITS   | \
    CLOCKS_SLEEP_EN0_CLK_PERI_SPI0_BITS | \
    CLOCKS_SLEEP_EN0_CLK_SYS_SPI0_BITS  | \
    CLOCKS_SLEEP_EN0_CLK_PERI_SPI1_BITS | \
    CLOCKS_SLEEP_EN0_CLK_SYS_SPI1_BITS )

#define UNUSED_SLEEP_EN1_MASK ( \
    CLOCKS_SLEEP_EN1_CLK_PERI_UART0_BITS | \
    CLOCKS_SLEEP_EN1_CLK_SYS_UART0_BITS  | \
    CLOCKS_SLEEP_EN1_CLK_PERI_UART1_BITS | \
    CLOCKS_SLEEP_EN1_CLK_SYS_UART1_BITS )

/* Periféricos adicionales que se apagan únicamente durante BAJO_CONSUMO */
#define DEEP_SLEEP_EN0_MASK ( \
    UNUSED_SLEEP_EN0_MASK                  | \
    CLOCKS_SLEEP_EN0_CLK_ADC_ADC_BITS      | \
    CLOCKS_SLEEP_EN0_CLK_SYS_ADC_BITS      | \
    CLOCKS_SLEEP_EN0_CLK_SYS_I2C0_BITS     | \
    CLOCKS_SLEEP_EN0_CLK_SYS_I2C1_BITS     | \
    CLOCKS_SLEEP_EN0_CLK_SYS_PLL_SYS_BITS  | \
    CLOCKS_SLEEP_EN0_CLK_SYS_PLL_USB_BITS )

#define DEEP_SLEEP_EN1_MASK ( \
    UNUSED_SLEEP_EN1_MASK                  | \
    CLOCKS_SLEEP_EN1_CLK_USB_USBCTRL_BITS  | \
    CLOCKS_SLEEP_EN1_CLK_SYS_USBCTRL_BITS )

static bool g_en_bajo_consumo = false;
static volatile bool g_alarma_disparada = false;

static int64_t on_wakeup_alarm(alarm_id_t id, void *user_data) {
    (void)id;
    (void)user_data;
    g_alarma_disparada = true;
    return 0; /* Alarma de un solo disparo */
}

void power_init(void) {
    /* Mantener buses internos, SRAM, Timer, IO, I2C0, I2C1 y ADC activos en WFI normal */
    clocks_hw->sleep_en0 = 0xFFFFFFFFu & ~UNUSED_SLEEP_EN0_MASK;
    clocks_hw->sleep_en1 = 0xFFFFFFFFu & ~UNUSED_SLEEP_EN1_MASK;

    g_en_bajo_consumo = false;
}

void power_esperar_evento(volatile const bool *hay_trabajo, uint32_t timeout_ms) {
    alarm_id_t alarm_id = 0;
    g_alarma_disparada = false;

    if (timeout_ms > 0u) {
        alarm_id = add_alarm_in_ms(timeout_ms, on_wakeup_alarm, NULL, true);
    }

    /* Secuencia sin carrera: deshabilitar IRQs, verificar condición, ejecutar WFI */
    uint32_t irq_state = save_and_disable_interrupts();
    if (!(*hay_trabajo) && !g_alarma_disparada) {
        __wfi();
    }
    restore_interrupts(irq_state);

    if (alarm_id > 0) {
        cancel_alarm(alarm_id);
    }
}

void power_entrar_bajo_consumo(void) {
    if (g_en_bajo_consumo) {
        return;
    }

    /* 1. Pasar clk_sys y clk_peri al cristal externo XOSC (12 MHz) */
    clock_configure(clk_sys,
                    CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX,
                    CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_XOSC_CLKSRC,
                    12u * MHZ,
                    12u * MHZ);

    clock_configure(clk_peri,
                    0u,
                    CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_XOSC_CLKSRC,
                    12u * MHZ,
                    12u * MHZ);

    /* 2. Apagar PLLs principales para reducir corriente estática */
    pll_deinit(pll_sys);
    pll_deinit(pll_usb);

    /* 3. En WFI apagar también I2C0, I2C1, ADC y USB, dejando TIMER e IO activos */
    clocks_hw->sleep_en0 = 0xFFFFFFFFu & ~DEEP_SLEEP_EN0_MASK;
    clocks_hw->sleep_en1 = 0xFFFFFFFFu & ~DEEP_SLEEP_EN1_MASK;

    g_en_bajo_consumo = true;
}

void power_salir_bajo_consumo(void) {
    if (!g_en_bajo_consumo) {
        return;
    }

    /* 1. Reactivar PLL_SYS a 125 MHz y PLL_USB a 48 MHz */
    pll_init(pll_sys, 1u, 1500u * MHZ, 6u, 2u);
    pll_init(pll_usb, 1u, 1440u * MHZ, 6u, 5u);

    clock_configure(clk_sys,
                    CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX,
                    CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS,
                    125u * MHZ,
                    125u * MHZ);

    clock_configure(clk_peri,
                    0u,
                    CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS,
                    125u * MHZ,
                    125u * MHZ);

    clock_configure(clk_usb,
                    0u,
                    CLOCKS_CLK_USB_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB,
                    48u * MHZ,
                    48u * MHZ);

    /* 2. Restaurar máscaras de reloj de I2C0, I2C1 y ADC */
    power_init();
}

bool power_en_bajo_consumo(void) {
    return g_en_bajo_consumo;
}