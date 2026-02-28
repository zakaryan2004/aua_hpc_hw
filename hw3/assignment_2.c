#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#define THREAD_COUNT 6

pthread_t threads[THREAD_COUNT];
pthread_barrier_t barrier;

void* thread_job(void *arg) {
    int id = *(int*) arg;
    int slpt = rand() % 10 + 1;
    printf("Thread %d is getting ready (%d seconds)...\n", id, slpt);
    sleep(slpt);
    printf("Thread %d is ready! Waiting!\n", id);
    pthread_barrier_wait(&barrier);

    return NULL;
}

int main() {
    int thread_ids[THREAD_COUNT];
    // Create a POSIX barirer which all threads (including main) use
    pthread_barrier_init(&barrier, NULL, THREAD_COUNT + 1);
    for (int i = 0; i < THREAD_COUNT; i++) {
        thread_ids[i] = i;
        if (pthread_create(&threads[i], NULL, thread_job, &thread_ids[i]) != 0) {
            perror("pthread_create");
            return 1;
        }
    }

    pthread_barrier_wait(&barrier);
    printf("Game started!\n");

    pthread_barrier_destroy(&barrier);
}

