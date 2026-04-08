#include "ata.h"
#include "io.h"
#include "screen.h"
#include "string.h"
#include "pmm.h"
#include "fat32.h"

// Drive parameters (populated via IDENTIFY or ramdisk)
static uint32_t total_sectors = 0;
static int drive_present = 0;

static uint16_t active_base_port = ATA_PRIMARY_BASE;
static uint8_t active_drive_sel = 0xE0; // Default to Master
static int      ramdisk_mode = 0;
static uint8_t* ramdisk_ptr  = 0;
static uint32_t partition_offset = 0;

// Helper: wait for drive to not be busy
static void ata_wait_bsy() {
    uint32_t timeout = 500000;
    while ((inb(active_base_port + ATA_REG_STATUS) & ATA_SR_BSY) && timeout--);
}

// Helper: wait for drive to be ready for data transfer
static void ata_wait_drq() {
    uint32_t timeout = 500000;
    while (!(inb(active_base_port + ATA_REG_STATUS) & ATA_SR_DRQ) && timeout--);
}

// Try to detect a real legacy ATA/IDE drive.
// Scans Primary Master/Slave, then Secondary Master/Slave.
// Returns 1 if found, 0 otherwise.
static int ata_detect_real_drive(void) {
    uint16_t bases[2] = {ATA_PRIMARY_BASE, ATA_SECONDARY_BASE};
    uint8_t drives[2] = {0xA0, 0xB0};

    // Scan the 4 possible IDE slots
    for (int b = 0; b < 2; b++) {
        uint16_t base = bases[b];
        for (int d = 0; d < 2; d++) {
            uint8_t drive_sel = drives[d];
            
            // Select drive
            outb(base + ATA_REG_HDDEVSEL, drive_sel);
            wait_io();

            // Send IDENTIFY command
            outb(base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
            wait_io();

            // Status 0x00 = no drive;  0xFF = no bus
            uint8_t status = inb(base + ATA_REG_STATUS);
            if (status == 0 || status == 0xFF) continue;

            // Wait for BSY with timeout
            uint32_t timeout = 100000;
            while ((inb(base + ATA_REG_STATUS) & ATA_SR_BSY) && timeout--);
            if (timeout == 0) continue;

            // Any error during IDENTIFY → not a usable drive (e.g. it's ATAPI CDROM)
            if (inb(base + ATA_REG_STATUS) & ATA_SR_ERR) continue;

            // Wait for DRQ (data ready)
            timeout = 100000;
            while (!(inb(base + ATA_REG_STATUS) & ATA_SR_DRQ) && timeout--);
            if (timeout == 0) continue;

            // Read IDENTIFY data (256 words)
            uint16_t data[256] = {0};
            for (int i = 0; i < 256; i++) {
                data[i] = inw(base + ATA_REG_DATA);
            }

            total_sectors = *((uint32_t*)(data + 60));
            if (total_sectors > 0) {
                // Found a valid drive!
                active_base_port = base;
                active_drive_sel = (drive_sel == 0xB0) ? 0xF0 : 0xE0;
                return 1; 
            }
        }
    }
    return 0;
}

// Helper: check if a sector contains a valid FAT32 BPB
static int is_fat32_bpb(uint8_t* sector) {
    fat32_bpb_t* b = (fat32_bpb_t*)sector;
    // Basic checks: signature 0x28 or 0x29, plus "FAT32   " string, plus 512 bytes per sector
    if (b->boot_signature != 0x28 && b->boot_signature != 0x29) return 0;
    if (b->bytes_per_sector != 512) return 0;
    // Check "FAT32" string in the fs_type field (offset 0x52)
    if (memcmp(b->fs_type, "FAT32   ", 8) != 0) return 0;
    return 1;
}

int ata_init(void) {
    print_string("ATA: Detecting storage...\n");
    partition_offset = 0;

    // ── 1. Try real legacy ATA first ────────────────────────────────────────
    if (ata_detect_real_drive()) {
        uint8_t sector[512];
        drive_present = 1; // Allow ata_read_sectors to work
        if (ata_read_sectors(0, 1, sector) == 0) {
            // Check if LBA 0 is a direct FAT32 BPB (raw volume)
            if (is_fat32_bpb(sector)) {
                print_string("ATA: Legacy IDE drive (Raw FAT32) found.\n");
                partition_offset = 0;
                return 0; // Success
            }
            
            // Check if LBA 0 is an MBR and look for FAT32 partitions (0x0C or 0x0B)
            if (sector[510] == 0x55 && sector[511] == 0xAA) {
                for (int i = 0; i < 4; i++) {
                    uint8_t* entry = &sector[446 + (i * 16)];
                    uint8_t type = entry[4];
                    if (type == 0x0B || type == 0x0C) { // FAT32 (CHS or LBA)
                        uint32_t start_lba = *(uint32_t*)&entry[8];
                        uint8_t p_sector[512];
                        if (ata_read_sectors(start_lba, 1, p_sector) == 0) {
                            if (is_fat32_bpb(p_sector)) {
                                print_string("ATA: Legacy IDE drive (MBR Partition ");
                                kprint_dec(i + 1);
                                print_string(") found at LBA ");
                                kprint_dec(start_lba);
                                print_char('\n');
                                partition_offset = start_lba;
                                return 0; // Success
                            }
                        }
                    }
                }
            }
        }
        drive_present = 0; // Invalid filesystem, reset
        print_string("ATA: Legacy drive found but no valid FAT32 partition detected.\n");
    }

    // ── 2. Fallback: Embedded Ramdisk ───────────────────────────────────────
    if (ramdisk_loaded) {
        ramdisk_mode  = 1;
        ramdisk_ptr   = (uint8_t*)(uint64_t)ramdisk_start;
        total_sectors = (uint32_t)(ramdisk_size / ATA_SECTOR_SIZE);
        drive_present = 1;
        partition_offset = 0;

        print_string("ATA: Using embedded ramdisk (FAT32). Sectors: ");
        kprint_dec(total_sectors);
        print_char('\n');
        return 0; // Success
    }

    print_string("ATA: No storage detected.\n");
    return -1; // Failure
}

uint32_t ata_get_partition_offset(void) {
    return partition_offset;
}

int ata_is_ramdisk(void) {
    return ramdisk_mode;
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
    outb(active_base_port + ATA_REG_HDDEVSEL, active_drive_sel | ((lba >> 24) & 0x0F));
    outb(active_base_port + ATA_REG_FEATURES, 0x00);
    outb(active_base_port + ATA_REG_SECCOUNT, count);
    outb(active_base_port + ATA_REG_LBA0,    (uint8_t) lba);
    outb(active_base_port + ATA_REG_LBA1,    (uint8_t)(lba >>  8));
    outb(active_base_port + ATA_REG_LBA2,    (uint8_t)(lba >> 16));
    outb(active_base_port + ATA_REG_COMMAND,  ATA_CMD_READ_PIO);

    uint16_t* ptr = (uint16_t*)buffer;
    for (int i = 0; i < count; i++) {
        ata_wait_bsy();
        ata_wait_drq();
        for (int j = 0; j < 256; j++) {
            *ptr++ = inw(active_base_port + ATA_REG_DATA);
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
    outb(active_base_port + ATA_REG_HDDEVSEL, active_drive_sel | ((lba >> 24) & 0x0F));
    outb(active_base_port + ATA_REG_FEATURES, 0x00);
    outb(active_base_port + ATA_REG_SECCOUNT, count);
    outb(active_base_port + ATA_REG_LBA0,    (uint8_t) lba);
    outb(active_base_port + ATA_REG_LBA1,    (uint8_t)(lba >>  8));
    outb(active_base_port + ATA_REG_LBA2,    (uint8_t)(lba >> 16));
    outb(active_base_port + ATA_REG_COMMAND,  ATA_CMD_WRITE_PIO);

    uint16_t* ptr = (uint16_t*)buffer;
    for (int i = 0; i < count; i++) {
        ata_wait_bsy();
        ata_wait_drq();
        for (int j = 0; j < 256; j++) {
            outw(active_base_port + ATA_REG_DATA, *ptr++);
        }
    }

    // Flush write cache
    outb(active_base_port + ATA_REG_COMMAND, 0xE7);
    ata_wait_bsy();
    return 0;
}

uint32_t ata_get_sector_count(void) {
    return total_sectors;
}
