// include/kernel/write_ahead_log.h
#pragma once
#include <stdint.h>

#define WAL_MAGIC 0xCA1C0501
#define WAL_PHYSICAL_ADDR 0x5000

#if defined(CALC_TINY_MEM) && CALC_TINY_MEM
    #define WAL_LOG_SIZE 64
#else
    #define WAL_LOG_SIZE 400
#endif

#pragma pack(push, 1)
typedef struct {
    uint32_t magic;                 // Verification word
    uint32_t cursor;                // Active char index
    char     history_log[WAL_LOG_SIZE]; // Replay trace buffer of input strings
} StateWAL;
#pragma pack(pop)

static inline StateWAL* get_state_wal(void) {
    return (StateWAL*)(uintptr_t)WAL_PHYSICAL_ADDR;
}

static inline void wal_log_char(char c) {
    StateWAL* s_wal = get_state_wal();
    if (s_wal->magic != WAL_MAGIC) {
        s_wal->magic = WAL_MAGIC;
        s_wal->cursor = 0;
    }
    if (s_wal->cursor < (WAL_LOG_SIZE - 1)) {
        s_wal->history_log[s_wal->cursor++] = c;
        s_wal->history_log[s_wal->cursor] = '\0';
    }
}

static inline void wal_clear_log(void) {
    StateWAL* s_wal = get_state_wal();
    s_wal->magic = WAL_MAGIC;
    s_wal->cursor = 0;
    s_wal->history_log[0] = '\0';
}
