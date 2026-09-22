#include <myos/pci.h>
#include <myos/cpu.h>
#include <myos/kernel.h>

static pci_device_t g_pci_devices[PCI_MAX_DEVICES];
static uint32_t     g_pci_device_count = 0;

uint32_t pci_read_config(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((1U << 31)
                                | ((uint32_t)bus << 16)
                                | ((uint32_t)device << 11)
                                | ((uint32_t)func << 8)
                                | (offset & 0xFC));
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

void pci_write_config(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint32_t val) {
    uint32_t address = (uint32_t)((1U << 31)
                                | ((uint32_t)bus << 16)
                                | ((uint32_t)device << 11)
                                | ((uint32_t)func << 8)
                                | (offset & 0xFC));
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, val);
}

static uint16_t pci_get_vendor_id(uint8_t bus, uint8_t dev, uint8_t func) {
    uint32_t r0 = pci_read_config(bus, dev, func, 0);
    return (uint16_t)(r0 & 0xFFFF);
}

static void pci_check_function(uint8_t bus, uint8_t dev, uint8_t func) {
    uint32_t r0 = pci_read_config(bus, dev, func, 0x00);
    uint16_t vendor_id = (uint16_t)(r0 & 0xFFFF);
    if (vendor_id == 0xFFFF) return; /* Device doesn't exist */

    uint16_t device_id = (uint16_t)((r0 >> 16) & 0xFFFF);

    uint32_t r2 = pci_read_config(bus, dev, func, 0x08);
    uint8_t revision    = (uint8_t)(r2 & 0xFF);
    uint8_t prog_if     = (uint8_t)((r2 >> 8) & 0xFF);
    uint8_t subclass_id = (uint8_t)((r2 >> 16) & 0xFF);
    uint8_t class_id    = (uint8_t)((r2 >> 24) & 0xFF);

    uint32_t r15 = pci_read_config(bus, dev, func, 0x3C);
    uint8_t irq_line = (uint8_t)(r15 & 0xFF);
    uint8_t irq_pin  = (uint8_t)((r15 >> 8) & 0xFF);

    if (g_pci_device_count < PCI_MAX_DEVICES) {
        pci_device_t* pdev = &g_pci_devices[g_pci_device_count++];
        pdev->bus         = bus;
        pdev->device      = dev;
        pdev->function    = func;
        pdev->vendor_id   = vendor_id;
        pdev->device_id   = device_id;
        pdev->class_id    = class_id;
        pdev->subclass_id = subclass_id;
        pdev->prog_if     = prog_if;
        pdev->revision    = revision;
        pdev->irq_line    = irq_line;
        pdev->irq_pin     = irq_pin;

        /* Read BARs 0..5 */
        for (int b = 0; b < 6; b++) {
            pdev->bar[b] = pci_read_config(bus, dev, func, 0x10 + (b * 4));
        }

        LOG_INFO("PCI", "%02x:%02x.%d [%s] %s (0x%04x:0x%04x)",
                 bus, dev, func,
                 pci_class_to_string(class_id, subclass_id),
                 pci_vendor_to_string(vendor_id),
                 vendor_id, device_id);
    }
}

static void pci_check_device(uint8_t bus, uint8_t dev) {
    uint16_t vendor_id = pci_get_vendor_id(bus, dev, 0);
    if (vendor_id == 0xFFFF) return;

    pci_check_function(bus, dev, 0);

    /* Check if multi-function device */
    uint32_t r3 = pci_read_config(bus, dev, 0, 0x0C);
    uint8_t header_type = (uint8_t)((r3 >> 16) & 0xFF);
    if (header_type & 0x80) {
        for (uint8_t func = 1; func < 8; func++) {
            if (pci_get_vendor_id(bus, dev, func) != 0xFFFF) {
                pci_check_function(bus, dev, func);
            }
        }
    }
}

void pci_scan(void) {
    g_pci_device_count = 0;
    LOG_INFO("PCI", "Scanning PCI buses...");

    for (uint16_t bus = 0; bus < 8; bus++) {
        for (uint8_t dev = 0; dev < 32; dev++) {
            pci_check_device((uint8_t)bus, dev);
        }
    }

    LOG_INFO("PCI", "Scan completed. Found %u PCI devices", g_pci_device_count);
}

void pci_init(void) {
    pci_scan();
}

uint32_t pci_get_device_count(void) {
    return g_pci_device_count;
}

const pci_device_t* pci_get_device(uint32_t index) {
    if (index >= g_pci_device_count) return NULL;
    return &g_pci_devices[index];
}

const char* pci_class_to_string(uint8_t class_id, uint8_t subclass_id) {
    switch (class_id) {
        case 0x00: return "Legacy Device";
        case 0x01:
            switch (subclass_id) {
                case 0x01: return "IDE Interface";
                case 0x06: return "SATA Controller (AHCI)";
                case 0x08: return "Non-Volatile Memory (NVMe)";
                default:   return "Mass Storage Controller";
            }
        case 0x02:
            switch (subclass_id) {
                case 0x00: return "Ethernet Controller";
                case 0x80: return "Network Controller";
                default:   return "Network Controller";
            }
        case 0x03:
            switch (subclass_id) {
                case 0x00: return "VGA Display Controller";
                default:   return "Display Controller";
            }
        case 0x04: return "Multimedia Device";
        case 0x05: return "Memory Controller";
        case 0x06: return "PCI-to-PCI Bridge";
        case 0x07: return "Simple Communication Controller";
        case 0x08: return "Base System Peripheral";
        case 0x09: return "Input Device Controller";
        case 0x0C:
            switch (subclass_id) {
                case 0x03: return "USB Controller (UHCI/OHCI/EHCI/xHCI)";
                case 0x05: return "SMBus Controller";
                default:   return "Serial Bus Controller";
            }
        default:   return "Unknown Device Class";
    }
}

const char* pci_vendor_to_string(uint16_t vendor_id) {
    switch (vendor_id) {
        case 0x8086: return "Intel Corporation";
        case 0x1022: return "Advanced Micro Devices (AMD)";
        case 0x10DE: return "NVIDIA Corporation";
        case 0x10EC: return "Realtek Semiconductor";
        case 0x1AF4: return "Red Hat / VirtIO";
        case 0x1234: return "QEMU Standard VGA / Bochs";
        case 0x15AD: return "VMware Inc.";
        case 0x80EE: return "Oracle VirtualBox";
        default:     return "Generic PCI Vendor";
    }
}
