/**
 * Assignment 4: Parallel Prime Counting
 * Objective: Parallel CPU-bound workload.
 * Instructions:
 * 1. Count the number of prime numbers from 1 to 20 million.
 * 2. Sequential version first.
 * 3. Parallel version:
 *     * Divide the numeric range into equal intervals
 *     * Each thread counts primes in its interval
 *     * Store result locally
 * 4. Combine in the main.
 * Expected Output: Print the final result and execution time for both versions
 */

#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define THREAD_COUNT 8
#define RANGE_START 1
#define RANGE_END 20000001 // 20 millon

bool is_prime(int num) {
    if (num < 2) {
        return false;
    }

    if (num == 2 || num == 3) {
        return true;
    }

    if (num % 2 == 0 || num % 3 == 0) {
        return false;
    }

    int root = sqrt((double)num) + 1;

    for (int i = 5; i < root; i += 6) {
        if (num % i == 0 || num % (i + 2) == 0) {
            return false;
        }
    }

    return true;
}

int count_primes(int start, int end) {
    int count = 0;
    if (start < 3) {
        start = 3;
        if (end >= 2) {
            count++;
        }
    }
    if (start % 2 == 0) {
        start++;
    }
    for (int i = start; i < end; i += 2) {
        if (is_prime(i)) {
            count++;
        }
    }

    return count;
}

void *thread_count_primes(void *arg) {
    int start = *((int *)arg);
    int end = start + (RANGE_END - RANGE_START) / THREAD_COUNT;
    int result = count_primes(start, end);

    int *ret_count = malloc(sizeof(int));
    *ret_count = result;

    return ret_count;
}

double get_time_diff(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

int main() {
    pthread_t threads[THREAD_COUNT];
    int thread_args[THREAD_COUNT] = {0};
    int total_count = 0;
    struct timespec start, end;

    // calculate sequentially
    clock_gettime(CLOCK_MONOTONIC, &start);
    int seq_result = count_primes(RANGE_START, RANGE_END);
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("Sequential: %d\n", seq_result);
    printf("Elapsed: %f\n", get_time_diff(start, end));

    // calculate using threads
    clock_gettime(CLOCK_MONOTONIC, &start);
    int chunk_size = (RANGE_END - RANGE_START) / THREAD_COUNT;
    for (size_t i = 0; i < THREAD_COUNT; i++) {
        thread_args[i] = i * chunk_size;

        if (pthread_create(&threads[i], NULL, thread_count_primes, (void *)&thread_args[i])) {
            perror("pthread_create failed");
            exit(1);
        }
    }

    for (size_t i = 0; i < THREAD_COUNT; i++) {
        int *result;
        if (pthread_join(threads[i], (void **)&result)) {
            perror("pthread_join failed");
            exit(1);
        }

        total_count += *result;
        free(result);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("Threaded: %d\n", total_count);
    printf("Elapsed: %f\n", get_time_diff(start, end));
}
