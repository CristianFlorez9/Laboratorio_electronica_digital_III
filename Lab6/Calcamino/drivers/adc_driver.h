/**
 * @file    adc_driver.h
 * @brief   Medición continua del voltaje de batería con el ADC.
 * @details Muestreo periódico disparado por temporizador a través de un divisor
 *          resistivo. La ISR guarda la lectura cruda y levanta un flag de
 *          muestra nueva; el promediado se hace en main. Entrega milivoltios.
 * @warning No convierte voltaje a porcentaje (responsabilidad de battery_model).
 * 
 */