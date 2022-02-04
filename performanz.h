#include "util.h"

/**
 * @brief run given implementation with given arguments and return average 
 * function running time. 
 * 
 * @param implementation version of julia algorithm
 * @param repetitions number of repeatitions of the function call
 * @param args arguments for julia function call
 * @param img image info
 * @param print print info if set to true
 * @return average running time of function 
 */
double measure(int implementation, long int repetitions, Arguments* args, Image* img, bool print);

/**
 * @brief Compare performances and scaling of naive, 
 * optimized and less optimized implementations with various fixed parameters
 */
void performance_comparison();
