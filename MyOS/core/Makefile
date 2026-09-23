CC ?= gcc
LD ?= ld
NASM ?= nasm
GRUB_MKRESCUE ?= grub-mkrescue

CFLAGS := -m64 -ffreestanding -fno-stack-protector -fno-pie -no-pie \
          -mno-red-zone -mno-mmx -mno-sse -mno-sse2 \
          -Wall -Wextra -Werror -Wno-missing-braces -Wno-array-bounds -O2 -I.

all: MyOS.iso

boot.o: boot.s
	$(NASM) -f elf64 $< -o $@

kernel.o: kernel.c font8x8.inc
	$(CC) $(CFLAGS) -c $< -o $@

myos.bin: boot.o kernel.o linker.ld
	$(LD) -m elf_x86_64 -nostdlib -T linker.ld -o $@ boot.o kernel.o

MyOS.iso: myos.bin grub.cfg
	mkdir -p iso/boot/grub
	cp myos.bin iso/boot/myos.bin
	cp grub.cfg iso/boot/grub/grub.cfg
	$(GRUB_MKRESCUE) -o $@ iso

run: MyOS.iso
	qemu-system-x86_64 -cdrom MyOS.iso -serial stdio -m 256M -vga std

clean:
	rm -rf *.o myos.bin MyOS.iso iso

.PHONY: all run clean
