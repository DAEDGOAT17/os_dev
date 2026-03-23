#ifndef FAT32_H
#define FAT32_H

#include "vfs.h"
#include <stdbool.h>
#include <stdint.h>

// FAT32 Attributes
#define FAT_ATTR_READ_ONLY 0x01
#define FAT_ATTR_HIDDEN 0x02
#define FAT_ATTR_SYSTEM 0x04
#define FAT_ATTR_VOLUME_ID 0x08
#define FAT_ATTR_DIRECTORY 0x10
#define FAT_ATTR_ARCHIVE 0x20
#define FAT_ATTR_LFN 0x0F

#define FAT32_EOC 0x0FFFFFF8

typedef struct __attribute__((packed)) {
  uint8_t bootjmp[3];
  uint8_t oem_name[8];
  uint16_t bytes_per_sector;
  uint8_t sectors_per_cluster;
  uint16_t reserved_sectors;
  uint8_t fat_count;
  uint16_t root_entry_count;
  uint16_t total_sectors_16;
  uint8_t media_type;
  uint16_t fat_size_16;
  uint16_t sectors_per_track;
  uint16_t head_count;
  uint32_t hidden_sectors;
  uint32_t total_sectors_32;

  // FAT32 specific
  uint32_t fat_size_32;
  uint16_t extended_flags;
  uint16_t fat_version;
  uint32_t root_cluster;
  uint16_t fs_info;
  uint16_t backup_boot_sector;
  uint8_t reserved[12];
  uint8_t drive_number;
  uint8_t reserved1;
  uint8_t boot_signature;
  uint32_t volume_id;
  uint8_t volume_label[11];
  uint8_t file_system_type[8];
} fat32_boot_sector_t;

typedef struct __attribute__((packed)) {
  uint8_t name[11];
  uint8_t attributes;
  uint8_t reserved;
  uint8_t creation_time_tenth;
  uint16_t creation_time;
  uint16_t creation_date;
  uint16_t last_access_date;
  uint16_t first_cluster_hi;
  uint16_t write_time;
  uint16_t write_date;
  uint16_t first_cluster_lo;
  uint32_t file_size;
} fat32_dir_entry_t;

typedef struct {
  uint8_t drive;
  fat32_boot_sector_t boot_sector;
  uint32_t fat_start;
  uint32_t data_start;
  uint32_t root_dir_cluster;
} fat32_fs_t;

bool fat32_init(uint8_t drive);
vfs_node_t *fat32_get_root();

#endif
