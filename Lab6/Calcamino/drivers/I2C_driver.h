
/**
 * @file    i2c_driver.h
 * @brief   Driver del bus I2C por interrupciones, dueño único del periférico.
 * @details Gestiona una cola de transacciones (escritura/lectura) y una máquina
 *          de estados de transferencia. Los drivers de sensor y pantalla lo usan
 *          sin tocar registros del I2C.
 * @note    Contexto: las funciones públicas se llaman desde main.
 *          La ISR solo mueve bytes entre el FIFO y el buffer, y notifica el
 *          fin de transferencia mediante callback o flag (ejecución acotada).
 * @warning No usa funciones *_blocking del SDK ni esperas activas.
 * 
 */