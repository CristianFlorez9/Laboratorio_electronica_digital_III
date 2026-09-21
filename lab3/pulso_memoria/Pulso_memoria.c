#include "pico/stdlib.h"
#define LED_MASK ((1 << 12)|(1 << 13)|(1 << 14)|(1 << 15))

int main(){
    gpio_init_mask(LED_MASK);
    gpio_set_dir_out_masked(LED_MASK);
    int led = 12;

    while(true){
        gpio_set_mask(1 << led);

        sleep_ms(500);

        gpio_clr_mask(1 << led);

        led++;

        if (led > 15){
            led = 12;
        }


    }

    return 0;
}