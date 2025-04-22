#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <complex.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>

#include <mpi.h>
//#include <gmp.h> //Useful(ish) library that handles rational arith for us

//#define Rational mpq_t

typedef struct Rational {
    unsigned long numerator;
    unsigned long denominator;
} Rational;

/*
Rational rational_halve(Rational in);
Rational rational_add(Rational lhs, Rational rhs);*/

enum tags {
    TAG_WORK_REQUEST = 2,
    TAG_WORK_RESPONSE,
    TAG_ENERGY,
    TAG_TERMINATE,
};

typedef struct FractalInfo {
    int rank;
    int num_procs;
    complex float* sub_grid; //pointer to the grid subsection (malloc'd on each proc)
    unsigned long sub_grid_size_x; //dimensions of the subsection of the grid we have
    unsigned long sub_grid_size_y;
    unsigned long sub_grid_offset_x; //the x and y start of the grid from the main grid
    unsigned long sub_grid_offset_y;
    unsigned long total_grid_width; //the total width and height of the entire grid
    unsigned long total_grid_height;
    complex float c; //da constant
    float left_point;
    float top_point; //despite this being imaginary, its stored real to make the conversion easier later
    float point_step; //how much distance there is between each point
    Rational energy;
    MPI_File file;
} FractalInfo;

typedef unsigned long ULONG_BUFFER[5];
#define ULONG_BUFFER_COUNT (sizeof(ULONG_BUFFER) / sizeof(unsigned long))
//MPI_Datatype mpi_ulong_vec;

void main_loop(FractalInfo info);
void compute_fractal(FractalInfo* info);

int main(int argc, char* argv[]) {
    FractalInfo info;
    
    struct timeval tv;
    gettimeofday(&tv, NULL);
    srand(tv.tv_usec); //yeah i know srand and rand are bad, but given what i need it for is minor (generating a random value between 0 and num_procs) its fine
    
    MPI_Init(&argc, &argv);
    
    MPI_Comm_rank(MPI_COMM_WORLD, &info.rank);
    MPI_Comm_size(MPI_COMM_WORLD, &info.num_procs);
    
    MPI_File_open(MPI_COMM_WORLD, "fractal.out", MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &info.file);
    
    if (info.rank == 0) {
        if (argc < 8) {
            fprintf(stderr, "Usage: program grid_size_x (1 size_t), grid_size_y (1 size_t), c (2 floats), left_point (1 float), top_point (1 float), point_step (1 float)\n");
            return 1;
        }
        info.total_grid_width = strtoul(argv[1], NULL, 0);
        info.total_grid_height = strtoul(argv[2], NULL, 0);
        float c_r = strtof(argv[3], NULL);
        float c_i = strtof(argv[4], NULL);
        info.c = c_r + (c_i * I);
        info.left_point = strtof(argv[5], NULL);
        info.top_point = strtof(argv[6], NULL);
        info.point_step = strtof(argv[7], NULL);
        printf("grid size: (%lu, %lu), c: (%f, %f), left: (%f), top: (%f), step: %f\n", info.total_grid_width, info.total_grid_height, crealf(info.c), cimagf(info.c), info.left_point, info.top_point, info.point_step);
    }
    //this send happens once so it is not worth the overhead to make a new type for this
    MPI_Bcast(&info.total_grid_width, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);
    MPI_Bcast(&info.total_grid_height, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);
    MPI_Bcast(&info.c, 2, MPI_FLOAT, 0, MPI_COMM_WORLD); //complex types are exactly equivalent to the two base types in a row
    MPI_Bcast(&info.left_point, 1, MPI_FLOAT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&info.top_point, 1, MPI_FLOAT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&info.point_step, 1, MPI_FLOAT, 0, MPI_COMM_WORLD);
    
    //MPI_Type_vector(sizeof(ULONG_BUFFER) / sizeof(unsigned long), 1, 1, MPI_UNSIGNED_LONG, &mpi_ulong_vec);
    
    //mpq_init(info.energy);
    //mpq_set_ui(info.energy, 1, info.num_procs); //in classic GNU fashion, all the type and function names are incomprehensible. This sets the fraction
    
    unsigned long y_div = info.num_procs;
    unsigned long x_div = 1;
    while (y_div > info.total_grid_height) {
        y_div /= 2;
        x_div *= 2;
    }
    unsigned long rank_x = info.rank % x_div;
    unsigned long rank_y = info.rank / x_div;
    
    info.sub_grid_offset_x = (info.total_grid_width / x_div) * rank_x;
    info.sub_grid_offset_y = (info.total_grid_height / y_div) * rank_y;
    
    info.sub_grid_size_y = info.total_grid_height / y_div;
    if (rank_y == y_div - 1) {
        info.sub_grid_size_y += info.total_grid_height % y_div;
    }
    info.sub_grid_size_x = info.total_grid_width / x_div;
    if (rank_x == x_div - 1) {
        info.sub_grid_size_y += info.total_grid_width % x_div;
    }
    /*info.sub_grid_offset_x = (info.total_grid_width / x_div) * (info.rank % x_div);
    info.sub_grid_offset_y = (info.total_grid_height / y_div) * (info.rank % y_div);
    if (info.rank % y_div == y_div - 1) { //note: still not correct, needs to be more complicated like if % x_div == x_div - 1 for the y so you know its on the right edge
        info.sub_grid_size_y = (info.total_grid_height / y_div) + (info.sub_grid_size_y = info.total_grid_height % y_div);
    } else {
        info.sub_grid_size_y = info.total_grid_height / y_div;
    }
    
    if (info.rank % x_div == x_div - 1) {
        info.sub_grid_size_x = (info.total_grid_width / x_div) + (info.sub_grid_size_x = info.total_grid_width % x_div);
    } else {
        info.sub_grid_size_x = info.total_grid_width / x_div;
    }*/
    info.sub_grid = malloc(sizeof(complex float) * (info.sub_grid_size_x * info.sub_grid_size_y));
    
    info.energy.denominator = info.total_grid_width * info.total_grid_height;
    info.energy.numerator = info.sub_grid_size_x * info.sub_grid_size_y;
    
    printf("Rank %d, subgrid size: (%lu, %lu), offset: (%lu, %lu), energy: %lu/%lu\n", info.rank, info.sub_grid_size_x, info.sub_grid_size_y, info.sub_grid_offset_x, info.sub_grid_offset_y, info.energy.numerator, info.energy.denominator);
    
    main_loop(info);
    
    MPI_File_close(&info.file);
    
    MPI_Finalize();
}

enum TriState {
    TRISTATE_NO_WORK,
    TRISTATE_NO_RESPONSE,
    TRISTATE_WORK,
};

enum TriState check_work_response(FractalInfo* info, int target) {
    int request;
    MPI_Status status;
    MPI_Iprobe(target, TAG_WORK_RESPONSE, MPI_COMM_WORLD, &request, &status);
    if (request) {
        ULONG_BUFFER buf;
        MPI_Recv(buf, ULONG_BUFFER_COUNT, MPI_UNSIGNED_LONG, target, TAG_WORK_RESPONSE, MPI_COMM_WORLD, &status);
        if (buf[0] != 0) { //empty buffer signals there's no work
            unsigned long old_size_x = info->sub_grid_size_x;
            unsigned long old_size_y = info->sub_grid_size_y;
            info->sub_grid_size_x = buf[0];
            info->sub_grid_size_y = buf[1];
            info->sub_grid_offset_x = buf[2];
            info->sub_grid_offset_y = buf[3];
            info->energy.numerator += buf[4];
            if (old_size_x != info->sub_grid_size_x || old_size_y != info->sub_grid_size_y) {
                info->sub_grid = realloc(info->sub_grid, sizeof(complex float) * (info->sub_grid_size_x * info->sub_grid_size_y));
            }
            return TRISTATE_WORK;
        }
        //printf("No work\n");
        return TRISTATE_NO_WORK;
    }
    return TRISTATE_NO_RESPONSE;
}

void main_loop(FractalInfo info) {
    bool compute = true;
    int target = -1;
    int skip_counter = 0;
    while (true) {
        if (compute)
            compute_fractal(&info);
        compute = false;

        int request;
        MPI_Status status;
        if (info.rank == info.num_procs - 1) {
            unsigned long energy;
            MPI_Iprobe(MPI_ANY_SOURCE, TAG_ENERGY, MPI_COMM_WORLD, &request, &status);
            while (request) {
                MPI_Recv(&energy, 1, MPI_UNSIGNED_LONG, status.MPI_SOURCE, TAG_ENERGY, MPI_COMM_WORLD, &status);
                MPI_Iprobe(MPI_ANY_SOURCE, TAG_ENERGY, MPI_COMM_WORLD, &request, &status);
                info.energy.numerator += energy;
                //printf("Final proc energy now at: %lu/%lu\n", info.energy.numerator, info.energy.denominator);
            }
            if (info.energy.numerator == info.energy.denominator) {
                for (int i = 0; i < info.num_procs; i++) {
                    if (i == info.rank) {
                        continue;
                    }
                    int signal = 1;
                    MPI_Send(&signal, 1, MPI_INT, i, TAG_TERMINATE, MPI_COMM_WORLD);
                }
                break;
            }
        }
        
        MPI_Iprobe(MPI_ANY_SOURCE, TAG_WORK_REQUEST, MPI_COMM_WORLD, &request, &status);
        if (request) {
            ULONG_BUFFER buf = {0};
            MPI_Recv(&target, 1, MPI_INT, status.MPI_SOURCE, TAG_WORK_REQUEST, MPI_COMM_WORLD, &status);
            MPI_Send(buf, ULONG_BUFFER_COUNT, MPI_UNSIGNED_LONG, target, TAG_WORK_RESPONSE, MPI_COMM_WORLD);
            //printf("Proc %d: Sent empty work response to proc %d\n", info.rank, target);
        }
        MPI_Iprobe(MPI_ANY_SOURCE, TAG_TERMINATE, MPI_COMM_WORLD, &request, &status);
        if (request) {
            int signal;
            MPI_Recv(&signal, 1, MPI_INT, status.MPI_SOURCE, TAG_TERMINATE, MPI_COMM_WORLD, &status);
            if (signal) {
                break;
            }
        } //this should be in a loop alongside checking if any work exists. it also needs to pass out its energy
        //should there be a controlling thread that just passes out data and work? Probably not but need to manage energy distribution without it
        //  you know what, just toss it to the last proc and let it handle it
        
        if (target == -1 && skip_counter < info.num_procs / 2) {
            target = info.rank;
            while (target == info.rank) {
                target = rand() % info.num_procs;
            }
            //printf("Proc %d now targetting proc %d to take work\n", info.rank, target);
            MPI_Send(&info.rank, 1, MPI_INT, target, TAG_WORK_REQUEST, MPI_COMM_WORLD);
        }
        if (target != -1) {
            enum TriState response = check_work_response(&info, target);
            if (response == TRISTATE_WORK) {
                target = -1;
                compute = true;
                continue;
            } else if (response == TRISTATE_NO_WORK) {
                target = -1;
                skip_counter++;
                //printf("Proc %d: post-inc skip %d\n", info.rank, skip_counter);
            }
        }
        usleep(500000); //sleep for half a second
    }
    //printf("Proc %d exiting\n", info.rank);
}

void compute_fractal(FractalInfo* info) {
    for (size_t y = 0; y < info->sub_grid_size_y; y++) {
        for (size_t x = 0; x < info->sub_grid_size_x; x++) {
            size_t x_offset = x + info->sub_grid_offset_x;
            size_t y_offset = y + info->sub_grid_offset_y;
            complex float* z = &info->sub_grid[y * info->sub_grid_size_x + x];
            *z = (info->left_point + x_offset * info->point_step) + (info->top_point - y_offset * info->point_step) * I;
            
            for (size_t itr = 0; itr < 200; itr++) {
                *z = cpowf(*z, 2) + info->c;
                
                if (cabsf(*z) > 200) {
                    //printf("Breaking at point (%zu, %zu)\n", x, y);
                    break;
                }
            }
        }
        if (y % 4 == 0 && (info->sub_grid_size_y - y) > info->num_procs * 2) { //stop them from fighting over each other at small remaining values
            int request;
            MPI_Status status;
            MPI_Iprobe(MPI_ANY_SOURCE, TAG_WORK_REQUEST, MPI_COMM_WORLD, &request, &status);
            if (request) {
                int target;
                MPI_Recv(&target, 1, MPI_INT, status.MPI_SOURCE, TAG_WORK_REQUEST, MPI_COMM_WORLD, &status);
                //printf("Proc %d: Processing work request\n", info->rank);
                //unsigned long old_num = info->energy.numerator;
                ULONG_BUFFER buf;
                buf[0] = ((info->sub_grid_size_y - y) / 2);
                buf[1] = info->sub_grid_size_x;
                buf[2] = info->sub_grid_offset_y + y + ((info->sub_grid_size_y - y) / 2) + ((info->sub_grid_size_y - y) % 2);
                buf[3] = info->sub_grid_offset_x;
                buf[4] = buf[0] * buf[1]; //energy, size_x * size_y
                
                info->sub_grid_size_y = ((info->sub_grid_size_y - y) / 2) + ((info->sub_grid_size_y - y) % 2);
                info->energy.numerator -= buf[4];
                MPI_Send(buf, ULONG_BUFFER_COUNT, MPI_UNSIGNED_LONG, target, TAG_WORK_RESPONSE, MPI_COMM_WORLD);
                /*if (buf[4] + info->energy.numerator != old_num) {
                    printf("Proc %d critical energy split failure!\n");
                    MPI_Abort(MPI_COMM_WORLD, 1);
                }*/
            }
        }
    }
    
    unsigned char* row = malloc(info->sub_grid_size_x);
    for (size_t y = 0; y < info->sub_grid_size_y; y++) {
        for (size_t x = 0; x < info->sub_grid_size_x; x++) {
            if (cabsf(info->sub_grid[y * info->sub_grid_size_x + x]) > 200) {
                row[x] = 0;
            } else {
                row[x] = 255;
            }
        }
        MPI_Status status;
        MPI_File_write_at(info->file, info->sub_grid_offset_x + (info->sub_grid_offset_y + y) * info->total_grid_width, row, info->sub_grid_size_x, MPI_UNSIGNED_CHAR, &status);
    }
    free(row);
    
    if (info->rank != info->num_procs - 1) {
        //printf("Proc %d sending %lu energy to final proc\n", info->rank, info->energy.numerator);
        MPI_Send(&info->energy.numerator, 1, MPI_UNSIGNED_LONG, info->num_procs - 1, TAG_ENERGY, MPI_COMM_WORLD); //drop all the energy to the last proc after finishing a section of the fractal
        //ideally last proc will take the longest and as such is usually collecting last, but even in the event it finishes early this still works
        info->energy.numerator = 0;
    }
    return;
}

/*
Rational rational_halve(Rational in) {
    Rational out = {in.numerator, in.denominator * 2};
    return out;
}

Rational rational_add(Rational lhs, Rational rhs) {
    Rational max, min;
    if (lhs.denominator >= rhs.denominator) {
        max = lhs;
        min = rhs;
    } else {
        max = rhs;
        min = lhs;
    }
    while (min.denominator != max.denominator) {
        min.numerator *= 2;
        min.denominator *= 2;
    }
    
    max.numerator += min.numerator;
    max.denominator += min.denominator;
    return max;
}*/
