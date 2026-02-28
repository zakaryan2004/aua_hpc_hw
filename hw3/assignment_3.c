#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#define THREAD_COUNT 6
#define TIME_INTERVALS 10

pthread_barrier_t barrier;
int current_temps[THREAD_COUNT];

void* roll_die(void *arg) {
    int tid = *((int*) arg);
    srand(time(0) ^ pthread_self());

    for (int i = 0; i < TIME_INTERVALS; i++) {
        int measured = rand() % 31 + 5; // from 5 to 35 degrees Celsius
        printf("Station %d Interval %d: Measured %d\n", tid, i + 1, measured);
        current_temps[tid] = measured;
        pthread_barrier_wait(&barrier); // wait for all threads to finish
        pthread_barrier_wait(&barrier); // wait for the main thread to process
    }

    return 0;
}


int main() {
    pthread_t threads[THREAD_COUNT];
    int thread_ids[THREAD_COUNT];
    int total_sum = 0;
    // Create a POSIX barirer which all threads (including main) use
    pthread_barrier_init(&barrier, NULL, THREAD_COUNT + 1);
    for (int i = 0; i < THREAD_COUNT; i++) {
        thread_ids[i] = i;
        if (pthread_create(&threads[i], NULL, roll_die, &thread_ids[i]) != 0) {
            perror("pthread_create");
            return 1;
        }
    }

    for (int i = 0; i < TIME_INTERVALS; i++) {
        pthread_barrier_wait(&barrier);
        int current_temps_sum = 0;
        for (int j = 0; j < THREAD_COUNT; j++) {
            current_temps_sum += current_temps[j];
        }
        printf("Time Interval %d: Average temperature: %f\n", i + 1, (float) current_temps_sum / THREAD_COUNT);
        total_sum += current_temps_sum;
        pthread_barrier_wait(&barrier);
    }

    for (int i = 0; i < THREAD_COUNT; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            return 1;
        }
    }
    
    printf("Total average temperature: %f\n", (float) total_sum / (THREAD_COUNT * TIME_INTERVALS));
    pthread_barrier_destroy(&barrier);
}

