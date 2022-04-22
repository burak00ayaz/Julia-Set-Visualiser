#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <complex.h>
#include <stdbool.h>
#include <limits.h>
#include <float.h>
#include <errno.h>

#include "bmp.h"
#include "naive.h"
#include "intrin_v0.h"
#include "intrin_v1.h"
#include "performanz.h"
#include "util.h"
#include "correctness.h"

// Default values for parameters
#define DEFAULT_WIDTH 2000
#define DEFAULT_HEIGHT 2000
#define DEFAULT_RES 0.0015
#define DEFAULT_N 500
#define DEFAULT_PATH "image.bmp"
#define DEFAULT_REPETITIONS 10 //for performance test
const float complex DEFAULT_START = (-1.5 + -1.5 * I);  
const float complex DEFAULT_C = (-0.53 + 0.5 * I);


void print_help(char* executable_name) {
	printf("Usage: %s [-V version] [-B repetitions] [-s <real>,<imag>]\n"
	       "                [-d <width>,<height>] [-n iterations] [-r step_size]\n"
		   "                [-c <real>,<imag>] [-o filename] [-x]\n\n", executable_name);

	printf("    -V version:          Choose the implementation. Use version=0 for optimized\n"
           "                         parallel implementation, version=1 for less optimized\n"
		   "                         parallel implementation and version=2 for naive\n"
		   "                         implementation.\n"
		   "                         Default: 0\n\n");

	printf("    -B[repetitions]:     If -B set, measure average running time of chosen\n"
           "                         implementation with optional argument repetitions\n"
		   "                         (repetitions=%d by default) as number of\n"
		   "                         repetitions of function call.\n"
		   "                         Use repetitions=0 to run detailed performance\n"
		   "                         comparison test.\n\n", DEFAULT_REPETITIONS);

	printf("    -s <real>,<imag>:    Choose the starting point in the complex plane which\n"
           "                         will be bottom left corner of the image. Give real and\n"
		   "                         imaginary parts of starting point as floating point\n"
		   "                         numbers seperated by a comma.\n"
		   "                         Default: %f + %f i\n\n", crealf(DEFAULT_START), cimagf(DEFAULT_START));

	printf("    -d <width>,<height>: Choose width and height of the image to be created.\n"
           "                         Give width and height as unsigned integer numbers\n"
		   "                         seperated by a comma.\n"
		   "                         Default: %u, %u\n\n", DEFAULT_WIDTH, DEFAULT_HEIGHT);

	printf("    -n iterations:       Choose the maximum number of iterations of the function\n"
           "                         call (f(z) = z^2 + c) per pixel.\n"
		   "                         Default: %d\n\n", DEFAULT_N);

	printf("    -r step_size:        Choose the gap between two neighboring pixels in the\n"
		   "                         complex plane. This parameter determines the resolution\n"
		   "                         of the image. Image will be more detailed if given\n"
		   "                         step_size is lower.\n"
		   "                         Tip: Use 3/n for step_size for an image of size n x n\n"
		   "                         to get a view of complete julia set in the resulting\n"
		   "                         image.\n"
		   "                         Default: %f\n\n", DEFAULT_RES);

	printf("    -c <real>,<imag>:    Choose complex c constant. Give real and imaginary\n"
		   "                         parts as floating point numbers seperated by a comma.\n"
		   "                         Use '-c rand' option to choose a random c value from\n"
		   "                         my favourites.\n"
		   "                         Default: %f + %f i\n\n", crealf(DEFAULT_C), cimagf(DEFAULT_C));
                   
	printf("    -o filename:         Choose path/filename for the image to be created.\n"
		   "                         Give filename with .bmp extension.\n"
		   "                         Default: %s\n\n", DEFAULT_PATH);

	printf("    -x:                  Run correctness test with user-given arguments.\n"
		   "                         All of the three implementations are tested against\n"
		   "                         a reference implementation.\n"
		   "                         Use -x to only run correctness test.\n"
		   "                         Use -xi to run correctness test and rerun to create\n"
		   "                         an image afterwards.\n"
	       "                         Use -x0 to run detailed correctness test with\n"
		   "                         fixed arguments.\n\n");
	printf("    -h or --help:        Prints complete usage information\n\n");       
}

void invalid_argument(char flag) {
	fprintf(stderr, "Invalid argument for option -%c, use -h or --help for help.\n", flag);
	exit(EXIT_FAILURE);
}
void missing_second_option(char flag) {
	fprintf(stderr, "Option -%c needs second argument, use -h or --help for help.\n", flag);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
	//initialize arguments with default values defined above
	//default values are used if not given by user
	int implementation = INTRIN_V0;
	float complex start = DEFAULT_START; 
	size_t width = DEFAULT_WIDTH;
	size_t height = DEFAULT_HEIGHT;
	unsigned n = DEFAULT_N;
	float res = DEFAULT_RES;
	float complex c = DEFAULT_C;
	char *path = DEFAULT_PATH; 
	unsigned char *img;
	long int repetitions = DEFAULT_REPETITIONS;

	//performance and correctness testing options
	bool benchmarking = false;
	int correctness = 0; //0: no correctness test, 1: run correctness test, 2: run correctness test and create image

	// helper variables for parsing arguments below
	char *token;
	char *endptr;

	static const struct option long_options[] = {{"help", no_argument, 0, 'h'},{NULL, 0, NULL, '?'}};
	int index = -1;
	int flag;

	while ((flag = getopt_long(argc, argv, "V:B::s:d:n:r:c:o:hx::", long_options, &index)) != -1) {
		switch (flag) {
			//help
			case 'h':
				print_help(argv[0]);
				return 0;
			//run correctness test
			case 'x':
				correctness = 1;
				if (optarg != NULL) {
					if (strcmp(optarg, "0") == 0) {
						test_correctness();
						return 0;
					}
					if (strcmp(optarg, "i") == 0) {
						correctness = 2;
					}
				}
				break;
			//implementation version
			case 'V':
				//only 0, 1 and 2 are valid arguments for this options
				if (optarg != NULL) {
					if (strcmp(optarg, "1") == 0) {
						implementation = INTRIN_V1;
					} else if (strcmp(optarg, "2") == 0) {
						implementation = NAIVE;
					} else if (strcmp(optarg, "0") != 0) {
						invalid_argument('V');
					}					
				}
				break;
			//benchmarking
			case 'B':
				benchmarking = true;

				if (optarg != NULL) {
					//run detailed performance comparison test if -B0 
					if (strcmp(optarg, "0") == 0) {
						performance_comparison();
						return 0;       
					}
					errno = 0;
					repetitions = strtol(optarg, &endptr, 10);
					if (errno != 0 || *endptr != '\0' || repetitions < 0 || repetitions > INT32_MAX) {
						invalid_argument('B');
					}
				}
				break;
			//parse a complex constant: s (starting point on complex plane) or c constant
			case 's':
			case 'c':
				//if -c rand is given, choose a random c from selected c values.
				if (flag == 'c' && optarg && strcmp(optarg, "rand") == 0) {
					c = get_random_c();
					break;
				}
				float re, im;

				//read real part of num
				token = strtok(optarg, ",");
				errno = 0;
				re = strtof(token, &endptr);

				//check if the provided argument is invalid
				if (errno != 0 || *endptr != '\0') {
					invalid_argument(flag);
				}

				token = strtok(NULL, ",");
				if (token == NULL) {
					missing_second_option(flag);
				}

				//read imaginary part of num
				errno = 0;
				im = strtof(token, &endptr);
				//check if the provided argument is invalid
				if (errno != 0 || *endptr != '\0') {
					invalid_argument(flag);
				}

				if (flag == 'c') {
					c = re + im * I;
				} else { //flag == 's'
					start = re + im * I;
				}
				break;
			//width and height of picture
			case 'd':
				//read width
				token = strtok(optarg, ",");

				errno = 0;
				long w = strtol(token, &endptr, 10);
				if (errno != 0 || *endptr != '\0' || w <= 0) {
					invalid_argument('d');
				}

				//read height
				token = strtok(NULL, ",");
				if (token == NULL) {
					missing_second_option('d');
				}
				errno = 0;
				long h = strtol(token, &endptr, 10);
				if (errno != 0 || *endptr != '\0' || h <= 0) {
					invalid_argument('d');
				}

				width = w;
				height = h;
				break;
			//maximum number of iterations
			case 'n':
				errno = 0;
				long val = strtol(optarg, &endptr, 10);
				if (errno != 0 || *endptr != '\0' || val < 0 || val >= UINT_MAX) {
					invalid_argument('n');
				}

				n = val;
				break;
			//resolution
			case 'r':
				res = strtof(optarg, &endptr);
				if (errno != 0 || *endptr != '\0' || res <= 0.0f) {
					invalid_argument('r');
				}

				break;
			//output file
			case 'o':
				//optarg is given path in this case
				//check for file extension .bmp
				if (strlen(optarg) < 5 || optarg[strlen(optarg) - 1] != 'p' || optarg[strlen(optarg) - 2] != 'm'
									|| optarg[strlen(optarg) - 3] != 'b' || optarg[strlen(optarg) - 4] != '.') {
					printf("Please include .bmp extension in your filename. -> <filename>.bmp\n");
					invalid_argument('o');
				}
				path = optarg;
				break;
			case '?':
				if (optopt == 's' || optopt == 'd' || optopt == 'n' || optopt == 'r' || optopt == 'c' || optopt == 'o' || optopt == 'h') {
					fprintf(stderr, "Option -%c needs an argument, use -h or --help for help.\n", optopt);
				}
				else {
					fprintf(stderr, "Unknown option '-%c', use -h or --help for help.\n", optopt);
				}
				return -1;
			default:
				abort();
		}
	}

	for (index = optind; index < argc; index++) {
		printf("Non-option argument %s. Use -h or --help for help.\n", argv[index]);
		return EXIT_FAILURE;
	}

	if (correctness != 0 && benchmarking) {
		fprintf(stderr, "Invalid arguments: -x and -B[repetitions] flags are set at the same time.\n");
		fprintf(stderr, "Can not run perfomance and correctness test simultaneously.\n");
		return EXIT_FAILURE;
	}

	Arguments* args = get_args(c, start, res, n);

	//correctness is 1 or 2.
	if (correctness != 0) {
		//test run all implementations for correctness
		test(args, width, height);

		//do not create an image, just return
		if (correctness == 1) 
			return 0;
	}
        
	//allocate memory for image array and create structs from variables
	img = malloc(height * width * 3);
	if (img == NULL) {
		fprintf(stderr, "Could not allocate memory for an image sized %lu x %lu.\n", width, height);
		return EXIT_FAILURE;
	}

	Image* my_img = get_img(width, height, img, n);

	//run performance test
	if (benchmarking) {
		measure(implementation, repetitions, args, my_img, true);
	}
	//run the algorithm 
	else {
		if (!correctness)
			printf("Arguments: {c = %.3f + %.3f i, start = %.3f + %.3f i,\n"
              	   "            resolution = %.6f, n = %u, width = %lu, height = %lu}\n", crealf(c), cimagf(c), 
			   									crealf(start), cimagf(start), res, n, width, height);
		switch (implementation) {
			case INTRIN_V0:
				printf("Running implementation Optimized (V0) ...\n\n");
				julia(c, start, width, height, res, n, img);
				break;
			case INTRIN_V1:
				printf("Running implementation Less Optimized (V1) ...\n\n");
				julia_V1(c, start, width, height, res, n, img);
				break;
			case NAIVE:
				printf("Running implementation Naive (V2) ...\n\n");
				julia_V2(c, start, width, height, res, n, img);
				break;
		}
	}

	//if -B flag not set, create the image
	if (!benchmarking) {
		generateBitmapImage(img, height, width, path);
		printf("--> Image %s is created.\n", path);
	}

	free(img);
	free(args);
	free(my_img);
	return 0;
}
