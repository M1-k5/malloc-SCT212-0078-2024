#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "myalloc.h"

#define NUM_OPS 50000
#define MAX_SIZE 4096

void run_benchmark(fit_policy_t policy, const char *name) {
    set_policy(policy);
    
    void **ptrs = malloc(NUM_OPS * sizeof(void *));
    if (!ptrs) return;
    
    srand(42);
    struct timespec start, end;
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int i = 0; i < NUM_OPS; i++) {
        size_t size = (rand() % MAX_SIZE) + 1;
        ptrs[i] = my_malloc(size);
        
        if (i > 100 && rand() % 2 == 0) {
            int free_idx = rand() % i;
            if (ptrs[free_idx] != NULL) {
                my_free(ptrs[free_idx]);
                ptrs[free_idx] = NULL;
            }
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_spent = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    
    heap_stats_t stats;
    heap_stats(&stats);
    
    double frag = 1.0 - ((double)stats.largest_free / stats.free_bytes);
    
    printf("%s:\n", name);
    printf("  Time taken: %.4f seconds\n", time_spent);
    printf("  Heap Bytes: %zu\n", stats.heap_bytes);
    printf("  Free Bytes: %zu\n", stats.free_bytes);
    printf("  Free Blocks: %zu\n", stats.free_blocks);
    printf("  Largest Free: %zu\n", stats.largest_free);
    printf("  External Fragmentation: %.4f\n\n", frag);
    
    for (int i = 0; i < NUM_OPS; i++) {
        if (ptrs[i]) my_free(ptrs[i]);
    }
    free(ptrs);
}

int main(void) {
    run_benchmark(FIT_FIRST, "First Fit");
    run_benchmark(FIT_BEST, "Best Fit");
    return 0;
}
