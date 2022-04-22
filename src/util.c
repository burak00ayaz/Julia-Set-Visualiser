#include <complex.h>  
#include <immintrin.h>
#include <emmintrin.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>

//Implementation versions
#define INTRIN_V0 0 //optimized SIMD version
#define INTRIN_V1 1 //less optimized SIMD version
#define NAIVE 2

//special value to use instead of iteration number for convergent pixels
#define BLACK 0

bool CORRECTNESS_TEST = false;
unsigned* CORRECTNESS_BUFFER = NULL;

const size_t image_sizes[10] = {500, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 4500, 5000};

const float complex c_values[10] = {-0.53 + 0.5 * I, -0.2 + 0.685 * I, 0.33 + 0.058 * I, 0.398 + -0.32 * I,
                                0.23 + -0.525 * I, 0 + -0.64 * I, -1.02 + -0.254 * I, -0.8 + -0.154 * I,
                                -0.745 + 0.03 * I, 0.33 + 0.4 * I};

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

float complex get_random_c() {
    time_t t;
    srand((unsigned) time(&t));
    return c_values[rand() % 10];
}

Arguments* get_args(float complex c, float complex start, float res, unsigned n) {
    Arguments* args = malloc(sizeof(Arguments));

    if (args == NULL) {
        fprintf(stderr, "Could not allocate memory for arguments struct.\n");
        exit(1);
    }
    args->c = c;
    args->start = start;
    args->res = res;
    args->n = n;
    float c_betrag = sqrtf(crealf(c) * crealf(c) + cimagf(c) * cimagf(c));
    float radius = (c_betrag > 2) ? c_betrag : 2; //r = max{|c|, 2}
    args->radius_sqr = radius*radius;
    return args;
}

Image* get_img(size_t width, size_t height, unsigned char* img, unsigned n) {
    Image* my_img = malloc(sizeof(Image));

    if (my_img == NULL) {
        fprintf(stderr, "Could not allocate memory for image struct.\n");
        exit(1);       
    }
    my_img->width = width;
    my_img->height = height;
    my_img->buffer = img;
    my_img->color_const = 255.0f / n;
    return my_img;
}

unsigned char map_to_color(unsigned iterations, float color_const) {
    if (iterations == BLACK) {
        return 0;
    }
    return 255 - (unsigned char)(iterations * color_const);
}

unsigned offset(Image* img, size_t y, size_t x) {
    return (y * img->width * 3) + (x * 3); //3 = bytes per pixel
}

void color_pixel(Image* img, size_t y, size_t x, unsigned iterations) {
    //if correctness test mode is enabled, write iterations number into correctness buffer and return.
    if (CORRECTNESS_TEST) {
        unsigned offset = y * img->width + x;
        CORRECTNESS_BUFFER[offset] = iterations;
        return;
    }
    unsigned char color = map_to_color(iterations, img->color_const);
    unsigned o = offset(img, y, x);

    //black - lila coloring
    img->buffer[o+2] = color >> 1; //red
    img->buffer[o+1] = color >> 2;  //green
    img->buffer[o]   = color;  //blue
}

//Print functions for debugging
void print_complex(float complex num) {
    printf("Zi = %.5f + %.5f i\n", crealf(num), cimagf(num));
}

void print_xmm(__m128 reg) {
    for (int i = 0; i < 4; i++) {
        printf("float %d: %f\n", i + 1, reg[i]);
    }
}

void print_xmm_complex(__m128 reals, __m128 imags) {
    for (int i = 0; i < 4; i++) {
        print_complex((float complex)reals[i] + imags[i] * I);
    }
}
