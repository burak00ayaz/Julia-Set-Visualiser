#include <time.h>
#include <complex.h>
#include <stdio.h>
#include <float.h>
#include <stdbool.h>

#include "intrin_v0.h"
#include "intrin_v1.h"
#include "naive.h"
#include "util.h"

const char* names[] = {"Optimized", "Less Optimized", "Naive"};

double measure(int implementation, long int repetitions, Arguments* args, Image* img, bool print) {
    void (*julia_impl)(float complex, float complex, size_t, size_t, float, unsigned, unsigned char*);

    switch (implementation) {
        case INTRIN_V0:
            julia_impl = julia; //select optimized implementation
            break;
        case INTRIN_V1:
            julia_impl = julia_V1; //select less optimized implementation
            break;
        case NAIVE:
            julia_impl = julia_V2; //select naive implementation
            break;
        default:
            fprintf(stderr, "Invalid argument. There is no implementation with id %d\n", implementation);
            exit(EXIT_FAILURE);
    }

    if (print) {
        printf("================================================================================\n");
        printf("%s implementation time measurement:\n", names[implementation]);  
        printf("    Repetitions: %ld\n", repetitions);
        printf("    Arguments: {c = %.3f + %.3f i, start = %.3f + %.3f i,\n"
            "                res = %.6f, n = %u, width = %lu, height = %lu}\n",
                            crealf(args->c), cimagf(args->c), crealf(args->start), cimagf(args->start), args->res,
                            args->n, img->width, img->height);
    }

    struct timespec start;
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i=0; i<repetitions; i++) {
        julia_impl(args->c, args->start, img->width, img->height, args->res, args->n, img->buffer);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double t = end.tv_sec - start.tv_sec + 1e-9 * (end.tv_nsec - start.tv_nsec);
    double average = t/repetitions;

    if (print) {
        printf("\n==========> Completed: Average = %f seconds\n", average);
    }
    return average;
}

void performance_comparison() {
    //these parameters do not change during entire test
    unsigned n = 500;
    float complex start = -1.5 + -1.5 * I;
    int repetitions = 10;

    printf("Starting performance comparison..\n");
    printf("Implementations are tested with image sizes varying from 500x500 to 5000x5000\n");
    printf("For every image size, all three implementations are tested with 10 different\n"
           "c values.\n"
           "Function calls are repeated multiple times for every c value.\n"
           "Average time for a function call is printed.\n\n");
    
    printf("Do you want to run detailed performance comparison test? [y/n] ");
    char response = getchar();
    if (response != 'y' && response != 'Y') {
        printf("Abort.\n");
        return;
    }

    Arguments* args;
    Image* img;
    unsigned char* buffer;

    for (int i=0; i<10; i++) {
        size_t size = image_sizes[i];

        printf("================================================================================\n");
        printf("Image size: %lu x %lu\n", size, size);
        buffer = malloc(size * size * 3);
        if (buffer == NULL) {
		    fprintf(stderr, "Could not allocate memory for an image sized %lu x %lu.\n", size, size);
		    exit(EXIT_FAILURE);
        }

        //decrease repetitions as image size gets larger
        if (i == 4) {
            repetitions = 5;
        } else if (i == 7) {
            repetitions = 3;
        }

        double intrin0_total = 0.0;
        double intrin1_total = 0.0;
        double naive_total = 0.0;


        printf("Testing with 10 different c values: 0/10\r");
        fflush(stdout);

        for (int c=0; c<10; c++) {
            //we need to adjust resolution according to image size to get a view of complete julia set in the resulting image
            args = get_args(c_values[c], start, 3.0f/size, n);
            img = get_img(size, size, buffer, n);
            
            intrin0_total += measure(INTRIN_V0, repetitions, args, img, false);
            intrin1_total += measure(INTRIN_V1, repetitions, args, img, false);
            naive_total += measure(NAIVE, repetitions, args, img, false);

            free(args);
            free(img);

            if (c != 9) {
                printf("Testing with 10 different c values: %d/10\r", c+1);
            } else {
                printf("Testing with 10 different c values: 10/10 -> Done.\n");
            }
            fflush(stdout);
        }

        //divide total to number of c constants
        printf("----> Naive (V2) average: %f\n", naive_total/10);
        printf("----> Less Optimized (V1) average: %f\n", intrin1_total/10);
        printf("----> Optimized (V0) average: %f\n", intrin0_total/10);

        fflush(stdout);
        free(buffer);
    }
}

