#ifndef MYOS_PMM_H
#define MYOS_PMM_H

#include <myos/types.h>
#include <myos/kernel.h>

#define PAGE_SIZE 4096

typedef struct {
    uint64_t total_memory;
    uint64_t free_memory;
    uint64_t used_memory;
    uint64_t total_frames;
    uint64_t free_frames;
    uint64_t used_frames;
} pmm_stats_t;

void pmm_init(struct multiboot_info* mb_info);
void* pmm_alloc_frame(void);
void* pmm_alloc_frames(size_t count);
void pmm_free_frame(void* ptr);
void pmm_free_frames(void* ptr, size_t count);
const pmm_stats_t* pmm_get_stats(void);

#endif /* MYOS_PMM_H */
