
/**
 * @file    step_detector.h
 * @brief   Algoritmo de detección de marcha y conteo de pasos.
 * @details Entrada: muestras de aceleración. Salida: eventos de paso, estado
 *          caminando/no caminando y cadencia. Distingue marcha lenta, habitual
 *          y rápida de otros movimientos, e incluye los pasos de arranque.
 *          Expone el instante del final de la marcha detectada, que es la
 *          referencia de inactividad.
 * @note    Contexto: solo main (nunca en ISR). Sin dependencias de hardware.
 *
 */