#ifndef MEDICION_H
#define MEDICION_H

#include <stdint.h>
#include <stdbool.h>

// NOTA: Para usar PWM como entrada, DEBEN ser pines impares (Canal B)
// y deben pertenecer a slices diferentes.
// Conecta tu señal físicamente a AMBOS pines.
#define PIN_FREQ_B  3  // Slice 0, Canal B
#define PIN_DUTY_B  7  // Slice 1, Canal B

void medicion_init(void);

// Llama a esto periódicamente (ej. cada 100ms) para procesar los contadores
void medicion_actualizar(void);

// Obtiene los datos procesados
void medicion_obtener_datos(uint64_t* periodo_us, uint64_t* tiempo_alto_us, uint64_t* ultimo_flanco_us);

#endif