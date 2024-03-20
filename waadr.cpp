#include <iostream>
#include <chrono>
#include <thread>
#include <stdio.h>
#include <wiringPi.h>
#include "led-matrix.h"
#include <Magick++.h>
#include <magick/image.h>
// additional libraries - tbd
#include <math.h>
#include <signal.h>
#include <unistd.h>
#include <exception>

const int buttonPins[4] = {1, 2, 3, 4}; // ADJUST WITH BUTTON PINS (BCM)

using rgb_matrix::Canvas;
using rgb_matrix::RGBMatrix;

// using rgb_matrix::FrameCanvas; // For scrolling images

// DeviceState class definition ------------------------

class DeviceState
{
public:
    DeviceState();
    ~DeviceState();
    void updateScreen(int);

private:
    int currentState;
    int sigilState;
    bool pingState;
    void loadSigil();
    void pingOut();
    RGBMatrix *matrix;
};

// DeviceState class functions

DeviceState::DeviceState()
{
    // Initialize the RGB matrix with
    rgb_matrix::RuntimeOptions runtime_opt;
    RGBMatrix::Options defaults;
    defaults.hardware_mapping = "regular"; // or e.g. "adafruit-hat"
    defaults.rows = 16;
    defaults.chain_length = 1;
    defaults.parallel = 1;
    matrix = RGBMatrix::CreateFromOptions(defaults, runtime_opt);
    if (matrix == NULL){
        perror("Error creating matrix object");
        exit(1);
    }
        
}

DeviceState::~DeviceState()
{
    //Clear and destroy matrix
    matrix->Clear();
    delete matrix;
}

void DeviceState::updateScreen(int buttonPressed)
{
    const char *filename = "sigils/5.ppm"; // Random sigil for testing loading

    ImageVector images = LoadImage(filename, matrix->width(), matrix->height());
    if (images.size() == 0)
    {
        fprintf(stderr, "Failed to Load image.\n");
        return 1;
    }

    CopyImageToCanvas(images[0], matrix);
}

void DeviceState::loadSigil()
{
}

void DeviceState::pingOut()
{
}

// Load sigil test ------------------------

volatile bool interrupt_received = false;
static void InterruptHandler(int signo)
{
    interrupt_received = true;
}
using ImageVector = std::vector<Magick::Image>;
// load the image and scale to size of matrix
static ImageVector LoadImage(const char *filename, int width, int height)
{
    ImageVector result;
    ImageVector frames;
    try
    {
        readImages(&frames, filename);
    }
    catch (std::exception &e)
    {
        if (e.what())
        {
            fprintf(stderr, "%s\n", e.what());
        }
        return result;
    }

    if (frames.empty())
    {
        fprintf(stderr, "No Image found");
        return result;
    }

    result.push_back(frames[0]);

    for (Magick::Image &image : result)
    {
        image.scale(Magick::Geometry(width, height));
    }

    return result;
}

void CopyImageToCanvas(const Magick::Image &image, Canvas *canvas)
{
    const int offset_x = 0, offset_y = 0;
    for (size_t y = 0; y < image.rows(); ++y)
    {
        for (size_t x = 0; x < image.columns(); ++x)
        {
            const Magick::Color &c = image.pixelColor(x, y);
            if (c.alphaQuantum() < 256)
            {
                canvas->SetPixel(x + offset_x, y + offset_y, ScaleQuantumToChar(c.redQuantum()), ScaleQuantumToChar(c.greenQuantum()),
                                 ScaleQuantumToChar(c.blueQuantum()));
            }
        }
    }
}

// ------------------------

// Main function definition ------------------------

int main(int argc, char *argv[])
{
    wiringPiSetupGpio();
    Magick::InitializeMagick(NULL);

    DeviceState waadr;
    int input;

    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    //RGBMatrix *matrix = RGBMatrix::CreateFromOptions(defaults, runtime_opt);
    

    //RGBMatrix *matrix = RGBMatrix::CreateFromFlags(matrix_options, runtime_opt);
    //if (matrix == NULL)
    //    return 1;

    waadr.updateScreen(1);

    while (!interrupt_received) // Look until ctrl-c pressed
        sleep(1000);

    
    delete waadr;
    /*
    initialize EVERYTHING: DeviceState object, screen, etc. (consider initializing screen as part of DeviceState object, in the constructor)
    while(true) loop
        checks button state at beginning of loop, maybe use a switch or if-else conditional
        runs updateScreen(buttonPressed) repeatedly. depending on the buttonPressed, updateScreen accordingly
        sleep_for a certain amount of milliseconds to prevent burning out processor
    */

    // while (true)
    // {
    //     input = -1;
    //     for (int i = 0; i < 4; i++)
    //     {
    //         if (digitalRead(buttonPins[i]) == HIGH)
    //         {
    //             input = i;
    //             break;
    //         }
    //     }

    //     if (i >= 0)
    //     {
    //         waadr.updateScreen(i);
    //     }

    //     std::this_thread::sleep_for(std::chrono::milliseconds(10));
    // }

    return 0;
}