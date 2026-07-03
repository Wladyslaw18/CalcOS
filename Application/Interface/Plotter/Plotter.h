/*
 * File: Plotter.h
 * Author: W. Kowal
 * Description: Decoupled plotting system. Routes all rendering through
 * the virtual DisplayDriver interface (display.h).
 * 
 * NO raw VESA or framebuffer dependencies. Works with:
 * - VESA framebuffer (bare-metal)
 * - SDL2 window (desktop host)
 * - ANSI terminal (headless)
 * - HTML5 Canvas (WASM)
 * - SSD1306 OLED (ESP32)
 * - Whatever implements display.h!
 * 
 * THE FLEX: Zero-dependency function plotting on ANY display backend.
 * No JRE required. No X11 required. Just a DisplayDriver*.
 */

#ifndef PLOTTER_H
#define PLOTTER_H

#include <stdint.h>
#include <stdbool.h>
#include "../../../include/calc/display.h"

// RGB UIColor conversion for cross-platform rendering
// Converts a VesaColorRGB (r, g, b) to the abstract 0x00RRGGBB UIColor format
// expected by the DisplayDriver interface. This keeps the PlotConfig struct
// stable while allowing any display backend to consume the colors.
#define RGB_TO_UICOLOR(r, g, b) (((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (b))

// Plotting area configuration defaults to full screen, but can be inset
typedef struct {
    uint32_t x_offset;    // Left margin in pixels (for axis labels)
    uint32_t y_offset;    // Bottom margin in pixels (or top margin for Y axis)
    double inv_x_range;   // Cached 1.0 / (x_max - x_min) TURNS FDIV INTO FMUL
    double inv_y_range;   // Cached 1.0 / (y_max - y_min) MECHANICAL SYMPATHY
    uint32_t width;       // Plot area width in pixels
    uint32_t height;      // Plot area height in pixels
    double x_min;         // World-space left bound
    double x_max;         // World-space right bound
    double y_min;         // World-space bottom bound
    double y_max;         // World-space top bound
    bool   auto_scale_y;  // If true, auto-compute y_min/y_max from data
    uint32_t bg_color;    // Background color (0x00RRGGBB)
    uint32_t grid_color;  // Grid line color (0x00RRGGBB)
    uint32_t axis_color;  // Axis line color (0x00RRGGBB)
} PlotConfig;

// Default plot config: 800x600 equivalent with 60px margins
#define PLOT_DEFAULT_MARGIN 60
#define PLOT_DEFAULT_WIDTH  800
#define PLOT_DEFAULT_HEIGHT 600

// Update reciprocals after changing x_min/x_max/y_min/y_max.
// Call this EVERY time you modify the range. Skip this and your FMUL gives garbage.
static inline void plot_config_update_reciprocals(PlotConfig* cfg) {
    double x_range = cfg->x_max - cfg->x_min;
    double y_range = cfg->y_max - cfg->y_min;
    cfg->inv_x_range = (x_range > 0.0) ? 1.0 / x_range : 1.0;
    cfg->inv_y_range = (y_range > 0.0) ? 1.0 / y_range : 1.0;
}

// Initialize a default plot config for an 800x600 display
static inline PlotConfig plot_default_config(void) {
    PlotConfig cfg;
    cfg.x_offset     = PLOT_DEFAULT_MARGIN;
    cfg.y_offset     = PLOT_DEFAULT_MARGIN;
    cfg.width        = PLOT_DEFAULT_WIDTH  - PLOT_DEFAULT_MARGIN * 2;
    cfg.height       = PLOT_DEFAULT_HEIGHT - PLOT_DEFAULT_MARGIN * 2;
    cfg.x_min        = -10.0;
    cfg.x_max        =  10.0;
    cfg.y_min        = -10.0;
    cfg.y_max        =  10.0;
    cfg.auto_scale_y = false;
    // Pre-compute reciprocals ZERO DIVISIONS IN THE HOT LOOP
    plot_config_update_reciprocals(&cfg);
    cfg.bg_color     = RGB_TO_UICOLOR(15,  15,  20);   // Dark navy
    cfg.grid_color   = RGB_TO_UICOLOR(40,  40,  50);   // Dim grid
    cfg.axis_color   = RGB_TO_UICOLOR(80,  150, 255);  // Blue axis
    return cfg;
}

// ============================================================
// CORE PLOTTING API
// ============================================================

// Clear the plot area with the background color
void plot_clear_area(const DisplayDriver* disp, const PlotConfig* cfg);

// Draw a grid (major lines) within the plot area
void plot_draw_grid(const DisplayDriver* disp, const PlotConfig* cfg);

// Draw X and Y axes (with arrows at ends if room)
void plot_draw_axes(const DisplayDriver* disp, const PlotConfig* cfg);

// Draw axis labels with numeric tick marks
void plot_draw_labels(const DisplayDriver* disp, const PlotConfig* cfg);

// ============================================================
// FUNCTION PLOTTING
// ============================================================

// Plot a single function y = f(x) over x [x_min, x_max]
// Evaluates at `samples` points (default: width in pixels for 1:1 mapping)
void plot_function(const DisplayDriver* disp, const PlotConfig* cfg,
                   double (*f)(double),
                   uint32_t color,
                   uint32_t samples);

// Plot multiple functions at once (max 8)
// `funcs` is an array of function pointers, `colors` is parallel color array
#define PLOT_MAX_OVERLAY 8
void plot_functions_overlay(const DisplayDriver* disp, const PlotConfig* cfg,
                            double (**funcs)(double),
                            uint32_t* colors,
                            uint32_t num_funcs,
                            uint32_t samples);

// ============================================================
// SCATTER PLOTS
// ============================================================

// Plot scatter points (x[i], y[i]) for i in [0, count)
// point_radius: size of each point in pixels (1 = single pixel)
void plot_scatter(const DisplayDriver* disp, const PlotConfig* cfg,
                  const double* x_data,
                  const double* y_data,
                  uint32_t count,
                  uint32_t color,
                  uint32_t point_radius);

// Plot a line graph connecting (x[i], y[i]) points
void plot_line_graph(const DisplayDriver* disp, const PlotConfig* cfg,
                     const double* x_data,
                     const double* y_data,
                     uint32_t count,
                     uint32_t color);

// ============================================================
// HISTOGRAM
// ============================================================

// Plot a histogram with `num_bins` bins from the data array
void plot_histogram(const DisplayDriver* disp, const PlotConfig* cfg,
                    const double* data,
                    uint32_t data_count,
                    uint32_t num_bins,
                    uint32_t bar_color);

// ============================================================
// COORDINATE HELPERS (pure math no DisplayDriver needed!)
// ============================================================

// Convert world coordinates to screen pixel coordinates
// Returns false if outside plot area
bool plot_world_to_screen(const PlotConfig* cfg, double wx, double wy,
                          uint32_t* sx, uint32_t* sy);

// Convert screen pixel coordinates to world coordinates
void plot_screen_to_world(const PlotConfig* cfg, uint32_t sx, uint32_t sy,
                          double* wx, double* wy);

// Recalculate auto y-scale for given data
void plot_auto_scale_y(PlotConfig* cfg,
                       const double* y_data,
                       uint32_t count,
                       double margin_percent);

#endif // PLOTTER_H
