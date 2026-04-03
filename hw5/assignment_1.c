#define HPC_HW_LIB_IMPLEMENTATION
#include <hpc_hw_lib.h>
#include <omp.h>
#include <stdio.h>

#define N 20000

typedef enum {
    NONE = 0, FAST, MEDIUM, SLOW
} ClassifiedSpeed;

typedef struct {
    int request_id;
    int user_id;
    int response_time_ms;
    ClassifiedSpeed speed;
} LogEntry;

#define tlog(str) printf("[THREAD %d] %s\n", omp_get_thread_num(), str)

int main() {
    LogEntry *entries = (LogEntry*) malloc(sizeof(LogEntry) * N);
    if (entries == NULL) {
        die("LogEntry malloc failed");
    }

    #pragma omp parallel num_threads(4)
    {
        #pragma omp single
        {
            tlog("Initializing all logs...");
            for (int i = 0; i < N; i++) {
                int user_id = rand() % 100;
                int response_time_ms = rand() % 400;
                entries[i] = (LogEntry){
                    .request_id = i,
                    .user_id = user_id,
                    .response_time_ms = response_time_ms,
                    .speed = NONE
                };
            }
            tlog("Logs initialized!");
        }
        // Since we haven't specified nowait, a barrier is
        // automatically added here, so we are sure that
        // all processing has finished
        
        tlog("Starting log processing...");
        #pragma omp for
        for (int i = 0; i < N; i++) {
            if (entries[i].response_time_ms < 100) {
                entries[i].speed = FAST;
            }
            else if (entries[i].response_time_ms <= 300) {
                entries[i].speed = MEDIUM;
            }
            else {
                entries[i].speed = SLOW;
            }
        }
        // A barrier is automatically added here

        #pragma omp single
        {
            int count_f = 0, count_m = 0, count_s = 0;
            for (int i = 0; i < N; i++) {
                switch (entries[i].speed) {
                    case FAST:
                        count_f++;
                        break;
                    case MEDIUM:
                        count_m++;
                        break;
                    case SLOW:
                        count_s++;
                        break;
                    case NONE:
                    default:
                        errno = -1;
                        die("unreachable");
                        break;
                }
            }
            printf("Fast:\t%d\nMedium:\t%d\nSlow:\t%d\n", count_f, count_m, count_s);
        }
    }

    free(entries);
}

