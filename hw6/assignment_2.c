#define HPC_HW_LIB_IMPLEMENTATION
#include <hpc_hw_lib.h>
#include <omp.h>
#include <math.h>
#include <float.h>

#define N 50000000
#define NUM_THREADS 4

double parallel_mindiff_naive(double *A, size_t len) {
    double min_diff = DBL_MAX;
    #pragma omp parallel for
    for (int i = 0; i < len - 1; i++) {
        double cur_diff = fabs(A[i] - A[i + 1]);
        if (cur_diff < min_diff) {
            min_diff = cur_diff;
        }
    }

    return min_diff;
}

double parallel_mindiff_critical(double *A, size_t len) {
    double min_diff = DBL_MAX;
    #pragma omp parallel for
    for (int i = 0; i < len - 1; i++) {
        double cur_diff = fabs(A[i] - A[i + 1]);
        #pragma omp critical
        if (cur_diff < min_diff) {
            min_diff = cur_diff;
        }
    }

    return min_diff;
}

double parallel_mindiff_reduction(double *A, size_t len) {
    double min_diff = DBL_MAX;
    #pragma omp parallel for reduction(min: min_diff)
    for (int i = 0; i < len - 1; i++) {
        double cur_diff = fabs(A[i] - A[i + 1]);
        if (cur_diff < min_diff) {
            min_diff = cur_diff;
        }
    }

    return min_diff;
}

int main() {
    omp_set_num_threads(NUM_THREADS);
    
    double *A = (double*) malloc(sizeof(double) * N);
    if (A == NULL) {
        die("Array A malloc failed");
    }

    printf("Initializing the large array...\n");
    for (int i = 0; i < N; i++) {
        // 0.0 to 10000.0
        A[i] = ((double) rand() / (double) RAND_MAX) * 10000;
    }
    printf("Array A of size %d initialized!\n", N);
        
    BENCHMARK_ADVANCED(
            "Naive version",
            {},
            double res_naive = parallel_mindiff_naive(A, N),
            printf("Min diff: %f\n", res_naive);
    );

    BENCHMARK_ADVANCED(
            "Critical version",
            {},
            double res_crit = parallel_mindiff_critical(A, N),
            printf("Min diff: %f\n", res_crit);
    );

    BENCHMARK_ADVANCED(
            "Reduction version",
            {},
            double res_red = parallel_mindiff_reduction(A, N),
            printf("Min diff: %f\n", res_red);
    );

    free(A);
}

