/**
 * @file medicion.h
 * @brief Utilidades para medir el tiempo de ejecución (en microsegundos) de las funciones del juego.
 * @details Se mide con time_us_32() (resolución de 1 us; el desbordamiento cada ~71 min no afecta
 *          porque solo se usan diferencias sin signo). Para desactivar TODA la medición basta con
 *          poner #MEDIR_TIEMPOS en 0: los helpers quedan vacíos y los printf de medición desaparecen.
 */

#ifndef MEDICION_H
#define MEDICION_H

#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include "pico/time.h"

/** @brief 1 = medir y reportar por consola; 0 = sin medición. */
#define MEDIR_TIEMPOS 1

/** @brief Acumulador de estadísticas de tiempo para funciones que se llaman muchas veces. */
typedef struct {
  uint32_t n;     /**< Cantidad de llamadas registradas. */
  uint32_t min;   /**< Tiempo mínimo (us). */
  uint32_t max;   /**< Tiempo máximo (us). */
  uint64_t suma;  /**< Suma de tiempos (us), para calcular el promedio. */
} MedEstadistica;

/** @brief Valor inicial de un #MedEstadistica. */
#define MED_ESTADISTICA_INICIAL {0, UINT32_MAX, 0, 0}

#if MEDIR_TIEMPOS

#define MED_PRINTF(...) printf(__VA_ARGS__)

/**
 * @brief Registra el tiempo de una llamada en el acumulador.
 * @param e Acumulador.
 * @param dt_us Duración de la llamada en microsegundos.
 */
static inline void med_registrar(MedEstadistica *e, uint32_t dt_us) {
  e->n++;
  if (dt_us < e->min) e->min = dt_us;
  if (dt_us > e->max) e->max = dt_us;
  e->suma += dt_us;
}

/**
 * @brief Imprime min / promedio / máximo del acumulador y lo reinicia.
 * @param nombre Texto que identifica la función medida.
 * @param e Acumulador (se reinicia después de imprimir).
 */
static inline void med_reportar(const char *nombre, MedEstadistica *e) {
  if (e->n == 0) {
    return;
  }

  printf("[TIEMPO] %s -> %" PRIu32 " llamadas | min: %" PRIu32 " us | prom: %" PRIu32
         " us | max: %" PRIu32 " us\n",
         nombre, e->n, e->min, (uint32_t)(e->suma / e->n), e->max);

  *e = (MedEstadistica)MED_ESTADISTICA_INICIAL;
}

#else

#define MED_PRINTF(...) ((void)0)

static inline void med_registrar(MedEstadistica *e, uint32_t dt_us) { (void)e; (void)dt_us; }
static inline void med_reportar(const char *nombre, MedEstadistica *e) { (void)nombre; (void)e; }

#endif

#endif