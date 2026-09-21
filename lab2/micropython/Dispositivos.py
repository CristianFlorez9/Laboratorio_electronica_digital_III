"""@package decoder_display
@brief Declaración de la interfaz de control de hardware, manejo de entradas y animaciones visuales

@details
Este módulo gestiona los perifericos del juego, Leds, botones y display 7 segmentos,
tambien se encarga se generar las secuencia para avisar de estados en el juego, como
Nivel acertado, Nivel errado, Juego completada o Juego terminado.

@date 2026-09-13
"""

import decoder_display
from machine import Pin
import time

## Pines de leds y botones
NUM_PINES_LEDS = [15, 14, 13, 12]
NUM_PINES_BOTONES = [17, 18, 19, 20]
NUM_PIN_LED_PALPITO = 16
NUM_PIN_BOTON_START = 26
## Pines de los displays 7 segmentos
NUM_PINES_SEGMENTOS = [27, 2, 3, 4, 5, 6, 7]
NUM_PINES_ENABLES = [8, 9, 10, 11]

# Objetos Hardware (Se llenarán en inicializar_dispositivos)
leds = []
botones = []
led_palpito = None
boton_start = None

# Configuración de tiempo antirrebote
TIEMPO_ANTIRREBOTE = 50

# Variables para antirrebote del juego
estado_estable_botones = [True, True, True, True]
lectura_anterior_botones = [True, True, True, True]
tiempo_cambio_botones = [0, 0, 0, 0]

# Variables antirrebote del botón start
estado_estable_start = True
lectura_anterior_start = True
tiempo_cambio_start = 0

tiempo_presionado_start = 0
presionado_start = False
evento_2seg_generado = False
ultima_conmutacion_palpito = 0
estado_led_palpito = False

def inicializar_dispositivos():
    """
    @brief Configuración de los pines INPUTS/OUTPUTS.
    
    @details Inicializa los pines para los leds, botones y display 7 segmentos
    leds y display 7 segmentos como salidas y botones como entradas.
    """
    global leds, botones, led_palpito, boton_start
    global estado_estable_botones, lectura_anterior_botones, tiempo_cambio_botones
    global estado_estable_start, lectura_anterior_start, tiempo_cambio_start
    
    # Inicializar LEDs
    for pin in NUM_PINES_LEDS:
        obj_pin = Pin(pin, Pin.OUT)
        obj_pin.value(0)
        leds.append(obj_pin)
        
    # Inicializar Botones
    for pin in NUM_PINES_BOTONES:
        obj_pin = Pin(pin, Pin.IN, Pin.PULL_UP)
        botones.append(obj_pin)
        
    estado_estable_botones = [True] * 4
    lectura_anterior_botones = [True] * 4
    tiempo_cambio_botones = [0] * 4
        
    # Inicializar LED palpito y Start
    led_palpito = Pin(NUM_PIN_LED_PALPITO, Pin.OUT)
    led_palpito.value(0)
    
    boton_start = Pin(NUM_PIN_BOTON_START, Pin.IN, Pin.PULL_UP)
    
    estado_estable_start = True
    lectura_anterior_start = True
    tiempo_cambio_start = 0
    
    decoder_display.configurar_display(NUM_PINES_SEGMENTOS, NUM_PINES_ENABLES)

def leer_boton_juego():
    """
    @brief Controla los botones para adivinar la secuencia.
    
    @details Lee el arreglo de botones, despues de determinar que esta en estado estable,
    retorna la posición del boton presionado.
    
    @return valor del indice correspondiente al boton presionado
    """
    global estado_estable_botones, lectura_anterior_botones, tiempo_cambio_botones
    tiempo_actual = time.ticks_ms()
    
    for i in range(4):
        lectura_actual = botones[i].value()
        
        if lectura_actual != lectura_anterior_botones[i]:
            tiempo_cambio_botones[i] = tiempo_actual
            lectura_anterior_botones[i] = lectura_actual
           
        if time.ticks_diff(tiempo_actual, tiempo_cambio_botones[i]) >= TIEMPO_ANTIRREBOTE:
            if estado_estable_botones[i] != lectura_actual:
                estado_estable_botones[i] = lectura_actual
                
                if estado_estable_botones[i] == 0:
                    return i
    return -1

def leer_boton_start(tiempo_actual):
    """
    @brief Lee y procesa el estado del botón INICIO/RESET con antirrebote.
    
    @details Una vez detecta el botón presionado, determina si es un comando de 
    RESET (botón presionado por 2 segundos o más) o un comando de INICIO 
    (botón presionado por menos de 2 segundos). Imprime en el monitor serial 
    el tiempo exacto que permaneció presionado.
    
    @param tiempo_actual (int) Tiempo actual del sistema en milisegundos para calcular la duración.
    
    @return (int)
        - 1: Evento de INICIO (pulsación corta).
        - 2: Evento de RESET (pulsación larga).
        - 0: No hay ningún evento nuevo.
    """
    global estado_estable_start, lectura_anterior_start, tiempo_cambio_start
    global tiempo_presionado_start, presionado_start, evento_2seg_generado
    
    lectura_actual = boton_start.value()
    
    if lectura_actual != lectura_anterior_start:
        tiempo_cambio_start = tiempo_actual
        lectura_anterior_start = lectura_actual
    
    if time.ticks_diff(tiempo_actual, tiempo_cambio_start) >= TIEMPO_ANTIRREBOTE:
        if estado_estable_start != lectura_actual:
            estado_estable_start = lectura_actual
            
    if estado_estable_start == 0:  # Botón presionado
        if not presionado_start:
            presionado_start = True
            evento_2seg_generado = False
            tiempo_presionado_start = tiempo_actual
        elif not evento_2seg_generado:
            tiempo_sostenido = time.ticks_diff(tiempo_actual, tiempo_presionado_start)
            if tiempo_sostenido >= 2000:
                evento_2seg_generado = True
                print(f"\n[BOTÓN START] Tiempo presionado: {tiempo_sostenido} ms -> Evento: RESET")
                return 2
    else:
        if presionado_start:
            presionado_start = False
            tiempo_sostenido = time.ticks_diff(tiempo_actual, tiempo_presionado_start)
            if not evento_2seg_generado and tiempo_sostenido < 2000:
                print(f"\n[BOTÓN START] Tiempo presionado: {tiempo_sostenido} ms -> Evento: INICIO")
                return 1
                
    return 0

def encender_led_secuencia(indice, estado):
    """
    @brief Secuencia aleatoria.
    
    @details Determina que led enciende según el estado y el indice del momento.
    
    @param indice led a activar (0 a 3).
    @param estado determina si el led se enciende o no.
    """
    if 0 <= indice < 4:
        leds[indice].value(1 if estado else 0)
        
def apagar_todos_leds():
    """
    @brief Apaga los leds antes de mostrar una animación o cuando no hay animación.
    """
    for led in leds:
        led.value(0)
    led_palpito.value(0)
    
def actualizar_palpito(porcentaje_tiempo, tiempo_actual):
    """
    @brief Control del led que plapita.
    
    @details Contorla que tan rápido palpita el led de acuerdo al tiempo restante para
    que el usuario ingrese la secuencia vista.
    
    @param porcentaje_tiempo porcentaje del tiempo restante de respuesta.
    @param tiempo_actual para calcular el timepo de encendido y apagado.
    """
    global ultima_conmutacion_palpito, estado_led_palpito
    
    if porcentaje_tiempo <= 0.0:
        led_palpito.value(0)
        return
        
    if porcentaje_tiempo > 1.0:
        porcentaje_tiempo = 1.0
        
    factor = (1.0 - porcentaje_tiempo)
    intervalo = 40 + int(560.0 * (factor * factor))
    
    if time.ticks_diff(tiempo_actual, ultima_conmutacion_palpito) >= intervalo:
        ultima_conmutacion_palpito = tiempo_actual
        estado_led_palpito = not estado_led_palpito
        led_palpito.value(1 if estado_led_palpito else 0)

def mostrar_en_displays(nivel, vidas, tiempo, tiempo_actual):
    """
    @brief llamado de la función encargada de decodificar el display 7 segmentos
    
    @param nivel nivel actual del juego (0 a 9).
    @param vidas vidas restantes (1 a 3).
    @param timepo tiempo acumulado.
    @param tiempo_actual tiempo actual del juego.
    """
    decoder_display.actualizar_display(nivel, vidas, tiempo, tiempo_actual)

def animacion_entrada_incorrecta(tiempo_actual):
    """
    @brief Secuencia para indicar una entrada incorrecta.
    
    @details 3 parapadeos de todos los leds para indicar un error
    75 ms encendido, 75 ms apagado.
    
    @param tiempo_actual para el calculo de tiempo de encendido y apagado.
    """
    ciclo = (tiempo_actual // 75) % 6
    estado = (ciclo % 2) == 0
    
    for led in leds:
        led.value(1 if estado else 0)

def animacion_secuencia_correcta():
    """
    @brief Secuencia para indicar que ingreso la secuencia mostrada.
    
    @details Enciende los leds de forma continua para indicar que acertó.
    """
    for led in leds:
        led.value(1)

def animacion_tiempo_agotado(tiempo_actual):
    """
    @brief Secuencia para indicar que se agoto el tiempo para introducir la secuencia vista.
    
    @details Parpadea dos veces para indicar que se agoto el tiempo antes de mostrar nuevamente la secuencia a adivinar.
    
    @param tiempo_actual para el cáluclo del tiempo a mostrar la secuencia
    """
    estado = (tiempo_actual // 100) % 2 == 0
    for led in leds:
        led.value(1 if estado else 0)

def animacion_perdida_vida(tiempo_actual):
    """
    @brief Secuencia cuando se pierde una vida.
    
    @details Todos los leds encendidos.
    
    @param timepo_actual tiempo del juego
    """
    for led in leds:
        led.value(1)

def animacion_victoria_nivel_9(tiempo_actual):
    """
    @brief Secuencia que se muestra tras superar el nivel 9 correctamente.
    
    @details Activa los leds uno a uno de manera ascendente y descendente.
    
    @param tiempo_actual para calcular el tiempo que se enciende un led
    """
    secuencia = [0, 1, 2, 3, 2, 1]
    paso = (tiempo_actual // 100) % 6
    
    for i in range(4):
        leds[i].value(1 if i == secuencia[paso] else 0)

def animacion_game_over(tiempo_actual):
    """
    @brief Secuencia cuando se acaban las vidas.
    
    @details Activa los cuatro leds y los apaga de forma descendente uno por uno.
    
    @param tiempo_actual para calcular el tiempo que se enciende cada led
    """
    leds_activos = 4 - ((tiempo_actual // 200) % 5)
    
    for i in range(4):
        leds[i].value(1 if i < leds_activos else 0)