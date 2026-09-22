#include <myos/pmm.h>
#include <myos/kernel.h>

extern uint8_t __kernel_start[];
extern uint8_t __kernel_end[];

#define MAX_FRAMES (1024 * 1024) /* Supports up to 4 GiB of physical memory */
static uint32_t pmm_bitmap[MAX_FRAMES / 32];
static pmm_stats_t g_pmm_stats;

static inline void bitmap_set(uint64_t frame) {
    pmm_bitmap[frame / 32] |= (1U << (frame % 32));
}

static inline void bitmap_clear(uint64_t frame) {
    pmm_bitmap[frame / 32] &= ~(1U << (frame % 32));
}

static inline bool bitmap_test(uint64_t frame) {
    return (pmm_bitmap[frame / 32] & (1U << (frame % 32))) != 0;
}

void pmm_init(struct multiboot_info* mb_info) {
    kmemset(pmm_bitmap, 0xFF, sizeof(pmm_bitmap)); /* Mark all memory as used initially */
    kmemset(&g_pmm_stats, 0, sizeof(pmm_stats_t));

    uint64_t highest_address = 0;

    if (mb_info && (mb_info->flags & (1 << 6))) {
        struct multiboot_mmap_entry* mmap = (struct multiboot_mmap_entry*)(uint64_t)mb_info->mmap_addr;
        uint32_t mmap_end = mb_info->mmap_addr + mb_info->mmap_length;

        while ((uint64_t)mmap < mmap_end) {
            if (mmap->type == 1) { /* Available RAM */
                uint64_t start_frame = mmap->addr / PAGE_SIZE;
                uint64_t frame_count = mmap->len / PAGE_SIZE;

                for (uint64_t i = 0; i < frame_count; i++) {
                    uint64_t f = start_frame + i;
                    if (f < MAX_FRAMES) {
                        bitmap_clear(f);
                        g_pmm_stats.total_frames++;
                        g_pmm_stats.free_frames++;
                    }
                }

                if (mmap->addr + mmap->len > highest_address) {
                    highest_address = mmap->addr + mmap->len;
                }
            }
            mmap = (struct multiboot_mmap_entry*)((uint64_t)mmap + mmap->size + sizeof(mmap->size));
        }
    } else {
        /* Fallback: assume 128 MiB available if no mmap provided */
        highest_address = 128 * 1024 * 1024;
        uint64_t frame_count = highest_address / PAGE_SIZE;
        for (uint64_t f = 0; f < frame_count && f < MAX_FRAMES; f++) {
            bitmap_clear(f);
            g_pmm_stats.total_frames++;
            g_pmm_stats.free_frames++;
        }
    }

    /* Reserve lower 1 MiB (BIOS, VGA, real mode vectors) */
    for (uint64_t f = 0; f < 256; f++) {
        if (!bitmap_test(f)) {
            bitmap_set(f);
            g_pmm_stats.free_frames--;
        }
    }

    /* Reserve Kernel memory */
    uint64_t kstart_frame = ((uint64_t)__kernel_start) / PAGE_SIZE;
    uint64_t kend_frame   = (((uint64_t)__kernel_end) + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint64_t f = kstart_frame; f <= kend_frame && f < MAX_FRAMES; f++) {
        if (!bitmap_test(f)) {
            bitmap_set(f);
            g_pmm_stats.free_frames--;
        }
    }

    g_pmm_stats.used_frames = g_pmm_stats.total_frames - g_pmm_stats.free_frames;
    g_pmm_stats.total_memory = g_pmm_stats.total_frames * PAGE_SIZE;
    g_pmm_stats.free_memory  = g_pmm_stats.free_frames * PAGE_SIZE;
    g_pmm_stats.used_memory  = g_pmm_stats.used_frames * PAGE_SIZE;

    LOG_INFO("PMM", "Physical Memory: Total: %lu MiB | Free: %lu MiB | Used: %lu MiB",
             g_pmm_stats.total_memory / (1024 * 1024),
             g_pmm_stats.free_memory / (1024 * 1024),
             g_pmm_stats.used_memory / (1024 * 1024));
}

void* pmm_alloc_frame(void) {
    for (uint64_t i = 0; i < MAX_FRAMES; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            g_pmm_stats.free_frames--;
            g_pmm_stats.used_frames++;
            g_pmm_stats.free_memory = g_pmm_stats.free_frames * PAGE_SIZE;
            g_pmm_stats.used_memory = g_pmm_stats.used_frames * PAGE_SIZE;
            return (void*)(i * PAGE_SIZE);
        }
    }
    PANIC("PMM: Out of physical memory frames!");
    return NULL;
}

void* pmm_alloc_frames(size_t count) {
    if (count == 0) return NULL;
    if (count == 1) return pmm_alloc_frame();

    size_t consecutive = 0;
    uint64_t start_frame = 0;

    for (uint64_t i = 0; i < MAX_FRAMES; i++) {
        if (!bitmap_test(i)) {
            if (consecutive == 0) start_frame = i;
            consecutive++;
            if (consecutive == count) {
                for (uint64_t j = start_frame; j < start_frame + count; j++) {
                    bitmap_set(j);
                }
                g_pmm_stats.free_frames -= count;
                g_pmm_stats.used_frames += count;
                g_pmm_stats.free_memory = g_pmm_stats.free_frames * PAGE_SIZE;
                g_pmm_stats.used_memory = g_pmm_stats.used_frames * PAGE_SIZE;
                return (void*)(start_frame * PAGE_SIZE);
            }
        } else {
            consecutive = 0;
        }
    }

    PANIC("PMM: Out of contiguous physical memory frames!");
    return NULL;
}

void pmm_free_frame(void* ptr) {
    uint64_t frame = (uint64_t)ptr / PAGE_SIZE;
    if (frame < MAX_FRAMES && bitmap_test(frame)) {
        bitmap_clear(frame);
        g_pmm_stats.free_frames++;
        g_pmm_stats.used_frames--;
        g_pmm_stats.free_memory = g_pmm_stats.free_frames * PAGE_SIZE;
        g_pmm_stats.used_memory = g_pmm_stats.used_frames * PAGE_SIZE;
    }
}

void pmm_free_frames(void* ptr, size_t count) {
    uint64_t start_frame = (uint64_t)ptr / PAGE_SIZE;
    for (size_t i = 0; i < count; i++) {
        pmm_free_frame((void*)((start_frame + i) * PAGE_SIZE));
    }
}

const pmm_stats_t* pmm_get_stats(void) {
    return &g_pmm_stats;
}
