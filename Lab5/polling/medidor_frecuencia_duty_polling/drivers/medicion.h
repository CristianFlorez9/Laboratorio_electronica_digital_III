#ifndef MEDICION_H
#define MEDICION_H

#include <stdint.h>
#include <stdbool.h>

#define SIGNAL_PIN 2

void medicion_init(void);

// Esta función debe llamarse constantemente dentro del while(1) del main
void medicion_actualizar(void);

void medicion_obtener_datos(uint64_t* periodo_us, uint64_t* tiempo_alto_us, uint64_t* ultimo_flanco_us);

#endif