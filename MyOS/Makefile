# MyOS Makefile for x86-64 Operating System

CC = gcc
LD = ld
NASM = nasm

CFLAGS = -m64 -ffreestanding -fno-stack-protector -fno-pic -fno-pie \
         -mno-red-zone -nostdlib -Wall -Wextra -Iinclude -O2 -g

NASMFLAGS = -f elf64

KERNEL_BIN = myos.kernel
ISO_NAME = MyOS.iso

# Source files
ASM_BOOT = boot/boot.s
ASM_SRCS = $(ASM_BOOT) \
           arch/x86_64/gdt_flush.s \
           arch/x86_64/idt_asm.s

C_SRCS = kernel/main.c \
         kernel/klog.c \
         kernel/process.c \
         kernel/scheduler.c \
         kernel/syscall.c \
         arch/x86_64/gdt.c \
         arch/x86_64/idt.c \
         arch/x86_64/cpu.c \
         memory/pmm.c \
         memory/vmm.c \
         memory/kheap.c \
         hal/hal.c \
         drivers/serial.c \
         drivers/pit.c \
         drivers/rtc.c \
         drivers/keyboard.c \
         drivers/mouse.c \
         drivers/pci.c \
         drivers/input.c \
         drivers/font8x16.c \
         drivers/fb.c \
         fs/vfs.c \
         fs/ramfs.c \
         gui/window.c \
         gui/desktop.c \
         user/shell.c \
         user/init.c

ASM_OBJS = $(ASM_SRCS:.s=.o)
C_OBJS = $(C_SRCS:.c=.o)
OBJS = $(ASM_OBJS) $(C_OBJS)

.PHONY: all clean iso qemu

all: $(KERNEL_BIN) $(ISO_NAME)

%.o: %.s
	$(NASM) $(NASMFLAGS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_BIN): $(OBJS) linker.ld
	$(LD) -n -T linker.ld -o $@ $(OBJS)

$(ISO_NAME): $(KERNEL_BIN) boot/grub.cfg
	mkdir -p isodir/boot/grub
	cp $(KERNEL_BIN) isodir/boot/$(KERNEL_BIN)
	cp boot/grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO_NAME) isodir/

qemu: $(ISO_NAME)
	qemu-system-x86_64 -cdrom $(ISO_NAME) -serial stdio -m 512M -vga std

clean:
	rm -rf $(OBJS) $(KERNEL_BIN) $(ISO_NAME) isodir
