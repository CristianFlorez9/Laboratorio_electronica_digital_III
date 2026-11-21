
/**
 * @file modelo_calorias.c
 * @brief Implementación del modelo de gasto calórico al caminar.
 *
 * Calcula el gasto calórico aproximado a partir de la masa corporal,
 * la estatura, la cadencia de pasos y el tiempo de actividad.
 *
 * Incluye el cálculo de la longitud del paso, la velocidad de marcha,
 * la estimación del consumo de oxígeno y la acumulación de calorías.
 */

#include "modelo_calorias.h"
#include <stddef.h>


/**
 * Calcula la longitud aproximada de un paso a partir de la estatura.
 *
 * La estatura se convierte de centímetros a metros y se multiplica
 * por un factor de longitud definido en el archivo de cabecera.
 */
float modelo_cal_longitud_paso_m(uint16_t estatura_cm)
{
    float estatura_m = (float)estatura_cm / 100.0f;

    return MODELO_CAL_FACTOR_PASO * estatura_m;
}


/**
 * Calcula la velocidad de desplazamiento en metros por minuto.
 *
 * Utiliza la cadencia de pasos y la longitud de cada paso.
 */
float modelo_cal_velocidad_m_min(float cadencia_spm, float long_paso_m)
{
    return cadencia_spm * long_paso_m;
}


/**
 * Comprueba si la velocidad de marcha está dentro del rango
 * establecido para el modelo.
 *
 * Retorna true si la velocidad es válida y false en caso contrario.
 */
bool modelo_cal_en_rango(float v_m_min)
{
    return ((v_m_min >= MODELO_CAL_V_MIN_M_MIN) &&
            (v_m_min <= MODELO_CAL_V_MAX_M_MIN));
}


/**
 * Calcula el gasto calórico por minuto.
 *
 * Estima el consumo de oxígeno a partir de la velocidad de marcha
 * y la masa corporal. También permite incluir el consumo de oxígeno
 * en reposo, dependiendo del tipo de cálculo seleccionado.
 *
 * El resultado se expresa en kcal por minuto.
 */
float modelo_cal_kcal_por_min(const modelo_cal_params_t *params, float v_m_min)
{
    float vo2_ml_kg_min;
    float vo2_ml_min;

    if (params == NULL)
    {
        return 0.0f;
    }

    /* Calcular el consumo de oxígeno durante el desplazamiento. */
    vo2_ml_kg_min = MODELO_CAL_COSTO_DESPLAZ * v_m_min;

    /* Incluir el consumo en reposo cuando se calcula el gasto total. */
    if (params->tipo == MODELO_CAL_TOTAL)
    {
        vo2_ml_kg_min += MODELO_CAL_VO2_REPOSO;
    }

    /* Convertir el consumo de oxígeno a gasto calórico. */
    vo2_ml_min = vo2_ml_kg_min * (float)params->masa_kg;

    return (vo2_ml_min / 1000.0f) * MODELO_CAL_KCAL_POR_LITRO_O2;
}


/**
 * Comprueba que los parámetros del modelo sean válidos.
 *
 * Verifica que la masa corporal y la estatura estén dentro
 * de los límites definidos para realizar los cálculos.
 */
bool modelo_cal_params_validos(const modelo_cal_params_t *params)
{
    if (params == NULL)
    {
        return false;
    }

    if ((params->masa_kg < MODELO_CAL_MASA_MIN_KG) ||
        (params->masa_kg > MODELO_CAL_MASA_MAX_KG))
    {
        return false;
    }

    if ((params->estatura_cm < MODELO_CAL_ESTATURA_MIN_CM) ||
        (params->estatura_cm > MODELO_CAL_ESTATURA_MAX_CM))
    {
        return false;
    }

    return true;
}


/**
 * Inicializa el modelo de gasto calórico.
 *
 * Comprueba los parámetros recibidos y, si son válidos,
 * almacena su configuración y reinicia los acumuladores.
 *
 * Retorna true si la inicialización fue correcta.
 */
bool modelo_cal_init(modelo_cal_t *m, const modelo_cal_params_t *params)
{
    if ((m == NULL) || !modelo_cal_params_validos(params))
    {
        return false;
    }

    m->params         = *params;
    m->acum_milikcal  = 0u;
    m->resto_milikcal = 0.0f;
    m->fuera_de_rango = 0u;

    return true;
}


/**
 * Agrega un intervalo de actividad al cálculo de calorías.
 *
 * Utiliza la cadencia y la duración de la actividad para calcular
 * el gasto calórico correspondiente.
 *
 * El resultado se acumula conservando las fracciones para evitar
 * pérdidas de precisión durante las sumas.
 *
 * También registra los intervalos cuya velocidad está fuera
 * del rango establecido para el modelo.
 *
 * Retorna true si la velocidad está dentro del rango permitido.
 */
bool modelo_cal_agregar(modelo_cal_t *m, float cadencia_spm, uint32_t duracion_ms)
{
    float    long_paso_m;
    float    v_m_min;
    float    kcal_por_min;
    float    incremento;
    uint32_t enteros;
    bool     en_rango;

    if ((m == NULL) || (cadencia_spm <= 0.0f) || (duracion_ms == 0u))
    {
        return false;
    }

    long_paso_m  = modelo_cal_longitud_paso_m(m->params.estatura_cm);
    v_m_min      = modelo_cal_velocidad_m_min(cadencia_spm, long_paso_m);
    kcal_por_min = modelo_cal_kcal_por_min(&m->params, v_m_min);

    /* Calcular las milicalorías correspondientes al intervalo. */
    incremento = kcal_por_min * (float)duracion_ms / 60.0f;

    /* Conservar la parte decimal para evitar pérdidas por truncamiento. */
    m->resto_milikcal += incremento;
    enteros            = (uint32_t)m->resto_milikcal;
    m->acum_milikcal  += enteros;
    m->resto_milikcal -= (float)enteros;

    en_rango = modelo_cal_en_rango(v_m_min);

    if (!en_rango)
    {
        m->fuera_de_rango++;
    }

    return en_rango;
}


/**
 * Obtiene el gasto calórico acumulado.
 *
 * Retorna el resultado en milicalorías.
 */
uint32_t modelo_cal_milikcal(const modelo_cal_t *m)
{
    if (m == NULL)
    {
        return 0u;
    }

    return m->acum_milikcal;
}


/**
 * Obtiene la cantidad de intervalos registrados fuera
 * del rango de velocidad establecido para el modelo.
 */
uint32_t modelo_cal_fuera_de_rango(const modelo_cal_t *m)
{
    if (m == NULL)
    {
        return 0u;
    }

    return m->fuera_de_rango;
}
