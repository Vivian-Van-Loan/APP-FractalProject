//gcc -fopenmp -o fractal_openmp fractal_openmp.c
//./fractal_openmp

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

int main(int argc, char **argv) {
  const int n_rows = 525;
  const int n_cols = 525;
  const int mid_row = 0.5 * (n_rows - 1);
  const int mid_col = 0.5 * (n_cols - 1);
  const double step = 0.00617;
  const int depth = 100;
  int grid[n_rows][n_cols];
  int row, col, i;
  double local_x, local_y, x, y, tmp;

  #pragma omp parallel for private(row, col, local_x, local_y, i, x, y, tmp)
  for (row = 0; row < n_rows; row++) {
    for (col = 0; col < n_cols; col++) {
      local_x = (col - mid_col) * step;
      local_y = (row - mid_row) * step;

      i = 0;
      x = 0;
      y = 0;
      while ((x*x + y*y < 4) && (i < depth)) {
        tmp = x*x - y*y + local_x;
        y = 2*x*y + local_y;
        x = tmp;
        i++;
      }

      grid[row][col] = i;
    }
  }

  FILE *fout = fopen("fractal_openmp_output.out", "w");
  if (fout == NULL) {
    fprintf(stderr, "Error: could not open output file.\n");
    return 1;
  }

  for (row = 0; row < n_rows; row++) {
    for (col = 0; col < n_cols; col++) {
      fprintf(fout, "%d", grid[row][col]);
      if (col != n_cols - 1) {
        fprintf(fout, ",");
      }
    }
    fprintf(fout, "\n");
  }

  fclose(fout);

  return 0;
}
