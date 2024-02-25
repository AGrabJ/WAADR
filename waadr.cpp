#include <iostream>
#include <chrono>
#include <thread>
#include <stdio.h>
#include <wiringPi.h>
#include "led-matrix.h"

#define PIN_BUTTON 16

int main(){
    wiringPiSetupGpio();
    pinMode(PIN_BUTTON, INPUT);

    std::cout<<"SET UP PIN 16!"<<std::endl;

    while(1){
        if(digitalRead(PIN_BUTTON) == HIGH){
            std::cout<<"PRESSED!"<<std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return 0;
}