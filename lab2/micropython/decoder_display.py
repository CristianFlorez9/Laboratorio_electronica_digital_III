"""@package decoder_display
@brief Módulo encargado de la inicialización y multiplexación de displays 7 segmentos

@details
Este módulo gestiona la multiplexación de los displays 7 segmentos
activa cada display en un instante de tiempo para visualizar: el nivel,
las vidas, y el tiempo.

@date 2026-09-13
"""

from machine import Pin
import time

# Listas pre-llenadas para guardar los objetos Pin
pines_seg = [None] * 7
pines_EN = [None] * 4

# Auxiliares
digito_activo = 0
aux_tiempo_display = 0

# Matriz lógica para decodificar el número
numeros = [
    [False, False, False, False, False, False, True],  # 0
    [True, False, False, True, True, True, True],      # 1
    [False, False, True, False, False, True, False],   # 2
    [False, False, False, False, True, True, False],   # 3
    [True, False, False, True, True, False, False],    # 4
    [False, True, False, False, True, False, False],   # 5
    [False, True, False, False, False, False, False],  # 6
    [False, False, False, True, True, True, True],     # 7
    [False, False, False, False, False, False, False], # 8
    [False, False, False, False, True, False, False],  # 9
]

def apagar_todo_seguro():
    """
    @brief Apaga los diplays 7 segmentos antes de la multiplexación
    
    @details Desactiva pines de Enable y pines de segmento.
    """
    for pin in pines_EN:
        if pin: pin.value(1)
    for pin in pines_seg:
        if pin: pin.value(1)

def configurar_display(pines_segmento, pines_digitos):
    """
    @brief Inicialización de los pines usados por los displays.
    
    @details Configura los pines como salida.
    
    @param pines_segmento ID de los segmentos (0 a 6).
    @param pines_digitos ID de los Enables (0 a 3).
    """
    global pines_seg, pines_EN
    for i in range(7):
        pines_seg[i] = Pin(pines_segmento[i], Pin.OUT)
    for i in range(4):
        pines_EN[i] = Pin(pines_digitos[i], Pin.OUT)
    
    apagar_todo_seguro()

def mostrar_numero(numero):
    """
    @brief Decodifica el número a mostrar.
    
    @details Busca el número en la matriz booleana para determinar que segmentos activar
    
    @param numero (INT) número que se desea mostrar.
    """
    num = int(max(0, min(numero, 9)))
    for i, pin in enumerate(pines_seg):
        pin.value(numeros[num][i])
        
def actualizar_display(nivel, vidas, tiempo, tiempo_actual):
    """
    @brief Multiplexación de los displays.
    
    @details Se encarga de llamar las funciones de apagar los segmentos y mostrar numero
    segun el, pasa el numeor recibido a su equivalente en la matriz booleana para posteriormente
    mostrarlo en los diplays, activando unicamente el segmento dispuesto para este.
    
    @param nivel nivel actual (0 a 9).
    @param vidas vidas restantes (1 a 3).
    @param tiempo tiempo acumulado (0 - 99)
    @param tiempo_actual tiempo actual del sistema
    """
    ## @biref variables auxiliares que controlan la multiplexación, indican que display se activa
    global aux_tiempo_display, digito_activo
    
    if (tiempo_actual - aux_tiempo_display >= 3):
        aux_tiempo_display = tiempo_actual
        
        apagar_todo_seguro()
        
        numero_a_mostrar = 0
        
        if (digito_activo == 0):
            numero_a_mostrar = tiempo % 10   # Unidades
        elif (digito_activo == 1):
            numero_a_mostrar = tiempo // 10  # Decenas
        elif (digito_activo == 2):
            numero_a_mostrar = vidas
        else:
            numero_a_mostrar = nivel
    
        mostrar_numero(numero_a_mostrar)
        pines_EN[digito_activo].value(0)
        
        digito_activo = (digito_activo + 1) % 4