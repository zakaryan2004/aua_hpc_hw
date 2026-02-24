#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>


#define THREAD_COUNT 6
#define DIE_FACES 6
#define ROUNDS 10


pthread_t threads[THREAD_COUNT];
pthread_barrier_t dice_barrier;
int scores[THREAD_COUNT];
int max_die_val;
int max_die_idx;
int winner_thread = -1;

typedef struct {
    int tid;
} thread_args;


void* roll_die(void *arg) {
    thread_args *targs = (thread_args*) arg;
    srand(time(0) ^ pthread_self());

    for (int i = 0; i < ROUNDS; i++) {
        int rolled_val = rand() % DIE_FACES + 1;
        printf("Thread %d (%ld) Rolled %d\n", targs->tid, pthread_self(), rolled_val);
        if (rolled_val > max_die_val) {
            max_die_val = rolled_val;
            max_die_idx = targs->tid;
        }
        pthread_barrier_wait(&dice_barrier);
        if (max_die_idx == targs->tid) {
            printf("Thread %d says: I won!\n", targs->tid);
            max_die_idx = -1;
            max_die_val = -1;
            scores[targs->tid]++;
            if (scores[targs->tid] > scores[winner_thread]) {
                winner_thread = targs->tid;
            }
        }
        pthread_barrier_wait(&dice_barrier);
    }

    free(targs);
    return 0;
}


int main() {
    // Create POSIX barirer which all threads (including main) use
    pthread_barrier_init(&dice_barrier, NULL, THREAD_COUNT);
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

    // for (int i = 0; i < ROUNDS; i++) {
        // pthread_join(threads[i], NULL);
        // pthread_barrier_wait(&dice_barrier);
        // printf("Round %d: Thread %d won with value %d\n", i + 1, max_die_idx, max_die_val);
        // scores[i]++;
        // pthread_barrier_wait(&dice_barrier);
    // }

    for (int i = 0; i < THREAD_COUNT; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            return 1;
        }
    }
    
    printf("Thread %d won overall, with %d wins\n", winner_thread, scores[winner_thread]);
}

