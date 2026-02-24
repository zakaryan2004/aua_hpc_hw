/**
 * Assignment 2: Parallel Array Sum
 * Objective: Create threads, split work across threads, and avoid shared writes
 * Instructions:
 * 1. Write a C program that creates a large array of integers (e.g., size = 50 million).
 * Fill the array with random numbers.
 * Compute the total sum:
 * 1. sequentially
 * 2. using N threads (No global shared counter, No mutex, No atomic variables. Each thread works on a separate index range)
 * Each thread should:
 * 1. Process a separate chunk of the array
 * 2. Store its partial sum in a thread-local variable
 * 3. Return the result to the main thread
 * Main thread should:
 * 1. Collect all partial sums
 * 2. Compute the final result
 * 3. Print execution time for both versions
 * Expected Output: Compute and print the final result, and print execution time for both versions
 */

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define THREAD_COUNT 4
#define ELEMENT_COUNT 50000000 // 50 million

typedef struct {
    int *arr_start;
    size_t count;
    // int64_t thread_sum;
} thread_args;

void rand_populate_arr(int *arr, size_t count) {
    for (size_t i = 0; i < count; i++) {
        arr[i] = (rand() % 1000) + 1;
    }
}

int64_t seq_sum_arr(int *arr, size_t count) {
    int64_t sum = 0;
    for (size_t i = 0; i < count; i++) {
        sum += arr[i];
    }

    return sum;
}

void *thread_job(void *arg) {
    thread_args *targs = (thread_args *)arg;
    int64_t local_sum = 0;

    for (size_t i = 0; i < targs->count; i++) {
        local_sum += targs->arr_start[i];
    }

    int64_t *sum = malloc(sizeof(int64_t));
    if (sum == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    *sum = local_sum;
    return (void*) sum;
}

double get_time_diff(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

int main() {
    pthread_t threads[THREAD_COUNT];
    thread_args thread_args_arr[THREAD_COUNT] = {0};
    int64_t total_sum = 0;
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
    int64_t seq_result = seq_sum_arr(numbers, ELEMENT_COUNT);
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("Sequential: %ld\n", seq_result);
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
        void *thread_sum;
        if (pthread_join(threads[i], &thread_sum) != 0) {
            perror("pthread_join failed");
            exit(1);
        }
        int64_t *partial = (int64_t *) thread_sum;
        total_sum += *partial;
        free(partial);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("Threaded: %ld\n", total_sum);
    printf("Elapsed: %f\n", get_time_diff(start, end));

    free(numbers);

    return 0;
}
