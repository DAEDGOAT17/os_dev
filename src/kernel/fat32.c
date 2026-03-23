#include "fat32.h"
#include "ata.h"
#include "kmalloc.h"
#include "screen.h"
#include "string.h"
#include <stddef.h>

// Global FAT32 file system state
static fat32_fs_t fat32_fs;
static bool fat32_initialized = false;

// VFS function prototypes for FAT32
static uint32_t fat32_read(vfs_node_t *node, uint32_t offset, uint32_t size,
                           uint8_t *buffer);
static uint32_t fat32_write(vfs_node_t *node, uint32_t offset, uint32_t size,
                            uint8_t *buffer);
static void fat32_open(vfs_node_t *node);
static void fat32_close(vfs_node_t *node);
static dirent_t *fat32_readdir(vfs_node_t *node, uint32_t index);
static vfs_node_t *fat32_finddir(vfs_node_t *node, char *name);

// Root directory node
static vfs_node_t fat32_root_node;

// Helper: Get cluster sector
static uint32_t fat32_cluster_to_sector(uint32_t cluster) {
  return fat32_fs.data_start +
         ((cluster - 2) * fat32_fs.boot_sector.sectors_per_cluster);
}

// Read FAT entry
uint32_t fat32_read_fat_entry(uint32_t cluster) {
  uint32_t fat_offset = cluster * 4;
  uint32_t fat_sector =
      fat32_fs.fat_start + (fat_offset / fat32_fs.boot_sector.bytes_per_sector);
  uint32_t entry_offset = fat_offset % fat32_fs.boot_sector.bytes_per_sector;

  uint8_t buffer[512];
  if (!ata_read_sectors(fat32_fs.drive, fat_sector, 1, (uint16_t *)buffer)) {
    return 0;
  }

  uint32_t *fat_entry = (uint32_t *)&buffer[entry_offset];
  return (*fat_entry) & 0x0FFFFFFF; // Mask off top 4 bits
}

// Write FAT entry
void fat32_write_fat_entry(uint32_t cluster, uint32_t value) {
  uint32_t fat_offset = cluster * 4;
  uint32_t fat_sector =
      fat32_fs.fat_start + (fat_offset / fat32_fs.boot_sector.bytes_per_sector);
  uint32_t entry_offset = fat_offset % fat32_fs.boot_sector.bytes_per_sector;

  uint8_t buffer[512];
  if (!ata_read_sectors(fat32_fs.drive, fat_sector, 1, (uint16_t *)buffer)) {
    return;
  }

  uint32_t *fat_entry = (uint32_t *)&buffer[entry_offset];
  *fat_entry = (*fat_entry & 0xF0000000) | (value & 0x0FFFFFFF);

  ata_write_sectors(fat32_fs.drive, fat_sector, 1, (uint16_t *)buffer);
}

// Read cluster
bool fat32_read_cluster(uint32_t cluster, uint8_t *buffer) {
  uint32_t sector = fat32_cluster_to_sector(cluster);
  return ata_read_sectors(fat32_fs.drive, sector,
                          fat32_fs.boot_sector.sectors_per_cluster,
                          (uint16_t *)buffer);
}

// Write cluster
bool fat32_write_cluster(uint32_t cluster, uint8_t *buffer) {
  uint32_t sector = fat32_cluster_to_sector(cluster);
  return ata_write_sectors(fat32_fs.drive, sector,
                           fat32_fs.boot_sector.sectors_per_cluster,
                           (uint16_t *)buffer);
}

// Convert 8.3 filename to regular string
static void fat32_83_to_string(const uint8_t *fat_name, char *output) {
  int out_idx = 0;

  // Copy name part (8 chars)
  for (int i = 0; i < 8 && fat_name[i] != ' '; i++) {
    output[out_idx++] = fat_name[i];
  }

  // Add extension if present
  if (fat_name[8] != ' ') {
    output[out_idx++] = '.';
    for (int i = 8; i < 11 && fat_name[i] != ' '; i++) {
      output[out_idx++] = fat_name[i];
    }
  }

  output[out_idx] = '\0';
}

// Read directory entries from a cluster
static dirent_t *fat32_readdir(vfs_node_t *node, uint32_t index) {
  if (!(node->flags & VFS_DIRECTORY))
    return NULL;

  uint32_t cluster = node->inode; // Store cluster in inode field
  uint32_t cluster_size = fat32_fs.boot_sector.sectors_per_cluster * 512;
  uint8_t *cluster_buffer = (uint8_t *)kmalloc(cluster_size);

  if (!cluster_buffer)
    return NULL;

  uint32_t current_index = 0;
  static dirent_t dirent;

  // Traverse cluster chain
  while (cluster < FAT32_EOC) {
    if (!fat32_read_cluster(cluster, cluster_buffer)) {
      kfree(cluster_buffer);
      return NULL;
    }

    fat32_dir_entry_t *entries = (fat32_dir_entry_t *)cluster_buffer;
    uint32_t entries_per_cluster = cluster_size / sizeof(fat32_dir_entry_t);

    for (uint32_t i = 0; i < entries_per_cluster; i++) {
      // Skip deleted, empty, or LFN entries
      if (entries[i].name[0] == 0x00) {
        kfree(cluster_buffer);
        return NULL; // End of directory
      }
      if (entries[i].name[0] == 0xE5)
        continue; // Deleted
      if (entries[i].attributes == FAT_ATTR_LFN)
        continue; // LFN
      if (entries[i].attributes & FAT_ATTR_VOLUME_ID)
        continue; // Volume label

      if (current_index == index) {
        // Found the entry
        fat32_83_to_string(entries[i].name, dirent.name);
        dirent.ino =
            (entries[i].first_cluster_hi << 16) | entries[i].first_cluster_lo;
        kfree(cluster_buffer);
        return &dirent;
      }
      current_index++;
    }

    // Move to next cluster
    cluster = fat32_read_fat_entry(cluster);
  }

  kfree(cluster_buffer);
  return NULL;
}

// Find directory entry by name
static vfs_node_t *fat32_finddir(vfs_node_t *node, char *name) {
  if (!(node->flags & VFS_DIRECTORY))
    return NULL;

  uint32_t cluster = node->inode;
  uint32_t cluster_size = fat32_fs.boot_sector.sectors_per_cluster * 512;
  uint8_t *cluster_buffer = (uint8_t *)kmalloc(cluster_size);

  if (!cluster_buffer)
    return NULL;

  // Traverse cluster chain
  while (cluster < FAT32_EOC) {
    if (!fat32_read_cluster(cluster, cluster_buffer)) {
      kfree(cluster_buffer);
      return NULL;
    }

    fat32_dir_entry_t *entries = (fat32_dir_entry_t *)cluster_buffer;
    uint32_t entries_per_cluster = cluster_size / sizeof(fat32_dir_entry_t);

    for (uint32_t i = 0; i < entries_per_cluster; i++) {
      if (entries[i].name[0] == 0x00) {
        kfree(cluster_buffer);
        return NULL; // End of directory
      }
      if (entries[i].name[0] == 0xE5)
        continue;
      if (entries[i].attributes == FAT_ATTR_LFN)
        continue;
      if (entries[i].attributes & FAT_ATTR_VOLUME_ID)
        continue;

      char entry_name[128];
      fat32_83_to_string(entries[i].name, entry_name);

      if (strcmp(entry_name, name) == 0) {
        // Found it! Create a VFS node
        vfs_node_t *new_node = (vfs_node_t *)kmalloc(sizeof(vfs_node_t));
        if (!new_node) {
          kfree(cluster_buffer);
          return NULL;
        }

        strcpy(new_node->name, entry_name);
        new_node->inode =
            (entries[i].first_cluster_hi << 16) | entries[i].first_cluster_lo;
        new_node->length = entries[i].file_size;
        new_node->flags = (entries[i].attributes & FAT_ATTR_DIRECTORY)
                              ? VFS_DIRECTORY
                              : VFS_FILE;
        new_node->read = fat32_read;
        new_node->write = fat32_write;
        new_node->open = fat32_open;
        new_node->close = fat32_close;
        new_node->readdir = fat32_readdir;
        new_node->finddir = fat32_finddir;

        kfree(cluster_buffer);
        return new_node;
      }
    }

    cluster = fat32_read_fat_entry(cluster);
  }

  kfree(cluster_buffer);
  return NULL;
}

// Read file data
static uint32_t fat32_read(vfs_node_t *node, uint32_t offset, uint32_t size,
                           uint8_t *buffer) {
  if (node->flags & VFS_DIRECTORY)
    return 0;

  uint32_t cluster = node->inode;
  uint32_t cluster_size = fat32_fs.boot_sector.sectors_per_cluster * 512;
  uint8_t *cluster_buffer = (uint8_t *)kmalloc(cluster_size);

  if (!cluster_buffer)
    return 0;

  uint32_t bytes_read = 0;
  uint32_t current_offset = 0;

  // Skip to starting cluster
  while (offset >= cluster_size && cluster < FAT32_EOC) {
    cluster = fat32_read_fat_entry(cluster);
    offset -= cluster_size;
    current_offset += cluster_size;
  }

  // Read data
  while (bytes_read < size && cluster < FAT32_EOC) {
    if (!fat32_read_cluster(cluster, cluster_buffer)) {
      kfree(cluster_buffer);
      return bytes_read;
    }

    uint32_t bytes_to_copy = cluster_size - offset;
    if (bytes_to_copy > size - bytes_read) {
      bytes_to_copy = size - bytes_read;
    }
    if (current_offset + offset + bytes_to_copy > node->length) {
      bytes_to_copy = node->length - current_offset - offset;
    }

    memcpy(buffer + bytes_read, cluster_buffer + offset, bytes_to_copy);
    bytes_read += bytes_to_copy;
    offset = 0; // Only first cluster has offset
    current_offset += cluster_size;

    cluster = fat32_read_fat_entry(cluster);
  }

  kfree(cluster_buffer);
  return bytes_read;
}

// Write file data (stub)
static uint32_t fat32_write(vfs_node_t *node, uint32_t offset, uint32_t size,
                            uint8_t *buffer) {
  // TODO: Implement write
  return 0;
}

static void fat32_open(vfs_node_t *node) {
  // Nothing to do
}

static void fat32_close(vfs_node_t *node) {
  // Nothing to do
}

// Initialize FAT32
bool fat32_init(uint8_t drive) {
  print_string("  [ ] FAT32 Init Drive ");
  kprint_dec(drive);
  print_string("...");

  fat32_fs.drive = drive;

  // Read boot sector into a temporary buffer first to avoid packed member
  // address issues
  uint8_t sector_buffer[512];
  if (!ata_read_sectors(drive, 0, 1, (uint16_t *)sector_buffer)) {
    return false;
  }

  // Copy to our file system structure
  memcpy(&fat32_fs.boot_sector, sector_buffer, sizeof(fat32_boot_sector_t));

  // Verify FAT32 signature (0x28 or 0x29)
  if (fat32_fs.boot_sector.boot_signature != 0x29 &&
      fat32_fs.boot_sector.boot_signature != 0x28) {
    return false;
  }

  // Calculate important sectors
  fat32_fs.fat_start = fat32_fs.boot_sector.reserved_sectors;
  fat32_fs.data_start = fat32_fs.fat_start + (fat32_fs.boot_sector.fat_count *
                                              fat32_fs.boot_sector.fat_size_32);
  fat32_fs.root_dir_cluster = fat32_fs.boot_sector.root_cluster;

  // Setup root directory node
  strcpy(fat32_root_node.name, "root");
  fat32_root_node.flags = VFS_DIRECTORY;
  fat32_root_node.inode = fat32_fs.root_dir_cluster;
  fat32_root_node.length = 0;
  fat32_root_node.read = fat32_read;
  fat32_root_node.write = fat32_write;
  fat32_root_node.open = fat32_open;
  fat32_root_node.close = fat32_close;
  fat32_root_node.readdir = fat32_readdir;
  fat32_root_node.finddir = fat32_finddir;

  fat32_initialized = true;
  print_string("OK\n");
  return true;
}

// Get root node
vfs_node_t *fat32_get_root() {
  if (!fat32_initialized)
    return NULL;
  return &fat32_root_node;
}

// Allocate cluster (stub)
uint32_t fat32_allocate_cluster() {
  // TODO: Implement cluster allocation
  return 0;
}

// Free cluster chain (stub)
void fat32_free_cluster_chain(uint32_t start_cluster) {
  // TODO: Implement cluster freeing
}
