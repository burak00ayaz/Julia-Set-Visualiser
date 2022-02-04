/**
 * @brief less optimized julia algorithm parallelized with SIMD
 * 
 * @param c c constant
 * @param start starting point on complex plane
 * @param width width of the image
 * @param height height of the image
 * @param res step size or resolution
 * @param n maximum number of iterations
 * @param img image buffer
 */
void julia_V1(float complex c, float complex start, size_t width, size_t height, float res, unsigned n, unsigned char* img);
