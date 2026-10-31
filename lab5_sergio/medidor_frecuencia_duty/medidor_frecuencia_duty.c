#include "pico/stdlib.h"
#include "drivers/UART.h"

int main() {

    uart_driver_init();


    while (true) {
        
        uart_driver_send_string("Hola\r\n");
        sleep_ms(1000);
    }

    return 0;
}