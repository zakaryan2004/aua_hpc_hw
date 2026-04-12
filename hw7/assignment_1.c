#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define NUM_THREADS 4

uint64_t fib_seq(int n) {
    if (n < 0) {
        printf("Can't get Fibonacci with negative index!\n");
        exit(EXIT_FAILURE);
    }

    if (n < 2) {
        return n;
    }

    return fib_seq(n - 1) + fib_seq(n - 2);
}

uint64_t fib(int n) {
    if (n <= 10) {
        return fib_seq(n);
    }

    uint64_t n1, n2;

    #pragma omp task shared(n1)
    n1 = fib(n - 1);

    #pragma omp task shared(n2)
    n2 = fib(n - 2);

    #pragma omp taskwait

    return n1 + n2;
}

int main() {
    int num;
    uint64_t result;
    printf("Enter an integer: ");
    scanf("%d", &num);

    #pragma omp parallel num_threads(NUM_THREADS)
    {
        #pragma omp single
        {
            result = fib(num);
        }
    }

    printf("n-th Fibonacci number: %lu\n", result);

    return 0;
}

