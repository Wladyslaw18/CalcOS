/*
 * File: input.h
 * Author: W. Kowal
 * Description: Input driver interface for the Bare Metal Calculator.
 * 
 * Implement this interface to connect any input source.
 * Available built-in implementations:
 * - UIInput_PS2 (keyboard interrupt-driven, x86_64)
 * - UIInput_Serial (COM1 serial terminal input)
 * - UIInput_Null (no-op, for testing)
 * 
 * Custom implementations can target:
 * - Touch screen (Raspberry Pi)
 * - USB HID (UHCI/EHCI driver)
 * - Web keyboard events (WASM)
 * - Stdin (user-space testing)
 * - Whatever input device you have!
 */

#ifndef CALC_INPUT_H
#define CALC_INPUT_H

#include <stdint.h>

// Forward declaration
typedef struct InputDriver InputDriver;

// Special key codes beyond ASCII range
#define UI_KEY_NONE      0x00000000
#define UI_KEY_UP        0x00010001
#define UI_KEY_DOWN      0x00010002
#define UI_KEY_LEFT      0x00010003
#define UI_KEY_RIGHT     0x00010004
#define UI_KEY_ENTER     0x0000000A
#define UI_KEY_BACKSPACE 0x00000008
#define UI_KEY_TAB       0x00000009
#define UI_KEY_ESCAPE    0x0000001B
#define UI_KEY_DELETE    0x0001007F
#define UI_KEY_HOME      0x00010010
#define UI_KEY_END       0x00010011
#define UI_KEY_PAGE_UP   0x00010012
#define UI_KEY_PAGE_DOWN 0x00010013
#define UI_KEY_F1        0x00010020
#define UI_KEY_F2        0x00010021
#define UI_KEY_F3        0x00010022
#define UI_KEY_F4        0x00010023
#define UI_KEY_F5        0x00010024
#define UI_KEY_F6        0x00010025
#define UI_KEY_F7        0x00010026
#define UI_KEY_F8        0x00010027
#define UI_KEY_F9        0x00010028
#define UI_KEY_F10       0x00010029
#define UI_KEY_F11       0x0001002A
#define UI_KEY_F12       0x0001002B

/*
 * InputDriver pure virtual interface for user input.
 * 
 * At minimum, implement key_available() and read_key().
 * read_line() has a default byte-by-byte implementation
 * that uses read_key() if left as NULL.
 */
struct InputDriver {
    // Driver context
    // Driver-specific data pointer. Set by the implementation for state access.
    void* context;
    
    // Input properties
    const char* name;        // Human-readable driver name
    
    // Key-level input
    // Returns non-zero if a key press is waiting to be read.
    int (*key_available)(const InputDriver* self);
    
    // Read the next key press. Returns UI_KEY_NONE if no key is available.
    // Blocks if key_available() returned non-zero.
    // Returns a Unicode code point for printable characters,
    // or a UI_KEY_* constant for special keys.
    uint32_t (*read_key)(const InputDriver* self);
    
    // Line-level input
    // Read an entire line of text (with echo via the display driver).
    // Returns the number of characters read (excluding null terminator).
    // If NULL, a default implementation using read_key() is used.
    uint32_t (*read_line)(const InputDriver* self,
                          char* buffer, uint32_t max_len,
                          void* display_driver); // DisplayDriver for echo
    
    // Lifecycle
    // Initialize the input driver (enable interrupts, init ports, etc.)
    int (*init)(const InputDriver* self);
    
    // Shutdown the input driver
    void (*deinit)(const InputDriver* self);
};

#endif // CALC_INPUT_H
