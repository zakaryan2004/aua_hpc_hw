#define HPC_HW_LIB_IMPLEMENTATION
#include <hpc_hw_lib.h>

#include <immintrin.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
    size_t a;
    size_t c;
    size_t g;
    size_t t;
} DNACount;

typedef struct {
    const unsigned char *dna_seq_start;
    int count;
} ThreadArgs;

DNACount MUTUAL_THREAD_DNA_COUNT;
DNACount MUTUAL_THREAD_SIMD_DNA_COUNT;
pthread_mutex_t mutex;


// ---------- UTILITY MACROS ----------
#define DNA_COUNTER(dna_char, struct_ptr)        \
    do {                                            \
        (struct_ptr)->a += (dna_char == 'A');       \
        (struct_ptr)->c += (dna_char == 'C');       \
        (struct_ptr)->g += (dna_char == 'G');       \
        (struct_ptr)->t += (dna_char == 'T');       \
    } while (0)
// ---------- END UTILITY MACROS ---------- 


// ---------- SCALAR METHOD---------- 
void dna_count_scalar(const unsigned char *dna_seq, size_t len, DNACount *out) {
    for (size_t i = 0; i < len; i++) {
        DNA_COUNTER(dna_seq[i], out);
    }
}
// ---------- END SCALAR METHOD ---------- 


// ---------- MULTITHREADING METHOD ---------- 
void *thread_job(void *args) {
    ThreadArgs thread_args = *((ThreadArgs*) args);
    const unsigned char *dna_seq = thread_args.dna_seq_start;
    int count = thread_args.count;
    DNACount out = {0};

    for (int i = 0; i < count; i++) {
        DNA_COUNTER(dna_seq[i], &out);
    }  

    pthread_mutex_lock(&mutex);
    MUTUAL_THREAD_DNA_COUNT.a += out.a;
    MUTUAL_THREAD_DNA_COUNT.c += out.c;
    MUTUAL_THREAD_DNA_COUNT.g += out.g;
    MUTUAL_THREAD_DNA_COUNT.t += out.t;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

void dna_count_threads(const unsigned char *dna_seq, size_t len, int num_threads) {
    pthread_t threads[num_threads];
    size_t chunk = len / num_threads;
    DNACount remaining_out = {0};
    
    for (int i = 0; i < num_threads; i++) {
        ThreadArgs *args = malloc(sizeof(ThreadArgs));
        if (args == NULL) {
            perror("Multithreading: malloc failed");
            exit(EXIT_FAILURE);
        }

        args->dna_seq_start = dna_seq + i * chunk;
        args->count = chunk;

        if (pthread_create(&threads[i], NULL, thread_job, args) != 0) {
            perror("Multithreading: pthread_create failed");
            exit(EXIT_FAILURE);
        }
    }


    for (size_t i = num_threads * chunk; i < len; i++) {
        DNA_COUNTER(dna_seq[i], &remaining_out);
    }  

    pthread_mutex_lock(&mutex);
    MUTUAL_THREAD_DNA_COUNT.a += remaining_out.a;
    MUTUAL_THREAD_DNA_COUNT.c += remaining_out.c;
    MUTUAL_THREAD_DNA_COUNT.g += remaining_out.g;
    MUTUAL_THREAD_DNA_COUNT.t += remaining_out.t;
    pthread_mutex_unlock(&mutex);

    for (int i = 0; i < num_threads; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Multithreading: pthread_join failed");
            exit(EXIT_FAILURE);
        }
    }
}
// ---------- END MULTITHREADING METHOD ---------- 


// ---------- SIMD METHOD ---------- 
void dna_count_simd(const unsigned char *dna_seq, size_t len, DNACount *out) {
    const __m256i target_a = _mm256_set1_epi8('A');
    const __m256i target_c = _mm256_set1_epi8('C');
    const __m256i target_g = _mm256_set1_epi8('G');
    const __m256i target_t = _mm256_set1_epi8('T');

    for (size_t i = 0; i < len; i += 32) {
        __m256i v = _mm256_loadu_si256((const __m256i*) (dna_seq + i));

        __m256i eq_a = _mm256_cmpeq_epi8(v, target_a);
        unsigned int mask_a = (unsigned)_mm256_movemask_epi8(eq_a);
        out->a += (uint64_t)__builtin_popcount(mask_a);
        
        __m256i eq_c = _mm256_cmpeq_epi8(v, target_c);
        unsigned int mask_c = (unsigned)_mm256_movemask_epi8(eq_c);
        out->c += (uint64_t)__builtin_popcount(mask_c);
        
        __m256i eq_g = _mm256_cmpeq_epi8(v, target_g);
        unsigned int mask_g = (unsigned)_mm256_movemask_epi8(eq_g);
        out->g += (uint64_t)__builtin_popcount(mask_g);
        
        __m256i eq_t = _mm256_cmpeq_epi8(v, target_t);
        unsigned int mask_t = (unsigned)_mm256_movemask_epi8(eq_t);
        out->t += (uint64_t)__builtin_popcount(mask_t);
    }

    for (size_t i = (len / 32) * 32; i < len; i++) {
        DNA_COUNTER(dna_seq[i], out);
    }
}
// ---------- END SIMD METHOD ---------- 


// ---------- MULTITHREADED SIMD METHOD ---------- 
void *thread_job_simd(void *args) {
    ThreadArgs thread_args = *((ThreadArgs*) args);
    const unsigned char *dna_seq = thread_args.dna_seq_start;
    int count = thread_args.count;
    DNACount out = {0};

    dna_count_simd(dna_seq, count, &out);

    pthread_mutex_lock(&mutex);
    MUTUAL_THREAD_SIMD_DNA_COUNT.a += out.a;
    MUTUAL_THREAD_SIMD_DNA_COUNT.c += out.c;
    MUTUAL_THREAD_SIMD_DNA_COUNT.g += out.g;
    MUTUAL_THREAD_SIMD_DNA_COUNT.t += out.t;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

void dna_count_threads_simd(const unsigned char *dna_seq, size_t len, int num_threads) {
    pthread_t threads[num_threads];
    size_t chunk = len / num_threads;
    DNACount remaining_out = {0};
    
    for (int i = 0; i < num_threads; i++) {
        ThreadArgs *args = malloc(sizeof(ThreadArgs));
        if (args == NULL) {
            perror("Multithreading SIMD: malloc failed");
            exit(EXIT_FAILURE);
        }

        args->dna_seq_start = dna_seq + i * chunk;
        args->count = chunk;

        if (pthread_create(&threads[i], NULL, thread_job_simd, args) != 0) {
            perror("Multithreading SIMD: pthread_create failed");
            exit(EXIT_FAILURE);
        }
    }

    dna_count_simd(dna_seq + (num_threads * chunk), len - (num_threads * chunk), &remaining_out);

    pthread_mutex_lock(&mutex);
    MUTUAL_THREAD_SIMD_DNA_COUNT.a += remaining_out.a;
    MUTUAL_THREAD_SIMD_DNA_COUNT.c += remaining_out.c;
    MUTUAL_THREAD_SIMD_DNA_COUNT.g += remaining_out.g;
    MUTUAL_THREAD_SIMD_DNA_COUNT.t += remaining_out.t;
    pthread_mutex_unlock(&mutex);

    for (int i = 0; i < num_threads; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Multithreading SIMD: pthread_join failed");
            exit(EXIT_FAILURE);
        }
    }
}
// ---------- END MULTITHREADED SIMD METHOD ---------- 


// ---------- HELPER FUNCTIONS ---------- 
const unsigned char* load_file_to_buf(const char *file_name, size_t *file_size) {
    FILE *file;
    unsigned char *buffer;

    if (file_name == NULL || file_size == NULL) {
        printf("ERROR: Arguments of load_file_to_buf cannot be NULL.");
        exit(EXIT_FAILURE);
    }

    file = fopen(file_name, "rb");
    if (file == NULL) {
        perror("fopen failed");
        exit(EXIT_FAILURE);
    }

    fseek(file, 0L, SEEK_END);
    *file_size = ftell(file);
    rewind(file);

    buffer = (unsigned char*) malloc(sizeof(unsigned char) * (*file_size + 1));
    if (buffer == NULL) {
        fclose(file);
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    if (fread(buffer, *file_size, 1, file) != 1) {
        fclose(file);
        free(buffer);
        perror("fread failed");
        exit(EXIT_FAILURE);
    }

    fclose(file);

    buffer[*file_size] = '\0';
    return buffer;
}
// ---------- END HELPER FUNCTIONS ---------- 

int main() {
    size_t file_size;
    const unsigned char *file_buf = load_file_to_buf("dna.txt", &file_size);

    if (file_buf == NULL) {
        printf("ERROR: Loading dna.txt into memory failed! Halting!");
        exit(EXIT_FAILURE);
    }

    BENCHMARK_ADVANCED(
        "Scalar",
        DNACount scalar_res = {0};,
        dna_count_scalar(file_buf, file_size, &scalar_res),
        printf("Counts (A C G T):\n%zu %zu %zu %zu\n",
                scalar_res.a, scalar_res.c, scalar_res.g, scalar_res.t);
    );
    
    BENCHMARK_ADVANCED(
        "Multithreaded (4 Threads)",
        {},
        dna_count_threads(file_buf, file_size, 4),
        printf("Counts (A C G T):\n%zu %zu %zu %zu\n",
                MUTUAL_THREAD_DNA_COUNT.a,
                MUTUAL_THREAD_DNA_COUNT.c,
                MUTUAL_THREAD_DNA_COUNT.g,
                MUTUAL_THREAD_DNA_COUNT.t
        );
    );

    BENCHMARK_ADVANCED(
        "SIMD (AVX2)",
        DNACount simd_res = {0};,
        dna_count_simd(file_buf, file_size, &simd_res),
        printf("Counts (A C G T):\n%zu %zu %zu %zu\n",
                simd_res.a, simd_res.c, simd_res.g, simd_res.t);
    );

    BENCHMARK_ADVANCED(
        "Multithreaded SIMD (AVX2) (4 Threads)",
        {},
        dna_count_threads_simd(file_buf, file_size, 4),
        printf("Counts (A C G T):\n%zu %zu %zu %zu\n",
                MUTUAL_THREAD_SIMD_DNA_COUNT.a,
                MUTUAL_THREAD_SIMD_DNA_COUNT.c,
                MUTUAL_THREAD_SIMD_DNA_COUNT.g,
                MUTUAL_THREAD_SIMD_DNA_COUNT.t
        );
    );
}
