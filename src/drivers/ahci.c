#include "ahci.h"
#include "screen.h"
#include "pci.h"

void ahci_init(uint32_t abar) {
    print_string("AHCI: Initializing HBA at 0x");
    kprint_hex(abar);
    print_char('\n');

    ahci_hba_t *hba = (ahci_hba_t *)(uint64_t)abar;
    
    // Check ports implemented
    uint32_t pi = hba->pi;
    for (int i = 0; i < 32; i++) {
        if (pi & (1 << i)) {
            // Port i is implemented
            uint32_t ssts = hba->ports[i].ssts;
            uint8_t det = ssts & 0x0F;
            uint8_t ipm = (ssts >> 8) & 0x0F;
            
            if (det == 3 && ipm == 1) { // Device present and active
                print_string("AHCI: Active drive detected on Port ");
                kprint_dec(i);
                print_string(" [Signature: 0x");
                kprint_hex(hba->ports[i].sig);
                print_string("]\n");
            }
        }
    }
}

void pci_init_ahci() {
    print_string("PCI: Scanning for AHCI Controllers...\n");
    for (uint32_t bus = 0; bus < 256; ++bus) {
        for (uint32_t device = 0; device < 32; ++device) {
            uint32_t vendor_dev = pci_read_config_dword(bus, device, 0, 0x00);
            if ((vendor_dev & 0xFFFF) == 0xFFFF) continue;

            uint32_t header = pci_read_config_dword(bus, device, 0, 0x0C);
            uint8_t header_type = (header >> 16) & 0xFF;
            uint32_t max_func = (header_type & 0x80) ? 8 : 1;

            for (uint32_t function = 0; function < max_func; ++function) {
                uint32_t id = pci_read_config_dword(bus, device, function, 0x00);
                if ((id & 0xFFFF) == 0xFFFF) continue;

                uint32_t cls = pci_read_config_dword(bus, device, function, 0x08);
                uint8_t class_code = (cls >> 24) & 0xFF;
                uint8_t subclass = (cls >> 16) & 0xFF;
                uint8_t prog_if = (cls >> 8) & 0xFF;

                // Class 0x01 (Storage), Subclass 0x06 (SATA), ProgIF 0x01 (AHCI)
                if (class_code == 0x01 && subclass == 0x06 && prog_if == 0x01) {
                    print_string("PCI: AHCI Controller Found at ");
                    kprint_dec(bus); print_char(':');
                    kprint_dec(device); print_char('.');
                    kprint_dec(function); print_char('\n');
                    
                    // ABAR is usually at BAR5 for AHCI (Offset 0x24)
                    uint32_t abar = pci_read_config_dword(bus, device, function, 0x24);
                    // Clear lower bits to get physical address
                    abar &= 0xFFFFFFF0;
                    
                    if (abar) {
                        ahci_init(abar);
                    }
                    return; // Initialize the first one we find
                }
            }
        }
    }
    print_string("PCI: No AHCI Controllers found.\n");
}
