// src/kernel/disk_marshal.c
#include "../../Platform/x86_64/PortIO.h" // Raw inb/outb/outw/inw wrappers
#include "../../include/kernel/mart_header.h"
#include "../../include/kernel/db.h"

#define IDE_DATA_PORT      0x1F0
#define IDE_SECTOR_COUNT   0x1F2
#define IDE_LBA_LOW        0x1F3
#define IDE_LBA_MID        0x1F4
#define IDE_LBA_HIGH       0x1F5
#define IDE_DRIVE_SELECT   0x1F6
#define IDE_CMD_PORT       0x1F7

static void wait_for_drive_ready(void) {
#if !defined(PLATFORM_HOST)
    // Poll status register: Busy (0x80) must be clear, DRQ (0x08) must be set
    while ((inb(IDE_CMD_PORT) & 0x88) != 0x08) {
        io_wait();
    }
#endif
}

bool ndb_marshal_to_disk(const NanoDatabase* db, uint32_t start_lba) {
    if (!db || !db->entries || db->capacity == 0) return false;

    // Calculate sectors to write: 1 sector (512B) for MartHeader + DB capacity*32B
    uint32_t payload_bytes = sizeof(MartHeader) + (db->capacity * sizeof(DbEntry));
    uint32_t sector_count = (payload_bytes + 511) / 512;

#if !defined(PLATFORM_HOST)
    outb(IDE_DRIVE_SELECT, (uint8_t)(0xE0 | ((start_lba >> 24) & 0x0F)));
    outb(IDE_SECTOR_COUNT, (uint8_t)sector_count);
    outb(IDE_LBA_LOW,      (uint8_t)start_lba);
    outb(IDE_LBA_MID,      (uint8_t)(start_lba >> 8));
    outb(IDE_LBA_HIGH,     (uint8_t)(start_lba >> 16));
    outb(IDE_CMD_PORT,     0x30); // 0x30 = Write Sectors Command
#endif

    MartHeader header = { MART_MAGIC, 2, CALC_DB_LAYOUT_SIG, 0 };
    const uint16_t* header_ptr = (const uint16_t*)&header;
    const uint16_t* db_entries_ptr = (const uint16_t*)db->entries;

    uint32_t header_words = sizeof(MartHeader) / 2; // 8 words (16B)
    uint32_t entries_words = (db->capacity * sizeof(DbEntry)) / 2;

    for (uint32_t s = 0; s < sector_count; s++) {
        wait_for_drive_ready();
        for (uint32_t w = 0; w < 256; w++) {
            uint32_t word_idx = s * 256 + w;
            uint16_t word_to_write = 0;
            if (word_idx < header_words) {
                word_to_write = header_ptr[word_idx];
            } else if (word_idx < header_words + entries_words) {
                word_to_write = db_entries_ptr[word_idx - header_words];
            } else {
                word_to_write = 0; // Zero padding up to sector boundary
            }
#if !defined(PLATFORM_HOST)
            outw(IDE_DATA_PORT, word_to_write);
#else
            (void)word_to_write;
            (void)start_lba;
#endif
        }
    }
    return true;
}
