cmake_minimum_required(VERSION 3.16)
project(MyOS LANGUAGES C ASM_NASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)

# Freestanding bare-metal kernel C compiler flags
set(KERNEL_C_FLAGS
    -ffreestanding
    -fno-builtin
    -fno-stack-protector
    -fno-pie
    -no-pie
    -m64
    -mno-red-zone
    -mno-mmx
    -mno-sse
    -mno-sse2
    -mcmodel=kernel
    -Wall
    -Wextra
    -O2
    -I${CMAKE_CURRENT_SOURCE_DIR}/include
)

set(CMAKE_EXE_LINKER_FLAGS
    "-nostdlib -no-pie -Wl,-m,elf_x86_64 -Wl,-z,max-page-size=0x1000 -T ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld"
)

# Source files
set(ASM_SOURCES
    boot/boot.asm
    arch/x86_64/gdt_flush.asm
    arch/x86_64/idt_stubs.asm
    scheduler/switch.asm
)

set(C_SOURCES
    kernel/kernel.c
    kernel/string.c
    kernel/printf.c
    kernel/syscall.c
    arch/x86_64/cpu.c
    arch/x86_64/gdt.c
    arch/x86_64/idt.c
    arch/x86_64/pic.c
    memory/pmm.c
    memory/vmm.c
    memory/kheap.c
    drivers/serial.c
    drivers/pit.c
    drivers/rtc.c
    drivers/keyboard.c
    drivers/mouse.c
    drivers/input.c
    drivers/pci.c
    drivers/font8x16.c
    hal/hal.c
    fs/vfs.c
    fs/ramfs.c
    scheduler/process.c
    scheduler/scheduler.c
    graphics/framebuffer.c
    graphics/window.c
    desktop/desktop.c
    apps/terminal.c
    apps/filemanager.c
    apps/taskmanager.c
    apps/sysinfo.c
    apps/settings.c
)

add_executable(myos.bin ${ASM_SOURCES} ${C_SOURCES})

target_compile_options(myos.bin PRIVATE
    $<$<COMPILE_LANGUAGE:C>:${KERNEL_C_FLAGS}>
    $<$<COMPILE_LANGUAGE:ASM_NASM>:-f;elf64>
)

# Custom target to build bootable MyOS.iso using grub-mkrescue / xorriso
add_custom_target(iso
    COMMAND ${CMAKE_COMMAND} -E make_directory iso_root/boot/grub
    COMMAND ${CMAKE_COMMAND} -E copy myos.bin iso_root/boot/myos.bin
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/boot/grub.cfg iso_root/boot/grub/grub.cfg
    COMMAND grub-mkrescue -o ${CMAKE_CURRENT_SOURCE_DIR}/build/MyOS.iso iso_root
    DEPENDS myos.bin
    COMMENT "Generating bootable MyOS.iso"
)
