# Julia-Set-Visualiser
Julia-Set-Visualiser is a fractal image generator written in C. It generates Julia set fractals by repeatedly iterating a complex quadratic function until divergence (or convergence). The number of iterations required to diverge at a point is recorded and used as part of the value to color the related pixel in the final, rendered image of the fractal.

## Purpose
The purpose of this project was to improve performance of Julia-set generation by vectorizing calculations and implementing a parallel, vectorized solution using SIMD extensions (Intrinsics). This project includes two vectorized SIMD implementations and one naive, non-parallel implementation for Julia-set generation. Performance tests in order to compare implementations are also included. Results show that Intrinsics implementation (v0) has a 3x shorter runtime than naive implementation on average.

## Usage
Create the executable `julia` by running `Makefile`:
```
$ make
```
Use `julia` as follows:
```
$ ./julia [-d <width>,<height>] [-c <real>,<imag>] [-r step_size] [-s <real>,<imag>] 
            [-n iterations] [-V version] [-o filename] [-B repetitions] [-x]
```
### Parameter Descriptions
* `-d <width>,<height>`: Choose width and height of the image to be created. Give width and height as unsigned integer numbers seperated by a comma.
* `-c <real>,<imag>`: Choose complex `c` constant. This parameter determines fractal's characteristic shape and pattern. Give real and imaginary parts as floating point numbers seperated by a comma. Use `-c rand` option to choose a random `c` value from my favourites.
* `-r step_size`: Choose the gap between two neighboring pixels in the complex plane. This parameter determines the resolution of the image. Image will be more detailed if given `step_size` is lower.
`Tip`: Use `3/n` for `step_size` for an image of size `n x n` to get a view of complete julia set in the resulting image.
* `-s <real>,<imag>`: Choose the starting point in the complex plane which will be bottom left corner of the image. Give real and imaginary parts of starting point as floating point numbers seperated by a comma.
* `-n iterations`: Choose the maximum number of iterations of the function call `f(z) = z^2 + c` per pixel.
* `-V version`:  Choose the implementation. Use `-V 0` for optimized parallel implementation, `-V 1` for less optimized parallel implementation and `-V 2` for naive implementation.
* `-o filename`: Choose a file name for the image to be created. Give file name with `.bmp` extension.
* `-B[repetitions]`: `#PerformanceTest` If `-B` set, measure average running time of chosen implementation with optional argument `repetitions` as number of repetitions of function call. Use `-B0` to run detailed performance comparison test.
* `-x`: `#CorrectnessTest` Run correctness test with user-given arguments. All of the three implementations are tested against a reference implementation. Use `-x` to only run correctness test. Use `-xi` to run correctness test and rerun to create an image afterwards. Use `-x0` to run detailed correctness test with fixed arguments.

All parameters are optional. Default value is used if a parameter is not provided.

## Examples
Provided example images are included in `examples` folder.

### Example 1
#### Input
```
./julia -c -0.53,0.5 -d 2000,2000 -r 0.0015 -o example1.bmp
```
#### Output
![Julia Set Fractal 1](/examples/example1.bmp?raw=true "Julia Set Fractal 1")

Let's create a higher resolution version of the previous image. Output image is not included due to large size.
#### Input
```
./julia -c -0.53,0.5 -d 5000,5000 -r 0.0006 -o example1.bmp
```

### Example 2 
By varying the `c` constant, we get different Julia sets.

#### Input
```
./julia -c -0.8,-0.154 -d 2000,2000 -r 0.0015 -o example2.bmp
```

#### Output
![Julia Set Fractal 2](/examples/example2.bmp?raw=true "Julia Set Fractal 2")

### Example 3
#### Input 
```
./julia -c 0.33,0.058 -d 2000,2000 -r 0.0015 -o example3.bmp
```

#### Output
![Julia Set Fractal 3](/examples/example3.bmp?raw=true "Julia Set Fractal 3")
