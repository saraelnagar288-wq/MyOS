#ifndef MYOS_MEMORY_H
#define MYOS_MEMORY_H

#include <stdint.h>

struct myos_memory_stats {
    uint64_t total_bytes;
    uint64_t heap_start;
    uint64_t heap_end;
    uint64_t next_free;
};

void memory_init(uint32_t mem_upper_kb);
void *kmalloc(uint64_t size, uint64_t alignment);
void memory_stats(struct myos_memory_stats *out);

#endif
