#ifndef MEMORY_MAP_H
#define MEMORY_MAP_H

#include <stdint.h>

// e820 memory map entry (24 bytes each, as returned by INT 0x15, AX=0xE820)
typedef struct __attribute__((packed)) {
    uint64_t base_addr;    // Base physical address of this region
    uint64_t length;       // Length of this region in bytes
    uint32_t type;         // 1 = Usable, 2 = Reserved, 3 = ACPI reclaimable, 4 = ACPI NVS, 5 = Bad
    uint32_t ext_attr;     // Extended attributes (bit 0 = non-volatile, etc.)
} E820Entry;

// The e820 table is stored at this fixed physical address by BootEntry.asm
#define E820_TABLE_ADDR   ((volatile E820Entry*)0x5000)
#define E820_MAX_ENTRIES  32

// Memory region types
#define E820_TYPE_USABLE  1
#define E820_TYPE_RESERVED 2
#define E820_TYPE_ACPI     3
#define E820_TYPE_NVS      4
#define E820_TYPE_BAD      5

// Read the e820 table populated by the bootloader.
// Returns the number of valid entries found.
// Zero means no e820 data available (fallback: assume all RAM is usable).
static inline uint32_t e820_get_entry_count(void) {
    // The boot sector stores e820 entries consecutively with a
    // terminator entry (type=0) at the first gap.
    // also guard against the all-zeros BIOS-not-present case.
    uint32_t count = 0;
    for (uint32_t i = 0; i < E820_MAX_ENTRIES; i++) {
        E820Entry entry = E820_TABLE_ADDR[i];
        if (entry.type == 0 && entry.base_addr == 0 && entry.length == 0) {
            break; // Reached end of table
        }
        if (entry.base_addr == 0 && entry.length == 0) {
            continue; // Skip zeroed entries
        }
        count++;
    }
    return count;
}

// Search the e820 table for the largest contiguous usable RAM region.
// Returns the base address and writes the size into *size.
// Falls back to hardcoded 16MB if no e820 data is available.
static inline uint64_t e820_find_largest_usable(uint64_t* size) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < E820_MAX_ENTRIES; i++) {
        E820Entry entry = E820_TABLE_ADDR[i];
        if (entry.type == 0) break;
        if (entry.type == E820_TYPE_USABLE) count++;
    }

    if (count == 0) {
        // Fallback: assume has at least 16MB at 0x0
        *size = 16 * 1024 * 1024ULL;
        return 0x0ULL;
    }

    uint64_t best_base = 0;
    uint64_t best_size = 0;
    for (uint32_t i = 0; i < E820_MAX_ENTRIES; i++) {
        E820Entry entry = E820_TABLE_ADDR[i];
        if (entry.type == 0) break;
        if (entry.type == E820_TYPE_USABLE && entry.length > best_size) {
            best_size = entry.length;
            best_base = entry.base_addr;
        }
    }

    *size = best_size;
    return best_base;
}

#endif // MEMORY_MAP_H
