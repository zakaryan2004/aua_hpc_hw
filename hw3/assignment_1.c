#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#define THREAD_COUNT 6
#define DIE_FACES 6
#define ROUNDS 10

pthread_t threads[THREAD_COUNT];
pthread_barrier_t dice_barrier;
int total_scores[THREAD_COUNT];
int current_scores[THREAD_COUNT];
int winner_thread_idx = -1;
int winner_thread_val = -1;

typedef struct {
    int tid;
} thread_args;

void* roll_die(void *arg) {
    thread_args *targs = (thread_args*) arg;
    srand(time(0) ^ pthread_self());

    for (int i = 0; i < ROUNDS; i++) {
        int rolled_val = rand() % DIE_FACES + 1;
        printf("Thread %d (%ld) Rolled %d\n", targs->tid, pthread_self(), rolled_val);
        current_scores[targs->tid] = rolled_val;
        pthread_barrier_wait(&dice_barrier);
        pthread_barrier_wait(&dice_barrier);
    }

    free(targs);
    return 0;
}


int main() {
    // Create a POSIX barirer which all threads (including main) use
    pthread_barrier_init(&dice_barrier, NULL, THREAD_COUNT + 1);
    for (int i = 0; i < THREAD_COUNT; i++) {
        thread_args *args = malloc(sizeof(thread_args));
        if (args == NULL) {
            perror("malloc");
            return 1;
        }

        args->tid = i;
        if (pthread_create(&threads[i], NULL, roll_die, args) != 0) {
            perror("pthread_create");
            return 1;
        }
    }

    for (int i = 0; i < ROUNDS; i++) {
        pthread_barrier_wait(&dice_barrier);
        int max_thread_val = -1, max_thread_idx = -1;
        for (int i = 0; i < THREAD_COUNT; i++) {
            if(current_scores[i] > max_thread_val) {
                max_thread_val = current_scores[i];
                max_thread_idx = i;
            }
        }
        printf("Round %d: Thread %d won with value %d\n", i + 1, max_thread_idx, max_thread_val);
        total_scores[max_thread_idx]++;
        if (total_scores[max_thread_idx] > winner_thread_val) {
            winner_thread_idx = max_thread_idx;
            winner_thread_val = total_scores[max_thread_idx];
        }
        pthread_barrier_wait(&dice_barrier);
    }

    for (int i = 0; i < THREAD_COUNT; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            return 1;
        }
    }
    
    printf("Thread %d won overall, with %d wins\n", winner_thread_idx, total_scores[winner_thread_idx]);
    pthread_barrier_destroy(&dice_barrier);
}

