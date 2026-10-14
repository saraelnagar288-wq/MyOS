# MyOS first-run flow: current behavior and limits

This milestone adds native framebuffer screens to the bootable x86-64 kernel.

## Flow

1. The kernel starts on the **Welcome to MyOS** screen.
2. Choose **Try MyOS** to open the existing live desktop without changing disks.
3. Choose **Install MyOS** to view the installation-readiness preview.
4. The **Sign in** screen currently provides a demo session only. Continuing opens the desktop.

## Keyboard controls

- Welcome: **Enter** or **T** to try MyOS; **I** to open Install preview.
- Install preview: **Esc** to go back; **T** to try MyOS.
- Sign in: **Enter** to continue the demo session; **Esc** to go back.
- Desktop: **S** opens the launcher; **Esc** closes it; **U** opens update settings where supported.

A PS/2 mouse can navigate the screens when the boot environment exposes compatible mouse hardware.

## Important safety and security limits

- The Install screen is **preview only**. No disk detection, partitioning, formatting, file copying, or bootloader installation is performed.
- Sign in is a **demo session**, not authentication. There are no protected user accounts or password verification. Do not enter real credentials.
- The current kernel has no persistent filesystem-backed home directory; desktop app panels are not a replacement for real file storage.
- The update settings panel does not download or install updates. A real updater still needs authenticated transport, signed release metadata, signature verification, safe image staging, and rollback.
- A successful QEMU boot test verifies kernel startup only; it does not establish physical-hardware compatibility or production readiness.

## Next implementation gates

1. Add a disk enumeration layer and read-only device/partition inspection first.
2. Add a filesystem reader/writer and tests against disposable virtual disks.
3. Implement an installer transaction with explicit target confirmation, checks, recovery, and no writes to host disks during tests.
4. Implement a real user database and password hashing only after a trusted storage and permission model exists.
5. Add a persistent filesystem and connect My Files/My Settings to real data.
6. Implement update signature verification and staged rollback before enabling automatic installation.
