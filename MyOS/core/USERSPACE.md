# MyOS Native OS Roadmap

## Phase 1 — Bootable desktop
- x86-64 Multiboot boot
- framebuffer desktop
- keyboard and PS/2 mouse
- basic window manager

## Phase 2 — Real OS foundation
- IDT/GDT/TSS
- interrupt-driven keyboard and mouse
- physical/virtual memory manager
- heap allocator
- timer
- syscall boundary
- process/thread model
- user mode

## Phase 3 — Storage
- PCI discovery
- ATA/AHCI disk driver
- VFS
- FAT32 filesystem
- persistent configuration and user database

## Phase 4 — Accounts
- first-boot setup
- Create Account
- Sign In / Sign Out
- lock screen
- guest session
- admin/standard users
- password hashing; never store plaintext passwords

## Phase 5 — Installer
- Live mode
- disk detection
- explicit destructive-action confirmation
- partition/format support
- install boot files + kernel + system files
- create first user
- first boot wizard

## Phase 6 — Desktop apps
- File Manager
- Terminal
- Settings
- Task Manager
- System Information
- Text Editor
- Calculator

## Phase 7 — Hardware and networking
- PCI
- USB
- storage
- audio
- Ethernet/Wi-Fi where hardware support is available
- AMD/Intel/NVIDIA graphics paths

## Safety rule
The installer must never silently format or overwrite a disk. Destructive storage operations require an explicit confirmation screen showing the target device and action.
