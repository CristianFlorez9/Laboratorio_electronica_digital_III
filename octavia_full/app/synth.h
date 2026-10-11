#ifndef SYNTH_H_
#define SYNTH_H_

#include <stdint.h> // Solo lo estrictamente necesario aquí

void synth_init(void);
void synth_tocar_nota(uint8_t octava, uint8_t nota);
void synth_liberar_nota(void);

#endif /* SYNTH_H_ */