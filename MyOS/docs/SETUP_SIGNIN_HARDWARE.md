# MyOS first-boot flow and hardware inventory

This change adds native framebuffer screens to the kernel desktop branch:

- **Setup screen:** choose installer preview, hardware detection, sign-in preview, or continue to the desktop as Guest.
- **Installer screen:** presents the intended language, disk selection, partition review, and install steps. It is explicitly a preview; it does not read, format, partition, or write to a disk.
- **Sign-in screen:** a Guest-session preview. Password storage, user accounts, and authentication are not implemented yet; Enter continues as Guest.
- **Hardware detection:** reports the CPU brand/vendor through CPUID, bootloader-reported memory, framebuffer dimensions, and a basic PCI scan of bus 0 for display, network, and storage controller classes.

## Keyboard controls

| Screen | Key | Action |
|---|---|---|
| Setup | I | Open installer preview |
| Setup | H | Open hardware detection |
| Setup | L | Open sign-in preview |
| Setup | Enter | Continue as Guest |
| Installer / Hardware / Sign-in | Esc | Return to Setup |
| Sign-in | Enter | Continue as Guest |
| Desktop | S | Open launcher |
| Desktop | Esc | Close launcher |

The UI can also be navigated with a PS/2 mouse where the boot environment exposes one.

## Detection limitations

- The current PCI inventory scans **bus 0 only**; it is not a complete PCI/PCIe topology walk and does not yet read GPU model names, USB devices, disks, or network interfaces.
- The memory figure is an estimate based on the Multiboot memory field supplied by GRUB. A full physical-memory map parser and safe allocator are still needed for reliable memory accounting.
- The existing boot path is a GRUB/Multiboot kernel image. A proper UEFI boot manager and signed boot chain are separate work.
- The installer cannot safely become a real installer until MyOS has disk drivers (at least AHCI/NVMe support as appropriate), a partition-table parser, a filesystem writer, a bootloader installation path, and a confirmation/rollback strategy.
- The sign-in page is a visual/session-flow prototype, not a security boundary. Do not enter or store real passwords.

## Validation

This change has been committed on feat/installer-signin-hardware. Run the repository's existing build and QEMU smoke tests before treating the screens or hardware report as validated on physical hardware.
