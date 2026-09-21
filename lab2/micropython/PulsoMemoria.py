"""@package PulsoMemoria (main)
@brief Módulo principal que instancia hardware y jugabilidad.

@details
Se ejecuta como una máquina de estados principal. Cada estado representa 
una etapa del juego, controlando sus parámetros y animaciones correspondientes:
- `INICIO`: Reposo y espera de jugador.
- `PRESENTACION`: Muestra la secuencia de LEDs.
- `INPUTS`: Lectura de botones.
- `NIVEL_COMPLETO` / `NIVEL_FALLIDO`: Retroalimentación visual.
- `FIN`: Fin de partida.

Flujo de trabajo:
- Instancia de inicialización
- While True: inicialización del timmer y control de estados,
  verificación de todos los estados para definir que mostrar en los leds

@author Cristian_FLorez, Daniel_Lopez
@date 2026-09-13
"""

import Dispositivos
import Juego
import time

Dispositivos.inicializar_dispositivos()
Juego.inicio_reset()

while True:
    tiempo_actual = time.ticks_ms()
    
    Juego.cambio_estado_tiempo(tiempo_actual)
    
    evento_start = Dispositivos.leer_boton_start(tiempo_actual)
    
    if evento_start == 1:
        if Juego.verificacion_estado() == Juego.INICIO or Juego.verificacion_estado() == Juego.FIN:
            Juego.iniciar_partida(tiempo_actual)
            
    elif evento_start == 2:
        Juego.inicio_reset()
            
    estado = Juego.verificacion_estado()
    
    if estado == Juego.INPUTS:
        boton_juego = Dispositivos.leer_boton_juego()
        if boton_juego != -1:
            Juego.procesar_boton(boton_juego, tiempo_actual)
    
    # Manejo de señales visuales y animaciones según el estado
    if estado == Juego.INICIO:
        Dispositivos.apagar_todos_leds()
        
    elif estado == Juego.PRESENTACION:
        led_activo = Juego.obtener_led_actual_presentacion(tiempo_actual)
        for i in range(4):
            Dispositivos.encender_led_secuencia(i, i == led_activo)
            
    elif estado == Juego.INPUTS:
        porcentaje = Juego.obtener_porcentaje_tiempo(tiempo_actual) / 100.0
        Dispositivos.actualizar_palpito(porcentaje, tiempo_actual)
        
    elif estado == Juego.NIVEL_COMPLETO:
        if Juego.obtener_nivel() >= 9:
            Dispositivos.animacion_victoria_nivel_9(tiempo_actual)
        else:
            Dispositivos.animacion_secuencia_correcta()
            
    elif estado == Juego.NIVEL_FALLIDO:
        Dispositivos.animacion_entrada_incorrecta(tiempo_actual)
        
    elif estado == Juego.FIN:
        if Juego.obtener_nivel() >= 9 and Juego.obtener_vidas() > 0:
            Dispositivos.animacion_victoria_nivel_9(tiempo_actual)
        else:
            Dispositivos.animacion_game_over(tiempo_actual)
            
    else:
        Dispositivos.apagar_todos_leds()
        
    # Multiplexado de displays (Nivel, Vidas y Tiempo acumulado)
    Dispositivos.mostrar_en_displays(
        Juego.obtener_nivel(), 
        Juego.obtener_vidas(),
        Juego.obtener_tiempo_acumulado(), 
        tiempo_actual
    )