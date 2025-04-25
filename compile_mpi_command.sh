mpicc fractal_mpi.c -lm -lgmp -o fractal_mpi
export SIZE=1024; rm fractal.out; mpirun -np 4 fractal_mpi $SIZE $SIZE 0.306576275 0.4433343679 -1 1 $(bc <<< "scale=15;2/$SIZE")