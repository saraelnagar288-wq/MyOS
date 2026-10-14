#include "memory.h"

#define PAGE_SIZE 4096ULL
#define HEAP_BASE 0x02000000ULL

static uint64_t total_bytes;
static uint64_t heap_start;
static uint64_t heap_end;
static uint64_t next_free;

/* Return 0 on overflow instead of wrapping an address back into low memory. */
static int align_up_checked(uint64_t value, uint64_t alignment, uint64_t *out)
{
    uint64_t remainder;
    uint64_t increment;

    if (!out || alignment == 0 || (alignment & (alignment - 1ULL)) != 0)
        return 0;

    remainder = value & (alignment - 1ULL);
    increment = remainder ? alignment - remainder : 0;
    if (value > UINT64_MAX - increment)
        return 0;

    *out = value + increment;
    return 1;
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
    uint64_t end;
    uint64_t next;

    if (size == 0 || alignment == 0 ||
        (alignment & (alignment - 1ULL)) != 0)
        return (void *)0;

    if (!align_up_checked(next_free, alignment, &start))
        return (void *)0;

    if (start < heap_start || start > heap_end || size > heap_end - start)
        return (void *)0;

    /* Guard both additions before calculating the allocation end and page end. */
    if (start > UINT64_MAX - size)
        return (void *)0;
    end = start + size;

    if (!align_up_checked(end, PAGE_SIZE, &next))
        return (void *)0;

    /* Keep allocator state valid even when the last allocation ends at heap_end. */
    if (next > heap_end)
        next = heap_end;
    next_free = next;

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
