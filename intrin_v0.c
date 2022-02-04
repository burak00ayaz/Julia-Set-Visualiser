#include <stdio.h>   /* Standard Library of Input and Output */
#include <complex.h> /* Standard Library of Complex Numbers */
#include <stdint.h>
#include <immintrin.h>
#include <emmintrin.h>

#include "bmp.h"
#include "util.h"
#include "naive.h"

/**
 * @brief iterates through all the starting points in the complex plane,
 * computes iteration number and passes the iteration numbers into color_pixel.
 * 
 * @param args julia arguments
 * @param img image data
 */
static void enumerate(Arguments* args, Image* img) {
    //4 complex numbers are kept in these two registers.
    __m128 _reals;
    __m128 _imags;

    //load and broadcast real and imaginary parts of c into seperate registers.
    __m128 cre = _mm_set1_ps(crealf(args->c));
    __m128 cim = _mm_set1_ps(cimagf(args->c));

    //broadcast escape radius to a register.
    __m128 rds = _mm_set1_ps(args->radius_sqr);

    //broadcast 2 constants into a register. needed for complex multiplication.
    __m128 twos = _mm_set1_ps(2.0f);

    float start_x = crealf(args->start);
    float start_y = cimagf(args->start); 

    //iterate all the points in the complex plane
    for (size_t y=0; y<img->height; y++) {
        float im = start_y  + y * args->res;  // imaginary value

        for (size_t x=0; x<img->width; x++) {
            float re = start_x + x * args->res;  //real value
            _reals[x % 4] = re;

            //begin computation after every 4th iteration (when _reals is filled with 4 new numbers)
            if (x % 4 == 3) {
                _imags = _mm_set1_ps(im);
                __m128i iterations = _mm_set1_epi32(0);
                //0xf - all last 4 bits set
                int mask = 15;

                //main iterations loop
                for (unsigned i=0; i<args->n; i++) {
                    __m128 _re = _mm_mul_ps(_reals, _reals); //re^2 (for each point)
                    __m128 _im = _mm_mul_ps(_imags, _imags); //im^2
                    __m128 abs = _mm_add_ps(_re, _im); //re^2 + im^2

                    //returns zeros for points outside of escape radius
                    abs = _mm_cmple_ps(abs, rds);

                    //points which get out of escape radius get removed from the mask 
                    mask = mask & _mm_movemask_ps(abs);

                    __m128i ones = _mm_set1_epi32(1); //1 constants
                    //remove 1 constant for points which got out
                    ones = _mm_and_si128(ones, (__m128i) abs); 
                    //increment iteration count of points which are already in radius
                    iterations = _mm_add_epi32(iterations, ones); 

                    //if all points out, end loop
                    if (mask == 0) {
                        break;
                    }
                    //complex multiplication
                    _imags = _mm_mul_ps(_reals, _imags); //re * im
                    _imags = _mm_mul_ps(_imags, twos); // 2 * re * im 
                    _imags = _mm_add_ps(_imags, cim); // 2*re*im + cim

                    _reals = _mm_sub_ps(_re, _im); //re^2 + im^2
                    _reals = _mm_add_ps(_reals, cre); //re^2 + im^2 + cre
                }

                unsigned results[4];
                _mm_storeu_si128((__m128i*) results, iterations);

                for (int i=0; i<4; i++) {
                    //point is still in. belongs to julia set.
                    if (results[i] == args->n) {
                        color_pixel(img, y, x-3+i, BLACK);
                    } 
                    //point was already outside. 
                    else if (results[i] == 0) {
                        color_pixel(img, y, x-3+i, 1);
                    } 
                    //point got outside in step number results[i]. choose color results[i]
                    else {
                        color_pixel(img, y, x-3+i, results[i]);
                    }
                }
            }
        }
    }    
}

/**
 * @brief this implementation of julia has the restriction, that the width should be divisible by 4.
 * when width not divisible by 4, some points (points in last columns, max 3 columns) 
 * can not be computed in enumerate() function.
 * compute rest of these points with naive approach
 * 
 * @param args julia arguments
 * @param img image data
 */
static void compute_last_points(Arguments* args, Image* img) {
    float start_x = crealf(args->start);
    float start_y = cimagf(args->start); 

    //this column and other columns right side of this column are not computed with previous
    //enumerate() call. Compute them with naive approach and finish the image
    size_t column = img->width - (img->width % 4);

    //iterate all the points in the complex plane
    for (size_t y=0; y<img->height; y++) {
        float im = start_y  + y * args->res;  // imaginary value

        for (size_t x=column; x<img->width; x++) {
            float re = start_x + x * args->res;  //real value
            
            unsigned iterations = iterate_naive(re, im, args);
            color_pixel(img, y, x, iterations);
        }
    }
}

void julia(float complex c, float complex start, size_t width, size_t height, float res, unsigned n, unsigned char* img) {
    Arguments* args = get_args(c, start, res, n);
    Image* my_img = get_img(width, height, img, n);

    enumerate(args, my_img);
    if (width % 4 != 0) {
        compute_last_points(args, my_img);
    }

    free(args);
    free(my_img);
}

