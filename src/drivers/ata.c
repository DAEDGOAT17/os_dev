#include "ata.h"
#include "io.h"
#include <stddef.h>

// ATA devices (Primary Master, Primary Slave, Secondary Master, Secondary
// Slave)
static ata_device_t ata_devices[4];

// Wait for drive to be ready with timeout
static bool ata_wait_ready(uint16_t base) {
  int timeout = 1000000;
  while (timeout--) {
    uint8_t status = inb(base + 7);
    if (!(status & ATA_SR_BSY))
      return true;
  }
  return false;
}

// Wait for data request
static bool ata_wait_drq(uint16_t base) {
  uint8_t status;
  int timeout = 1000000;

  while (timeout--) {
    status = inb(base + 7);
    if (status & ATA_SR_DRQ)
      return true;
    if (status & ATA_SR_ERR)
      return false;
  }
  return false;
}

// Soft reset
static void ata_soft_reset(uint16_t ctrl) {
  outb(ctrl, 0x04); // Set SRST bit
  for (int i = 0; i < 4; i++)
    inb(ctrl);      // Wait
  outb(ctrl, 0x00); // Clear SRST bit
}

// Identify drive
static bool ata_identify(ata_device_t *dev) {
  uint16_t buffer[256];

  // Check if the bus is even there
  uint8_t status = inb(dev->base + 7);
  if (status == 0xFF)
    return false; // Floating bus

  // Select drive
  outb(dev->base + 6, dev->drive);
  if (!ata_wait_ready(dev->base))
    return false;

  // Send IDENTIFY command
  outb(dev->base + 7, ATA_CMD_IDENTIFY);

  // Check again
  status = inb(dev->base + 7);
  if (status == 0)
    return false; // No drive here

  // Wait for response
  if (!ata_wait_ready(dev->base))
    return false;

  // Check for errors (SATA/ATAPI devices return non-zero in these ports)
  if (inb(dev->base + 4) != 0 || inb(dev->base + 5) != 0) {
    return false; // Not an ATA device
  }

  // Wait for data
  if (!ata_wait_drq(dev->base))
    return false;

  // Read identification data
  for (int i = 0; i < 256; i++) {
    buffer[i] = inw(dev->base);
  }

  // Extract model string (words 27-46)
  for (int i = 0; i < 40; i += 2) {
    dev->model[i] = buffer[27 + i / 2] >> 8;
    dev->model[i + 1] = buffer[27 + i / 2] & 0xFF;
  }
  dev->model[40] = '\0';

  // Get size in sectors (words 60-61 for 28-bit LBA)
  dev->size = ((uint32_t)buffer[61] << 16) | buffer[60];

  return true;
}

// Initialize ATA driver
void ata_init() {
  // Initialize device structures
  ata_devices[0].base = ATA_PRIMARY_DATA;
  ata_devices[0].ctrl = ATA_PRIMARY_CONTROL;
  ata_devices[0].drive = ATA_MASTER;

  ata_devices[1].base = ATA_PRIMARY_DATA;
  ata_devices[1].ctrl = ATA_PRIMARY_CONTROL;
  ata_devices[1].drive = ATA_SLAVE;

  ata_devices[2].base = ATA_SECONDARY_DATA;
  ata_devices[2].ctrl = ATA_SECONDARY_CONTROL;
  ata_devices[2].drive = ATA_MASTER;

  ata_devices[3].base = ATA_SECONDARY_DATA;
  ata_devices[3].ctrl = ATA_SECONDARY_CONTROL;
  ata_devices[3].drive = ATA_SLAVE;

  // Detect drives
  for (int i = 0; i < 4; i++) {
    ata_devices[i].exists = ata_identify(&ata_devices[i]);
  }
}

// Read sectors from drive
bool ata_read_sectors(uint8_t drive, uint32_t lba, uint8_t sectors,
                      uint16_t *buffer) {
  if (drive >= 4 || !ata_devices[drive].exists)
    return false;

  ata_device_t *dev = &ata_devices[drive];

  // Wait for drive to be ready
  if (!ata_wait_ready(dev->base))
    return false;

  // Select drive and set LBA mode
  outb(dev->base + 6, (dev->drive) | ((lba >> 24) & 0x0F));

  // Set sector count
  outb(dev->base + 2, sectors);

  // Set LBA
  outb(dev->base + 3, (uint8_t)lba);
  outb(dev->base + 4, (uint8_t)(lba >> 8));
  outb(dev->base + 5, (uint8_t)(lba >> 16));

  // Send READ command
  outb(dev->base + 7, ATA_CMD_READ_SECTORS);

  // Read sectors
  for (int s = 0; s < sectors; s++) {
    if (!ata_wait_drq(dev->base))
      return false;

    // Read 256 words (512 bytes)
    for (int i = 0; i < 256; i++) {
      buffer[s * 256 + i] = inw(dev->base);
    }
  }

  return true;
}

// Write sectors to drive
bool ata_write_sectors(uint8_t drive, uint32_t lba, uint8_t sectors,
                       uint16_t *buffer) {
  if (drive >= 4 || !ata_devices[drive].exists)
    return false;

  ata_device_t *dev = &ata_devices[drive];

  // Wait for drive to be ready
  if (!ata_wait_ready(dev->base))
    return false;

  // Select drive and set LBA mode
  outb(dev->base + 6, (dev->drive) | ((lba >> 24) & 0x0F));

  // Set sector count
  outb(dev->base + 2, sectors);

  // Set LBA
  outb(dev->base + 3, (uint8_t)lba);
  outb(dev->base + 4, (uint8_t)(lba >> 8));
  outb(dev->base + 5, (uint8_t)(lba >> 16));

  // Send WRITE command
  outb(dev->base + 7, ATA_CMD_WRITE_SECTORS);

  // Write sectors
  for (int s = 0; s < sectors; s++) {
    if (!ata_wait_drq(dev->base))
      return false;

    // Write 256 words (512 bytes)
    for (int i = 0; i < 256; i++) {
      outw(dev->base, buffer[s * 256 + i]);
    }
  }

  // Flush cache
  outb(dev->base + 7, ATA_CMD_CACHE_FLUSH);
  ata_wait_ready(dev->base);

  return true;
}

// Get device info
ata_device_t *ata_get_device(uint8_t drive) {
  if (drive >= 4)
    return NULL;
  if (!ata_devices[drive].exists)
    return NULL;
  return &ata_devices[drive];
}
