/*
    hpc_hw_lib.h - A simple stb-style single-header library for the HPC homework
    assignments. Contains utility macros, type definitions, and helper
    functions helpful for different scenarios.

    USAGE:
    Include this header in your .c files and call the functions as needed.

    To provide the implementation, in *one* source file, do this:
        #define HPC_HW_LIB_IMPLEMENTATION
        #include "hpc_hw_lib.h"
    
    By default, without the define, this file only acts as a header file.
*/

#ifndef HPC_HW_LIB_H
#define HPC_HW_LIB_H

#ifndef HPC_HW_DEF
#ifdef HPC_HW_STATIC
#define HPC_HW_DEF static
#else
#define HPC_HW_DEF extern
#endif
#endif

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif

#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

// Public Macros
// Simple Benchmarking macro that takes a method name and a function call, and prints
// the elapsed time for that function call
#define BENCHMARK(method_name, func_call)                                           \
    do {                                                                            \
        struct timespec t_start, t_end;                                             \
        clock_gettime(CLOCK_MONOTONIC, &t_start);                                   \
        func_call;                                                                  \
        clock_gettime(CLOCK_MONOTONIC, &t_end);                                     \
        printf("%s:\n", method_name);                                               \
        printf("Elapsed: %f sec\n\n", hpc_get_time_diff(t_start, t_end));           \
    } while (0)

// Advanced Benchmarking macro that can support setup and teardown code as well
// Usecase example: Allocating memory for the code being benchmarked, printing
// results after the benchmark, etc.
#define BENCHMARK_ADVANCED(method_name, setup_block, benchmark_block, end_block)    \
    do {                                                                            \
        setup_block;                                                                \
        struct timespec t_start, t_end;                                             \
        clock_gettime(CLOCK_MONOTONIC, &t_start);                                   \
        benchmark_block;                                                            \
        clock_gettime(CLOCK_MONOTONIC, &t_end);                                     \
        printf("%s:\n", method_name);                                               \
        end_block;                                                                  \
        printf("Elapsed: %f sec\n\n", hpc_get_time_diff(t_start, t_end));           \
    } while (0)

// Function declarations
HPC_HW_DEF double hpc_get_time_diff(struct timespec start, struct timespec end);
HPC_HW_DEF void die(char *msg);

#ifdef HPC_HW_LIB_IMPLEMENTATION

HPC_HW_DEF double hpc_get_time_diff(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

HPC_HW_DEF void die(char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

#endif // HPC_HW_LIB_IMPLEMENTATION

#endif // HPC_HW_LIB_H
