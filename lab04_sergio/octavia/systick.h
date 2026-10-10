#ifndef SYSTICK_H_
#define SYSTICK_H_

#include <stdint.h>
#include <stdbool.h>



void systick_init(uint32_t reload_value);

void systick_set_state(bool enable);

/*
SYST_CSR (Control and Status Register): Permite encender el contador, seleccionar la fuente de reloj (reloj del procesador) y habilitar la generación de la excepción/interrupción (TICKINT).

SYST_RVR (Reload Value Register): Aquí escribirás el valor matemático de 24 bits que define la frecuencia de muestreo de tus ondas.

SYST_CVR (Current Value Register): Se utiliza para limpiar el contador antes de iniciar.

*/



#endif