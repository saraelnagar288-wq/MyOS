#include <myos/cpu.h>
#include <myos/kernel.h>

static cpu_info_t g_cpu_info;

void cpuid(uint32_t leaf, uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx) {
    __asm__ volatile ("cpuid"
                      : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
                      : "a"(leaf), "c"(0));
}

void cpu_init(void) {
    kmemset(&g_cpu_info, 0, sizeof(cpu_info_t));

    uint32_t eax, ebx, ecx, edx;

    /* Get Vendor String */
    cpuid(0, &eax, &ebx, &ecx, &edx);
    ((uint32_t*)g_cpu_info.vendor)[0] = ebx;
    ((uint32_t*)g_cpu_info.vendor)[1] = edx;
    ((uint32_t*)g_cpu_info.vendor)[2] = ecx;
    g_cpu_info.vendor[12] = '\0';

    /* Get Features and Stepping */
    cpuid(1, &eax, &ebx, &ecx, &edx);
    g_cpu_info.stepping = eax & 0xF;
    g_cpu_info.model    = (eax >> 4) & 0xF;
    g_cpu_info.family   = (eax >> 8) & 0xF;

    g_cpu_info.has_fpu  = (edx & (1 << 0)) != 0;
    g_cpu_info.has_tsc  = (edx & (1 << 4)) != 0;
    g_cpu_info.has_apic = (edx & (1 << 9)) != 0;
    g_cpu_info.has_sse  = (edx & (1 << 25)) != 0;
    g_cpu_info.has_sse2 = (edx & (1 << 26)) != 0;
    g_cpu_info.has_sse3 = (ecx & (1 << 0)) != 0;
    g_cpu_info.has_avx  = (ecx & (1 << 28)) != 0;

    /* Logical Cores count */
    g_cpu_info.cores_detected = (ebx >> 16) & 0xFF;
    if (g_cpu_info.cores_detected == 0) {
        g_cpu_info.cores_detected = 1;
    }

    /* Extended CPUID: Brand string & NX bit */
    cpuid(0x80000000, &eax, &ebx, &ecx, &edx);
    if (eax >= 0x80000001) {
        uint32_t ex_eax, ex_ebx, ex_ecx, ex_edx;
        cpuid(0x80000001, &ex_eax, &ex_ebx, &ex_ecx, &ex_edx);
        g_cpu_info.has_nx = (ex_edx & (1 << 20)) != 0;
    }

    if (eax >= 0x80000004) {
        uint32_t* brand = (uint32_t*)g_cpu_info.brand;
        cpuid(0x80000002, &brand[0], &brand[1], &brand[2], &brand[3]);
        cpuid(0x80000003, &brand[4], &brand[5], &brand[6], &brand[7]);
        cpuid(0x80000004, &brand[8], &brand[9], &brand[10], &brand[11]);
        g_cpu_info.brand[48] = '\0';
    } else {
        kstrcpy(g_cpu_info.brand, "x86_64 Compatible Processor");
    }

    /* Trim leading whitespace in brand string if any */
    char* b = g_cpu_info.brand;
    while (*b == ' ') b++;
    if (b != g_cpu_info.brand) {
        kmemmove(g_cpu_info.brand, b, kstrlen(b) + 1);
    }

    LOG_INFO("CPU", "Vendor: %s | Model: %s", g_cpu_info.vendor, g_cpu_info.brand);
    LOG_INFO("CPU", "Features: FPU:%d SSE:%d SSE2:%d SSE3:%d AVX:%d APIC:%d NX:%d Cores:%d",
             g_cpu_info.has_fpu, g_cpu_info.has_sse, g_cpu_info.has_sse2, g_cpu_info.has_sse3,
             g_cpu_info.has_avx, g_cpu_info.has_apic, g_cpu_info.has_nx, g_cpu_info.cores_detected);
}

const cpu_info_t* cpu_get_info(void) {
    return &g_cpu_info;
}
