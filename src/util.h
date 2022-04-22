#ifndef MY_UTILS
#define MY_UTILS

#include <immintrin.h>
#include <stdbool.h>

//Implementation versions
#define INTRIN_V0 0 //optimized SIMD version
#define INTRIN_V1 1 //less optimized SIMD version
#define NAIVE 2

//special value to use instead of iteration number for convergent pixels
#define BLACK 0

//set this global variable true, so that color_pixel works in correctness test mode.
//Which means color_pixel will not write rgb values into image buffer, instead it will write 
//given 'unsigned iterations' argument into global array CORRECTNESS_BUFFER.
//So that we will be able to compare different implementations by computed iteration numbers.
extern bool CORRECTNESS_TEST;
extern unsigned* CORRECTNESS_BUFFER;

//10 different image sizes (for performance comparison and correctness test)
extern const size_t image_sizes[10];

//10 C values which create some interesting shaped julia sets
extern const float complex c_values[10];

typedef struct {
    float complex c;
    float complex start;
    float res;
    unsigned n;
    float radius_sqr; //r^2, helper variable (escape radius squared)
} Arguments;

typedef struct {
    size_t width;
    size_t height;
    unsigned char* buffer;
    float color_const; //equals 255/N. used in map_to_color()
} Image;

/**
 * @return returns a random c value from 10 selected fixed c values
 */
float complex get_random_c();

/**
 * @brief get Arguments struct with given parameters.
 * Escape radius is selected in this function. We set it to radius := max{|c|,2}
 * Proof and correctness of this is in Ausarbeitung.pdf included.
 */
Arguments* get_args(float complex c, float complex start, float res, unsigned n);

/**
 * @brief Get the Image struct with given parameters
 */
Image* get_img(size_t width, size_t height, unsigned char* img, unsigned n);

/**
 * @brief this function takes how many steps it takes for a series 
 * to get out of escape radius and maps it to a value [0,255]
 * bigger the iterations is (closer to convergent), less the mapped rgb value,
 * which means closer to black.
 * 
 * @param iterations how many steps it took to get out of escape radius
 * macro BLACK is used for convergent pixels.
 * 
 * @param color_const equals 255/n (n = max number of iterations)
 * @return unsigned value [0,255]
 */
unsigned char map_to_color(unsigned iterations, float color_const);

/**
 * @brief get offset of a particular pixel in the picture
 * 
 * @param img 
 * @param y 
 * @param x 
 * @return unsigned 
 */
unsigned offset(Image* img, size_t y, size_t x);

/**
 * @brief coloring function. gets the iteration steps number as argument and colors the corresponding pixel
 * When working in CORRECTNESS_TEST mode, writes iteration number into CORRECTNESS_BUFFER and returns.
 * 
 * @param img struct containing info about image
 * @param y coordinate [0,height]
 * @param x coordinate [0,width]
 * @param iterations how many steps it took to get out of escape radius
 */
void color_pixel(Image* img, size_t y, size_t x, unsigned iterations);

//Print functions for debugging
void print_complex(float complex num);

void print_xmm(__m128 reg);

void print_xmm_complex(__m128 reals, __m128 imags);

#endif

