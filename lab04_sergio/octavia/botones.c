#include "botones.h"
#include "hardware/gpio.h"


void botones_init(void) {
    gpio_init(BOTON_0);
    gpio_init(BOTON_1);
    gpio_init(BOTON_2);
    gpio_init(BOTON_3);
    gpio_init(BOTON_4);

    gpio_set_dir_masked(ALL_BOTONES_MASK, 0);

    gpio_pull_up(BOTON_0);
    gpio_pull_up(BOTON_1);
    gpio_pull_up(BOTON_2);
    gpio_pull_up(BOTON_3);
    gpio_pull_up(BOTON_4);
}

bool boton_read(uint32_t boton_pin) {

    return gpio_get(boton_pin);
}

uint32_t botones_read_all_masked(void) {



    uint32_t todos_los_gpios = gpio_get_all();


    return (todos_los_gpios & ALL_BOTONES_MASK);
}