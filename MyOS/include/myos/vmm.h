#ifndef MYOS_VMM_H
#define MYOS_VMM_H

#include <myos/types.h>

#define PAGE_PRESENT  (1ULL << 0)
#define PAGE_WRITABLE (1ULL << 1)
#define PAGE_USER     (1ULL << 2)
#define PAGE_PWT      (1ULL << 3)
#define PAGE_PCD      (1ULL << 4)
#define PAGE_ACCESSED (1ULL << 5)
#define PAGE_DIRTY    (1ULL << 6)
#define PAGE_HUGE     (1ULL << 7)
#define PAGE_GLOBAL   (1ULL << 8)
#define PAGE_NX       (1ULL << 63)

typedef uint64_t page_entry_t;
typedef page_entry_t* page_table_t;

typedef struct {
    page_table_t pml4;
    uint64_t cr3_phys;
} page_directory_t;

void vmm_init(void);
page_directory_t* vmm_create_address_space(void);
void vmm_destroy_address_space(page_directory_t* dir);
void vmm_switch_pml4(page_directory_t* dir);
page_directory_t* vmm_get_kernel_directory(void);

bool vmm_map_page(page_directory_t* dir, uint64_t virt, uint64_t phys, uint64_t flags);
bool vmm_unmap_page(page_directory_t* dir, uint64_t virt);
uint64_t vmm_virt_to_phys(page_directory_t* dir, uint64_t virt);

#endif /* MYOS_VMM_H */
