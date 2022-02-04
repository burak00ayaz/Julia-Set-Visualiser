#include <stdio.h>      /* Standard Library of Input and Output */
#include <complex.h>    /* Standard Library of Complex Numbers */
#include <float.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "bmp.h"
#include "util.h"

unsigned iterate_naive(float a, float b, Arguments* args) {
    float tmp_a;

    for (unsigned i=1; i<args->n; i++) {
        //naive julia function
        //z = z * z + args->c;
        tmp_a = a;
        a = a*a - b*b + crealf(args->c);
        b = 2*tmp_a*b + cimagf(args->c);

        //check if complex number is outside of escape radius in complex plane
        if (a*a + b*b > args->radius_sqr) {
            return i;
        }
    }
    return BLACK; //choose color 0 -> black
}

/**
 * @brief iterates all the starting points in the complex plane.
 * for each point, computes the series and colors corresponding pixel
 */
static void enumerate(Arguments* args, Image* img) {
    float start_x = crealf(args->start);
    float start_y = cimagf(args->start); 

    //iterate all the points in the complex plane
    for (size_t y=0; y<img->height; y++) {
        float im = start_y + y * args->res;  // imaginary value

        for (size_t x=0; x<img->width; x++) { 
            float re = start_x + x * args->res;  //real value
            
            unsigned iterations = iterate_naive(re, im, args);
            color_pixel(img, y, x, iterations);
        }
    }
}

void julia_V2(float complex c, float complex start, size_t width, size_t height, float res, unsigned n, unsigned char* img) {
    Arguments* args = get_args(c, start, res, n);
    Image* my_img = get_img(width, height, img, n);

    enumerate(args, my_img);

    free(args);
    free(my_img);
}
