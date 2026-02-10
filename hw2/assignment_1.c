#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#define THREAD_COUNT 6
#define DIE_FACES 6

pthread_t threads[THREAD_COUNT];
int max_die_val;
int max_die_idx;

typedef struct {
    int tid;
} thread_args;


void* roll_die(void *arg) {
    thread_args *targs = arg;
    srand(time(0) ^ pthread_self());
    int rolled_val = rand() % DIE_FACES + 1;
    if (targs->tid == 69) {
        rolled_val = 51;
    }
    printf("Thread %d (%ld) Rolled %d\n", targs->tid, pthread_self(), rolled_val);
    if (rolled_val > max_die_val) {
        max_die_val = rolled_val;
        max_die_idx = targs->tid;
    }
    return 0;
}

int main() {
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
        
    for (int i = 0; i < THREAD_COUNT; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            return 1;
        }
    }
    printf("Thread %d won with value %d\n", max_die_idx, max_die_val);
}
