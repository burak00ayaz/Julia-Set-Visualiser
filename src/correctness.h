/**
 * @brief test correctness of all three implementations with parameters given by user.
 * Correctness test is based on computed iteration numbers for each pixel in the image.
 * Results are compared with reference implementation's results.
 * 
 * @param args julia arguments
 * @param width width of image
 * @param height height of image
 */
void test(Arguments* args, size_t width, size_t height);

/**
 * @brief detailed correctness test with fixed parameters
 * This function will test if computed iteration numbers for each pixel are correct.
 * All three implementations are run and computed iteration numbers are compared
 * with the reference implementation.
 */
void test_correctness();