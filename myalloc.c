#define _DEFAULT_SOURCE
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "myalloc.h"

typedef struct block {
    size_t size;
    struct block *next;
    int free;
    int magic;
} block_t;

#define MAGIC 0xDEADBEEF
#define ALIGN 16

static size_t align_up(size_t n) {
    return (n + ALIGN - 1) & ~(ALIGN - 1);
}

#define HDR align_up(sizeof(block_t))
#define MIN_PAYLOAD 16

static block_t *head = NULL;
static block_t *tail = NULL;
static size_t heap_bytes = 0;
static fit_policy_t policy = FIT_FIRST;

static void *payload(block_t *b) {
    return (char *)b + HDR;
}

static block_t *header(void *ptr) {
    return (block_t *)((char *)ptr - HDR);
}
