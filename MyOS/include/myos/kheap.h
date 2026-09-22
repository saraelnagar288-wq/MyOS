#ifndef MYOS_KHEAP_H
#define MYOS_KHEAP_H

#include <myos/types.h>

#define KHEAP_INITIAL_SIZE (4 * 1024 * 1024) /* 4 MiB initial kernel heap */

typedef struct {
    size_t total_heap;
    size_t used_heap;
    size_t free_heap;
    size_t allocation_count;
} kheap_stats_t;

void kheap_init(void);
void* kmalloc(size_t size);
void* kzalloc(size_t size);
void* kmalloc_aligned(size_t size, size_t alignment);
void* krealloc(void* ptr, size_t new_size);
void kfree(void* ptr);
const kheap_stats_t* kheap_get_stats(void);

#endif /* MYOS_KHEAP_H */
