// include/kernel/mart_header.h
#pragma once
#include <stddef.h>
#include "db.h"

#define MART_MAGIC 0x4D415254 // "MART"

// Compile-time FNV-1a hash of the DbEntry structural offsets.
// If you rearrange key, val_raw, or flags, the signature changes!
#define CALC_DB_LAYOUT_SIG ( \
    (uint32_t)((((2166136261u ^ sizeof(DbEntry)) * 16777619u) \
    ^ offsetof(DbEntry, key)) * 16777619u \
    ^ offsetof(DbEntry, val_raw)) * 16777619u \
    ^ offsetof(DbEntry, flags) \
)

#pragma pack(push, 1)
typedef struct {
    uint32_t magic;      // 0x4D415254
    uint32_t version;    // Save version
    uint32_t layout_sig; // CALC_DB_LAYOUT_SIG
    uint32_t reserved;   // Zero padding
} MartHeader;
#pragma pack(pop)
