#include <iostream>
#include <chrono>
#include <thread>
#include <stdio.h>
#include <wiringPi.h>
#include "led-matrix.h"
#include <Magick++.h>
#include <magick/image.h>
#include <math.h>
#include <signal.h>
#include <unistd.h>
#include <exception>
#include <utility>
#include <string.h>

using rgb_matrix::Canvas;
using rgb_matrix::RGBMatrix;

// Global definitions ------------------------

const int buttonPins[4] = {12, 16, 26, 21}; // ADJUST WITH BUTTON PINS (BCM)

volatile bool interrupt_received = false;
static void InterruptHandler(int signo)
{
    interrupt_received = true;
}

// Supporting image function definitions ------------------------

using ImageVector = std::vector<Magick::Image>;
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


// DeviceState class definition ------------------------

class DeviceState
{
public:
    DeviceState();
    ~DeviceState();
    void updateScreen(int);//updates screen given button pressed as a parameter

private:
    int currState; //currState is 0 if on main menu, 1 if displaying sigil, 2 if pinging, 3 if listening
    int currSel;
    int optionPos[8][2] = {{0,0},{0,4},{0,8},{0,12},{21,0},{21,4},{21,8},{21,12}};
    int optionColor[3] = {0,255,0};
    int selColor[3] = {255,255,255};
    int optionSize[1] = {4,10};
    void drawSigil(std::string);//loads a particular sigil when given the path as a parameter
    void drawMenu();
    void drawBox(int pos[], int size[], int color[]);
    RGBMatrix *matrix;
};

// DeviceState class functions

DeviceState::DeviceState()
{   
    // Initialize the RGB matrix
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

    //set default state of the device upon startup
    currState = 0;
    currSel = 0;
    drawMenu();
        
}

DeviceState::~DeviceState()
{
    //Clear and destroy matrix
    matrix->Clear();
    delete matrix;
}

void DeviceState::updateScreen(int buttonPressed)
{
    //buttonPressed: 0 = up, 1 = down, 2 = select, 3 = ping enable/disable
    if(currState == 0){// Main menu logic
        if(buttonPressed == 0){
            if(currSel < 7){
                currSel++;
                drawMenu();
            }
        } else if (buttonPressed == 1){
            if(currSel>0){
                currSel--;
                drawMenu();
            }

        } else if(buttonPressed == 2){
            //Sigil option selected
            
            std::string fullPath = "sigils/" + std::to_string((currSel+1)) + ".ppm";
            drawSigil(fullPath);
            currState = 1;
        } else if(buttonPressed == 3){
            
        }
    } else if(currState == 1){//Sigil display logic
        //If a sigil is being displayed, return to main menu
        drawMenu();
        currState = 0;
    } else if(currState == 2){

    } else if(currState == 3){

    }

}

void DeviceState::drawMenu(){
    //draw main menu here
    //border or highlight the element corresponding to the currSel
    matrix->Clear();
    
    for(int i = 0; i<8; i++){
        if(i == currSel){
            drawBox(optionPos[i],optionSize,selColor);
        } else{
            drawBox(optionPos[i],optionSize,optionColor);
        }
    }

}

void DeviceState::drawBox(int pos[], int size[], int color[]){
    for(int i = 0; i<size[0]; i++){
        for(int j = 0; j<size[1]; j++){
            matrix->SetPixel(pos[0] + i,pos[1] + j,color[0],color[1],color[2]);
        }
    }
    
}

void DeviceState::drawSigil(std::string path)
{
    const char *filename = path.data(); // Random sigil for testing loading

    matrix->Clear();

    ImageVector images = LoadImage(filename, matrix->width(), matrix->height());
    if (images.size() == 0)
    {
        fprintf(stderr, "Failed to Load image.\n");
        exit(2);
    }

    CopyImageToCanvas(images[0], matrix);
}


// Main function definition ------------------------

int main(int argc, char *argv[])
{
    wiringPiSetupGpio();

    for(int i = 0; i<4; i++){
        pinMode(buttonPins[i], INPUT);
        pullUpDnControl(buttonPins[i], PUD_DOWN);
    }

    Magick::InitializeMagick(NULL);

    DeviceState waadr;
    int input, prevInput;
    input = -1;
    prevInput = -2;

    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);
    
    //Note for vina for later: FOR LOOP BELOW, consider the possibility of a button being held. do not repeat input in that case.
    while (!interrupt_received)
    {
        for (int i = 0; i < 4; i++)
        {
            if (digitalRead(buttonPins[i]) == HIGH)
            {
                input = i;
                break;
            }
            input = -1;
        }

        if (input >= 0 && input != prevInput)
        {
            waadr.updateScreen(input);
            prevInput = input;
        } else if(input >= 0 && prevInput == input){
            
        } else{
            prevInput = -2;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return 0;
}