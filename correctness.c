#include <complex.h>
#include <stdio.h>
#include "util.h"
#include "intrin_v0.h"
#include "intrin_v1.h"
#include "naive.h"


/**
 * @brief reference implementation of iteration function.
 * adapted from the pseudo-code algorithm from Beauty of Fractals by Peitgen, H.O. and Richter, P. 1989.
 * 
 * Takes a starting point on complex plane as input and
 * applyies function z -> z^2 + c. returns how many 
 * applications it took to get out of escape radius
 * 
 * @param x real part of complex num
 * @param y imaginary part of complex num
 * @param args julia arguments
 * @return number of iteration steps
 */
unsigned iterate_reference(float x, float y, Arguments* args) {
    //x represents the real part of z
    //y represents the imaginary part of z

    float p = crealf(args->c); //c = p + qi
    float q = cimagf(args->c);

    unsigned k = 0; //iteration
    unsigned K = args->n; //max_iteration

    do {
        float xtemp = x;
        x = x*x - y*y + p;  //x(k+1) := x(k)^2 + y(k)^2 + p
        y = 2*xtemp*y  + q; //y(k+1) := 2 * y(k) * x(k) + q
        k++;                // k := k + 1
    } while (x*x + y*y <= args->radius_sqr && k < K); // r <= M and n < K. 
  

    if (k == K) //iteration == max_iteration
        return BLACK; //choose color 0 (black)
    else
        return k; //case r > M, choose color k
}

void test(Arguments* args, size_t width, size_t height) {
    printf("Correctness test:\n");
    printf("    Arguments: {c = %.3f + %.3f i, start = %.3f + %.3f i,\n"
           "                res = %.6f, n = %u, width = %lu, height = %lu}\n",
                        crealf(args->c), cimagf(args->c), crealf(args->start), cimagf(args->start), args->res,
                        args->n, width, height);

    //make color_pixel function work in correctness test mode
    CORRECTNESS_TEST = true;

    unsigned* iterations0 = malloc(height * width * sizeof(unsigned));
    unsigned* iterations1 = malloc(height * width * sizeof(unsigned));
    unsigned* iterations2 = malloc(height * width * sizeof(unsigned));

    if (iterations0 == NULL || iterations1 == NULL || iterations2 == NULL) {
        fprintf(stderr, "Could not allocate memory for a buffer sized %lu x %lu.\n", width, height);
        exit(EXIT_FAILURE);
    }

    CORRECTNESS_BUFFER = iterations0;
    //run optimized implementation
    //image buffer will not be used anyway since we are in CORRECTNESS_TEST mode, just give null
    julia(args->c, args->start, width, height, args->res, args->n, 0);


    CORRECTNESS_BUFFER = iterations1;
    //run less optimized implementation
    julia_V1(args->c, args->start, width, height, args->res, args->n, 0);

    CORRECTNESS_BUFFER = iterations2;
    //run naive implementation
    julia_V2(args->c, args->start, width, height, args->res, args->n, 0);

    //at this point iteration numbers computed by naive and both optimized implementations are written into
    //corresponding iterations0, iterations1, iterations2 arrays.

    float start_x = crealf(args->start);
    float start_y = cimagf(args->start);    

    //run reference implementation and compare results
    //for each pixel in image, run reference implementation and get 
    //iteration number for that particular pixel.
    for (size_t y=0; y<height; y++) {
        float im = start_y  + y * args->res;  // imaginary value
        for (size_t x=0; x<width; x++) { 
            float re = start_x + x * args->res;  //real value
            
            unsigned iter = iterate_reference(re, im, args);
            unsigned offset = y * width + x;

            //compare iteration number you get from reference implementation with values computed by our implementations.
            if (iterations0[offset] != iter) {
                printf("--> Failed: Optimized implementation did not compute iteration number correctly.\n");
                exit(0);
            }
            if (iterations1[offset] != iter) {
                printf("--> Failed: Less optimized implementation did not compute iteration number correctly.\n");
                exit(0);
            }
            if (iterations2[offset] != iter) {
                printf("--> Failed: Naive implementation did not compute iteration number correctly.\n");
                exit(0); 
            }
        }
    }
    printf("--> Passed. All implementations computed each iteration count correctly.\n\n");
    free(iterations0);
    free(iterations1);
    free(iterations2);
    CORRECTNESS_TEST = false;
}

void test_correctness() {
    printf("Starting detailed correctness test..\n");
    printf("Implementations are tested against reference implementation with\n"
           "10 different c values and image sizes varying from 500x500 to 3000x3000.\n"
           "After each function call, iteration numbers computed for each pixel by all three\n"
           "implementations are compared to reference.\n\n"
           "Test passes if iteration numbers computed by all four functions\n"
           "(optimized, less optimized, naive, reference) are exactly the same.\n\n");
    
    printf("Do you want to run correctness test? [y/n] ");

    char response = getchar();
    if (response != 'y' && response != 'Y') {
        printf("Abort.\n");
        return;
    }

    //make color_pixel function work in correctness test mode
    CORRECTNESS_TEST = true;

    //we will save the iteration numbers computed by our implementations in these buffers
    unsigned* iterations0;
    unsigned* iterations1;
    unsigned* iterations2;

    //these parameters do not change during entire test
    float start = -1.5 + -1.5 * I;
    unsigned n = 200;

    Arguments* args;

    for (int s=0; s<6; s++) {
        size_t size = image_sizes[s];
        printf("================================================================================\n");
        printf("Image size: %lu x %lu\n", size, size);
        fflush(stdout);

        iterations0 = malloc(sizeof(unsigned) * size * size);
        iterations1 = malloc(sizeof(unsigned) * size * size);
        iterations2 = malloc(sizeof(unsigned) * size * size);

        if (iterations0 == NULL || iterations1 == NULL || iterations2 == NULL) {
            fprintf(stderr, "Could not allocate memory for a buffer sized %lu x %lu.\n", size, size);
            exit(EXIT_FAILURE);
        }

        printf("Testing with c value:\n");                
        for (int j=0; j<10; j++) {
            float complex c = c_values[j];
            args = get_args(c, start, 3.0/size, n);

            printf("    %.3f + %.3fi --->", crealf(c), cimagf(c));
            fflush(stdout);

            //iteration numbers will be written by color_pixel function into iterations0 array 
            CORRECTNESS_BUFFER = iterations0;

            //run optimized implementation
            julia(args->c, args->start, size, size, 3.0f/size, args->n, 0);

            //run less optimized implementation
            CORRECTNESS_BUFFER = iterations1;
            julia_V1(args->c, args->start, size, size, 3.0/size, args->n, 0);

            //run naive implementation
            CORRECTNESS_BUFFER = iterations2;
            julia_V2(args->c, args->start, size, size, 3.0/size, args->n, 0);

            float start_x = crealf(args->start);
            float start_y = cimagf(args->start);  

            //run reference implementation and compare results
            for (size_t y=0; y<size; y++) {
                float im = start_y  + y * args->res;  // imaginary value
                for (size_t x=0; x<size; x++) { 
                    float re = start_x + x * args->res;  //real value

                    unsigned iter = iterate_reference(re, im, args);
                    unsigned offset = y * size + x;

                    if (iterations0[offset] != iter) {
                        printf(" Failed\nOptimized implementation did not compute iteration number correctly.\n");
                        exit(0);
                    }
                    if (iterations1[offset] != iter) {
                        printf(" Failed\nLess optimized implementation did not compute iteration number correctly.\n");
                        exit(0);
                    }
                    if (iterations2[offset] != iter) {
                        printf(" Failed\nNaive implementation did not compute iteration number correctly.\n");
                        exit(0);                       
                    }
                }
            }
            fprintf(stderr, " Passed\n");
            free(args); 
        }
        free(iterations0);
        free(iterations1);
        free(iterations2);
    } 
    printf("\nFinished. All tests passed.\n");
    CORRECTNESS_TEST = false;
}