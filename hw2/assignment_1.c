/**
 * Assignment 1:  Basic Thread Creation
 * Objective: Familiarize yourself with creating threads.
 * Instructions:
 * 1. Write a C program that creates three threads.
 * 2. Each thread should print a message, including its thread ID, to
 * indicate it is running (e.g., "Thread X is running").
 * 3. Ensure the main thread waits for each of the threads to complete
 * using pthread_join.
 * Expected Output: Each thread prints a message, and the program exits
 * only after all threads are complete.
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define THREAD_COUNT 3

void *thread_job(void *arg) {
    printf("Thread %d is running\n", *((int *)arg));
    return NULL;
}

int main() {
    pthread_t threads[THREAD_COUNT];
    int thread_args[3];
    for (int i = 0; i < THREAD_COUNT; i++) {
        thread_args[i] = i;
        if (pthread_create(&threads[i], NULL, thread_job, &thread_args[i]) != 0) {
            perror("pthread_create failed");
            exit(1);
        }
    }

    for (int i = 0; i < THREAD_COUNT; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join failed");
            exit(1);
        }
    }

    return 0;
}
