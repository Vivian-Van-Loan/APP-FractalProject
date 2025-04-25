#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <complex.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#include <omp.h>

int main(int argc, char** argv) {
    unsigned long width;
    unsigned long height;
    complex float* grid;
    
    complex float c;
    float left_point;
    float top_point;
    float point_step;
    
    if (argc < 8) {
        fprintf(stderr, "Usage: program grid_size_x (1 size_t), grid_size_y (1 size_t), c (2 floats), left_point (1 float), top_point (1 float), point_step (1 float)\n");
        return 1;
    }
    width = strtoul(argv[1], NULL, 0);
    height = strtoul(argv[2], NULL, 0);
    float c_r = strtof(argv[3], NULL);
    float c_i = strtof(argv[4], NULL);
    c = c_r + (c_i * I);
    left_point = strtof(argv[5], NULL);
    top_point = strtof(argv[6], NULL);
    point_step = strtof(argv[7], NULL);
    
    grid = malloc(height * width * sizeof(complex float));
    
    double start_time = omp_get_wtime();
    #pragma omp parallel for
    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++) {
            complex float* z = &grid[y * width + x];
            *z = (left_point + x * point_step) + (top_point - y * point_step) * I;
            
            for (size_t itr = 0; itr < 200; itr++) {
                *z = cpowf(*z, 2) + c;
                
                if (cabsf(*z) > 200) {
                    //printf("Breaking at point (%zu, %zu)\n", x, y);
                    break;
                }
            }
        }
    }
    double end_time = omp_get_wtime();
    printf("Time for all %s threads to finish computing fractal of size (%zu, %zu): %f seconds\n", getenv("OMP_NUM_THREADS"), width, height, end_time - start_time);
    
    int fd = open("fractal.out", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++) {
            unsigned char val;
            if (cabsf(grid[y * width + x]) > 200) {
                val = 0;
            } else {
                val = 255;
            }
            write(fd, &val, 1);
        }
    }
}
