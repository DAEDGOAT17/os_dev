// pci.h
#ifndef PCI_H
#define PCI_H

#include <stdint.h>

// Read a 32-bit config dword from PCI configuration space
uint32_t pci_read_config_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);

// Scan the PCI bus and print any Multimedia Audio Controllers (class 0x04, subclass 0x03)
void pci_scan_multimedia();

#endif
