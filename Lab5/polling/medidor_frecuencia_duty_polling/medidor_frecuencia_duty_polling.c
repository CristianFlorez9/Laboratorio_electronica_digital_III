#include "medicion.h"
#include "reporte.h"

int main() {
    // Inicialización
    medicion_init();
    reporte_init();

    while (1) {
        // Ejecutar a máxima velocidad para no perder flancos
        medicion_actualizar(); 
        
        // Ejecutar a máxima velocidad (la función internamente 
        // decide si ignora el llamado o transmite porque ya pasaron 100ms)
        reporte_actualizar();  
    }
}