#define _POSIX_C_SOURCE 199309L

#include <immintrin.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
    size_t a;
    size_t c;
    size_t g;
    size_t t;
} DNACount;

typedef struct {
    unsigned char *text_start;
    int count;
} ThreadArgs;

DNACount MUTUAL_THREAD_DNA_COUNT;
DNACount MUTUAL_THREAD_SIMD_DNA_COUNT;
pthread_mutex_t mutex;

void uppercase_char(unsigned char *ch);

// ---------- UTILITY MACROS ----------
#define BENCHMARK_UPPERCASE(method_name, func_call)                                 \
    do {                                                                            \
        struct timespec t_start, t_end;                                             \
        clock_gettime(CLOCK_MONOTONIC, &t_start);                                   \
        func_call;                                                                  \
        clock_gettime(CLOCK_MONOTONIC, &t_end);                                     \
        printf("%s:\n", method_name);                                               \
        printf("Elapsed: %f sec\n\n", get_time_diff(t_start, t_end));               \
    } while (0)

#define DNA_SEQ_SWITCH(dna_char, struct_ptr)                    \
    switch (dna_char) {                                         \
         case 'A':                                              \
            (struct_ptr)->a++;                                  \
            break;                                              \
        case 'C':                                               \
            (struct_ptr)->c++;                                  \
            break;                                              \
        case 'G':                                               \
            (struct_ptr)->g++;                                  \
            break;                                              \
        case 'T':                                               \
            (struct_ptr)->t++;                                  \
            break;                                              \
        default:                                                \
            printf("ERROR: INVALID DNA SEQUENCE. Halting!");    \
            exit(EXIT_FAILURE);                                 \
    }
// ---------- END UTILITY MACROS ---------- 


// ---------- SCALAR METHOD---------- 
void dna_count_scalar(unsigned char *buffer, size_t len, DNACount *out) {
    for (size_t i = 0; i < len; i++) {
        DNA_SEQ_SWITCH(buffer[i], out);
    }
}
// ---------- END SCALAR METHOD ---------- 


// ---------- MULTITHREADING METHOD ---------- 
void *thread_job(void *args) {
    ThreadArgs thread_args = *((ThreadArgs*) args);
    unsigned char *buffer = thread_args.text_start;
    int count = thread_args.count;

    for (int i = 0; i < count; i++) {
        uppercase_char(&buffer[i]);
    } 

    return NULL;
}

void uppercase_threads(unsigned char *buffer, size_t len, int num_threads) {
    pthread_t threads[num_threads];
    size_t chunk = len / num_threads;
    
    for (int i = 0; i < num_threads; i++) {
        ThreadArgs *args = malloc(sizeof(ThreadArgs));
        if (args == NULL) {
            perror("Multithreading: malloc failed");
            exit(EXIT_FAILURE);
        }

        args->text_start = buffer + i * chunk;
        args->count = chunk;

        if (pthread_create(&threads[i], NULL, thread_job, args) != 0) {
            perror("Multithreading: pthread_create failed");
            exit(EXIT_FAILURE);
        }
    }

    for (size_t i = num_threads * chunk; i < len; i++) {
        uppercase_char(&buffer[i]);
    }  

    for (int i = 0; i < num_threads; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Multithreading: pthread_join failed");
            exit(EXIT_FAILURE);
        }
    }
}
// ---------- END MULTITHREADING METHOD ---------- 


// ---------- SIMD METHOD ---------- 
void uppercase_simd(unsigned char *buffer, size_t len) {
    size_t i = 0;

    const __m256i target_a = _mm256_set1_epi8('a' - 1);
    const __m256i target_z = _mm256_set1_epi8('z' + 1);

    // it's enough to flip one bit of an ASCII char
    // to convert it from lowercase to uppercase, bit 6
    // this mask sets the 6th bit to 1 for all elements
    const __m256i flip = _mm256_set1_epi8(32); // 0x20

    for (; i < len; i += 32) {
        __m256i v = _mm256_loadu_si256((const __m256i*) (buffer + i));
        __m256i gt = _mm256_cmpgt_epi8(v, target_a);
        __m256i lt = _mm256_cmpgt_epi8(target_z, v);
        __m256i is_lower = _mm256_and_si256(gt, lt);

        // if a character is_lower, then it's 0xFF
        // if not, it's 0 (this is how cmptgt_epi8 did it)
        // 0xFF & 0x20 = 0x20
        // 0x00 & 0x20 = 0x00
        // so, to_flip_mask will have 6th bits on for the characters
        // that are lowercased, and we can easily uppercase them now
        __m256i to_flip_mask = _mm256_and_si256(is_lower, flip);
        v = _mm256_xor_si256(v, to_flip_mask);

        _mm256_storeu_si256((__m256i*)(buffer + i), v);
    }

    // handle the tail sequentially
    for (; i < len; i++) {
        uppercase_char(&buffer[i]);
    }
}
// ---------- END SIMD METHOD ---------- 


// ---------- MULTITHREADED SIMD METHOD ---------- 
void *thread_job_simd(void *args) {
    ThreadArgs thread_args = *((ThreadArgs*) args);
    unsigned char *buffer = thread_args.text_start;
    int count = thread_args.count;

    uppercase_simd(buffer, count);

    return NULL;
}

void uppercase_threads_simd(unsigned char *buffer, size_t len, int num_threads) {
    pthread_t threads[num_threads];
    size_t chunk = len / num_threads;
    
    for (int i = 0; i < num_threads; i++) {
        ThreadArgs *args = malloc(sizeof(ThreadArgs));
        if (args == NULL) {
            perror("Multithreading SIMD: malloc failed");
            exit(EXIT_FAILURE);
        }

        args->text_start = buffer + i * chunk;
        args->count = chunk;

        if (pthread_create(&threads[i], NULL, thread_job_simd, args) != 0) {
            perror("Multithreading SIMD: pthread_create failed");
            exit(EXIT_FAILURE);
        }
    }

    uppercase_simd(buffer + (num_threads * chunk), len - (num_threads * chunk));

    for (int i = 0; i < num_threads; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Multithreading SIMD: pthread_join failed");
            exit(EXIT_FAILURE);
        }
    }
}
// ---------- END MULTITHREADED SIMD METHOD ---------- 


// ---------- HELPER FUNCTIONS ---------- 
unsigned char* load_file_to_buf(const char *file_name, size_t *file_size) {
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

void save_buf_to_file(const unsigned char *buf, size_t buf_size, const char *file_name) {
    FILE *file;

    if (buf == NULL || buf_size == 0 || file_name == NULL) {
        printf("ERROR: Arguments of save_buf_to_file cannot be NULL/0.");
        exit(EXIT_FAILURE);
    }

    file = fopen(file_name, "wb");
    if (file == NULL) {
        perror("fopen failed");
        exit(EXIT_FAILURE);
    }

    if (fwrite(buf, sizeof(unsigned char), buf_size, file) < buf_size) {
        fclose(file);
        perror("fwrite failed");
        exit(EXIT_FAILURE);
    }

    fclose(file);
}

double get_time_diff(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

void uppercase_char(unsigned char *ch) {
    if (*ch >= 'a' && *ch <= 'z') { // a=97, z=122
        *ch -= 32; // move to uppercase range
    }
}
// ---------- END HELPER FUNCTIONS ---------- 

int main() {
    size_t file_size;
    unsigned char *file_buf_mt = load_file_to_buf("random_text.txt", &file_size);
    unsigned char *file_buf_simd = load_file_to_buf("random_text.txt", &file_size);
    unsigned char *file_buf_simd_mt = load_file_to_buf("random_text.txt", &file_size);

    if (file_buf_mt == NULL || file_buf_simd == NULL || file_buf_simd_mt == NULL) {
        printf("ERROR: Loading random_text.txt into memory failed! Halting!");
        exit(EXIT_FAILURE);
    }

    // DNACount scalar_res = {0};
    // BENCHMARK_UPPERCASE(
    //     "Scalar",
    //     dna_count_scalar(file_buf, file_size, &scalar_res)
    // );

    BENCHMARK_UPPERCASE(
        "Multithreaded (4 Threads)",
        uppercase_threads(file_buf_mt, file_size, 4)
    );

    save_buf_to_file(file_buf_mt, file_size, "random_text_out_mt.txt");

    BENCHMARK_UPPERCASE(
        "SIMD (AVX2)",
        uppercase_simd(file_buf_simd, file_size)
    );
 
    BENCHMARK_UPPERCASE(
        "Multithreaded SIMD (AVX2) (4 Threads)",
        uppercase_threads_simd(file_buf_simd_mt, file_size, 4)
    );

    printf("Comparing the three buffers...\n");
    if (memcmp(file_buf_mt, file_buf_simd, file_size) == 0 &&
        memcmp(file_buf_mt, file_buf_simd_mt, file_size) == 0) {
        printf("The buffers are equal to each other!\n");
        printf("To make sure that the output is correct, compare manually.\n");
        printf("E.g. fold -w1 random_text(_out_mt).txt | sort | uniq -c\n");
    }
}
