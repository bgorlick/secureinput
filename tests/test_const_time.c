#define _POSIX_C_SOURCE 199309L
#include "test_const_time.h"
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <math.h>

#define NUM_TRIALS 100000
#define MAX_TEST_LENGTH 64
#define WARMUP_ITERATIONS 4000
#define NUM_RUNS 5

// Implementation of test_get_password_input
int test_get_password_input(pw_state_t *pw_state, const char *prompt) {
    (void)prompt; // Unused in this test version
    size_t i;
    for (i = 0; i < pw_state->capacity - 1; i++) {
        if (i >= pw_state->length) {
            pw_state->password[i] = '\0';
        }
    }
    pw_state->password[pw_state->capacity - 1] = '\0';
    return 0;
}

// Implementation of test_getpw_asm
void test_getpw_asm() {
    size_t i;
    for (i = 0; i < pwstate.capacity - 1; i++) {
        if (i >= pwstate.length) {
            pwstate.password[i] = '\0';
        }
    }
    pwstate.password[pwstate.capacity - 1] = '\0';
}

double measure_time(const char* input, void (*test_func)(pw_state_t*, const char*)) {
    struct timespec start, end;
    
    // Simulate input
    strcpy(pwstate.password, input);
    pwstate.length = strlen(input);

    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // Call the test function
    test_func(&pwstate, "");

    clock_gettime(CLOCK_MONOTONIC, &end);
    return (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
}

void warmup(void (*test_func)(pw_state_t*, const char*)) {
    char input[MAX_TEST_LENGTH + 1] = {0};
    memset(input, 'a', MAX_TEST_LENGTH);
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        measure_time(input, test_func);
    }
}

typedef struct {
    double mean;
    double std_dev;
} test_result_t;

test_result_t run_test(const char* input, void (*test_func)(pw_state_t*, const char*)) {
    double times[NUM_TRIALS * NUM_RUNS];
    int idx = 0;

    for (int run = 0; run < NUM_RUNS; run++) {
        for (int trial = 0; trial < NUM_TRIALS; trial++) {
            times[idx++] = measure_time(input, test_func);
        }
        // Progress indicator
        fprintf(stderr, ".");
        fflush(stderr);
    }
    fprintf(stderr, "\n");

    // Calculate mean
    double sum = 0;
    for (int i = 0; i < NUM_TRIALS * NUM_RUNS; i++) {
        sum += times[i];
    }
    double mean = sum / (NUM_TRIALS * NUM_RUNS);

    // Calculate standard deviation
    double sum_sq_diff = 0;
    for (int i = 0; i < NUM_TRIALS * NUM_RUNS; i++) {
        double diff = times[i] - mean;
        sum_sq_diff += diff * diff;
    }
    double std_dev = sqrt(sum_sq_diff / (NUM_TRIALS * NUM_RUNS));

    return (test_result_t){mean, std_dev};
}

void run_tests(const char* impl_name, void (*test_func)(pw_state_t*, const char*)) {
    printf("Testing %s implementation:\n", impl_name);
    
    // Warmup
    warmup(test_func);
    
    for (int length = 1; length <= MAX_TEST_LENGTH; length++) {
        char input[MAX_TEST_LENGTH + 1] = {0};
        memset(input, 'a', length);

        test_result_t result = run_test(input, test_func);
        printf("Length %2d: Average time = %.2f ns, Std Dev = %.2f ns\n", 
               length, result.mean, result.std_dev);
    }
    printf("\n");
}

void c_wrapper(pw_state_t* state, const char* prompt) {
    test_get_password_input(state, prompt);
}

void asm_wrapper(pw_state_t* state, const char* prompt) {
    (void)state;  // Unused in this wrapper
    (void)prompt; // Unused in this wrapper
    test_getpw_asm();
}

void check_timing_resolution() {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double resolution = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    printf("Timing resolution: %.2f ns\n\n", resolution);
}

int main() {
    if (init_pw_state(&pwstate) != 0) {
        fprintf(stderr, "Failed to initialize password state\n");
        return 1;
    }

    check_timing_resolution();

    run_tests("C", c_wrapper);
    run_tests("ASM", asm_wrapper);

    free_pw_state(&pwstate);
    return 0;
}