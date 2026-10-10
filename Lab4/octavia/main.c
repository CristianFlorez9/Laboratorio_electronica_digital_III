#include "pico/stdlib.h"
#include "octavia.h"

int main(void)
{
    octavia_iniciar();

    while (true) {
        octavia_ejecutar();
    }
}