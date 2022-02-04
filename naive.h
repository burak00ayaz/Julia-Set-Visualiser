
#include "util.h"

/**
 * @brief takes a starting point on complex plane as input and
 * applyies function z -> z^2 + c. returns how many 
 * applications it took to get out of escape radius
 * 
 * @param a real value of starting point
 * @param b imaginary value of starting point
 * @param args arguments
 * @return unsigned number of iterations it took to get out of escape radius
 * or BLACK if point does not get out in maximum number of iterations (convergent)
 */
unsigned iterate_naive(float a, float b, Arguments* args);

/**
 * @brief non-parallel naive implementation in plain C
 * 
 * @param c c constant
 * @param start starting point on complex plane
 * @param width width of the image
 * @param height height of the image
 * @param res step size or resolution
 * @param n maximum number of iterations
 * @param img image buffer
 */
void julia_V2(float complex c, float complex start, size_t width, size_t height, float res, unsigned n, unsigned char* img);
