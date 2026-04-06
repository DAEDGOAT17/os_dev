#include "ata.h"
#include "io.h"
#include "screen.h"
#include "string.h"
#include "pmm.h"

// Drive parameters (populated via IDENTIFY or ramdisk)
static uint32_t total_sectors = 0;
static int drive_present = 0;

// Ramdisk mode: set when no real ATA drive found but a ramdisk module is available
static int      ramdisk_mode = 0;
static uint8_t* ramdisk_ptr  = 0;

// Helper: wait for drive to not be busy
static void ata_wait_bsy() {
    uint32_t timeout = 500000;
    while ((inb(ATA_PRIMARY_BASE + ATA_REG_STATUS) & ATA_SR_BSY) && timeout--);
}

// Helper: wait for drive to be ready for data transfer
static void ata_wait_drq() {
    uint32_t timeout = 500000;
    while (!(inb(ATA_PRIMARY_BASE + ATA_REG_STATUS) & ATA_SR_DRQ) && timeout--);
}

// Try to detect a real legacy ATA/IDE drive (primary master, PIO mode).
// Returns 1 if found, 0 otherwise.
static int ata_detect_real_drive(void) {
    // Select master drive
    outb(ATA_PRIMARY_BASE + ATA_REG_HDDEVSEL, 0xA0);
    wait_io();

    // Send IDENTIFY command
    outb(ATA_PRIMARY_BASE + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    wait_io();

    // Status 0x00 = no drive;  0xFF = no bus (AHCI/NVMe laptop)
    uint8_t status = inb(ATA_PRIMARY_BASE + ATA_REG_STATUS);
    if (status == 0 || status == 0xFF) {
        return 0;
    }

    // Wait for BSY with timeout
    uint32_t timeout = 100000;
    while ((inb(ATA_PRIMARY_BASE + ATA_REG_STATUS) & ATA_SR_BSY) && timeout--);
    if (timeout == 0) return 0;

    // Any error during IDENTIFY → not a usable drive
    if (inb(ATA_PRIMARY_BASE + ATA_REG_STATUS) & ATA_SR_ERR) return 0;

    // Wait for DRQ (data ready)
    ata_wait_drq();

    // Read IDENTIFY data (256 words)
    uint16_t data[256] = {0};
    for (int i = 0; i < 256; i++) {
        data[i] = inw(ATA_PRIMARY_BASE + ATA_REG_DATA);
    }

    total_sectors = *((uint32_t*)(data + 60));
    return (total_sectors > 0) ? 1 : 0;
}

int ata_init(void) {
    print_string("ATA: Detecting storage...\n");

    // ── 1. Try real legacy ATA first ────────────────────────────────────────
    if (ata_detect_real_drive()) {
        print_string("ATA: Legacy IDE drive found. Sectors: ");
        kprint_dec(total_sectors);
        print_string(" (");
        kprint_dec(total_sectors / 2048);
        print_string(" MB)\n");
        drive_present = 1;
        return 1;
    }

    // ── 2. No real drive – try the embedded ramdisk ──────────────────────────
    // The ramdisk is a FAT32 disk.img embedded in the ISO as a Multiboot2
    // module with the label "disk".  pmm_init() detected it earlier and stored
    // its physical address + size in ramdisk_start / ramdisk_size.
    if (ramdisk_loaded) {
        ramdisk_mode  = 1;
        ramdisk_ptr   = (uint8_t*)(uint64_t)ramdisk_start;
        total_sectors = (uint32_t)(ramdisk_size / ATA_SECTOR_SIZE);
        drive_present = 1;

        print_string("ATA: No legacy disk. Using embedded ramdisk. Sectors: ");
        kprint_dec(total_sectors);
        print_string(" (");
        kprint_dec(total_sectors / 2048);
        print_string(" MB)\n");
        return 1;
    }

    // ── 3. Nothing found ────────────────────────────────────────────────────
    print_string("ATA: No disk and no ramdisk module found.\n");
    return 0;
}

int ata_read_sectors(uint32_t lba, uint8_t count, void* buffer) {
    if (!drive_present) return -1;

    // ── Ramdisk path: simple memory copy ────────────────────────────────────
    if (ramdisk_mode) {
        uint64_t offset = (uint64_t)lba * ATA_SECTOR_SIZE;
        uint64_t length = (uint64_t)count * ATA_SECTOR_SIZE;
        if (offset + length > ramdisk_size) return -1;
        memcpy(buffer, ramdisk_ptr + offset, (uint32_t)length);
        return 0;
    }

    // ── Legacy ATA PIO path ──────────────────────────────────────────────────
    outb(ATA_PRIMARY_BASE + ATA_REG_HDDEVSEL, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_BASE + ATA_REG_FEATURES, 0x00);
    outb(ATA_PRIMARY_BASE + ATA_REG_SECCOUNT, count);
    outb(ATA_PRIMARY_BASE + ATA_REG_LBA0,    (uint8_t) lba);
    outb(ATA_PRIMARY_BASE + ATA_REG_LBA1,    (uint8_t)(lba >>  8));
    outb(ATA_PRIMARY_BASE + ATA_REG_LBA2,    (uint8_t)(lba >> 16));
    outb(ATA_PRIMARY_BASE + ATA_REG_COMMAND,  ATA_CMD_READ_PIO);

    uint16_t* ptr = (uint16_t*)buffer;
    for (int i = 0; i < count; i++) {
        ata_wait_bsy();
        ata_wait_drq();
        for (int j = 0; j < 256; j++) {
            *ptr++ = inw(ATA_PRIMARY_BASE + ATA_REG_DATA);
        }
    }
    return 0;
}

int ata_write_sectors(uint32_t lba, uint8_t count, const void* buffer) {
    if (!drive_present) return -1;

    // ── Ramdisk path: write to RAM (volatile – lost on reboot) ──────────────
    if (ramdisk_mode) {
        uint64_t offset = (uint64_t)lba * ATA_SECTOR_SIZE;
        uint64_t length = (uint64_t)count * ATA_SECTOR_SIZE;
        if (offset + length > ramdisk_size) return -1;
        memcpy(ramdisk_ptr + offset, buffer, (uint32_t)length);
        return 0;
    }

    // ── Legacy ATA PIO path ──────────────────────────────────────────────────
    outb(ATA_PRIMARY_BASE + ATA_REG_HDDEVSEL, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_BASE + ATA_REG_FEATURES, 0x00);
    outb(ATA_PRIMARY_BASE + ATA_REG_SECCOUNT, count);
    outb(ATA_PRIMARY_BASE + ATA_REG_LBA0,    (uint8_t) lba);
    outb(ATA_PRIMARY_BASE + ATA_REG_LBA1,    (uint8_t)(lba >>  8));
    outb(ATA_PRIMARY_BASE + ATA_REG_LBA2,    (uint8_t)(lba >> 16));
    outb(ATA_PRIMARY_BASE + ATA_REG_COMMAND,  ATA_CMD_WRITE_PIO);

    uint16_t* ptr = (uint16_t*)buffer;
    for (int i = 0; i < count; i++) {
        ata_wait_bsy();
        ata_wait_drq();
        for (int j = 0; j < 256; j++) {
            outw(ATA_PRIMARY_BASE + ATA_REG_DATA, *ptr++);
        }
    }

    // Flush write cache
    outb(ATA_PRIMARY_BASE + ATA_REG_COMMAND, 0xE7);
    ata_wait_bsy();
    return 0;
}

uint32_t ata_get_sector_count(void) {
    return total_sectors;
}
