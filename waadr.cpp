#include <iostream>
#include <chrono>
#include <thread>
#include <stdio.h>
#include <wiringPi.h>
#include "led-matrix.h"

#define PIN_BUTTON 16

using rgb_matrix::Canvas;
using rgb_matrix::RGBMatrix;

int main()
{
    wiringPiSetupGpio();
    pinMode(PIN_BUTTON, INPUT);

    std::cout << "SET UP PIN 16!" << std::endl;

    while (1)
    {
        if (digitalRead(PIN_BUTTON) == HIGH)
        {
            std::cout << "PRESSED!" << std::endl; // When the button is pressed, the message "Pressed!" will be printed
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return 0;
}