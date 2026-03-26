// This version is the "truthful" version of SIMD benchmarks.
// The benchmark also includes the time taken to separate the channels into three separate
// buffers before processing. This is because separating the channels is a very expensive
// operation which must be taken into account when comparing the performance of the SIMD
// method to the scalar/multithreading method.

#define HPC_HW_LIB_IMPLEMENTATION
#include <hpc_hw_lib.h>

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
} PPMPixel;

typedef struct {
    PPMPixel *buf_start;
    size_t buf_size;
} ThreadArgs;

#define NUM_THREADS_FOR_MT_METHOD 4


// ---------- UTILITY MACROS ----------
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define NUM_THREADS_STR STR(NUM_THREADS_FOR_MT_METHOD)
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
    
    fscanf(file, "%*s"); // P6 header, ignore
    fscanf(file, "%d", width);
    fscanf(file, "%d", height);
    fscanf(file, "%d", max_color);

    *buf_size = *width * *height;

    buf = load_ppm_to_buf(file, *buf_size);
    fclose(file);
    return buf;
}

void separate_buf_into_channels(PPMPixel *buf, size_t *buf_size, unsigned char **red_buf,
                                unsigned char **green_buf, unsigned char **blue_buf) {
    size_t aligned_bytes = ALIGN_SIZE(*buf_size, MEM_ALIGNMENT);

    *red_buf = (unsigned char *)aligned_alloc(MEM_ALIGNMENT, aligned_bytes);
    *green_buf = (unsigned char *)aligned_alloc(MEM_ALIGNMENT, aligned_bytes);
    *blue_buf = (unsigned char *)aligned_alloc(MEM_ALIGNMENT, aligned_bytes);

    for (size_t i = 0; i < *buf_size; i++) {
        (*red_buf)[i] = buf[i].r;
        (*green_buf)[i] = buf[i].g;
        (*blue_buf)[i] = buf[i].b;
    }
}

void combine_channels_into_buf(unsigned char *red_buf, unsigned char *green_buf,
                                    unsigned char *blue_buf, PPMPixel *buf,
                                    size_t buf_size) {
    for (size_t i = 0; i < buf_size; i++) {
        buf[i].r = red_buf[i];
        buf[i].g = green_buf[i];
        buf[i].b = blue_buf[i];
    }
}

void write_ppm_file(char *filename, const PPMPixel *buf, size_t buf_size, int width,
                    int height, int max_color) {
    FILE *file = fopen(filename, "wb");

    fprintf(file, "P6\n");
    fprintf(file, "%d %d\n", width, height);
    fprintf(file, "%d\n", max_color);

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
// ---------- END HELPER FUNCTIONS ---------- 


// ---------- SCALAR METHOD---------- 
void grayscale_seq(PPMPixel *buf, size_t buf_size) {
    uint16_t r_coeff = (uint16_t) (0.299 * 256);
    uint16_t g_coeff = (uint16_t) (0.587 * 256);
    uint16_t b_coeff = (uint16_t) (0.114 * 256);

    for (size_t i = 0; i < buf_size; i++) {
        // char gray = (char) (0.299 * buf[i].r + 0.587 * buf[i].g + 0.114 * buf[i].b);
        char gray = (char)((uint16_t)(buf[i].r * r_coeff + buf[i].g * g_coeff +
                                      buf[i].b * b_coeff) >> 8);
        buf[i] = (PPMPixel){.r = gray, .g = gray, .b = gray};
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
// note: x * 0.299 is the same as (x * 256 * 0.299) / 256 ~ (x * 76) >> 8
void grayscale_simd(unsigned char *red_buf, unsigned char *green_buf,
                    unsigned char *blue_buf, size_t buf_size) {
    size_t i = 0;

    const __m256i coeff_r = _mm256_set1_epi16((unsigned char) (0.299 * 256));
    const __m256i coeff_g = _mm256_set1_epi16((unsigned char) (0.587 * 256));
    const __m256i coeff_b = _mm256_set1_epi16((unsigned char) (0.114 * 256));

    // process 32 pixels (96 bytes), but 16 pixels at a time, and then pack the results
    // into one 256-bit register to store back to memory in one write
    for (; i + 31 < buf_size; i += 32) {
        // load 16 pixels (8-bit ints, 48 bytes) into three 128-bit (16-byte) registers
        __m128i r1 = _mm_load_si128((const __m128i*) (red_buf + i));
        __m128i g1 = _mm_load_si128((const __m128i*) (green_buf + i));
        __m128i b1 = _mm_load_si128((const __m128i*) (blue_buf + i));

        // extend 8-bit ints to 16-bit to prevent overflow during multiplication
        __m256i r1_16 = _mm256_cvtepu8_epi16(r1);
        __m256i g1_16 = _mm256_cvtepu8_epi16(g1);
        __m256i b1_16 = _mm256_cvtepu8_epi16(b1);
        
        // multiply by coefficients
        __m256i rc1 = _mm256_mullo_epi16(r1_16, coeff_r);
        __m256i gc1 = _mm256_mullo_epi16(g1_16, coeff_g);
        __m256i bc1 = _mm256_mullo_epi16(b1_16, coeff_b);

        // sum the results and shift right by 8 to divide by 256
        // since we multiplied by 256 at the beginning to convert to integers
        __m256i sum1 = _mm256_add_epi16(_mm256_add_epi16(rc1, gc1), bc1);
        __m256i gray1_16 = _mm256_srli_epi16(sum1, 8);

        // repeat for the next 16 pixels (next 48 bytes)
        __m128i r2 = _mm_load_si128((const __m128i*) (red_buf + i + 16));
        __m128i g2 = _mm_load_si128((const __m128i*) (green_buf + i + 16));
        __m128i b2 = _mm_load_si128((const __m128i*) (blue_buf + i + 16));

        __m256i r2_16 = _mm256_cvtepu8_epi16(r2);
        __m256i g2_16 = _mm256_cvtepu8_epi16(g2);
        __m256i b2_16 = _mm256_cvtepu8_epi16(b2);
        
        __m256i rc2 = _mm256_mullo_epi16(r2_16, coeff_r);
        __m256i gc2 = _mm256_mullo_epi16(g2_16, coeff_g);
        __m256i bc2 = _mm256_mullo_epi16(b2_16, coeff_b);

        __m256i sum2 = _mm256_add_epi16(_mm256_add_epi16(rc2, gc2), bc2);
        __m256i gray2_16 = _mm256_srli_epi16(sum2, 8);

        // Pack 16-bit integers to 8-bit
        __m256i gray = _mm256_packus_epi16(gray1_16, gray2_16);

        // Swap the middle two 64-bit blocks to fix the lane crossing done by packus
        gray = _mm256_permute4x64_epi64(gray, _MM_SHUFFLE(3, 1, 2, 0));

        _mm256_store_si256((__m256i*)(red_buf + i), gray);
        _mm256_store_si256((__m256i*)(green_buf + i), gray);
        _mm256_store_si256((__m256i*)(blue_buf + i), gray);
    }

    // handle the tail sequentially
    uint16_t r_coeff = (uint16_t) (0.299 * 256);
    uint16_t g_coeff = (uint16_t) (0.587 * 256);
    uint16_t b_coeff = (uint16_t) (0.114 * 256);

    for (; i < buf_size; i++) {
        // char gray = (char)(0.299 * red_buf[i] +
        //                  0.587 * green_buf[i] +
        //                  0.114 * blue_buf[i]);
        char gray = (char)((uint16_t)(red_buf[i] * r_coeff + green_buf[i] * g_coeff +
                                      blue_buf[i] * b_coeff) >> 8);

        red_buf[i] = gray;
        green_buf[i] = gray;
        blue_buf[i] = gray;
    }
}

void grayscale_simd_wrapper(PPMPixel *buf, size_t buf_size) {
    unsigned char *red_buf, *green_buf, *blue_buf;
    separate_buf_into_channels(buf, &buf_size, &red_buf, &green_buf, &blue_buf);

    grayscale_simd(red_buf, green_buf, blue_buf, buf_size);

    combine_channels_into_buf(red_buf, green_buf, blue_buf, buf, buf_size);

    free(red_buf);
    free(green_buf);
    free(blue_buf);
}
// ---------- END SIMD METHOD ---------- 


// ---------- MULTITHREADED SIMD METHOD ---------- 
void *thread_job_simd(void *arg) {
    ThreadArgs thread_args = *((ThreadArgs*) arg);
    PPMPixel *buf = thread_args.buf_start;
    size_t buf_size = thread_args.buf_size;

    grayscale_simd_wrapper(buf, buf_size);

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

    grayscale_simd_wrapper(buf + (NUM_THREADS_FOR_MT_METHOD * chunk),
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
        load_ppm_file("cat.ppm", &buf_size, &width, &height, &max_color);
    PPMPixel *file_buf_mt =
        (PPMPixel *)copy_buf((unsigned char *)file_buf_seq, buf_size * 3);
    PPMPixel *file_buf_simd =
        (PPMPixel *)copy_buf((unsigned char *)file_buf_seq, buf_size * 3);
    PPMPixel *file_buf_simd_mt =
        (PPMPixel *)copy_buf((unsigned char *)file_buf_seq, buf_size * 3);

    if (file_buf_seq == NULL || file_buf_mt == NULL || file_buf_simd == NULL ||
        file_buf_simd_mt == NULL) {
        printf("ERROR: Loading cat.ppm into memory failed! Halting!");
        exit(EXIT_FAILURE);
    }

    BENCHMARK("Scalar", grayscale_seq(file_buf_seq, buf_size));
    write_ppm_file("cat_gray_seq.ppm", file_buf_seq, buf_size, width, height, max_color);

    BENCHMARK("Multithreaded ("NUM_THREADS_STR" Threads)",
                  grayscale_threads(file_buf_mt, buf_size));
    write_ppm_file("cat_gray_mt.ppm", file_buf_mt, buf_size, width, height, max_color);

    BENCHMARK("SIMD (AVX2)",
                  grayscale_simd_wrapper(file_buf_simd, buf_size));
    write_ppm_file("cat_gray_simd.ppm", file_buf_simd, buf_size, width, height,
                   max_color);

    BENCHMARK("Multithreaded SIMD (AVX2) ("NUM_THREADS_STR" Threads)",
                  grayscale_threads_simd(file_buf_simd_mt, buf_size));
    write_ppm_file("cat_gray_simd_mt.ppm", file_buf_simd_mt, buf_size, width, height,
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
