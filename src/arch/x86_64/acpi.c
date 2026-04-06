#include "acpi.h"
#include "screen.h"

static uint32_t system_timer_gsi = 2; // Default fallback (ISA IRQ0 -> INTIN 2)

static inline int string_cmp(const char* a, const char* b, int len) {
    for (int i = 0; i < len; i++) {
        if (a[i] != b[i]) return 0;
    }
    return 1;
}

void acpi_init(void* rsdp_addr) {
    if (!rsdp_addr) return;
    
    acpi_rsdp_t* rsdp = (acpi_rsdp_t*)rsdp_addr;
    if (!string_cmp(rsdp->signature, "RSD PTR ", 8)) return;

    acpi_header_t* rsdt = (acpi_header_t*)(uint64_t)rsdp->rsdt_address;
    if (!rsdt || !string_cmp(rsdt->signature, "RSDT", 4)) {
        // Fallback to XSDT if RSDT is null or invalid (ACPI 2.0+)
        if (rsdp->revision >= 2 && rsdp->xsdt_address) {
            rsdt = (acpi_header_t*)(uint64_t)rsdp->xsdt_address;
            if (!rsdt || !string_cmp(rsdt->signature, "XSDT", 4)) return;
        } else {
            return;
        }
    }

    uint32_t entries = (rsdt->length - sizeof(acpi_header_t)) / (string_cmp(rsdt->signature, "XSDT", 4) ? 8 : 4);
    acpi_header_t* madt = 0;

    // Search for Multiple APIC Description Table (MADT/APIC)
    for (uint32_t i = 0; i < entries; i++) {
        acpi_header_t* table;
        if (string_cmp(rsdt->signature, "XSDT", 4)) {
            table = (acpi_header_t*)(*(uint64_t*)((uint64_t)rsdt + sizeof(acpi_header_t) + i * 8));
        } else {
            table = (acpi_header_t*)(uint64_t)(*(uint32_t*)((uint64_t)rsdt + sizeof(acpi_header_t) + i * 4));
        }

        if (table && string_cmp(table->signature, "APIC", 4)) {
            madt = table;
            break;
        }
    }

    if (!madt) return;

    // Parse MADT Records (Offset 0x2C from header skip local apic addr and flags)
    uint8_t* ptr = (uint8_t*)madt + sizeof(acpi_header_t) + 8; 
    uint8_t* end = (uint8_t*)madt + madt->length;

    while (ptr < end) {
        uint8_t type = ptr[0];
        uint8_t length = ptr[1];

        if (type == 2) { // Interrupt Source Override
            uint8_t bus = ptr[2];
            uint8_t irq = ptr[3];
            uint32_t gsi = *(uint32_t*)(ptr + 4);

            if (bus == 0 && irq == 0) { // Legacy ISA Timer
                system_timer_gsi = gsi;
            }
        }
        ptr += length;
    }
}

uint32_t get_timer_gsi() {
    return system_timer_gsi;
}
