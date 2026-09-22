#include <myos/kheap.h>
#include <myos/pmm.h>
#include <myos/vmm.h>
#include <myos/kernel.h>

#define HEAP_MAGIC 0x4D594F53 /* "MYOS" */

typedef struct heap_block {
    uint32_t magic;
    size_t size;
    bool is_free;
    struct heap_block* next;
    struct heap_block* prev;
} heap_block_t;

static uint8_t* g_heap_start = NULL;
static size_t   g_heap_size  = 0;
static heap_block_t* g_first_block = NULL;
static kheap_stats_t g_heap_stats;

void kheap_init(void) {
    /* Allocate 4 MiB of contiguous frames for the initial kernel heap */
    size_t frames = KHEAP_INITIAL_SIZE / PAGE_SIZE;
    g_heap_start = (uint8_t*)pmm_alloc_frames(frames);
    g_heap_size  = KHEAP_INITIAL_SIZE;

    g_first_block = (heap_block_t*)g_heap_start;
    g_first_block->magic = HEAP_MAGIC;
    g_first_block->size = g_heap_size - sizeof(heap_block_t);
    g_first_block->is_free = true;
    g_first_block->next = NULL;
    g_first_block->prev = NULL;

    g_heap_stats.total_heap = g_heap_size;
    g_heap_stats.used_heap = sizeof(heap_block_t);
    g_heap_stats.free_heap = g_first_block->size;
    g_heap_stats.allocation_count = 0;

    LOG_INFO("KHEAP", "Kernel heap initialized at 0x%016lx (%lu KiB)",
             (uint64_t)g_heap_start, g_heap_size / 1024);
}

void* kmalloc(size_t size) {
    if (size == 0) return NULL;

    /* Align size to 16 bytes */
    size = ALIGN_UP(size, 16);

    heap_block_t* curr = g_first_block;
    while (curr) {
        if (curr->magic != HEAP_MAGIC) {
            PANIC("KHEAP: Heap corruption detected (invalid magic)!");
            return NULL;
        }

        if (curr->is_free && curr->size >= size) {
            /* Can we split this block? */
            if (curr->size >= size + sizeof(heap_block_t) + 32) {
                heap_block_t* next = (heap_block_t*)((uint8_t*)curr + sizeof(heap_block_t) + size);
                next->magic = HEAP_MAGIC;
                next->size = curr->size - size - sizeof(heap_block_t);
                next->is_free = true;
                next->next = curr->next;
                next->prev = curr;

                if (curr->next) {
                    curr->next->prev = next;
                }
                curr->next = next;
                curr->size = size;
            }

            curr->is_free = false;
            g_heap_stats.used_heap += curr->size + sizeof(heap_block_t);
            g_heap_stats.free_heap = g_heap_stats.total_heap - g_heap_stats.used_heap;
            g_heap_stats.allocation_count++;

            return (void*)((uint8_t*)curr + sizeof(heap_block_t));
        }
        curr = curr->next;
    }

    PANIC("KHEAP: Out of kernel heap memory!");
    return NULL;
}

void* kzalloc(size_t size) {
    void* ptr = kmalloc(size);
    if (ptr) {
        kmemset(ptr, 0, size);
    }
    return ptr;
}

void* kmalloc_aligned(size_t size, size_t alignment) {
    /* Simple aligned allocation */
    size_t total_size = size + alignment + sizeof(void*);
    void* raw = kmalloc(total_size);
    if (!raw) return NULL;

    uintptr_t addr = (uintptr_t)raw + sizeof(void*);
    uintptr_t aligned = ALIGN_UP(addr, alignment);
    void** ptr_storage = (void**)(aligned - sizeof(void*));
    *ptr_storage = raw;

    return (void*)aligned;
}

void kfree(void* ptr) {
    if (!ptr) return;

    heap_block_t* block = (heap_block_t*)((uint8_t*)ptr - sizeof(heap_block_t));
    if (block->magic != HEAP_MAGIC) {
        PANIC("KHEAP: Attempted to free corrupted or invalid block!");
        return;
    }

    block->is_free = true;
    g_heap_stats.used_heap -= (block->size + sizeof(heap_block_t));
    g_heap_stats.free_heap = g_heap_stats.total_heap - g_heap_stats.used_heap;
    if (g_heap_stats.allocation_count > 0) {
        g_heap_stats.allocation_count--;
    }

    /* Coalesce with next block if free */
    if (block->next && block->next->is_free) {
        block->size += sizeof(heap_block_t) + block->next->size;
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }

    /* Coalesce with prev block if free */
    if (block->prev && block->prev->is_free) {
        block->prev->size += sizeof(heap_block_t) + block->size;
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
    }
}

void* krealloc(void* ptr, size_t new_size) {
    if (!ptr) return kmalloc(new_size);
    if (new_size == 0) {
        kfree(ptr);
        return NULL;
    }

    heap_block_t* block = (heap_block_t*)((uint8_t*)ptr - sizeof(heap_block_t));
    if (block->size >= new_size) {
        return ptr;
    }

    void* new_ptr = kmalloc(new_size);
    if (new_ptr) {
        kmemcpy(new_ptr, ptr, block->size);
        kfree(ptr);
    }
    return new_ptr;
}

const kheap_stats_t* kheap_get_stats(void) {
    return &g_heap_stats;
}
