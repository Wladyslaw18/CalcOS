/*
 * InputAPI.h — Stable proxy interface for the input driver service
 *
 * Functions take a void* context parameter (the underlying InputDriver*).
 *
 * Null Object: ker_input_null — always reports no key available
 */

#ifndef INPUT_API_H
#define INPUT_API_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ─── Input Service Interface ─── */
typedef struct {
    /* Driver context (the underlying InputDriver*) */
    void* context;

    /* Returns non-zero if a key is waiting to be read */
    int (*key_available)(void* ctx);

    /* Read the next key. Returns 0 if no key available.
     * For printable chars: returns the Unicode code point.
     * For special keys: returns UI_KEY_* constants from input.h */
    uint32_t (*read_key)(void* ctx);

    /* Initialize the input driver. Returns 0 on success. */
    int (*init)(void* ctx);

    /* Shutdown the input driver. */
    void (*deinit)(void* ctx);

} InputAPI;

/* ─── Null Object (no input) ─── */
extern const InputAPI ker_input_null;

#ifdef __cplusplus
}
#endif

#endif /* INPUT_API_H */
