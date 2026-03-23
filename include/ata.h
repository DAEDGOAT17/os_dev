#ifndef ATA_H
#define ATA_H

#include <stdbool.h>
#include <stdint.h>

// ATA Registers
#define ATA_PRIMARY_DATA 0x1F0
#define ATA_PRIMARY_ERR 0x1F1
#define ATA_PRIMARY_SECCOUNT 0x1F2
#define ATA_PRIMARY_LBA_LO 0x1F3
#define ATA_PRIMARY_LBA_MID 0x1F4
#define ATA_PRIMARY_LBA_HI 0x1F5
#define ATA_PRIMARY_DRIVE_SEL 0x1F6
#define ATA_PRIMARY_COMMAND 0x1F7
#define ATA_PRIMARY_CONTROL 0x3F6

#define ATA_SECONDARY_DATA 0x170
#define ATA_SECONDARY_CONTROL 0x376

// ATA Commands
#define ATA_CMD_READ_SECTORS 0x20
#define ATA_CMD_WRITE_SECTORS 0x30
#define ATA_CMD_IDENTIFY 0xEC
#define ATA_CMD_CACHE_FLUSH 0xE7

// ATA Status Bits
#define ATA_SR_BSY 0x80
#define ATA_SR_DRDY 0x40
#define ATA_SR_DF 0x20
#define ATA_SR_DSC 0x10
#define ATA_SR_DRQ 0x08
#define ATA_SR_CORR 0x04
#define ATA_SR_IDX 0x02
#define ATA_SR_ERR 0x01

// Drive Selection
#define ATA_MASTER 0xA0
#define ATA_SLAVE 0xB0

typedef struct {
  uint16_t base;
  uint16_t ctrl;
  uint8_t drive;
  bool exists;
  char model[41];
  uint32_t size;
} ata_device_t;

void ata_init();
bool ata_read_sectors(uint8_t drive, uint32_t lba, uint8_t sectors,
                      uint16_t *buffer);
bool ata_write_sectors(uint8_t drive, uint32_t lba, uint8_t sectors,
                       uint16_t *buffer);
ata_device_t *ata_get_device(uint8_t drive);

#endif
