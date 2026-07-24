// include/kernel/db.h
#pragma once
#include <stdint.h>
#include <stdbool.h>

#define NDB_MAX_TX_KEYS 8         // Max keys modified per transaction

typedef enum {
    DB_FLAG_EMPTY       = 0x00,
    DB_FLAG_ACTIVE      = 0x01,   // Key-value pair is active
    DB_FLAG_DIRTY       = 0x02,   // Modified in memory journal, needs disk flush
    DB_FLAG_TRANSACTION = 0x04    // Key is locked under active transaction
} DbFlags;

#pragma pack(push, 1)
typedef struct {
    char     key[16];   // 16 bytes: Unique null-terminated key string
    uint64_t val_raw;   // 8 bytes: Raw payload (can be double, int64_t, or pointer)
    uint8_t  flags;     // 1 byte: Operational state flags (DbFlags)
    uint8_t  pad[7];    // 7 bytes: Explicit padding to guarantee exactly 32-byte size!
} DbEntry;
#pragma pack(pop)

typedef struct {
    uint16_t entry_index; // Index inside the primary DbEntry table
    uint64_t old_value;   // Value before transaction modification
    uint8_t  old_flags;   // Flags before transaction modification
} RollbackEntry;

typedef struct {
    RollbackEntry logs[NDB_MAX_TX_KEYS]; // Fixed array, NO heap malloc
    uint32_t      count;                 // Current modified key count
    bool          is_active;             // Transaction active state flag
} TransactionSession;

typedef struct {
    DbEntry*           entries;   // Pointer to contiguous memory (external buffer)
    uint32_t           capacity;  // Dynamic size constraint
    TransactionSession tx;        // Active transaction session
} NanoDatabase;

// API Prototypes
void ndb_init(NanoDatabase* db, DbEntry* buffer, uint32_t capacity);
bool ndb_tx_begin(NanoDatabase* db);
void ndb_tx_commit(NanoDatabase* db);
void ndb_tx_rollback(NanoDatabase* db);
bool ndb_set(NanoDatabase* db, const char* key, uint64_t val);
bool ndb_get(const NanoDatabase* db, const char* key, uint64_t* out_val);
