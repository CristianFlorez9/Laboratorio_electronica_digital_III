/**
 * @file    step_detector.c
 * @brief   Implementación del detector de pasos con histéresis y confirmación retroactiva.
 */

#include "step_detector.h"
#include "pico/time.h"
#include <math.h>

/* --- Parámetros calibrables del algoritmo (MPU6050 ±2g: 1G = 16384 LSB) --- */
#define STEP_THRESHOLD_HIGH     18800u  /* ~1.15G: disparo de pico de paso */
#define STEP_THRESHOLD_LOW      17200u  /* ~1.05G: histéresis de liberación */
#define STEP_MIN_INTERVAL_MS    280u    /* Máx. ~214 SPM (filtra vibraciones/rebotes) */
#define STEP_MAX_INTERVAL_MS    1500u   /* Mín. ~40 SPM (marcha lenta) */
#define WALKING_TIMEOUT_MS      2000u   /* Tiempo sin pasos para declarar fin de marcha */
#define STEPS_TO_CONFIRM        3u      /* Pasos rítmicos requeridos para confirmar marcha */

static uint32_t step_counter = 0u;
static bool     walking = false;
static uint32_t last_step_time = 0u;
static int      cadence_spm = 0;
static bool     above_threshold = false;

/* Buffer de pasos candidatos antes de confirmar marcha */
static uint8_t  candidate_count = 0u;
static uint32_t candidate_times[STEPS_TO_CONFIRM];

void step_detector_init(void) {
    step_counter    = 0u;
    walking         = false;
    last_step_time  = 0u;
    cadence_spm     = 0;
    above_threshold = false;
    candidate_count = 0u;
}

void step_detector_update(int16_t accel_x, int16_t accel_y, int16_t accel_z) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());

    /* 1. Magnitud vectorial de aceleración (invariante a la orientación en cintura) */
    float fx = (float)accel_x;
    float fy = (float)accel_y;
    float fz = (float)accel_z;
    uint32_t magnitude = (uint32_t)sqrtf((fx * fx) + (fy * fy) + (fz * fz));

    /* 2. Detección de flanco con histéresis */
    bool pico_detectado = false;
    if (!above_threshold && (magnitude > STEP_THRESHOLD_HIGH)) {
        above_threshold = true;
        pico_detectado = true;
    } else if (above_threshold && (magnitude < STEP_THRESHOLD_LOW)) {
        above_threshold = false;
    }

    /* 3. Validación temporal y confirmación rítmica */
    if (pico_detectado) {
        if (walking) {
            uint32_t dt = current_time - last_step_time;
            if (dt >= STEP_MIN_INTERVAL_MS) {
                step_counter++;
                if (dt <= STEP_MAX_INTERVAL_MS) {
                    cadence_spm = (int)(60000u / dt);
                }
                last_step_time = current_time;
            }
        } else {
            if (candidate_count == 0u) {
                candidate_times[0] = current_time;
                candidate_count = 1u;
            } else {
                uint32_t dt = current_time - candidate_times[candidate_count - 1u];

                if (dt > STEP_MAX_INTERVAL_MS) {
                    /* Movimiento aislado previo descartado; iniciar nueva secuencia */
                    candidate_times[0] = current_time;
                    candidate_count = 1u;
                } else if (dt >= STEP_MIN_INTERVAL_MS) {
                    candidate_times[candidate_count] = current_time;
                    candidate_count++;

                    if (candidate_count >= STEPS_TO_CONFIRM) {
                        uint32_t span_ms = current_time - candidate_times[0];
                        walking = true;
                        step_counter += STEPS_TO_CONFIRM;
                        if (span_ms > 0u) {
                            cadence_spm = (int)((60000u * (STEPS_TO_CONFIRM - 1u)) / span_ms);
                        }
                        last_step_time = current_time;
                        candidate_count = 0u;
                    }
                }
            }
        }
    }

    /* 4. Verificar detención de la marcha */
    if (walking && ((current_time - last_step_time) > WALKING_TIMEOUT_MS)) {
        walking = false;
        cadence_spm = 0;
        candidate_count = 0u;
    }
}

uint32_t step_count(void) {
    return step_counter;
}

bool is_walking(void) {
    return walking;
}

int cadence(void) {
    return cadence_spm;
}

uint32_t step_last_time_ms(void) {
    return last_step_time;
}