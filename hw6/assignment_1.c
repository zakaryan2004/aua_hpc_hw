#define HPC_HW_LIB_IMPLEMENTATION
#include <hpc_hw_lib.h>
#include <omp.h>
#include <stdint.h>
#include <string.h>

#define N 100000000
#define NUM_THREADS 4

void parallel_count_naive(uint8_t *A, size_t len, uint64_t *hist) {
    #pragma omp parallel for
    for (int i = 0 ; i < len; i++) {
        hist[A[i]]++;
    }
}

void parallel_count_critical(uint8_t *A, size_t len, uint64_t *hist) {
    #pragma omp parallel for
    for (int i = 0 ; i < len; i++) {
        #pragma omp critical
        hist[A[i]]++;
    } 
}

void parallel_count_reduction(uint8_t *A, size_t len, uint64_t *hist) {
    #pragma omp parallel for reduction(+:hist[:256])
    for (int i = 0 ; i < len; i++) {
        hist[A[i]]++;
    }
}

int main() {
    omp_set_num_threads(NUM_THREADS);
    
    uint64_t hist_naive[256] = {0};
    uint64_t hist_critical[256] = {0};
    uint64_t hist_reduction[256] = {0};
    uint8_t *A = (uint8_t*) malloc(sizeof(uint8_t) * N);
    if (A == NULL) {
        die("Array A malloc failed");
    }

    printf("Initializing the large array...\n");
    for (int i = 0; i < N; i++) {
        A[i] = rand() % 256;
    }
    printf("Array A of size %d initialized!\n", N);
        
    BENCHMARK_ADVANCED(
            "Naive version",
            {},
            parallel_count_naive(A, N, hist_naive),
            PRINT_ARR(hist_naive, 256, "%zu");
    );

    BENCHMARK_ADVANCED(
            "Critical version",
            {},
            parallel_count_critical(A, N, hist_critical),
            PRINT_ARR(hist_critical, 256, "%zu");
    );

    BENCHMARK_ADVANCED(
            "Reduction version",
            {},
            parallel_count_reduction(A, N, hist_reduction),
            PRINT_ARR(hist_reduction, 256, "%zu");
    );

    if (memcmp(hist_critical, hist_reduction, 256 * sizeof(uint64_t)) != 0) {
        printf("hist_critical and hist_reduction are NOT identical!\n");
        exit(EXIT_FAILURE);
    }

    printf("hist_critical and hist_reduction are identical!\n");

    free(A);
}

