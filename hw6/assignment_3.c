#define HPC_HW_LIB_IMPLEMENTATION
#include <hpc_hw_lib.h>
#include <omp.h>

#define N 50000000
#define NUM_THREADS 4

double parallel_filtered_sum_reduction(double *A, size_t len) {
    double max = A[0];
    double T;
    double sum = 0;
    #pragma omp parallel
    {
        #pragma omp for reduction(max: max)
        for (int i = 1; i < len; i++) {
            if (max < A[i]) {
                max = A[i];
            }
        }

        #pragma omp single
        {
            T = 0.8 * max;
        }

        #pragma omp for reduction(+:sum)
        for (int i = 0; i < len; i++) {
            sum += A[i] * (A[i] > T);
        }
    }
    
    return sum;
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
        
    double result = parallel_filtered_sum_reduction(A, N);
    printf("Result: %f\n", result);

    free(A);
}

