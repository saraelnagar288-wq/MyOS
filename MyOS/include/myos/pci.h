#ifndef MYOS_PCI_H
#define MYOS_PCI_H

#include <myos/types.h>

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

#define PCI_MAX_DEVICES 64

typedef struct {
    uint8_t  bus;
    uint8_t  device;
    uint8_t  function;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t  class_id;
    uint8_t  subclass_id;
    uint8_t  prog_if;
    uint8_t  revision;
    uint32_t bar[6];
    uint8_t  irq_line;
    uint8_t  irq_pin;
} pci_device_t;

void pci_init(void);
void pci_scan(void);
uint32_t pci_get_device_count(void);
const pci_device_t* pci_get_device(uint32_t index);
const char* pci_class_to_string(uint8_t class_id, uint8_t subclass_id);
const char* pci_vendor_to_string(uint16_t vendor_id);

uint32_t pci_read_config(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset);
void pci_write_config(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint32_t val);

#endif /* MYOS_PCI_H */
