// src/kernel/db.c
#include "../../include/kernel/db.h"
#include "../../Infrastructure/Utils/MemoryUtils.h"

void ndb_init(NanoDatabase* db, DbEntry* buffer, uint32_t capacity) {
    fast_memset(db, 0, sizeof(NanoDatabase));
    db->entries = buffer;
    db->capacity = capacity;
    if (buffer && capacity > 0) {
        fast_memset(buffer, 0, capacity * sizeof(DbEntry));
    }
}

bool ndb_tx_begin(NanoDatabase* db) {
    if (db->tx.is_active) return false;
    db->tx.count = 0;
    db->tx.is_active = true;
    return true;
}

void ndb_tx_commit(NanoDatabase* db) {
    if (!db || !db->entries || !db->tx.is_active) return;
    for (uint32_t i = 0; i < db->tx.count; i++) {
        uint32_t idx = db->tx.logs[i].entry_index;
        if (idx < db->capacity) {
            db->entries[idx].flags &= ~DB_FLAG_TRANSACTION;
        }
    }
    db->tx.is_active = false;
}

void ndb_tx_rollback(NanoDatabase* db) {
    if (!db || !db->entries || !db->tx.is_active) return;
    for (int32_t i = (int32_t)db->tx.count - 1; i >= 0; i--) {
        RollbackEntry* log = &db->tx.logs[i];
        if (log->entry_index < db->capacity) {
            DbEntry* entry = &db->entries[log->entry_index];
            entry->val_raw = log->old_value;
            entry->flags = log->old_flags;
        }
    }
    db->tx.is_active = false;
}

// Simple linear string search for O(1) small DB size
static int find_key_index(const NanoDatabase* db, const char* key) {
    if (!db->entries || db->capacity == 0) return -1;
    for (uint32_t i = 0; i < db->capacity; i++) {
        if ((db->entries[i].flags & DB_FLAG_ACTIVE) != 0) {
            // Compare key strings safely (max 16 chars)
            bool match = true;
            for (int k = 0; k < 16; k++) {
                if (db->entries[i].key[k] != key[k]) {
                    match = false;
                    break;
                }
                if (key[k] == '\0') break;
            }
            if (match) return (int)i;
        }
    }
    return -1;
}

static int find_empty_slot(const NanoDatabase* db) {
    if (!db->entries || db->capacity == 0) return -1;
    for (uint32_t i = 0; i < db->capacity; i++) {
        if ((db->entries[i].flags & DB_FLAG_ACTIVE) == 0) {
            return (int)i;
        }
    }
    return -1;
}

bool ndb_set(NanoDatabase* db, const char* key, uint64_t val) {
    if (!db || !db->entries || db->capacity == 0 || !key) return false;
    
    int idx = find_key_index(db, key);
    if (idx < 0) {
        idx = find_empty_slot(db);
        if (idx < 0) return false; // DB Full
    }

    DbEntry* entry = &db->entries[idx];

    // Log for rollback if transaction is active
    if (db->tx.is_active) {
        if (db->tx.count >= NDB_MAX_TX_KEYS) return false; // Transaction limit reached
        
        RollbackEntry* log = &db->tx.logs[db->tx.count];
        log->entry_index = (uint16_t)idx;
        log->old_value = entry->val_raw;
        log->old_flags = entry->flags;
        db->tx.count++;
    }

    // Copy key string up to 15 chars + null terminator
    int k = 0;
    for (; k < 15 && key[k] != '\0'; k++) {
        entry->key[k] = key[k];
    }
    entry->key[k] = '\0';
    
    entry->val_raw = val;
    entry->flags = DB_FLAG_ACTIVE | DB_FLAG_DIRTY;
    
    if (db->tx.is_active) {
        entry->flags |= DB_FLAG_TRANSACTION;
    }

    return true;
}

bool ndb_get(const NanoDatabase* db, const char* key, uint64_t* out_val) {
    if (!db || !db->entries || db->capacity == 0 || !key || !out_val) return false;
    int idx = find_key_index(db, key);
    if (idx < 0) return false;
    
    *out_val = db->entries[idx].val_raw;
    return true;
}
