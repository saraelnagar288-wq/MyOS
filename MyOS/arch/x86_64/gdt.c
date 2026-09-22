#include <myos/gdt.h>
#include <myos/kernel.h>

extern void gdt_flush(uint64_t gdt_ptr_addr);
extern void tss_flush(uint16_t tss_sel);

static uint64_t s_gdt[8];
static struct tss_entry s_tss;
static struct gdt_ptr s_gdt_ptr;

static void set_gdt_gate(int num, uint64_t base, uint64_t limit, uint8_t access, uint8_t gran) {
    uint64_t descriptor = 0;

    descriptor |= (limit & 0xFFFFULL);
    descriptor |= (base & 0xFFFFFFULL) << 16;
    descriptor |= ((uint64_t)access) << 40;
    descriptor |= ((uint64_t)(limit >> 16) & 0x0FULL) << 48;
    descriptor |= ((uint64_t)gran & 0xF0ULL) << 48;
    descriptor |= ((base >> 24) & 0xFFULL) << 56;

    s_gdt[num] = descriptor;
}

static void set_tss_gate(int num, uint64_t base, uint32_t limit) {
    /* 64-bit TSS descriptor takes two consecutive 8-byte entries (16 bytes total) */
    uint64_t low = 0;
    low |= (limit & 0xFFFFULL);
    low |= (base & 0xFFFFFFULL) << 16;
    low |= (0x89ULL) << 40; /* Present, 64-bit TSS Available */
    low |= ((uint64_t)(limit >> 16) & 0x0FULL) << 48;
    low |= ((base >> 24) & 0xFFULL) << 56;

    uint64_t high = (base >> 32) & 0xFFFFFFFFULL;

    s_gdt[num] = low;
    s_gdt[num + 1] = high;
}

void gdt_init(void) {
    kmemset(s_gdt, 0, sizeof(s_gdt));
    kmemset(&s_tss, 0, sizeof(s_tss));

    /* 0x00: Null descriptor */
    s_gdt[0] = 0;

    /* 0x08: Kernel Code (Ring 0, Executable, Readable, 64-bit) */
    set_gdt_gate(1, 0, 0xFFFFFFFF, 0x9A, 0x20);

    /* 0x10: Kernel Data (Ring 0, Writable) */
    set_gdt_gate(2, 0, 0xFFFFFFFF, 0x92, 0x00);

    /* 0x18: User Data (Ring 3, Writable) */
    set_gdt_gate(3, 0, 0xFFFFFFFF, 0xF2, 0x00);

    /* 0x20: User Code (Ring 3, Executable, Readable, 64-bit) */
    set_gdt_gate(4, 0, 0xFFFFFFFF, 0xFA, 0x20);

    /* 0x28: TSS (Task State Segment) */
    s_tss.iomap_base = sizeof(struct tss_entry);
    set_tss_gate(5, (uint64_t)&s_tss, sizeof(struct tss_entry) - 1);

    s_gdt_ptr.limit = sizeof(s_gdt) - 1;
    s_gdt_ptr.base = (uint64_t)&s_gdt;

    gdt_flush((uint64_t)&s_gdt_ptr);
    tss_flush(GDT_TSS_SEG);

    LOG_INFO("GDT", "64-bit Global Descriptor Table & TSS initialized");
}

void tss_set_stack(uint64_t stack) {
    s_tss.rsp0 = stack;
}
