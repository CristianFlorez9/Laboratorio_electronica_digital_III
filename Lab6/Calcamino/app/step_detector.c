#include <math.h>
#include <stdint.h>

#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/timer.h" 
#include "step_detector.h"

// Variables estáticas (ocultas a otros archivos)
// Se quita 'volatile' porque esto corre en el main, no en interrupciones.
static uint32_t step_counter = 0;
static bool walking = false;
static uint32_t last_step_time = 0;
static int cadence_rpm = 0;
static bool was_above_threshold = false; // Para detectar el "cruce" del umbral

// --- Parámetros ajustables del algoritmo ---
// 1G = 16384. Un paso típico genera un impacto de ~1.15G a 1.3G.
static const uint32_t STEP_THRESHOLD = 19000; 
// Tiempo mínimo que debe pasar entre paso y paso (4 pasos/seg = 250ms max)
static const uint32_t STEP_DEBOUNCE_MS = 250; 
// Si pasan 2000 ms sin pasos, consideramos que se detuvo
static const uint32_t WALKING_TIMEOUT_MS = 2000; 


void step_detector_init(void) {
    step_counter = 0;
    walking = false;
    last_step_time = 0;
    cadence_rpm = 0;
    was_above_threshold = false;
}

// Inyección de datos desde el main
void step_detector_update(int16_t accel_x, int16_t accel_y, int16_t accel_z) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());

    // Calcular magnitud del vector de aceleración
    // Convertimos a float primero para evitar desbordamiento (overflow) al multiplicar
    float fx = (float)accel_x;
    float fy = (float)accel_y;
    float fz = (float)accel_z;
    float discriminant = fx*fx + fy*fy + fz*fz;
    uint32_t magnitude = (uint32_t)sqrtf(discriminant);

    // 2. Detección de picos con hiséresis/flanco
    bool is_above = (magnitude > STEP_THRESHOLD);
    
    // Si cruzó el umbral hacia arriba y no estábamos ya arriba...
    if (is_above && !was_above_threshold) {
        
        // ...y ha pasado suficiente tiempo desde el último paso (filtro rebote)
        if (current_time - last_step_time > STEP_DEBOUNCE_MS) {
            step_counter++;
            walking = true;
            
            // Calcular cadencia (RPM) si no es el primer paso
            if (last_step_time > 0) {
                uint32_t time_diff = current_time - last_step_time;
                cadence_rpm = 60000 / time_diff; 
            }
            last_step_time = current_time;
        }
    }
    was_above_threshold = is_above; // Guardar estado para la próxima iteración

    // 3. Verificar si el usuario se detuvo
    if (walking && ((current_time - last_step_time) > WALKING_TIMEOUT_MS)) {
        walking = false;
        cadence_rpm = 0; // Si te detienes, la cadencia cae a 0
    }
}

uint32_t step_count(void) {
    return step_counter;
}

bool is_walking(void) {
    return walking;
}

int cadence(void) {
    return cadence_rpm;
}
