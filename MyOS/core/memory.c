#include "memory.h"

#define PAGE_SIZE 4096ULL
#define HEAP_BASE 0x02000000ULL

static uint64_t total_bytes;
static uint64_t heap_start;
static uint64_t heap_end;
static uint64_t next_free;

static uint64_t align_up(uint64_t value, uint64_t alignment)
{
    return (value + alignment - 1ULL) & ~(alignment - 1ULL);
}

void memory_init(uint32_t mem_upper_kb)
{
    total_bytes = ((uint64_t)mem_upper_kb + 1024ULL) * 1024ULL;
    heap_start = HEAP_BASE;
    heap_end = total_bytes > HEAP_BASE + PAGE_SIZE
        ? total_bytes
        : HEAP_BASE;
    next_free = heap_start;
}

void *kmalloc(uint64_t size, uint64_t alignment)
{
    uint64_t start;

    if (size == 0 || alignment == 0 || (alignment & (alignment - 1ULL)) != 0)
        return (void *)0;

    start = align_up(next_free, alignment);
    if (start < heap_start || start > heap_end || size > heap_end - start)
        return (void *)0;

    next_free = align_up(start + size, PAGE_SIZE);
    return (void *)(uintptr_t)start;
}

void memory_stats(struct myos_memory_stats *out)
{
    if (!out)
        return;

    out->total_bytes = total_bytes;
    out->heap_start = heap_start;
    out->heap_end = heap_end;
    out->next_free = next_free;
}
