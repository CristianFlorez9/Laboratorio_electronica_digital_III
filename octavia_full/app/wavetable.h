#ifndef WAVETABLE_H_
#define WAVETABLE_H_

#include <stdint.h>

#define TABLE_SIZE 256

// Declaramos que este arreglo existe en otro archivo (.c)
// Al ser "const", va directo a la memoria Flash
extern const uint8_t tabla_onda[TABLE_SIZE];

#endif /* WAVETABLE_H_ */