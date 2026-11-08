#ifndef MEDICION_H
#define MEDICION_H

#include <stdint.h>
#include <stdbool.h>


#define SIGNAL_PIN 2

void medicion_init(void);

void medicion_obtener_datos(uint64_t* periodo_us, uint64_t* tiempo_alto_us, uint64_t* ultimo_flanco_us);

#endif