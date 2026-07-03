/*
 * File: display.h
 * Author: W. Kowal
 * Description: Display driver interface for the Bare Metal Calculator.
 * 
 * Implement this interface to create your own rendering backend.
 * The calculator engine calls these functions to display output,
 * completely decoupled from the actual display hardware.
 * 
 * Available built-in implementations:
 * - UIDisplay_VGA (text mode, 8025)
 * - UIDisplay_VESA (framebuffer, configurable resolution)
 * - UIDisplay_Serial (COM1 serial terminal)
 * - UIDisplay_Null (no-op, for testing)
 * 
 * Custom implementations can target:
 * - SDL/GLFW window (user-space)
 * - Web canvas (WASM target)
 * - OLED/LCD display (embedded)
 * - SSH terminal (remote)
 * - Whatever you want!
 */

#ifndef CALC_DISPLAY_H
#define CALC_DISPLAY_H

#include <stdint.h>
#include <stddef.h>

// Forward declaration
typedef struct DisplayDriver DisplayDriver;

// Color in 0x00RRGGBB format (UI abstraction, not hardware-specific)
typedef uint32_t UIColor;

// Predefined UI colors (nice defaults for any rendering backend)
#define UI_COLOR_BLACK       0x00000000
#define UI_COLOR_WHITE       0x00FFFFFF
#define UI_COLOR_RED         0x00FF0000
#define UI_COLOR_GREEN       0x0000FF00
#define UI_COLOR_BLUE        0x000000FF
#define UI_COLOR_YELLOW      0x00FFFF00
#define UI_COLOR_CYAN        0x0000FFFF
#define UI_COLOR_MAGENTA     0x00FF00FF
#define UI_COLOR_GRAY        0x00808080
#define UI_COLOR_LIGHT_GRAY  0x00C0C0C0
#define UI_COLOR_DARK_RED    0x00800000
#define UI_COLOR_DARK_GREEN  0x00008000
#define UI_COLOR_DARK_BLUE   0x00000080
#define UI_COLOR_ORANGE      0x00FFA500

/*
 * DisplayDriver pure virtual interface for rendering.
 * 
 * All function pointers are optional (can be NULL for unsupported operations).
 * At minimum, a display driver should provide put_char or put_pixel.
 * 
 * The 'self' pointer allows the driver to access its own context data
 * without needing global state. Implementations typically embed this
 * struct at the start of a larger driver-specific struct.
 */
struct DisplayDriver {
    // Driver context
    // Driver-specific data pointer. Set by the implementation.
    // This is how driver functions access their own state without globals.
    void* context;
    
    // Display properties
    uint32_t width;          // Width in pixels (0 for text mode)
    uint32_t height;         // Height in pixels (0 for text mode)
    uint32_t char_width;     // Character cell width in pixels
    uint32_t char_height;    // Character cell height in pixels
    uint32_t text_cols;      // Number of text columns (0 if not applicable)
    uint32_t text_rows;      // Number of text rows (0 if not applicable)
    uint32_t bpp;            // Bits per pixel (0 for text mode)
    
    // Character rendering
    // Draw a single character at pixel position (x, y) with foreground/background color.
    // For text-mode displays, x/y are character positions (not pixels).
    void (*put_char)(const DisplayDriver* self, char c,
                     uint32_t x, uint32_t y,
                     UIColor fg, UIColor bg);
    
    // Write a null-terminated string starting at (x, y).
    void (*write_str)(const DisplayDriver* self, const char* str,
                      uint32_t x, uint32_t y,
                      UIColor fg, UIColor bg);
    
    // Set the hardware/text cursor position.
    void (*set_cursor)(const DisplayDriver* self, uint32_t x, uint32_t y);
    
    // Scroll the display content by the given number of lines.
    void (*scroll)(const DisplayDriver* self, int32_t lines);
    
    // Pixel rendering
    // Set a single pixel at (x, y) to the given color.
    void (*put_pixel)(const DisplayDriver* self, uint32_t x, uint32_t y,
                      uint8_t r, uint8_t g, uint8_t b);
    
    // Clear the entire display with a background color.
    void (*clear)(const DisplayDriver* self, UIColor bg);
    
    // Draw a line from (x0, y0) to (x1, y1) in the given color.
    void (*draw_line)(const DisplayDriver* self,
                      int32_t x0, int32_t y0,
                      int32_t x1, int32_t y1,
                      UIColor color);
    
    // Fill a rectangle with a solid color.
    void (*fill_rect)(const DisplayDriver* self,
                      uint32_t x, uint32_t y,
                      uint32_t w, uint32_t h,
                      UIColor color);
    
    // Plotting integration
    // Draw a mathematical function on the display using the driver's own drawing primitives.
    // This bypasses the Plotter module for drivers that have native rendering.
    void (*plot_function)(const DisplayDriver* self,
                          double (*f)(double),
                          double x_min, double x_max,
                          double y_min, double y_max,
                          UIColor color);
    
    // Lifecycle
    // Flush any pending rendering (required for double-buffered displays).
    void (*present)(const DisplayDriver* self);
};

#endif // CALC_DISPLAY_H
