#include <stdio.h>
#include <omp.h>

#define N 50000000
#define NUM_THREADS 4


int fib(int n) {
    if (n == 0) {
        return 0;
    }
    if (n == 1) {
        return 1;
    }

    int n1, n2;

    #pragma omp task shared(n1) if (n > 10)
    {
        n1 = fib(n - 1);
    }

    #pragma omp task shared(n2) if (n > 10)
    {
        n2 = fib(n - 2);
    }

    #pragma omp taskwait

    return n1 + n2;
}

int main() {
    int num, result;
    printf("Enter an integer: ");
    scanf("%d", &num);

    #pragma omp parallel num_threads(4)
    {
        #pragma omp single
        {
            result = fib(num);
        }
    }

    printf("n-th Fibonacci number: %d\n", result);

    return 0;
}

