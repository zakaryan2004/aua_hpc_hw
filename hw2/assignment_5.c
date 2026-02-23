/**
 * 5. CPU Core Exploration
 * Objective: Understand scheduling behavior.
 * Instructions:
 * Create N threads.
 * Each thread runs a heavy loop for several seconds. Just have some big number for iterations.
 * Each thread prints:
 *      Thread ID
 *      CPU core ID (sched_getcpu())
 * Observe:
 *      Do threads run on different CPUs?
 *      Does OS migrate threads?
 * Try running with taskset.
 * taskset -c 0 ./my_program
 * If you are using your own laptop with multiple cores, you can also trydifferent combinations:
 * taskset -c 0,1,2,3 ./my_program
 * taskset -c 0,2 ./my_program
 * taskset -c 0-3 ./my_program
 * taskset -c 0-2,5,7 ./my_program
 * Expected Output: Print the thread IDs and CPU numbers.
 *
 */

#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>

#define THREAD_COUNT 8

void *thread_job(void *arg) {
    int thread_id = *((int *)arg);

    int x = 2;
    while (1) {
        x *= x;
        printf("CPU %d runs Thread %d\n", sched_getcpu(), thread_id);
    }

    return NULL;
}

int main() {
    pthread_t threads[THREAD_COUNT];
    int thread_ids[THREAD_COUNT];

    for (size_t i = 0; i < THREAD_COUNT; i++) {
        thread_ids[i] = i;
        if (pthread_create(&threads[i], NULL, thread_job, (void *)&thread_ids[i])) {
            perror("pthread_create failed");
            exit(1);
        }
    }

    for (size_t i = 0; i < THREAD_COUNT; i++) {
        if (pthread_join(threads[i], NULL)) {
            perror("pthread_join failed");
            exit(1);
        }
    }
}
