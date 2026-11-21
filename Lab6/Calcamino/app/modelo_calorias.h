
/**
 * @file modelo_calorias.h
 * @brief Modelo de estimación del gasto calórico al caminar.
 *
 * Calcula el gasto calórico aproximado durante la actividad física
 * utilizando la masa corporal, la estatura, la cadencia de pasos
 * y la duración de la actividad.
 *
 * Funcionamiento:
 * 1. Se estima la longitud del paso a partir de la estatura.
 * 2. Se calcula la velocidad de marcha utilizando la cadencia.
 * 3. Se estima el consumo de oxígeno mediante la ecuación de
 *    caminata del ACSM para terreno plano.
 * 4. Se convierte el consumo de oxígeno en gasto calórico.
 *
 * Ecuaciones utilizadas:
 *
 * Longitud del paso:
 *   longitud = 0.413 * estatura [m]
 *
 * Velocidad de marcha:
 *   velocidad = cadencia * longitud del paso
 *
 * Consumo de oxígeno:
 *   Gasto total:      VO2 = 0.1 * velocidad + 3.5
 *   Gasto adicional:  VO2 = 0.1 * velocidad
 *
 * Gasto calórico:
 *   kcal/min = VO2 * masa / 1000 * 5
 *
 * El modelo permite calcular el gasto total durante la caminata,
 * incluyendo el consumo en reposo, o únicamente el gasto adicional
 * producido por la actividad física.
 *
 * Rango de aplicación:
 * - Velocidad entre 50 y 100 m/min.
 * - Caminata sobre terreno plano.
 * - Valores orientados a población adulta.
 *
 * Consideraciones:
 * - La longitud del paso es una aproximación basada en la estatura.
 * - No se consideran pendientes ni cargas adicionales.
 * - No se tienen en cuenta diferencias individuales de eficiencia
 *   física, edad, sexo o composición corporal.
 * - Los resultados son estimaciones y no mediciones fisiológicas.
 * - Las velocidades fuera del rango establecido también se calculan,
 *   pero se contabilizan como intervalos fuera de rango.
 *
 * Fuentes de referencia:
 * - ACSM, Guidelines for Exercise Testing and Prescription.
 * - Factor antropométrico aproximado de 0.413 para estimar
 *   la longitud del paso a partir de la estatura.
 */

#ifndef MODELO_CALORIAS_H
#define MODELO_CALORIAS_H

#include <stdint.h>
#include <stdbool.h>

/* Constantes utilizadas en los cálculos del modelo. */

#define MODELO_CAL_FACTOR_PASO          0.413f
#define MODELO_CAL_COSTO_DESPLAZ        0.1f
#define MODELO_CAL_VO2_REPOSO           3.5f
#define MODELO_CAL_KCAL_POR_LITRO_O2    5.0f

/* Límites de velocidad para la aplicación del modelo. */

#define MODELO_CAL_V_MIN_M_MIN          50.0f
#define MODELO_CAL_V_MAX_M_MIN          100.0f

/* Límites aceptados para los parámetros personales. */

#define MODELO_CAL_MASA_MIN_KG          20u
#define MODELO_CAL_MASA_MAX_KG          250u
#define MODELO_CAL_ESTATURA_MIN_CM      100u
#define MODELO_CAL_ESTATURA_MAX_CM      230u


/**
 * Define el tipo de gasto calórico que realizará el modelo.
 */
typedef enum
{
    MODELO_CAL_TOTAL = 0,   /**< Incluye el consumo de reposo. */
    MODELO_CAL_ADICIONAL    /**< Considera únicamente el gasto de la actividad. */
} modelo_cal_tipo_t;


/**
 * Almacena los datos personales necesarios para los cálculos.
 */
typedef struct
{
    uint16_t          masa_kg;      /**< Masa corporal en kilogramos. */
    uint16_t          estatura_cm;  /**< Estatura en centímetros. */
    modelo_cal_tipo_t tipo;         /**< Tipo de gasto calórico. */
} modelo_cal_params_t;


/**
 * Almacena la información y los resultados de una sesión.
 *
 * Incluye los parámetros personales, el gasto calórico acumulado,
 * la fracción pendiente de acumulación y los intervalos que
 * estuvieron fuera del rango de velocidad establecido.
 */
typedef struct
{
    modelo_cal_params_t params;           /**< Parámetros de la sesión. */
    uint32_t            acum_milikcal;    /**< Gasto acumulado en milicalorías. */
    float               resto_milikcal;   /**< Fracción pendiente de acumular. */
    uint32_t            fuera_de_rango;   /**< Intervalos fuera del rango permitido. */
} modelo_cal_t;


/**
 * Estima la longitud del paso a partir de la estatura.
 *
 * Retorna la longitud del paso en metros.
 */
float modelo_cal_longitud_paso_m(uint16_t estatura_cm);


/**
 * Calcula la velocidad de marcha utilizando la cadencia
 * y la longitud del paso.
 *
 * Retorna la velocidad en metros por minuto.
 */
float modelo_cal_velocidad_m_min(float cadencia_spm, float long_paso_m);


/**
 * Comprueba si la velocidad de marcha está dentro
 * del rango de aplicación del modelo.
 */
bool modelo_cal_en_rango(float v_m_min);


/**
 * Calcula el gasto calórico por minuto a partir de la velocidad
 * de marcha y los parámetros personales.
 *
 * El resultado se expresa en kcal por minuto.
 */
float modelo_cal_kcal_por_min(const modelo_cal_params_t *params, float v_m_min);


/**
 * Comprueba que los parámetros personales estén dentro
 * de los límites establecidos para el modelo.
 */
bool modelo_cal_params_validos(const modelo_cal_params_t *params);


/**
 * Inicializa una nueva sesión de cálculo calórico.
 *
 * Almacena los parámetros personales y reinicia los acumuladores.
 *
 * Retorna true si la inicialización fue correcta.
 */
bool modelo_cal_init(modelo_cal_t *m, const modelo_cal_params_t *params);


/**
 * Agrega un intervalo de caminata al gasto calórico acumulado.
 *
 * Utiliza la cadencia y la duración del intervalo para calcular
 * las calorías consumidas.
 *
 * También registra los intervalos cuya velocidad está fuera
 * del rango de aplicación del modelo.
 *
 * Retorna true si el intervalo está dentro del rango permitido.
 */
bool modelo_cal_agregar(modelo_cal_t *m, float cadencia_spm, uint32_t duracion_ms);


/**
 * Obtiene el gasto calórico acumulado durante la sesión.
 *
 * Retorna el resultado en milicalorías.
 */
uint32_t modelo_cal_milikcal(const modelo_cal_t *m);


/**
 * Obtiene la cantidad de intervalos registrados fuera
 * del rango de velocidad establecido.
 */
uint32_t modelo_cal_fuera_de_rango(const modelo_cal_t *m);

#endif /* MODELO_CALORIAS_H */
