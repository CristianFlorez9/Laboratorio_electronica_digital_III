/**
 * @file    modelo_bateria.h
 * @brief   Modelo aproximado de capacidad: voltaje (mV) a porcentaje restante.
 * @details Curva de descarga de la química elegida, con interpolación y
 *          suavizado. Documenta supuestos y limitaciones (carga, temperatura,
 *          envejecimiento).
 * @note    Funciones puras, verificables en PC con valores conocidos.
 *
 */