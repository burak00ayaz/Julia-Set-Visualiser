#include <stdio.h>   /* Standard Library of Input and Output */
#include <complex.h> /* Standard Library of Complex Numbers */
#include <stdbool.h>
#include <float.h>
#include <stdint.h>
#include <immintrin.h>
#include <emmintrin.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include "bmp.h"
#include "util.h"
#include "naive.h"

//4 complex numbers are managed with this data structure together
typedef struct {
    size_t x_coords[4];
    size_t y_coords[4];
    float reals[4];
    float imags[4];
    unsigned count[4];
    unsigned bits[4];
    int population; //number of non-empty entries in the data structure. always [0,4]
    void* adress; //use free() on this to free the struct
} four_complexes;


//xmm version of four_complexes
typedef struct {
    __m128 reals; //Real parts
    __m128 imags; //imaginary parts
    __m128 dist;  //distance from origin
    __m128i count; //iteration count. all 4 entries [0,N]
} xmm_four_complexes;


typedef struct {
    __m128 cre; //filled with real parts of c
    __m128 cim; //imaginary part
    __m128 twos; //filled with 2 constants
    __m128i ones; //filled with 1 constants
    __m128i ns;   //filled with n (max iter) contants 
    __m128 radius_sqr; //filled with r^2 with r = max{|c|, 2}
} xmm_helpers;


/**
 * @brief setup a four_complexes data structure instance with 16-byte aligned memory and return
 */
four_complexes* get_aligned_four_complexes() {
    void* memory = malloc(sizeof(four_complexes) + 15);
    if (memory == NULL) {
        fprintf(stderr, "Could not allocate memory for four_complexes struct.\n");
        exit(1);
    }
    four_complexes* ret = (four_complexes*) (((uintptr_t)memory+15) & ~ (uintptr_t)0x0F);
    ret->adress = memory;
    ret->population = 0;

    //initialize reals array with FLT_MAX'es. We do this to mark the entries as empty.
    for (int i=0; i<4; i++) {
        ret->reals[i] = FLT_MAX;
    }
    return ret;
}

/**
 * @brief Get an xmm four complexes object
 */
xmm_four_complexes* get_xmm_four_complexes() {
    xmm_four_complexes* ret = malloc(sizeof(xmm_four_complexes));
    if (ret == NULL) {
        fprintf(stderr, "Could not allocate memory for xmm_four_complexes struct.\n");
        exit(1);       
    }
    ret->count = _mm_set1_epi32(0);
    //no need to initialize rest of registers, they will be loaded later
    return ret;
}

/**
 * @brief Get helper registers
 */
xmm_helpers* get_xmm_helpers(Arguments* args) {
    xmm_helpers* ret = malloc(sizeof(xmm_helpers));
    if (ret == NULL) {
        fprintf(stderr, "Could not allocate memory for xmm_helpers struct.\n");
        exit(1);       
    }
    ret->cre = _mm_set1_ps(crealf(args->c));
    ret->cim = _mm_set1_ps(cimagf(args->c));
    ret->twos = _mm_set1_ps(2.0f);
    ret->ones = _mm_set1_epi32(1);
    ret->ns = _mm_set1_epi32(args->n);
    ret->radius_sqr = _mm_set1_ps(args->radius_sqr);
    return ret;
}

/**
 * @brief Optimized application of julia function f(z) = z^2 + c
 * applies function for four complex numbers at once
 * writes new values into given registers @param a and @param b. 
 * writes |z|^2 for each point into @param dist
 * 
 * @param a real parts of four complex numbers
 * @param b imaginary parts of four complex numbers
 * @param dist escape radius (squared)
 * @param helpers helper registers containing constants and c
 */
void next_of_four(__m128 *a, __m128 *b, __m128 *dist, xmm_helpers* helpers) {
    __m128 _a = _mm_load_ps((float const *)a);
    __m128 _b = _mm_load_ps((float const *)b);

    *a = _mm_mul_ps(_a, *a);
    *b = _mm_mul_ps(_b, *b);

    *dist = _mm_add_ps(*a, *b); // Compute |z|^2 for all the four complex numbers

    *a = _mm_sub_ps(*a, *b);
    *a = _mm_add_ps(*a, helpers->cre); // Mit dieser Instruktion Realteile fertig

    *b = _mm_mul_ps(_a, _b);
    *b = _mm_mul_ps(*b, helpers->twos);
    *b = _mm_add_ps(*b, helpers->cim); // Mit dieser Instruktion Imaginarteile fertig
}

/**
 * @brief used only for last points, when population of array is less than 4. 
 * called only once. 
 * completes computations for remaining elements in four_complexes.
 * uses same algorithm from naive implementation
 */
static void compute_last_points(Arguments* args, four_complexes* nums, Image* img) {
    for (int j=0; j<4; j++) {
        if (nums->reals[j] != FLT_MAX) {
            float a = nums->reals[j];
            float b = nums->imags[j];
            float tmp_a;
            
            unsigned i; //iteration

            if (nums->count[j] == 0) {
                unsigned iter = iterate_naive(a, b, args);
                color_pixel(img, nums->y_coords[j], nums->x_coords[j], iter);
                continue;
            }

            //start from current iteration count, till n
            for (i = nums->count[j]; i<args->n; i++) {
                //check if complex number is outside of escape radius in complex plane
                if (a*a + b*b > args->radius_sqr) {
                    color_pixel(img, nums->y_coords[j], nums->x_coords[j], i);
                    break;
                }
                //naive julia function
                //z = z * z + args->c;
                tmp_a = a;
                a = a*a - b*b + crealf(args->c);
                b = 2*tmp_a*b + cimagf(args->c);
            }
            if (i == args->n) {
                color_pixel(img, nums->y_coords[j], nums->x_coords[j], BLACK);
            }
                
        }
    }
}

/**
 * @brief insert given complex number and corresponding 
 * coordinates in image into four_complexes data structure
 * 
 * @param nums given point is inserted into this struct
 * @param a real part of complex number
 * @param b imaginary part of complex number
 * @param y y coordinate in image
 * @param x x coordinate in image 
 */
void insert(four_complexes* nums, float a, float b, size_t y, size_t x) {
    for (int i=0; i<4; i++) {
        //found an empty entry for the new number
        if (nums->reals[i] == FLT_MAX) {
            nums->reals[i] = a;
            nums->imags[i] = b;
            nums->count[i] = 0;
            nums->x_coords[i] = x;
            nums->y_coords[i] = y;
            nums->population++;
            return;
        }
    }
}

/**
 * @brief iterates through all the starting points in the complex plane,
 * computes iteration number and passes the iteration numbers into color_pixel.
 * 
 * @param args Arguments
 * @param img Image info
 * @param nums arrays holding 4 complex numbers' data
 * @param xmms sse registers holding 4 complex numbers' data
 * @param helpers helper sse registers
 */
static void enumerate(Arguments* args, Image* img, four_complexes* nums, 
                                xmm_four_complexes* xmms, xmm_helpers* helpers) {

    float start_x = crealf(args->start);
    float start_y = cimagf(args->start); 

    //iterate all the points in the complex plane
    for (size_t y=0; y<img->height; y++) {
        float im = start_y  + y * args->res;  // imaginary value

        for (size_t x=0; x<img->width; x++) { 
            float re = start_x + x * args->res;  //real value
            insert(nums, re, im, y, x);

            if (nums->population < 4) {
                continue;
            }
            //data structure contains four complex numbers, fully populated. ready to compute
            xmms->reals = _mm_load_ps(nums->reals);
            xmms->imags = _mm_load_ps(nums->imags);
            xmms->count = _mm_load_si128((__m128i*) nums->count);

            bool full = true;

            //continue to use next_of_four while we have 4 numbers
            while (full) {
                //update all 4 numbers by applying given function z -> z^2 + c
                next_of_four(&xmms->reals, &xmms->imags, &xmms->dist, helpers);

                //increment iterations counter for each point
                xmms->count = _mm_add_epi32(xmms->count, helpers->ones);
                _mm_store_si128((__m128i*) nums->count, xmms->count);

                //check for points outside of radius
                xmms->dist = _mm_cmpgt_ps(xmms->dist, helpers->radius_sqr);
                _mm_store_ps((float*) nums->bits, xmms->dist);


                for (int i=0; i<4; i++) {
                    //bit is set, which means point got outside of radius
                    if (nums->bits[i] != 0) {
                        //point was already outside. iteration count should be 1
                        if (nums->count[i] == 1)
                            color_pixel(img, nums->y_coords[i], nums->x_coords[i], 1);
                        else
                            //count[i] - 1, because dist gives us distances of previous iteration. (see next_of_four function)
                            color_pixel(img, nums->y_coords[i], nums->x_coords[i], nums->count[i]-1);
                        nums->population--;
                        nums->reals[i] = FLT_MAX;
                        full = false;
                    }
                    //maximum numbers of iterations exceeded
                    else if (nums->count[i] >= args->n) {
                        color_pixel(img, nums->y_coords[i], nums->x_coords[i], BLACK);
                        nums->population--;
                        nums->reals[i] = FLT_MAX;
                        full = false;
                    }
                }
            }
            //save reals
            static float tmp[4];
            memcpy(tmp, nums->reals, sizeof(float) * 4);

            //update reals and imags for next iterations
            _mm_store_ps(nums->reals, xmms->reals);
            _mm_store_ps(nums->imags, xmms->imags);

            //mark empty places in reals again
            for (int i=0; i<4; i++) {
                if (tmp[i] == FLT_MAX) {
                    nums->reals[i] = FLT_MAX;
                }
            }
        }
    }
}

void julia_V1(float complex c, float complex start, size_t width, size_t height, float res, unsigned n, unsigned char* img) {
    Arguments* args = get_args(c, start, res, n);
    Image* my_img = get_img(width, height, img, n);
    four_complexes* nums = get_aligned_four_complexes();
    xmm_four_complexes* xmms = get_xmm_four_complexes();
    xmm_helpers* helpers = get_xmm_helpers(args);

    enumerate(args, my_img, nums, xmms, helpers);

    //finish the remaining points in four_complexes
    compute_last_points(args, nums, my_img);

    free(nums->adress);
    free(xmms);
    free(helpers);
    free(args);
    free(my_img);
}