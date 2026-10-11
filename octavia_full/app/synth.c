#include "synth.h"
#include "wavetable.h"
#include "notes.h"
#include "drivers/systick.h"
#include "drivers/dac.h"
#include "hardware/exception.h"

volatile uint32_t acumulador_fase = 0;
volatile uint32_t incremento_fase = 0;
volatile float nivel_envolvente = 0.0f; 

#define F_CPU 133000000.0f
#define F_MUESTREO 40000.0f
#define RELOAD_VALOR 3324

// Declaración anticipada de la ISR
void isr_systick(void);

void synth_init() {
    notes_init(F_MUESTREO);
    dac_init();
    
    // Registro de la interrupción antes de iniciar el timer
    exception_set_exclusive_handler(15, isr_systick);
    
    // Iniciar el timer
    systick_init(RELOAD_VALOR);
}

void isr_systick() {
    acumulador_fase += incremento_fase;
    uint8_t indice = acumulador_fase >> 24;
    
    // Leer la tabla
    uint8_t muestra_cruda = tabla_onda[indice];
    
    // Para atenuar sin perder el centro DC (128):
    // Se resta 128, se escala la amplitud y se vuelve a sumar 128
    int16_t muestra_centrada = (int16_t)muestra_cruda - 128;
    muestra_centrada = (int16_t)(muestra_centrada * nivel_envolvente);
    
    uint8_t muestra_final = (uint8_t)(muestra_centrada + 128);
    
    dac_escribir(muestra_final);
}

void synth_tocar_nota(uint8_t octava, uint8_t nota) {
    incremento_fase = notes_get_incremento(octava, nota);
    acumulador_fase = 0; 
    nivel_envolvente = 1.0f;
}

void synth_liberar_nota() {
    nivel_envolvente = 0.0f; 
}