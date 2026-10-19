#ifndef WAVETABLE_H_
#define WAVETABLE_H_

#include <stdint.h>

#define TABLE_SIZE 256

/**
* @brief Tabla con los valores de un ciclo de la señal sen(x)
* @note Declarado como extern para indicar que esta declarado en otro archivo .c,
* el calificador const indica que esta almacenado en memoria flash.
*/
extern const uint8_t tabla_onda[TABLE_SIZE];

#endif /* WAVETABLE_H_ */
