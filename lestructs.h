#pragma once
#include <cstdint>
// Define a structure to represent the LE header
typedef struct {
    uint16_t signature;              // Signature, should be 0x454C ("LE")
    uint8_t  byte_order;             // Byte order: 0 for little-endian, non-zero for big-endian
    uint8_t  word_order;             // Word order: 0 for little-endian, non-zero for big-endian
    uint32_t executable_format_level;
    uint16_t cpu_type;               // CPU type
    uint16_t target_os;              // Target operating system
    uint32_t module_version;
    uint32_t module_type_flags;
    uint32_t num_memory_pages;
    uint32_t initial_object_cs_number;
    uint32_t initial_eip;
    uint32_t initial_object_ss_number;
    uint32_t initial_esp;
    uint32_t memory_page_size;
    uint32_t bytes_on_last_page;
    uint32_t fixup_section_size;
    uint32_t fixup_section_checksum;
    uint32_t loader_section_size;
    uint32_t loader_section_checksum;
    uint32_t object_table_offset;
    uint32_t object_table_entries;
    uint32_t object_page_map_offset;
    uint32_t object_iterate_data_map_offset;
    uint32_t resource_table_offset;
    uint32_t resource_table_entries;
    uint32_t resident_names_table_offset;
    uint32_t entry_table_offset;
    uint32_t module_directives_table_offset;
    uint32_t module_directives_entries;
    uint32_t fixup_page_table_offset;
    uint32_t fixup_record_table_offset;
    uint32_t imported_modules_name_table_offset;
    uint32_t imported_modules_count;
    uint32_t imported_procedure_name_table_offset;
    uint32_t per_page_checksum_table_offset;
    uint32_t data_pages_offset;
    uint32_t preload_page_count;
    uint32_t non_resident_names_table_offset;
    uint32_t non_resident_names_table_length;
    uint32_t non_resident_names_table_checksum;
    uint32_t automatic_data_object;
    uint32_t debug_information_offset;
    uint32_t debug_information_length;
    uint32_t preload_instance_pages_number;
    uint32_t demand_instance_pages_number;
    uint32_t heap_allocation;
    uint32_t stack_allocation;
    // Add more fields as needed based on your information
} LEHeader;

typedef struct {
    uint8_t source_type;    // Source type
    uint8_t flags;          // Target Flags
    uint8_t source_offset;  // Source offset (may be a count for Source List Flag)
    // Add more fields as needed based on the specific format
} FixupTypeRecord;

typedef struct {
    uint32_t virtualMemorySize;
    uint32_t relocationBaseAddress;
    uint32_t objectFlags;
    uint32_t objectPageTableIndex;
    uint32_t numberOfPageTableEntries;
    uint32_t reserved;
} ObjectTable;

typedef struct {
    uint16_t reserved;
    uint16_t pagenum;
} ObjectPageMapEntries;