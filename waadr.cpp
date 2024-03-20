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
    // initialize LED matrix object? LEXIE!!
};

// DeviceState class functions

DeviceState::DeviceState()
{
    // initialize LED matrix canvas n stuff. :3
}

DeviceState::~DeviceState()
{
    //
}

void DeviceState::updateScreen(int buttonPressed)
{
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
    // int input;
    // wiringPiSetupGpio();
    // DeviceState waadr;

    // Magick
    Magick::InitializeMagick(NULL);

    // Initialize the RGB matrix with
    RGBMatrix::Options matrix_options;
    rgb_matrix::RuntimeOptions runtime_opt;

    // if (!rgb_matrix::ParseOptionsFromFlags(&argc, &argv, &matrix_options, &runtime_opt))
    // {
    //     return usage(argv[0]);
    // }

    // if (argc != 2)
    //     return usage(argv[0]);

    RGBMatrix::Options defaults;
    defaults.hardware_mapping = "regular"; // or e.g. "adafruit-hat"
    defaults.rows = 32;
    defaults.chain_length = 1;
    defaults.parallel = 1;
    defaults.show_refresh_rate = true;

    const char *filename = "sigils/6.ppm"; // Random sigil for testing loading

    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    RGBMatrix *matrix = RGBMatrix::CreateFromOptions(matrix_options, runtime_opt);
    if (matrix == NULL)
        return 1;

    //RGBMatrix *matrix = RGBMatrix::CreateFromFlags(matrix_options, runtime_opt);
    //if (matrix == NULL)
    //    return 1;

    ImageVector images = LoadImage(filename, matrix->width(), matrix->height());
    if (images.size() == 0)
    {
        fprintf(stderr, "Failed to Load image.\n");
        return 1;
    }

    CopyImageToCanvas(images[0], matrix);

    while (!interrupt_received) // Look until ctrl-c pressed
        sleep(1000);

    matrix->Clear();
    delete matrix;

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