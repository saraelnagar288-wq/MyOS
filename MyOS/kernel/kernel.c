#include <myos/kernel.h>
#include <myos/cpu.h>
#include <myos/gdt.h>
#include <myos/idt.h>
#include <myos/pmm.h>
#include <myos/vmm.h>
#include <myos/kheap.h>
#include <myos/hal.h>
#include <myos/drivers.h>
#include <myos/pci.h>
#include <myos/fs.h>
#include <myos/process.h>
#include <myos/scheduler.h>
#include <myos/syscall.h>
#include <myos/gfx.h>
#include <myos/desktop.h>

void kernel_main(uint32_t magic, struct multiboot_info* mb_info) {
    /* 1. Early serial port initialization for debug output */
    serial_init();
    kprintf("\n=======================================================\n");
    kprintf("          Starting MyOS 0.1.0-alpha (x86_64)           \n");
    kprintf("=======================================================\n");

    /* 2. Verify Multiboot bootloader compliance */
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        LOG_ERROR("BOOT", "Invalid Multiboot magic: 0x%x (expected 0x%x)",
                  magic, MULTIBOOT_BOOTLOADER_MAGIC);
        kernel_panic("Non-multiboot bootloader detected", __FILE__, __LINE__);
    }
    LOG_INFO("BOOT", "Multiboot header verified successfully");

    /* 3. Core CPU & Descriptor Tables */
    gdt_init();
    idt_init();
    pic_init();
    cpu_init();

    /* 4. Memory Management Subsystem */
    pmm_init(mb_info);
    vmm_init();
    kheap_init();

    /* 5. Hardware Abstraction Layer & Timers */
    hal_init();
    pit_init(1000); /* 1000 Hz system tick */

    /* 6. Virtual File System & Initial RAMFS */
    vfs_init();

    /* 7. PCI Subsystem & Bus Discovery */
    pci_init();

    /* 8. System Call Interface */
    syscall_init();

    /* 9. Process Management & Preemptive Scheduler */
    process_init();
    scheduler_init();

    /* 10. Framebuffer & 2D Graphics Engine */
    gfx_init(mb_info);

    /* 11. Enable Hardware Interrupts */
    sti();
    LOG_INFO("KERNEL", "Hardware interrupts enabled");

    /* 12. Launch Desktop Environment and Window Manager */
    desktop_init();
    desktop_run();

    /* Fallback idle loop */
    while (1) {
        hlt();
    }
}
