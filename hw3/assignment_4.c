#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#define THREAD_COUNT 6
#define STAGES 3

pthread_t threads[THREAD_COUNT];
pthread_barrier_t barrier;

void* thread_job(void *arg) {
    int id = *(int*) arg;

    pthread_barrier_wait(&barrier);
    int slpt = rand() % 10 + 1;
    printf("Thread %d: Stage 1 (%d seconds)...\n", id, slpt);
    sleep(slpt);
    printf("Thread %d finished Stage 1! Waiting!\n", id);
    pthread_barrier_wait(&barrier);

    pthread_barrier_wait(&barrier);
    slpt = rand() % 10 + 1;
    printf("Thread %d: Stage 2 (%d seconds)...\n", id, slpt);
    sleep(slpt);
    printf("Thread %d finished Stage 2! Waiting!\n", id);
    pthread_barrier_wait(&barrier);

    pthread_barrier_wait(&barrier);
    slpt = rand() % 10 + 1;
    printf("Thread %d: Stage 3 (%d seconds)...\n", id, slpt);
    sleep(slpt);
    printf("Thread %d finished Stage 3! Waiting!\n", id);
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

    for (int i = 1; i <= STAGES; i++) {
        printf("All threads are starting Stage %d...\n", i);
        pthread_barrier_wait(&barrier);
        pthread_barrier_wait(&barrier);
        printf("Stage %d complete!\n\n", i);
        // pthread_barrier_wait(&barrier);
    }

    pthread_barrier_destroy(&barrier);
}

