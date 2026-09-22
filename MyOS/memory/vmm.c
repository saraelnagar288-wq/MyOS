#include <myos/vmm.h>
#include <myos/pmm.h>
#include <myos/cpu.h>
#include <myos/kernel.h>

static page_directory_t g_kernel_directory;

#define PML4_INDEX(virt) (((virt) >> 39) & 0x1FF)
#define PDPT_INDEX(virt) (((virt) >> 30) & 0x1FF)
#define PD_INDEX(virt)   (((virt) >> 21) & 0x1FF)
#define PT_INDEX(virt)   (((virt) >> 12) & 0x1FF)

static void invlpg(uint64_t vaddr) {
    __asm__ volatile ("invlpg (%0)" : : "r"(vaddr) : "memory");
}

page_directory_t* vmm_get_kernel_directory(void) {
    return &g_kernel_directory;
}

void vmm_init(void) {
    /* Allocate PML4 table for kernel */
    g_kernel_directory.pml4 = (page_table_t)pmm_alloc_frame();
    g_kernel_directory.cr3_phys = (uint64_t)g_kernel_directory.pml4;
    kmemset(g_kernel_directory.pml4, 0, PAGE_SIZE);

    /* Identity-map lower 128 MiB (kernel, device buffers, ACPI, BIOS) */
    uint64_t num_pages = (128 * 1024 * 1024) / PAGE_SIZE;
    for (uint64_t i = 0; i < num_pages; i++) {
        uint64_t addr = i * PAGE_SIZE;
        vmm_map_page(&g_kernel_directory, addr, addr, PAGE_PRESENT | PAGE_WRITABLE);
    }

    /* Switch to kernel's new PML4 */
    vmm_switch_pml4(&g_kernel_directory);

    LOG_INFO("VMM", "Initialized 4-level paging (PML4: 0x%016lx)", g_kernel_directory.cr3_phys);
}

void vmm_switch_pml4(page_directory_t* dir) {
    if (dir && dir->cr3_phys) {
        write_cr3(dir->cr3_phys);
    }
}

bool vmm_map_page(page_directory_t* dir, uint64_t virt, uint64_t phys, uint64_t flags) {
    if (!dir || !dir->pml4) return false;

    uint64_t pml4_i = PML4_INDEX(virt);
    uint64_t pdpt_i = PDPT_INDEX(virt);
    uint64_t pd_i   = PD_INDEX(virt);
    uint64_t pt_i   = PT_INDEX(virt);

    page_table_t pml4 = dir->pml4;

    /* PML4 -> PDPT */
    if (!(pml4[pml4_i] & PAGE_PRESENT)) {
        void* frame = pmm_alloc_frame();
        kmemset(frame, 0, PAGE_SIZE);
        pml4[pml4_i] = (uint64_t)frame | PAGE_PRESENT | PAGE_WRITABLE | (flags & PAGE_USER);
    }
    page_table_t pdpt = (page_table_t)(pml4[pml4_i] & ~0xFFFULL);

    /* PDPT -> PD */
    if (!(pdpt[pdpt_i] & PAGE_PRESENT)) {
        void* frame = pmm_alloc_frame();
        kmemset(frame, 0, PAGE_SIZE);
        pdpt[pdpt_i] = (uint64_t)frame | PAGE_PRESENT | PAGE_WRITABLE | (flags & PAGE_USER);
    }
    page_table_t pd = (page_table_t)(pdpt[pdpt_i] & ~0xFFFULL);

    /* PD -> PT */
    if (!(pd[pd_i] & PAGE_PRESENT)) {
        void* frame = pmm_alloc_frame();
        kmemset(frame, 0, PAGE_SIZE);
        pd[pd_i] = (uint64_t)frame | PAGE_PRESENT | PAGE_WRITABLE | (flags & PAGE_USER);
    }
    page_table_t pt = (page_table_t)(pd[pd_i] & ~0xFFFULL);

    /* Set Page Table Entry */
    pt[pt_i] = (phys & ~0xFFFULL) | flags | PAGE_PRESENT;
    invlpg(virt);

    return true;
}

bool vmm_unmap_page(page_directory_t* dir, uint64_t virt) {
    if (!dir || !dir->pml4) return false;

    uint64_t pml4_i = PML4_INDEX(virt);
    uint64_t pdpt_i = PDPT_INDEX(virt);
    uint64_t pd_i   = PD_INDEX(virt);
    uint64_t pt_i   = PT_INDEX(virt);

    page_table_t pml4 = dir->pml4;
    if (!(pml4[pml4_i] & PAGE_PRESENT)) return false;
    page_table_t pdpt = (page_table_t)(pml4[pml4_i] & ~0xFFFULL);

    if (!(pdpt[pdpt_i] & PAGE_PRESENT)) return false;
    page_table_t pd = (page_table_t)(pdpt[pdpt_i] & ~0xFFFULL);

    if (!(pd[pd_i] & PAGE_PRESENT)) return false;
    page_table_t pt = (page_table_t)(pd[pd_i] & ~0xFFFULL);

    pt[pt_i] = 0;
    invlpg(virt);
    return true;
}

uint64_t vmm_virt_to_phys(page_directory_t* dir, uint64_t virt) {
    if (!dir || !dir->pml4) return 0;

    uint64_t pml4_i = PML4_INDEX(virt);
    uint64_t pdpt_i = PDPT_INDEX(virt);
    uint64_t pd_i   = PD_INDEX(virt);
    uint64_t pt_i   = PT_INDEX(virt);

    page_table_t pml4 = dir->pml4;
    if (!(pml4[pml4_i] & PAGE_PRESENT)) return 0;
    page_table_t pdpt = (page_table_t)(pml4[pml4_i] & ~0xFFFULL);

    if (!(pdpt[pdpt_i] & PAGE_PRESENT)) return 0;
    page_table_t pd = (page_table_t)(pdpt[pdpt_i] & ~0xFFFULL);

    if (!(pd[pd_i] & PAGE_PRESENT)) return 0;
    page_table_t pt = (page_table_t)(pd[pd_i] & ~0xFFFULL);

    if (!(pt[pt_i] & PAGE_PRESENT)) return 0;

    return (pt[pt_i] & ~0xFFFULL) | (virt & 0xFFF);
}

page_directory_t* vmm_create_address_space(void) {
    page_directory_t* dir = (page_directory_t*)pmm_alloc_frame();
    dir->pml4 = (page_table_t)pmm_alloc_frame();
    dir->cr3_phys = (uint64_t)dir->pml4;
    kmemset(dir->pml4, 0, PAGE_SIZE);

    /* Share kernel mappings (higher half or PML4[0]) */
    dir->pml4[0] = g_kernel_directory.pml4[0];

    return dir;
}

void vmm_destroy_address_space(page_directory_t* dir) {
    if (!dir) return;
    if (dir->pml4) {
        pmm_free_frame(dir->pml4);
    }
    pmm_free_frame(dir);
}
