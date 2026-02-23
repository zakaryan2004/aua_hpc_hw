/**
 * Parallel Maximum Search
 * Objective: Parallel reduction without synchronization.
 * Instructions:
 * Write a C program that creates a large array of integers (e.g., size = 50 million).
 * Fill the array with random numbers.
 * Find the maximum value sequentially.
 * Find the maximum using 4 threads.
 * Each thread:
 * Searches maximum in its own chunk.
 * Stores local max in a struct passed to the thread.
 * Returns result to main.
 * Main thread:
 * Finds the global maximum from 4 local maximums.
 * Expected Output: Compute and print the final result and print execution time for both versions (sequential and threads).
 */

#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define THREAD_COUNT 4
#define ELEMENT_COUNT 50000000 // 50 million

typedef struct {
    int *arr_start;
    size_t count;
    int thread_max;
} thread_args;

void rand_populate_arr(int *arr, size_t count) {
    for (size_t i = 0; i < count; i++) {
        arr[i] = rand(); // from 0 to RAND_MAX
    }
}

int seq_max_arr(int *arr, size_t count) {
    int max = arr[0];
    for (size_t i = 1; i < count; i++) {
        if (arr[i] > max) {
            max = arr[i];
        }
    }

    return max;
}

void *thread_job(void *arg) {
    thread_args *targs = (thread_args *)arg;
    int local_max = targs->arr_start[0];

    for (size_t i = 1; i < targs->count; i++) {
        if (targs->arr_start[i] > local_max) {
            local_max = targs->arr_start[i];
        }
    }

    targs->thread_max = local_max;

    return NULL;
}

double get_time_diff(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

int main() {
    pthread_t threads[THREAD_COUNT];
    thread_args thread_args_arr[THREAD_COUNT] = {0};
    int total_max = INT_MIN;
    struct timespec start, end;

    int *numbers = malloc(ELEMENT_COUNT * sizeof(int));
    if (numbers == NULL) {
        perror("malloc failed");
        exit(1);
    }

    srand(time(0));
    rand_populate_arr(numbers, ELEMENT_COUNT);

    // calculate sequentially
    clock_gettime(CLOCK_MONOTONIC, &start);
    int seq_result = seq_max_arr(numbers, ELEMENT_COUNT);
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("Sequential: %d\n", seq_result);
    printf("Elapsed: %f\n", get_time_diff(start, end));

    // calculate using threads
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (size_t i = 0; i < THREAD_COUNT; i++) {
        thread_args_arr[i].arr_start =
            numbers + i * (ELEMENT_COUNT / THREAD_COUNT);
        thread_args_arr[i].count = ELEMENT_COUNT / THREAD_COUNT;

        if (pthread_create(&threads[i], NULL, thread_job, &thread_args_arr[i]) != 0) {
            perror("pthread_create failed");
            exit(1);
        }
    }

    for (size_t i = 0; i < THREAD_COUNT; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join failed");
            exit(1);
        }
        if (thread_args_arr[i].thread_max > total_max) {
            total_max = thread_args_arr[i].thread_max;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("Threaded: %d\n", total_max);
    printf("Elapsed: %f\n", get_time_diff(start, end));

    free(numbers);

    return 0;
}
