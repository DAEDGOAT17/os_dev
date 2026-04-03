#include "ata.h"
#include "io.h"
#include "screen.h"

// Drive parameters (populated via IDENTIFY)
static uint32_t total_sectors = 0;
static int drive_present = 0;

// Helper: wait for drive to not be busy
static void ata_wait_bsy() {
    while (inb(ATA_PRIMARY_BASE + ATA_REG_STATUS) & ATA_SR_BSY);
}

// Helper: wait for drive to be ready for data transfer
static void ata_wait_drq() {
    while (!(inb(ATA_PRIMARY_BASE + ATA_REG_STATUS) & ATA_SR_DRQ));
}

int ata_init(void) {
    print_string("ATA: Initializing Primary Master...\n");

    // Select drive (0x00 for Master)
    outb(ATA_PRIMARY_BASE + ATA_REG_HDDEVSEL, 0xA0);
    wait_io();

    // Send IDENTIFY command
    outb(ATA_PRIMARY_BASE + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    wait_io();

    // Check if status is 0 (no drive) or 0xFF (no bus/unassigned)
    uint8_t status = inb(ATA_PRIMARY_BASE + ATA_REG_STATUS);
    if (status == 0 || status == 0xFF) {
        if (status == 0) print_string("ATA: No drive detected (Status 0).\n");
        else print_string("ATA: Drive disabled/Empty bus (Status 0xFF).\n");
        return 0;
    }

    // Wait for BSY to clear with a simple timeout
    uint32_t timeout = 100000;
    while ((inb(ATA_PRIMARY_BASE + ATA_REG_STATUS) & ATA_SR_BSY) && timeout--);
    
    if (timeout == 0) {
        print_string("ATA: Timeout waiting for BSY.\n");
        return 0;
    }
    
    // Check for error
    if (inb(ATA_PRIMARY_BASE + ATA_REG_STATUS) & ATA_SR_ERR) {
        print_string("ATA: Error during IDENTIFY.\n");
        return 0;
    }

    ata_wait_drq();

    // Read IDENTIFY data (256 words)
    uint16_t data[256];
    for (int i = 0; i < 256; i++) {
        uint16_t val;
        asm volatile ("inw %1, %0" : "=a"(val) : "Nd"(ATA_PRIMARY_BASE + ATA_REG_DATA));
        data[i] = val;
    }

    // Total sectors is at offset 60 (2 words)
    total_sectors = *((uint32_t*)(data + 60));

    print_string("ATA: Drive found. Sectors: ");
    kprint_dec(total_sectors);
    print_string(" (");
    kprint_dec(total_sectors / 2048); // MB approx
    print_string(" MB)\n");

    drive_present = 1;
    return 1;
}

int ata_read_sectors(uint32_t lba, uint8_t count, void* buffer) {
    if (!drive_present) return -1;

    outb(ATA_PRIMARY_BASE + ATA_REG_HDDEVSEL, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_BASE + ATA_REG_FEATURES, 0x00);
    outb(ATA_PRIMARY_BASE + ATA_REG_SECCOUNT, count);
    outb(ATA_PRIMARY_BASE + ATA_REG_LBA0, (uint8_t)lba);
    outb(ATA_PRIMARY_BASE + ATA_REG_LBA1, (uint8_t)(lba >> 8));
    outb(ATA_PRIMARY_BASE + ATA_REG_LBA2, (uint8_t)(lba >> 16));
    outb(ATA_PRIMARY_BASE + ATA_REG_COMMAND, ATA_CMD_READ_PIO);

    uint16_t* ptr = (uint16_t*)buffer;

    for (int i = 0; i < count; i++) {
        ata_wait_bsy();
        ata_wait_drq();

        for (int j = 0; j < 256; j++) {
            uint16_t val;
            asm volatile ("inw %1, %0" : "=a"(val) : "Nd"(ATA_PRIMARY_BASE + ATA_REG_DATA));
            *ptr++ = val;
        }
    }

    return 0;
}

int ata_write_sectors(uint32_t lba, uint8_t count, const void* buffer) {
    if (!drive_present) return -1;

    outb(ATA_PRIMARY_BASE + ATA_REG_HDDEVSEL, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_BASE + ATA_REG_FEATURES, 0x00);
    outb(ATA_PRIMARY_BASE + ATA_REG_SECCOUNT, count);
    outb(ATA_PRIMARY_BASE + ATA_REG_LBA0, (uint8_t)lba);
    outb(ATA_PRIMARY_BASE + ATA_REG_LBA1, (uint8_t)(lba >> 8));
    outb(ATA_PRIMARY_BASE + ATA_REG_LBA2, (uint8_t)(lba >> 16));
    outb(ATA_PRIMARY_BASE + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);

    uint16_t* ptr = (uint16_t*)buffer;

    for (int i = 0; i < count; i++) {
        ata_wait_bsy();
        ata_wait_drq();

        for (int j = 0; j < 256; j++) {
            uint16_t val = *ptr++;
            asm volatile ("outw %0, %1" : : "a"(val), "Nd"(ATA_PRIMARY_BASE + ATA_REG_DATA));
        }
    }

    // Flush cache
    outb(ATA_PRIMARY_BASE + ATA_REG_COMMAND, 0xE7);
    ata_wait_bsy();

    return 0;
}

uint32_t ata_get_sector_count(void) {
    return total_sectors;
}
