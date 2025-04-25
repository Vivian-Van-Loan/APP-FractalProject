gcc -fopenmp -lm -o fractal_openmp_julia fractal_openmp_julia.c
export SIZE=1024; export OMP_NUM_THREADS=4; ./fractal_openmp_julia $SIZE $SIZE 0.306576275 0.4433343679 -1 1 $(bc <<< "scale=15;2/$SIZE")
