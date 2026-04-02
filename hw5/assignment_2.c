#define HPC_HW_LIB_IMPLEMENTATION
#include <hpc_hw_lib.h>
#include <omp.h>
#include <stdio.h>

#define N 10000
#define REQUESTED_NUM_THREADS 4

typedef enum {
    NONE = 0, HIGH, NORMAL
} ClassifiedPriority;

typedef struct {
    int order_id;
    float distance_km;
    ClassifiedPriority priority;
} OrderEntry;

#define tlog(str) printf("[THREAD %d] %s\n", omp_get_thread_num(), str)

int main() {
    OrderEntry *entries = (OrderEntry*) malloc(sizeof(OrderEntry) * N);
    if (entries == NULL) {
        die("OrderEntry malloc failed");
    }
    int shared_threshold_km = 0;
    int thread_high_count[REQUESTED_NUM_THREADS] = {0};

    #pragma omp parallel num_threads(REQUESTED_NUM_THREADS)
    {
        int num_threads = omp_get_num_threads();
        #pragma omp single
        {
            tlog("Initializing all orders...");
            for (int i = 0; i < N; i++) {
                // 0.1km to 50.0km with 100m precision
                float distance = (rand() % 500 + 1) / 10;
                entries[i] = (OrderEntry){
                    .order_id = i,
                    .distance_km = distance,
                    .priority = NONE
                };
            }
            tlog("Orders initialized!");
        }
        // Since we haven't specified nowait, a barrier is
        // automatically added here, so we are sure that 
        // all processing has finished
        
        #pragma omp single
        {
            tlog("Setting shared threshold of 20km...");
            shared_threshold_km = 20.0;
            tlog("Shared threshold set!");
        }

        tlog("Starting order classification...");
        #pragma omp for
        for (int i = 0; i < N; i++) {
            if (entries[i].distance_km < shared_threshold_km) {
                entries[i].priority = HIGH;
            }
            else {
                entries[i].priority = NORMAL;
            }
        }
        // Since we haven't specified nowait, a barrier is
        // automatically added here, so we are sure that 
        // all processing has finished
        
        #pragma omp single
        {
            printf("All threads have finished order classification\n");
        }

        int local_thread_count = 0;
        #pragma omp for
        for (int i = 0; i < N; i++) {
            if (entries[i].priority == HIGH) {
                local_thread_count++;
            }
        }
        thread_high_count[omp_get_thread_num()] = local_thread_count;
        
        #pragma omp barrier

        #pragma omp single
        {
            int count = 0;
            for (int i = 0; i < num_threads; i++) {
                printf("Thread %d:\t%d\n", i, thread_high_count[i]);
                count += thread_high_count[i];
            }
            printf("Total count:\t%d\n", count);
        }
    }

    free(entries);
}

