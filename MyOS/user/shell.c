#include <myos/kernel.h>
#include <myos/fs.h>
#include <myos/hal.h>
#include <myos/cpu.h>
#include <myos/pmm.h>
#include <myos/kheap.h>
#include <myos/pci.h>
#include <myos/process.h>
#include <myos/scheduler.h>

void cmd_help(int argc, char* argv[]) {
    (void)argc; (void)argv;
    kprintf("MyOS MyShell built-in commands:\n");
    kprintf("  help     - Display this help message\n");
    kprintf("  clear    - Clear the terminal screen\n");
    kprintf("  echo     - Print arguments to console\n");
    kprintf("  uname    - Show operating system name and version\n");
    kprintf("  uptime   - Show system uptime\n");
    kprintf("  date     - Show current date and time from RTC\n");
    kprintf("  ls       - List directory contents (e.g. ls / or ls /etc)\n");
    kprintf("  cat      - Display file contents (e.g. cat /system/motd)\n");
    kprintf("  mkdir    - Create a new directory\n");
    kprintf("  touch    - Create an empty file\n");
    kprintf("  ps       - List running processes and threads\n");
    kprintf("  kill     - Terminate a process by PID\n");
    kprintf("  lscpu    - Display CPU model and detected features\n");
    kprintf("  lspci    - Display detected PCI hardware devices\n");
    kprintf("  free     - Display physical and heap memory statistics\n");
    kprintf("  reboot   - Restart the computer\n");
    kprintf("  shutdown - Power off the computer\n");
}

void cmd_uname(int argc, char* argv[]) {
    (void)argc; (void)argv;
    kprintf("MyOS 0.1.0 x86-64 SMP (gcc 12.2.0)\n");
}

void cmd_uptime(int argc, char* argv[]) {
    (void)argc; (void)argv;
    uint64_t ms = hal_get_uptime_ms();
    uint64_t sec = ms / 1000;
    uint64_t min = sec / 60;
    uint64_t hr  = min / 60;
    kprintf("Uptime: %02lu:%02lu:%02lu (%lu ms)\n", hr, min % 60, sec % 60, ms);
}

void cmd_date(int argc, char* argv[]) {
    (void)argc; (void)argv;
    rtc_time_t t;
    hal_get_time(&t);
    kprintf("Current Time: %04u-%02u-%02u %02u:%02u:%02u UTC\n",
            t.year, t.month, t.day, t.hour, t.minute, t.second);
}

void cmd_ls(int argc, char* argv[]) {
    const char* path = (argc > 1) ? argv[1] : "/";
    vfs_node_t* node = vfs_lookup(path);
    if (!node) {
        kprintf("ls: cannot access '%s': No such directory\n", path);
        return;
    }

    if (!(node->flags & FS_DIRECTORY)) {
        kprintf("%s\n", node->name);
        return;
    }

    uint32_t index = 0;
    vfs_dirent_t* entry;
    while ((entry = vfs_readdir(node, index++)) != NULL) {
        if (entry->type == FS_DIRECTORY) {
            kprintf("  [DIR]  %-16s\n", entry->name);
        } else {
            kprintf("  [FILE] %-16s %6lu bytes\n", entry->name, (unsigned long)entry->size);
        }
    }
}

void cmd_cat(int argc, char* argv[]) {
    if (argc < 2) {
        kprintf("Usage: cat <filename>\n");
        return;
    }

    vfs_node_t* node = vfs_lookup(argv[1]);
    if (!node) {
        kprintf("cat: %s: No such file or directory\n", argv[1]);
        return;
    }

    if (node->flags & FS_DIRECTORY) {
        kprintf("cat: %s: Is a directory\n", argv[1]);
        return;
    }

    uint8_t buffer[512];
    uint32_t offset = 0;
    uint32_t bytes_read;

    while ((bytes_read = vfs_read(node, offset, sizeof(buffer) - 1, buffer)) > 0) {
        buffer[bytes_read] = '\0';
        kprintf("%s", (char*)buffer);
        offset += bytes_read;
    }
    kprintf("\n");
}

void cmd_mkdir(int argc, char* argv[]) {
    if (argc < 2) {
        kprintf("Usage: mkdir <directory_name>\n");
        return;
    }
    ramfs_create_dir(argv[1]);
    kprintf("Directory '%s' created.\n", argv[1]);
}

void cmd_touch(int argc, char* argv[]) {
    if (argc < 2) {
        kprintf("Usage: touch <filename>\n");
        return;
    }
    ramfs_create_file(argv[1], (const uint8_t*)"", 0);
    kprintf("File '%s' created.\n", argv[1]);
}

void cmd_ps(int argc, char* argv[]) {
    (void)argc; (void)argv;
    uint32_t total = 0;
    const process_t* table = process_get_table(&total);

    kprintf("PID  STATE       PRI  NAME\n");
    kprintf("------------------------------------\n");
    for (uint32_t i = 0; i < total; i++) {
        if (table[i].pid != 0 || i == 0) {
            const char* state_str = "UNKNOWN";
            switch (table[i].state) {
                case PROCESS_STATE_READY:      state_str = "READY  "; break;
                case PROCESS_STATE_RUNNING:    state_str = "RUNNING"; break;
                case PROCESS_STATE_BLOCKED:    state_str = "BLOCKED"; break;
                case PROCESS_STATE_SLEEPING:   state_str = "SLEEP  "; break;
                case PROCESS_STATE_TERMINATED: state_str = "ZOMBIE "; break;
            }
            uint8_t pri = table[i].threads ? table[i].threads->priority : 0;
            kprintf("%-4u %s %-4u %s\n", table[i].pid, state_str, pri, table[i].name);
        }
    }
}

void cmd_kill(int argc, char* argv[]) {
    if (argc < 2) {
        kprintf("Usage: kill <pid>\n");
        return;
    }
    /* Simple string to int */
    uint32_t pid = 0;
    const char* s = argv[1];
    while (*s >= '0' && *s <= '9') {
        pid = pid * 10 + (*s++ - '0');
    }

    process_t* proc = process_get_by_pid(pid);
    if (!proc) {
        kprintf("kill: (%u) - No such process\n", pid);
        return;
    }
    process_terminate(proc, 9);
    kprintf("Killed process %u (%s)\n", pid, proc->name);
}

void cmd_lscpu(int argc, char* argv[]) {
    (void)argc; (void)argv;
    const cpu_info_t* cpu = cpu_get_info();
    kprintf("Architecture:          x86-64 (AMD64)\n");
    kprintf("CPU Vendor:            %s\n", cpu->vendor);
    kprintf("Model Name:            %s\n", cpu->brand);
    kprintf("Family/Model/Step:     %u / %u / %u\n", cpu->family, cpu->model, cpu->stepping);
    kprintf("Cores Detected:        %u\n", cpu->cores_detected);
    kprintf("Features:              %s%s%s%s%s%s%s\n",
            cpu->has_fpu  ? "fpu "  : "",
            cpu->has_tsc  ? "tsc "  : "",
            cpu->has_apic ? "apic " : "",
            cpu->has_sse  ? "sse "  : "",
            cpu->has_sse2 ? "sse2 " : "",
            cpu->has_sse3 ? "sse3 " : "",
            cpu->has_avx  ? "avx "  : "");
}

void cmd_lspci(int argc, char* argv[]) {
    (void)argc; (void)argv;
    uint32_t count = pci_get_device_count();
    kprintf("Found %u PCI devices:\n", count);
    for (uint32_t i = 0; i < count; i++) {
        const pci_device_t* dev = pci_get_device(i);
        kprintf("%02x:%02x.%d %s: %s (0x%04x:0x%04x)\n",
                dev->bus, dev->device, dev->function,
                pci_class_to_string(dev->class_id, dev->subclass_id),
                pci_vendor_to_string(dev->vendor_id),
                dev->vendor_id, dev->device_id);
    }
}

void cmd_free(int argc, char* argv[]) {
    (void)argc; (void)argv;
    const pmm_stats_t* pmm = pmm_get_stats();
    const kheap_stats_t* heap = kheap_get_stats();

    kprintf("               Total        Used        Free\n");
    kprintf("Physical RAM:  %-10lu  %-10lu  %-10lu (KiB)\n",
            pmm->total_memory / 1024, pmm->used_memory / 1024, pmm->free_memory / 1024);
    kprintf("Kernel Heap:   %-10lu  %-10lu  %-10lu (KiB)\n",
            heap->total_heap / 1024, heap->used_heap / 1024, heap->free_heap / 1024);
}

void shell_execute(char* line) {
    /* Tokenize line into argv */
    char* argv[16];
    int argc = 0;

    char* p = line;
    while (*p) {
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') *p++ = '\0';
        if (*p == '\0') break;

        if (argc < 16) {
            argv[argc++] = p;
        }
        while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') p++;
    }

    if (argc == 0) return;

    if (kstrcmp(argv[0], "help") == 0)     cmd_help(argc, argv);
    else if (kstrcmp(argv[0], "uname") == 0)    cmd_uname(argc, argv);
    else if (kstrcmp(argv[0], "uptime") == 0)   cmd_uptime(argc, argv);
    else if (kstrcmp(argv[0], "date") == 0)     cmd_date(argc, argv);
    else if (kstrcmp(argv[0], "ls") == 0)       cmd_ls(argc, argv);
    else if (kstrcmp(argv[0], "cat") == 0)      cmd_cat(argc, argv);
    else if (kstrcmp(argv[0], "mkdir") == 0)    cmd_mkdir(argc, argv);
    else if (kstrcmp(argv[0], "touch") == 0)    cmd_touch(argc, argv);
    else if (kstrcmp(argv[0], "ps") == 0)       cmd_ps(argc, argv);
    else if (kstrcmp(argv[0], "kill") == 0)     cmd_kill(argc, argv);
    else if (kstrcmp(argv[0], "lscpu") == 0)    cmd_lscpu(argc, argv);
    else if (kstrcmp(argv[0], "lspci") == 0)    cmd_lspci(argc, argv);
    else if (kstrcmp(argv[0], "free") == 0)     cmd_free(argc, argv);
    else if (kstrcmp(argv[0], "reboot") == 0)   hal_reboot();
    else if (kstrcmp(argv[0], "shutdown") == 0) hal_shutdown();
    else {
        kprintf("mysh: command not found: %s\n", argv[0]);
    }
}
