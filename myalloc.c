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
    unsigned int magic;
} block_t;

#define MAGIC 0xDEADBEEFu
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

static block_t *grow(size_t need) {
    size_t chunk = 65536;
    if (need + HDR > chunk) {
        chunk = need + HDR;
    }
    chunk = align_up(chunk);

    void *old_break = sbrk(0);
    if (old_break == (void *)-1) {
        return NULL;
    }

    uintptr_t addr = (uintptr_t)old_break;
    size_t pad = (ALIGN - (addr % ALIGN)) % ALIGN;
    chunk += pad;

    void *new_mem = sbrk(chunk);
    if (new_mem == (void *)-1) {
        return NULL;
    }

    block_t *b = (block_t *)((char *)new_mem + pad);
    b->size = chunk - pad - HDR;
    b->next = NULL;
    b->free = 1;
    b->magic = MAGIC;

    if (tail) {
        tail->next = b;
    } else {
        head = b;
    }
    tail = b;
    
    heap_bytes += chunk;
    return b;
}

static block_t *find_fit(size_t need) {
    block_t *best = NULL;
    size_t best_size = (size_t)-1;
    
    for (block_t *b = head; b != NULL; b = b->next) {
        if (b->free && b->size >= need) {
            if (policy == FIT_FIRST) {
                return b;
            }
            if (b->size < best_size) {
                best = b;
                best_size = b->size;
            }
        }
    }
    return best;
}

void set_policy(fit_policy_t p) {
    policy = p;
}

static void split_block(block_t *b, size_t need) {
    if (b->size >= need + HDR + MIN_PAYLOAD) {
        block_t *rest = (block_t *)((char *)b + HDR + need);
        rest->size = b->size - need - HDR;
        rest->free = 1;
        rest->magic = MAGIC;
        rest->next = b->next;
        
        b->next = rest;
        b->size = need;
        
        if (tail == b) {
            tail = rest;
        }
    }
}

void *my_malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    
    size_t need = align_up(size);
    block_t *b = find_fit(need);
    
    if (b == NULL) {
        b = grow(need);
        if (b == NULL) {
            return NULL;
        }
    }

    split_block(b, need);
    b->free = 0;
    return payload(b);
}

void my_free(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    block_t *b = header(ptr);
    
    if (b->magic != MAGIC) {
        write(2, "my_free: invalid pointer\n", 25);
        return;
    }
    
    if (b->free) {
        write(2, "my_free: double free detected\n", 30);
        return;
    }

    b->free = 1;

    for (block_t *cur = head; cur != NULL; cur = cur->next) {
        while (cur->free && cur->next != NULL && cur->next->free) {
            char *end = (char *)payload(cur) + cur->size;
            if (end == (char *)cur->next) {
                if (tail == cur->next) {
                    tail = cur;
                }
                cur->size += HDR + cur->next->size;
                cur->next = cur->next->next;
            } else {
                break;
            }
        }
    }
}

void *my_calloc(size_t n, size_t size) {
    if (n == 0 || size == 0) {
        return NULL;
    }
    if (size > ((size_t)-1) / n) {
        return NULL;
    }
    
    size_t total = n * size;
    void *ptr = my_malloc(total);
    if (ptr != NULL) {
        memset(ptr, 0, total);
    }
    return ptr;
}

void *my_realloc(void *ptr, size_t size) {
    if (ptr == NULL) {
        return my_malloc(size);
    }
    if (size == 0) {
        my_free(ptr);
        return NULL;
    }

    block_t *b = header(ptr);
    size_t need = align_up(size);

    if (b->size >= need) {
        return ptr;
    }

    if (b->next != NULL && b->next->free) {
        char *end = (char *)payload(b) + b->size;
        if (end == (char *)b->next) {
            if (b->size + HDR + b->next->size >= need) {
                if (tail == b->next) {
                    tail = b;
                }
                b->size += HDR + b->next->size;
                b->next = b->next->next;
                return ptr;
            }
        }
    }

    void *new_ptr = my_malloc(size);
    if (new_ptr == NULL) {
        return NULL;
    }
    
    size_t copy_size = b->size < size ? b->size : size;
    memcpy(new_ptr, ptr, copy_size);
    my_free(ptr);
    
    return new_ptr;
}

void heap_stats(heap_stats_t *out) {
    size_t free_bytes = 0;
    size_t free_blocks = 0;
    size_t largest_free = 0;

    for (block_t *b = head; b != NULL; b = b->next) {
        if (b->free) {
            free_bytes += b->size;
            free_blocks++;
            if (b->size > largest_free) {
                largest_free = b->size;
            }
        }
    }

    out->heap_bytes = heap_bytes;
    out->free_bytes = free_bytes;
    out->free_blocks = free_blocks;
    out->largest_free = largest_free;
}

void heap_dump(void) {
    char buf[128];
    int i = 0;
    for (block_t *b = head; b != NULL; b = b->next) {
        int len = snprintf(buf, sizeof(buf), "[%d] %p size=%zu %s\n", 
                           i++, (void *)b, b->size, b->free ? "FREE" : "USED");
        write(1, buf, len);
    }
}

int heap_check(void) {
    uintptr_t prev_end = 0;
    block_t *prev = NULL;

    for (block_t *b = head; b != NULL; b = b->next) {
        if (b->magic != MAGIC) {
            return 0;
        }
        
        uintptr_t payload_addr = (uintptr_t)payload(b);
        if (payload_addr % ALIGN != 0) {
            return 0;
        }

        if (prev != NULL) {
            if ((uintptr_t)b < prev_end) {
                return 0;
            }
            if (prev->free && b->free) {
                return 0;
            }
        }

        prev_end = payload_addr + b->size;
        prev = b;
    }
    return 1;
}
