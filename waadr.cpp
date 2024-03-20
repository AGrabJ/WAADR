
#include "include/led-matrix.h"
#include "include/pixel-mapper.h"
#include "include/canvas.h"
#include "include/content-streamer.h"
#include "include/thread.h"
#include "include/threaded-canvas-manipulator.h"
#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <algorithm>

using std::max;
using std::min;

#define TERM_ERR "\033[1;31m"
#define TERM_NORM "\033[0m"

using namespace rgb_matrix;

volatile bool interrupt_received = false;

static void InterruptHandler(int signo) { 
  interrupt_received = true; 
}





const char* ReadLine(FILE* file, char* buffer, size_t bufferSize) {
    if (file == nullptr || buffer == nullptr || bufferSize == 0) {
        return nullptr;
    }

    // Read a line from the file
    if (fgets(buffer, static_cast<int>(bufferSize), file) == nullptr) {
        return nullptr;
    }

    // Remove the newline character at the end of the line, if present
    char* newlinePos = strchr(buffer, '\n');
    if (newlinePos != nullptr) {
        *newlinePos = '\0';
    }

    return buffer;
}

void RunSimpleSquare(Canvas *canvas) {
    while (!interrupt_received) {
        const int width = canvas->width() - 1;
        const int height = canvas->height() - 1;
        // Borders
        DrawLine(canvas, 0, 0, width, 0, Color(255, 0, 0));
        DrawLine(canvas, 0, height, width, height, Color(255, 255, 0));
        DrawLine(canvas, 0, 0, 0, height, Color(0, 0, 255));
        DrawLine(canvas, width, 0, width, height, Color(0, 255, 0));

        // Diagonals.
        DrawLine(canvas, 0, 0, width, height, Color(255, 255, 255));
        DrawLine(canvas, 0, height, width, 0, Color(255, 0, 255));

        // Add a delay or other logic as needed
        usleep(100 * 1000);
    }
}


class ImageScroller {
public:
    ImageScroller(RGBMatrix *m, int scroll_jumps, int scroll_ms = 30)
        : scroll_jumps_(scroll_jumps),
          scroll_ms_(scroll_ms),
          horizontal_position_(0),
          matrix_(m) {
        offscreen_ = matrix_->CreateFrameCanvas();
    }

// loads images from file
bool LoadPPM(const char *filename) {
  FILE *f = fopen(filename, "r");
  // check if file exists

  if (f == NULL && access(filename, F_OK) == -1) {
    fprintf(stderr, "File \"%s\" doesn't exist\n", filename);
    return false;
  }
  if (f == NULL)
    return false;
  char header_buf[256];
  const char *line = ReadLine(f, header_buf, sizeof(header_buf));
#define EXIT_WITH_MSG(m)                                                       \
  {                                                                            \
    fprintf(stderr, "%s: %s |%s", filename, m, line);                          \
    fclose(f);                                                                 \
    return false;                                                              \
  }
  if (sscanf(line, "P6 ") == EOF)
    EXIT_WITH_MSG("Can only handle P6 as PPM type.");
  line = ReadLine(f, header_buf, sizeof(header_buf));
  int new_width, new_height;
  if (!line || sscanf(line, "%d %d ", &new_width, &new_height) != 2)
    EXIT_WITH_MSG("Width/height expected");
  int value;
  line = ReadLine(f, header_buf, sizeof(header_buf));
  if (!line || sscanf(line, "%d ", &value) != 1 || value != 255)
    EXIT_WITH_MSG("Only 255 for maxval allowed.");
  const size_t pixel_count = new_width * new_height;
  Pixel *new_image = new Pixel[pixel_count];
  assert(sizeof(Pixel) == 3); // we make that assumption.
  if (fread(new_image, sizeof(Pixel), pixel_count, f) != pixel_count) {
    line = "";
    EXIT_WITH_MSG("Not enough pixels read.");
  }
#undef EXIT_WITH_MSG
  fclose(f);
  fprintf(stderr, "Read image '%s' with %dx%d\n", filename, new_width,
          new_height);
  horizontal_position_ = 0;
  MutexLock l(&mutex_new_image_);
  new_image_.Delete(); // in case we reload faster than is picked up
  new_image_.image = new_image;
  new_image_.width = new_width;
  new_image_.height = new_height;
  return true;
}
void Run() {
  const int screen_height = offscreen_->height();
  const int screen_width = offscreen_->width();
  while (!interrupt_received) {
    {
      MutexLock l(&mutex_new_image_);
      if (new_image_.IsValid()) {
        current_image_.Delete();
        current_image_ = new_image_;
        new_image_.Reset();
      }
    }
    if (!current_image_.IsValid()) {
      usleep(100 * 1000);
      continue;
    }
    for (int x = 0; x < screen_width; ++x) {
      for (int y = 0; y < screen_height; ++y) {
        const Pixel &p = current_image_.getPixel(
            (horizontal_position_ + x) % current_image_.width, y);
        offscreen_->SetPixel(x, y, p.red, p.green, p.blue);
      }
    }
    offscreen_ = matrix_->SwapOnVSync(offscreen_);
    horizontal_position_ += scroll_jumps_;
    if (horizontal_position_ < 0)
      horizontal_position_ = current_image_.width;
    if (scroll_ms_ <= 0) {
      // No scrolling. We don't need the image anymore.
      current_image_.Delete();
    } else {
      usleep(scroll_ms_ * 1000);
    }
  }
}
public:
  struct Pixel {
    Pixel() : red(0), green(0), blue(0){}
    uint8_t red;
    uint8_t green;
    uint8_t blue;   
  };
  struct Image {
    Image() : width(-1), height(-1), image(NULL) {}
    ~Image() { Delete(); }
    void Delete() { delete [] image; Reset(); }
    void Reset() { image = NULL; width = -1; height = -1; }
    inline bool IsValid() { return image && height > 0 && width > 0; }
    const Pixel &getPixel(int x, int y) {
      static Pixel black;
      if (x < 0 || x >= width || y < 0 || y >= height) return black;
      return image[x + width * y];
    }
    
    int width;
    int height;
    Pixel *image;
  };

  // Read line, skip comments.
  char *ReadLine(FILE *f, char *buffer, size_t len) {
    char *result;
    do {
      result = fgets(buffer, len, f);
    } while (result != NULL && result[0] == '#');
    return result;
  }

  // Current image is only manipulated in our thread.
  Image current_image_;

  // New image can be loaded from another thread, then taken over in main thread
  Mutex mutex_new_image_;
  Image new_image_;

  FrameCanvas* offscreen_;
    int horizontal_position_;  // Horizontal position of the scrolling image
    int scroll_jumps_;
    int scroll_ms_;
    RGBMatrix *matrix_;
};







int main(int argc, char *argv[]) {
  int demo = -1;
  int scroll_ms = -1;
 

  //doesn't move when negative scroll_ms

  const char *demo_parameter = NULL;
  RGBMatrix::Options matrix_options;
  rgb_matrix::RuntimeOptions runtime_opt;

  // These are the defaults when no command-line flags are given.
  matrix_options.rows = 16;
  matrix_options.cols = 32;
  matrix_options.chain_length = 1;
  matrix_options.parallel = 1;

  // First things first: extract the command line flags that contain
  // relevant matrix options.

  RGBMatrix *matrix = RGBMatrix::CreateFromOptions(matrix_options, runtime_opt);
  if (matrix == NULL)
    return 1;

  printf("Size: %dx%d. Hardware gpio mapping: %s\n", matrix->width(),
         matrix->height(), matrix_options.hardware_mapping);

  Canvas *canvas = matrix;

  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);

  // The DemoRunner objects are filling
  // the matrix continuously.

  // Set up an interrupt handler to be able to stop animations while they go
  // on. Each demo tests for while (!interrupt_received) {},
  // so they exit as soon as they get a signal.

  RunSimpleSquare(canvas);
  ImageScroller scroller(matrix, 1, 30);
  printf("Press <CTRL-C> to exit and reset LEDs\n");
 
  
   const char *image_filename = "sigils/1.ppm"; // Change this to your image file
    if (!scroller.LoadPPM(image_filename)) {
        fprintf(stderr, "Failed to load image file.\n");
        delete canvas;
        return 1;
    }
    scroller.Run();

    // Directly set the pixels on the canvas with the loaded image data
    for (int x = 0; x < canvas->width(); ++x) {
        for (int y = 0; y < canvas->height(); ++y) {
            const ImageScroller::Pixel &p = scroller.current_image_.getPixel(x, y); // Assuming current_image_ contains the loaded image
            canvas->SetPixel(x, y, p.red, p.green, p.blue);
        }
    }
    // Now, run our particular demo; it will exit when it sees interrupt_received.

    matrix->SwapOnVSync(scroller.offscreen_);

    // Keep the program running until interrupted
    while (!interrupt_received) {
        usleep(1000000); // Sleep for 1 second before checking interruption again
    }
  delete canvas;

  printf("Received CTRL-C. Exiting.\n");
  return 0;
}