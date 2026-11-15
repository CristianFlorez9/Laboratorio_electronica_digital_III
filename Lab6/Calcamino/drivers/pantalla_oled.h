
/**
 * @file    pantalla_oled.h
 * @brief   Driver de la pantalla gráfica con framebuffer y envío asíncrono.
 * @details El dibujo se hace en un framebuffer en RAM (contexto main). El envío
 *          al panel se hace por bloques mediante el bus, con callback de fin de
 *          transferencia, sin bloquear la adquisición. Permite apagar y
 *          encender el panel.
 * @note    Si se adapta una biblioteca de terceros, debe verificarse que no
 *          tenga transferencias bloqueantes ni esperas activas.
 * @warning No decide qué se muestra (responsabilidad de ui).
 * 
 */