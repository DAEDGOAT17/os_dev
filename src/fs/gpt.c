#include "gpt.h"
#include "screen.h"
#include "string.h"

int gpt_parse(uint8_t *lba1_buffer, uint8_t *entries_buffer) {
    gpt_header_t* header = (gpt_header_t*)lba1_buffer;
    
    // Check "EFI PART" signature
    if (header->signature != 0x5452415020494645ULL) {
        print_string("GPT: Invalid EFI PART signature!\n");
        return -1;
    }
    
    print_string("GPT: Valid Header Found.\n");
    print_string("GPT: Found ");
    kprint_dec(header->num_partition_entries);
    print_string(" partition entries.\n");
    
    gpt_entry_t* entries = (gpt_entry_t*)entries_buffer;
    
    for (uint32_t i = 0; i < header->num_partition_entries; i++) {
        // Skip unused partitions (all GUID bytes = 0)
        if (entries[i].partition_type_guid.data1 == 0 &&
            entries[i].partition_type_guid.data2 == 0) continue;
            
        print_string("  Part ");
        kprint_dec(i + 1);
        print_string(": LBA ");
        kprint_dec((uint32_t)entries[i].starting_lba);
        print_string(" -> ");
        kprint_dec((uint32_t)entries[i].ending_lba);
        print_char('\n');
    }
    
    return 0;
}
