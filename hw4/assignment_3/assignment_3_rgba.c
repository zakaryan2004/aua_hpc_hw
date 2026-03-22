// This version uses PAM format (P7) images instead of P6 so that we can have an alpha
// channel, which allows us to have 32-bit pixels that works nicely with SIMD operations
// The SIMD of this version won't separate the colors into channels because that's a
// very expensive operation, and instead will just load interleaved RGBA data into SIMD
// registers and process them directly.

#define _POSIX_C_SOURCE 199309L

#include <immintrin.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} PPMPixel;

typedef struct {
    PPMPixel *buf_start;
    size_t buf_size;
} ThreadArgs;

#define NUM_THREADS_FOR_MT_METHOD 4
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define NUM_THREADS_STR STR(NUM_THREADS_FOR_MT_METHOD)


// ---------- UTILITY MACROS ----------
#define BENCHMARK_PPM(method_name, func_call)                                       \
    do {                                                                            \
        struct timespec t_start, t_end;                                             \
        clock_gettime(CLOCK_MONOTONIC, &t_start);                                   \
        func_call;                                                                  \
        clock_gettime(CLOCK_MONOTONIC, &t_end);                                     \
        printf("%s:\n", method_name);                                               \
        printf("Elapsed: %f sec\n\n", get_time_diff(t_start, t_end));               \
    } while (0)
// ---------- END UTILITY MACROS ---------- 


// ---------- HELPER FUNCTIONS ----------
// Helper macro to pad the allocation size to a multiple of the alignment
#define ALIGN_SIZE(size, alignment) (((size) + ((alignment) - 1)) & ~((alignment) - 1))
#define MEM_ALIGNMENT 32 // 32 bytes for AVX2

// This function intentionally loads file from the given position,
// not the beginning of the file, so we can ignore the header easier
PPMPixel* load_ppm_to_buf(FILE *file, size_t buf_size) {
    PPMPixel *buffer;

    if (file == NULL || buf_size == 0) {
        printf("ERROR: Arguments of load_ppm_to_buf cannot be NULL.");
        exit(EXIT_FAILURE);
    }

    // Calculate total bytes and pad to the nearest multiple of 32
    size_t total_bytes = buf_size * sizeof(PPMPixel);
    size_t aligned_bytes = ALIGN_SIZE(total_bytes, MEM_ALIGNMENT);

    // we don't need this to be null-terminated since it's binary image data
    buffer = (PPMPixel*) aligned_alloc(MEM_ALIGNMENT, aligned_bytes);
    if (buffer == NULL) {
        fclose(file);
        perror("File load: aligned_alloc failed");
        exit(EXIT_FAILURE);
    }

    if (fread(buffer, sizeof(PPMPixel), buf_size, file) != buf_size) {
        fclose(file);
        free(buffer);
        perror("File load: fread failed");
        exit(EXIT_FAILURE);
    }

    return buffer;
}

void save_buf_to_file(const PPMPixel *buf, size_t buf_size, FILE *file) {
    if (buf == NULL || buf_size == 0 || file == NULL) {
        printf("ERROR: Arguments of save_buf_to_file cannot be NULL/0.");
        exit(EXIT_FAILURE);
    }

    if (fwrite(buf, sizeof(PPMPixel), buf_size, file) < buf_size) {
        fclose(file);
        perror("fwrite failed");
        exit(EXIT_FAILURE);
    }
}

PPMPixel* load_ppm_file(char *filename, size_t *buf_size, int *width, int *height,
                        int *max_color) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("fopen failed");
        exit(EXIT_FAILURE);
    }
    PPMPixel *buf;

    fscanf(file, "%*s\n"); // P7 header, ignore
    fscanf(file, "WIDTH %d\n", width);
    fscanf(file, "HEIGHT %d\n", height);
    fscanf(file, "DEPTH %*d\n"); // ignore depth since we know it's 4 for RGBA
    fscanf(file, "MAXVAL %d\n", max_color);
    fscanf(file, "TUPLTYPE %*s\n"); // ignore tupltype
    fscanf(file, "ENDHDR\n"); // ignore end of header

    *buf_size = *width * *height;

    buf = load_ppm_to_buf(file, *buf_size);
    fclose(file);
    return buf;
}

void write_ppm_file(char *filename, const PPMPixel *buf, size_t buf_size, int width,
                    int height, int max_color) {
    FILE *file = fopen(filename, "wb");

    fprintf(file, "P7\n");
    fprintf(file, "WIDTH %d\n", width);
    fprintf(file, "HEIGHT %d\n", height);
    fprintf(file, "DEPTH 4\n");
    fprintf(file, "MAXVAL %d\n", max_color);
    fprintf(file, "TUPLTYPE RGB_ALPHA\n");
    fprintf(file, "ENDHDR\n");

    save_buf_to_file(buf, buf_size, file);

    fclose(file);
}

unsigned char* copy_buf(unsigned char *buf, size_t buf_size) {
    size_t aligned_bytes = ALIGN_SIZE(buf_size, MEM_ALIGNMENT);

    unsigned char *new_buf = (unsigned char *)aligned_alloc(MEM_ALIGNMENT, aligned_bytes);
    if (new_buf == NULL) {
        perror("copy_buf: aligned_alloc failed");
        exit(EXIT_FAILURE);
    }

    memcpy(new_buf, buf, buf_size);
    return new_buf;
}

double get_time_diff(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

void warmup_pam_buffer(PPMPixel *buf, size_t buf_size) {
    volatile unsigned long accum = 0;
    for (size_t i = 0; i < buf_size; i++) {
        accum += buf[i].r + buf[i].g + buf[i].b + buf[i].a;
    }
    if (accum == 0xFFFFFFFFul) {
        printf("\n");
    }
}
// ---------- END HELPER FUNCTIONS ---------- 


// ---------- SCALAR METHOD---------- 
void grayscale_seq(PPMPixel *buf, size_t buf_size) {
    uint16_t r_coeff = (uint16_t) (0.299 * 128);
    uint16_t g_coeff = (uint16_t) (0.587 * 128);
    uint16_t b_coeff = (uint16_t) (0.114 * 128);

    for (size_t i = 0; i < buf_size; i++) {
        // char gray = (char) (0.299 * buf[i].r + 0.587 * buf[i].g + 0.114 * buf[i].b);
        char gray = (char)((uint16_t)(buf[i].r * r_coeff + buf[i].g * g_coeff +
                                      buf[i].b * b_coeff) >> 7);
        buf[i] = (PPMPixel){.r = gray, .g = gray, .b = gray, .a = buf[i].a};
    }
}
// ---------- END SCALAR METHOD ---------- 


// ---------- MULTITHREADED METHOD ---------- 
void *thread_job(void *arg) {
    ThreadArgs thread_args = *((ThreadArgs*) arg);
    PPMPixel *buf = thread_args.buf_start;
    size_t buf_size = thread_args.buf_size;

    grayscale_seq(buf, buf_size);

    return NULL;
}

void grayscale_threads(PPMPixel *buf, size_t buf_size) {
    pthread_t threads[NUM_THREADS_FOR_MT_METHOD];
    size_t chunk = buf_size / NUM_THREADS_FOR_MT_METHOD;

    for (int i = 0; i < NUM_THREADS_FOR_MT_METHOD; i++) {
        ThreadArgs *args = malloc(sizeof(ThreadArgs));
        if (args == NULL) {
            perror("Multithreaded: malloc failed");
            exit(EXIT_FAILURE);
        }

        args->buf_start = buf + i * chunk;
        args->buf_size = chunk;

        if (pthread_create(&threads[i], NULL, thread_job, args) != 0) {
            perror("Multithreaded: pthread_create failed");
            exit(EXIT_FAILURE);
        }
    }

    grayscale_seq(buf + (NUM_THREADS_FOR_MT_METHOD * chunk),
                  buf_size - (NUM_THREADS_FOR_MT_METHOD * chunk));

    for (int i = 0; i < NUM_THREADS_FOR_MT_METHOD; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Multithreaded: pthread_join failed");
            exit(EXIT_FAILURE);
        }
    }
}
// ---------- END MULTITHREADED METHOD ----------


// ---------- SIMD METHOD ----------
// char gray = (char) (0.299 * buf[i] + 0.587 * buf[i + 1] + 0.114 * buf[i + 2]);
// note: x * 0.299 is the same as (x * 128 * 0.299) / 128 ~ (x * 38) >> 7
void grayscale_simd(PPMPixel *buf, size_t buf_size) {
    size_t i = 0;

    uint16_t coeff_r = (uint16_t) (0.299 * 128); // 38
    uint16_t coeff_g = (uint16_t) (0.587 * 128); // 75
    uint16_t coeff_b = (uint16_t) (0.114 * 128); // 14
    
    // multiply mask
    const __m256i coeffs = _mm256_setr_epi8(
        coeff_r, coeff_g, coeff_b, 0,   coeff_r, coeff_g, coeff_b, 0,
        coeff_r, coeff_g, coeff_b, 0,   coeff_r, coeff_g, coeff_b, 0,
        coeff_r, coeff_g, coeff_b, 0,   coeff_r, coeff_g, coeff_b, 0,
        coeff_r, coeff_g, coeff_b, 0,   coeff_r, coeff_g, coeff_b, 0
    );

    // shuffle mask
    // _mm256_shuffle_epi8 will turn -1 into 0 
    const __m256i rgb_shuffle_mask = _mm256_setr_epi8(
        0, 0, 0, -1,  4, 4, 4, -1,  8, 8, 8, -1,  12, 12, 12, -1,
        0, 0, 0, -1,  4, 4, 4, -1,  8, 8, 8, -1,  12, 12, 12, -1
    );

    // 0xFF000000 is 255 in the last byte and 0 on the rest (little-endian)
    const __m256i alpha_mask = _mm256_set1_epi32(0xFF000000);

    // process 8 pixels at a time
    for (; i + 7 < buf_size; i += 8) {
        // load 8 pixels (32 bits) into 128-bit register
        __m256i pixels = _mm256_load_si256((const __m256i*)(buf + i));
        // __m128i px_8 = _mm_load_si128((const __m128i*)(buf + i));

        // use the _mm256_maddubs_epi16 (multiply-add) instruction
        // Multiply packed 8-bit ints in a and b, producing 16-bit ints.
        // Horizontally add adjacent pairs of 16-bit ints and return the result.
        // The result will contain 16 16-bit integers, like this
        // [R1*38+G1*75, B1*14+0, R2*38+G2*75, B2*14+0,
        //  R3*38+G3*75, B3*14+0, R4*38+G4*75, B4*14+0,
        //  R5*38+G5*75, B5*14+0, R6*38+G6*75, B6*14+0,
        //  R7*38+G7*75, B7*14+0, R8*38+G8*75, B8*14+0]
        __m256i partial_sum = _mm256_maddubs_epi16(pixels, coeffs);
        // __m256i partial_sum = _mm256_madd_epi16(px_16, coeffs);
    
        // Shift right by 2 bytes (16 bits) to line up the (B*14) under the (R*38+G*75)
        __m256i shifted = _mm256_srli_si256(partial_sum, 2);

        // Vertically add them together. Because of the shift, there will be garbage
        // values in every other position.
        // But we only need every 4 bytes, which will have correct values
        __m256i sum16 = _mm256_add_epi16(partial_sum, shifted);

        // After that we need to shift right by 7 to divide by 128
        // since we multiplied by 128 at the beginning to convert to integers
        __m256i gray16 = _mm256_srli_epi16(sum16, 7);

        // Current result: [gray1, 0, gray2, 0, gray3, 0, gray4, 0, ...]
        // Using shuffle: [gray1, gray1, gray1, 0, gray2, gray2, gray2, 0, ...]
        __m256i gray_rgb = _mm256_shuffle_epi8(gray16, rgb_shuffle_mask);

        // Use OR to "blend" the grayscale values with 255 in the last byte
        __m256i gray8_rgba = _mm256_or_si256(gray_rgb, alpha_mask);

        // Store 256 bits back to memory
        _mm256_store_si256((__m256i*)(buf + i), gray8_rgba);
    }

    // handle the tail sequentially
    for (; i < buf_size; i++) {
        char gray = (char)((uint16_t)(buf[i].r * coeff_r + buf[i].g * coeff_g +
                                      buf[i].b * coeff_b) >> 7);

        buf[i] = (PPMPixel){.r = gray, .g = gray, .b = gray, .a = 255};
    }
}
// ---------- END SIMD METHOD ---------- 


// ---------- MULTITHREADED SIMD METHOD ---------- 
void *thread_job_simd(void *arg) {
    ThreadArgs thread_args = *((ThreadArgs*) arg);
    PPMPixel *buf = thread_args.buf_start;
    size_t buf_size = thread_args.buf_size;

    grayscale_simd(buf, buf_size);

    return NULL;
}

void grayscale_threads_simd(PPMPixel *buf, size_t buf_size) {
    pthread_t threads[NUM_THREADS_FOR_MT_METHOD];
    size_t chunk = buf_size / NUM_THREADS_FOR_MT_METHOD;

    for (int i = 0; i < NUM_THREADS_FOR_MT_METHOD; i++) {
        ThreadArgs *args = malloc(sizeof(ThreadArgs));
        if (args == NULL) {
            perror("Multithreaded SIMD: malloc failed");
            exit(EXIT_FAILURE);
        }

        args->buf_start = buf + i * chunk;
        args->buf_size = chunk;

        if (pthread_create(&threads[i], NULL, thread_job_simd, args) != 0) {
            perror("Multithreaded SIMD: pthread_create failed");
            exit(EXIT_FAILURE);
        }
    }

    grayscale_simd(buf + (NUM_THREADS_FOR_MT_METHOD * chunk),
                  buf_size - (NUM_THREADS_FOR_MT_METHOD * chunk));

    for (int i = 0; i < NUM_THREADS_FOR_MT_METHOD; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Multithreaded SIMD: pthread_join failed");
            exit(EXIT_FAILURE);
        }
    }
}
// ---------- END MULTITHREADED SIMD METHOD ---------- 


int main() {
    size_t buf_size;
    int width, height, max_color;

    PPMPixel *file_buf_seq =
        load_ppm_file("cat_rgba.pam", &buf_size, &width, &height, &max_color);
    PPMPixel *file_buf_mt =
        (PPMPixel *)copy_buf((unsigned char *)file_buf_seq, buf_size * sizeof(PPMPixel));
    PPMPixel *file_buf_simd =
        (PPMPixel *)copy_buf((unsigned char *)file_buf_seq, buf_size * sizeof(PPMPixel));
    PPMPixel *file_buf_simd_mt =
        (PPMPixel *)copy_buf((unsigned char *)file_buf_seq, buf_size * sizeof(PPMPixel));

    if (file_buf_seq == NULL || file_buf_mt == NULL || file_buf_simd == NULL ||
        file_buf_simd_mt == NULL) {
        printf("ERROR: Loading cat_rgba.pam into memory failed! Halting!");
        exit(EXIT_FAILURE);
    }

    printf("Warming up the buffers...\n");
    warmup_pam_buffer(file_buf_seq, buf_size);
    warmup_pam_buffer(file_buf_mt, buf_size);
    warmup_pam_buffer(file_buf_simd, buf_size);
    warmup_pam_buffer(file_buf_simd_mt, buf_size);
    printf("Warmup done!\n\n");

    BENCHMARK_PPM(
        "Scalar",
        grayscale_seq(file_buf_seq, buf_size)
    );
    write_ppm_file("cat_rgba_gray_seq.pam", file_buf_seq, buf_size, width, height,
                   max_color);

    BENCHMARK_PPM("Multithreaded ("NUM_THREADS_STR" Threads)",
                  grayscale_threads(file_buf_mt, buf_size));
    write_ppm_file("cat_rgba_gray_mt.pam", file_buf_mt, buf_size, width, height,
                   max_color);

    BENCHMARK_PPM("SIMD (AVX2)", grayscale_simd(file_buf_simd, buf_size));
    write_ppm_file("cat_rgba_gray_simd.pam", file_buf_simd, buf_size, width, height,
                   max_color);

    BENCHMARK_PPM("Multithreaded SIMD (AVX2) ("NUM_THREADS_STR" Threads)",
                  grayscale_threads_simd(file_buf_simd_mt, buf_size));
    write_ppm_file("cat_rgba_gray_simd_mt.pam", file_buf_simd_mt, buf_size, width, height,
                   max_color);

    printf("Comparing the four buffers...\n");
    int all_equal = 1;
    if (memcmp(file_buf_seq, file_buf_mt, buf_size * sizeof(PPMPixel)) != 0) {
        all_equal = 0;
        printf("The sequential and multithreaded buffers are NOT equal to each other!\n");
    }

    if (memcmp(file_buf_seq, file_buf_simd, buf_size * sizeof(PPMPixel)) != 0) {
        all_equal = 0;
        printf("The sequential and SIMD buffers are NOT equal to each other!\n");
    }

    if (memcmp(file_buf_seq, file_buf_simd_mt, buf_size * sizeof(PPMPixel)) != 0) {
        all_equal = 0;
        printf("The sequential and SIMD-MT buffers are NOT equal to each other!\n");
    }

    if (all_equal) {
        printf("The buffers are equal to each other!\n");
    }

    free(file_buf_seq);
    free(file_buf_mt);
    free(file_buf_simd);
    free(file_buf_simd_mt);
}
