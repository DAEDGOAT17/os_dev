#ifndef GPT_H
#define GPT_H

#include <stdint.h>

// GUID structure
typedef struct {
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint8_t  data4[8];
} __attribute__((packed)) gpt_guid_t;

// GPT Header Structure (LBA 1)
typedef struct {
    uint64_t signature;     // "EFI PART" (0x5452415020494645)
    uint32_t revision;
    uint32_t header_size;
    uint32_t header_crc32;
    uint32_t reserved;
    uint64_t my_lba;
    uint64_t alternate_lba;
    uint64_t first_usable_lba;
    uint64_t last_usable_lba;
    gpt_guid_t disk_guid;
    uint64_t partition_entry_lba;
    uint32_t num_partition_entries;
    uint32_t size_of_partition_entry;
    uint32_t partition_entry_array_crc32;
    // Followed by padding out to 512 bytes
} __attribute__((packed)) gpt_header_t;

// GPT Partition Entry
typedef struct {
    gpt_guid_t partition_type_guid;
    gpt_guid_t unique_partition_guid;
    uint64_t starting_lba;
    uint64_t ending_lba;
    uint64_t attributes;
    uint16_t partition_name[36]; // UTF-16LE
} __attribute__((packed)) gpt_entry_t;

// Parse the GPT layout starting from LBA 1
int gpt_parse(uint8_t *lba1_buffer, uint8_t *entries_buffer);

#endif
