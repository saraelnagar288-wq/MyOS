# MyOS networking and TLS implementation plan

## Current status

The native `MyOS/core` kernel currently has framebuffer, keyboard/mouse input and a serial log. It does **not** yet contain a network interface driver, Ethernet frame I/O, ARP, IPv4, DHCP, DNS, TCP, TLS, or an in-kernel update client. The update settings screen is UI only. Do not report the VM as online or enable update installation until those layers are implemented and tested.

The separate `MyOS/drivers/pci.c` file enumerates PCI devices in the other kernel tree, but PCI enumeration is not a network driver and does not prove connectivity.

## Stage 1 — Hyper-V test environment

1. Keep the host Windows network connection unchanged; do not bridge or disable the host Wi-Fi adapter.
2. In Hyper-V Manager, open the VM's **Settings → Network Adapter** and attach it to the existing external or default virtual switch. This gives the guest a virtual Ethernet path that uses the host's connectivity; it does not pass the host Wi-Fi radio through as a guest Wi-Fi device.
3. For a kernel that does not yet support Hyper-V's synthetic NIC, evaluate a **Legacy Network Adapter** only if the VM generation supports it. Treat this as a separate emulated-device target; identify its PCI vendor/device ID in the guest before writing a driver.
4. First prove connectivity in a supported guest OS or a small driver test. A working Windows host or VM switch alone is not proof that MyOS can transmit packets.

## Stage 2 — Kernel network interface

Implement in small reviewable changes:
- PCI enumeration across the relevant buses and a network-device inventory.
- A driver for one explicitly selected virtual NIC; bounds-checked RX/TX rings/buffers and timeouts.
- Ethernet framing and ARP, then IPv4 with checksum and packet-length validation.
- DHCP lease acquisition and DNS lookup with bounded retries/timeouts.
- TCP with retransmission, sequence/window validation, and connection teardown.
- A network status API for the desktop. Report **Disconnected**, **Link**, **DHCP**, and **Online** only when each state is actually observed.

Do not label the guest Wi-Fi-connected: with a Hyper-V virtual switch the guest normally sees Ethernet, even when the host's upstream connection is Wi-Fi.

## Stage 3 — TLS and updates

- Use a maintained, portable TLS 1.2+ implementation that can be built for the kernel or a future user-mode environment; do not invent cryptography.
- Provide a trustworthy source of time or a documented certificate-validation policy. Validate certificate chain, hostname, validity, and supported algorithms. Never silently fall back to plaintext or accept all certificates.
- Fetch only HTTPS update metadata from a pinned/configured origin.
- Verify signed release metadata using a public key embedded in or securely provisioned to MyOS. SHA-256 alone detects corruption but does not authenticate who published an update.
- Stage downloads to a temporary location, verify size/hash/signature, and only then offer installation. Keep rollback/recovery; automatic installation remains disabled until these are tested.

## Stage 4 — Physical HP ProDesk support

After the virtual adapter path works, inventory the physical Wi-Fi adapter's PCI/USB IDs and choose a supported driver. Wi-Fi requires additional 802.11 scanning/authentication and WPA2/WPA3 support; it is not supplied by TCP/TLS. Keep the Ethernet/Hyper-V path working as a fallback.

## Acceptance tests

- A known virtual adapter is identified by vendor/device ID.
- DHCP obtains a valid address, gateway and DNS server, with failure shown clearly.
- DNS resolves a test hostname; TCP connects to a controlled test endpoint.
- TLS succeeds for the valid test certificate and fails for expired, hostname-mismatched and untrusted certificates.
- Corrupt or unsigned update metadata and altered ISO files are rejected.
- No update is installed on transport, DNS, TLS, signature, or disk-write failure.
- QEMU/Hyper-V testing uses disposable virtual disks; never test installer/update writes against host disks.

## Safety gate

Until these acceptance tests pass, the desktop must state that online updates are unavailable. A mock status, successful host-side Python request, or visible network icon is not evidence of in-kernel connectivity.
