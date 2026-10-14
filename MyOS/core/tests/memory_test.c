#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "memory.h"

int main(void)
{
    struct myos_memory_stats stats;
    void *first;
    void *aligned;

    memory_init(64U * 1024U); /* 64 MiB above the first MiB. */
    memory_stats(&stats);
    assert(stats.total_bytes > stats.heap_start);
    assert(stats.heap_end > stats.heap_start);
    assert(stats.next_free == stats.heap_start);

    assert(kmalloc(0, 8) == 0);
    assert(kmalloc(8, 0) == 0);
    assert(kmalloc(8, 3) == 0);

    first = kmalloc(1, 1);
    assert(first != 0);
    assert((uintptr_t)first == stats.heap_start);

    aligned = kmalloc(32, 4096);
    assert(aligned != 0);
    assert(((uintptr_t)aligned & 4095U) == 0);
    assert((uintptr_t)aligned > (uintptr_t)first);

    memory_stats(&stats);
    assert(stats.next_free >= (uintptr_t)aligned + 32U);
    assert(stats.next_free <= stats.heap_end);

    /* Huge requests and invalid alignment must fail without corrupting state. */
    {
        uint64_t before = stats.next_free;
        assert(kmalloc(UINT64_MAX, 8) == 0);
        assert(kmalloc(16, UINT64_MAX) == 0);
        memory_stats(&stats);
        assert(stats.next_free == before);
    }

    /* Exhaustion must fail cleanly, and a failed request must not move the heap. */
    {
        uint64_t before = stats.next_free;
        uint64_t remaining = stats.heap_end - before;
        assert(kmalloc(remaining + 1U, 1) == 0);
        memory_stats(&stats);
        assert(stats.next_free == before);
    }

    puts("memory allocator tests passed");
    return 0;
}
