#include <iostream>
#include <stdio.h>
#include <bcm2835>

#define PIN RPI_GPIO_P1_16

int main(){
    if(!bcm2835_init()){
        return 1;
    }

    bcm2835_gpio_fsel(PIN, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_set_pud(PIN, BCM2835_GPIO_PUD_UP);

    while(1){
        uint8_t value = bcm2835_gpio_lev(PIN);
        printf("read from pin 15: %d\n", value);
        // wait a bit
        delay(500);
    }

    bcm2835_close();
    return 0;
}