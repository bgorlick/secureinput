#define _POSIX_C_SOURCE 199309L
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "secureinput.h"  // Include your secureinput header file

#define DEFAULT_ITERATIONS 10000
#define WARMUP_ITERATIONS 1000
#define PASSWORD_MAX_LENGTH 64

// Declare the external pwstate variable used by the ASM code
//pw_state_t pwstate;

// External declarations for ASM functions (already declared in your secureinput.h)
extern void *secure_alloc_asm(size_t size);
extern void secure_free_asm(void *ptr, size_t size);

// Utility to perform warmup
void warmup(const char *version) {
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        if (strcmp(version, "c") == 0 || strcmp(version, "both") == 0) {
            void *ptr = secure_alloc(PASSWORD_MAX_LENGTH);
            secure_free(ptr, PASSWORD_MAX_LENGTH);
        }
        if (strcmp(version, "asm") == 0 || strcmp(version, "both") == 0) {
            void *ptr = secure_alloc_asm(PASSWORD_MAX_LENGTH);
            secure_free_asm(ptr, PASSWORD_MAX_LENGTH);
        }
    }
}

// Benchmark C version of secure memory allocation and free separately
double benchmark_secure_alloc_c(int iterations, double *alloc_time, double *free_time, double *total_time) {
    struct timespec start_alloc, end_alloc, start_total, end_total;
    struct timespec start_free, end_free;
    double total_alloc_time = 0.0, total_free_time = 0.0;

    clock_gettime(CLOCK_MONOTONIC, &start_total);  // Start total runtime
    for (int i = 0; i < iterations; i++) {
        clock_gettime(CLOCK_MONOTONIC, &start_alloc);
        void *ptr = secure_alloc(PASSWORD_MAX_LENGTH);
        clock_gettime(CLOCK_MONOTONIC, &end_alloc);
        total_alloc_time += (end_alloc.tv_sec - start_alloc.tv_sec) * 1e9 + (end_alloc.tv_nsec - start_alloc.tv_nsec);

        clock_gettime(CLOCK_MONOTONIC, &start_free);
        secure_free(ptr, PASSWORD_MAX_LENGTH);
        clock_gettime(CLOCK_MONOTONIC, &end_free);
        total_free_time += (end_free.tv_sec - start_free.tv_sec) * 1e9 + (end_free.tv_nsec - start_free.tv_nsec);
    }
    clock_gettime(CLOCK_MONOTONIC, &end_total);  // End total runtime

    *alloc_time = total_alloc_time / iterations / 1e9;  // Average alloc time in seconds
    *free_time = total_free_time / iterations / 1e9;    // Average free time in seconds
    *total_time = (end_total.tv_sec - start_total.tv_sec) * 1e9 + (end_total.tv_nsec - start_total.tv_nsec);  // Total time
    return *alloc_time;
}

// Benchmark ASM version of secure memory allocation and free separately
double benchmark_secure_alloc_asm(int iterations, double *alloc_time, double *free_time, double *total_time) {
    struct timespec start_alloc, end_alloc, start_total, end_total;
    struct timespec start_free, end_free;
    double total_alloc_time = 0.0, total_free_time = 0.0;

    clock_gettime(CLOCK_MONOTONIC, &start_total);  // Start total runtime
    for (int i = 0; i < iterations; i++) {
        clock_gettime(CLOCK_MONOTONIC, &start_alloc);
        void *ptr = secure_alloc_asm(PASSWORD_MAX_LENGTH);
        clock_gettime(CLOCK_MONOTONIC, &end_alloc);
        total_alloc_time += (end_alloc.tv_sec - start_alloc.tv_sec) * 1e9 + (end_alloc.tv_nsec - start_alloc.tv_nsec);

        clock_gettime(CLOCK_MONOTONIC, &start_free);
        secure_free_asm(ptr, PASSWORD_MAX_LENGTH);
        clock_gettime(CLOCK_MONOTONIC, &end_free);
        total_free_time += (end_free.tv_sec - start_free.tv_sec) * 1e9 + (end_free.tv_nsec - start_free.tv_nsec);
    }
    clock_gettime(CLOCK_MONOTONIC, &end_total);  // End total runtime

    *alloc_time = total_alloc_time / iterations / 1e9;  // Average alloc time in seconds
    *free_time = total_free_time / iterations / 1e9;    // Average free time in seconds
    *total_time = (end_total.tv_sec - start_total.tv_sec) * 1e9 + (end_total.tv_nsec - start_total.tv_nsec);  // Total time
    return *alloc_time;
}

// Parse command line arguments for iterations and version
void parse_args(int argc, char *argv[], int *iterations, char *version) {
    *iterations = DEFAULT_ITERATIONS;  // Set default iterations
    strcpy(version, "both");           // Default to benchmarking both C and ASM

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            *iterations = atoi(argv[i + 1]);
            i++;  // Skip next argument since it's the iteration value
        } else if (strcmp(argv[i], "-ver") == 0 && i + 1 < argc) {
            strcpy(version, argv[i + 1]);
            i++;  // Skip next argument since it's the version value
        }
    }
}

int main(int argc, char *argv[]) {
    // Initialize pwstate variable before running the ASM functions
    pwstate.capacity = PASSWORD_MAX_LENGTH;
    pwstate.password = secure_alloc(PASSWORD_MAX_LENGTH);  // Or secure_alloc_asm if preferred

    int iterations;
    char version[10];
    
    // Parse command-line arguments
    parse_args(argc, argv, &iterations, version);

    // Warmup phase to ensure cache and system stability
    printf("Warming up with %d iterations...\n", WARMUP_ITERATIONS);
    warmup(version);

    // Variables to hold total runtime for C and ASM benchmarks
    double alloc_time_c = 0.0, free_time_c = 0.0, total_time_c = 0.0;
    double alloc_time_asm = 0.0, free_time_asm = 0.0, total_time_asm = 0.0;

    // Benchmark based on the selected version
    if (strcmp(version, "c") == 0 || strcmp(version, "both") == 0) {
        printf("Benchmarking C version with %d iterations...\n", iterations);
        benchmark_secure_alloc_c(iterations, &alloc_time_c, &free_time_c, &total_time_c);
        printf("Average time for secure_alloc (C): %.9f seconds\n", alloc_time_c);
        printf("Average time for secure_free (C): %.9f seconds\n", free_time_c);
        printf("Total runtime for C version: %.9f seconds\n", total_time_c / 1e9);
    }

    if (strcmp(version, "asm") == 0 || strcmp(version, "both") == 0) {
        printf("Benchmarking ASM version with %d iterations...\n", iterations);
        benchmark_secure_alloc_asm(iterations, &alloc_time_asm, &free_time_asm, &total_time_asm);
        printf("Average time for secure_alloc_asm (ASM): %.9f seconds\n", alloc_time_asm);
        printf("Average time for secure_free_asm (ASM): %.9f seconds\n", free_time_asm);
        printf("Total runtime for ASM version: %.9f seconds\n", total_time_asm / 1e9);
    }

    // Clean up
    secure_free(pwstate.password, pwstate.capacity);

    return 0;
}
