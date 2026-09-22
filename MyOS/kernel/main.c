#include <myos/kernel.h>
#include <myos/cpu.h>
#include <myos/gdt.h>
#include <myos/idt.h>
#include <myos/pmm.h>
#include <myos/vmm.h>
#include <myos/kheap.h>
#include <myos/hal.h>
#include <myos/input.h>
#include <myos/pci.h>
#include <myos/fs.h>
#include <myos/process.h>
#include <myos/scheduler.h>
#include <myos/syscall.h>
#include <myos/gfx.h>

extern void serial_init(void);
extern void keyboard_init(void);
extern void mouse_init(uint32_t screen_w, uint32_t screen_h);
extern void init_process_main(void);

void kernel_main(uint64_t magic, struct multiboot_info* mb_info) {
    /* 1. Early Serial Log Initialization */
    serial_init();

    kprintf("\n");
    kprintf("=====================================================\n");
    kprintf("   Welcome to MyOS Kernel v0.1.0 (x86-64 SMP)       \n");
    kprintf("   Architected from Bootloader to Graphical Desktop  \n");
    kprintf("=====================================================\n\n");

    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        LOG_WARNING("BOOT", "Warning: Non-standard multiboot magic: 0x%lx", magic);
    } else {
        LOG_INFO("BOOT", "Booted via standard Multiboot-compliant bootloader");
    }

    /* 2. Global Descriptor Table & TSS */
    gdt_init();

    /* 3. Interrupt Descriptor Table & 8259 PIC */
    idt_init();

    /* 4. CPU Detection & Features */
    cpu_init();

    /* 5. Physical Memory Manager (PMM) */
    pmm_init(mb_info);

    /* 6. Virtual Memory Manager (VMM 4-Level Paging) */
    vmm_init();

    /* 7. Kernel Heap Allocator (kmalloc, kfree) */
    kheap_init();

    /* 8. Hardware Abstraction Layer & PIT Timer */
    hal_init();

    /* 9. Framebuffer 2D Graphics Engine */
    gfx_init(mb_info);
    framebuffer_t* fb = gfx_get_framebuffer();

    /* 10. Unified Input Subsystem */
    input_init();

    /* 11. PS/2 Keyboard & Mouse Drivers */
    keyboard_init();
    mouse_init(fb->width, fb->height);

    /* 12. PCI Bus Hardware Enumeration */
    pci_init();

    /* 13. Virtual File System & RAMFS Mount */
    vfs_init();

    /* 14. Process & Thread Management */
    process_init();

    /* 15. Round-Robin Preemptive Scheduler */
    scheduler_init();

    /* 16. System Call Handler */
    syscall_init();

    /* 17. Enable Hardware Interrupts */
    sti();
    LOG_INFO("KERNEL", "Hardware interrupts enabled (IF=1)");

    /* 18. Spawn Init System (PID 1) */
    process_t* init_proc = process_create("init", init_process_main, 10);
    if (!init_proc) {
        PANIC("Failed to spawn Init process!");
    }

    /* 19. Start Scheduler & Launch Desktop */
    scheduler_start();

    /* Direct jump to init process routine for single-core bootstrap */
    init_process_main();

    /* Kernel Idle Loop */
    while (1) {
        hlt();
    }
}
