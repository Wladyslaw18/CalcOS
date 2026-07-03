/*
 * File: Plotter.c
 * Author: W. Kowal
 * Description: Hardware-decoupled plotting implementation.
 * 
 * ALL rendering goes through the DisplayDriver virtual interface.
 * Zero direct hardware access. Zero VESA dependencies.
 * 
 * Coordinate transforms are pure integer math no floating-point overhead
 * once the transform matrix is computed. ALL STACK, NO HEAP.
 */

#include "Plotter.h"
#include "../../Output/Formatter.h"
#include <stdarg.h>

// ============================================================
// COLOR EXTRACTION HELPERS
// ============================================================

// Extract R, G, B from a 0x00RRGGBB UIColor
#define UIColor_R(c) ((uint8_t)((c) >> 16))
#define UIColor_G(c) ((uint8_t)((c) >> 8))
#define UIColor_B(c) ((uint8_t)(c))

// ============================================================
// LOCAL MATH HELPERS no libm dependency (bare-metal friendly)
// ============================================================

static double local_floor(double x) {
    if (x >= 9007199254740992.0 || x <= -9007199254740992.0) return x;
    if (x >= 0.0) return (double)(int64_t)x;
    int64_t i = (int64_t)x;
    return (x == (double)i) ? x : (double)(i - 1);
}

static double local_ceil(double x) {
    if (x >= 9007199254740992.0 || x <= -9007199254740992.0) return x;
    double r = (double)(int64_t)x;
    return (x > r) ? r + 1.0 : r;
}

// Fast log10 approximation by extracting IEEE 754 exponent
static double local_log10_approx(double x) {
    if (x <= 0.0) return 0.0;
    union { double d; uint64_t i; } u;
    u.d = x;
    int64_t exp = ((int64_t)((u.i >> 52) & 0x7FF) - 1023);
    return (double)exp * 0.3010299956639812; // log10(2)
}

// 10^n for integer n (positive or negative)
static double local_pow10(int n) {
    double r = 1.0;
    if (n >= 0) {
        for (int i = 0; i < n; i++) r *= 10.0;
    } else {
        for (int i = 0; i < -n; i++) r /= 10.0;
    }
    return r;
}

// IEEE 754 bit-check for NaN
static inline int local_isnan(double x) {
    union { double d; uint64_t i; } u;
    u.d = x;
    return ((u.i & 0x7FF0000000000000ULL) == 0x7FF0000000000000ULL) &&
           ((u.i & 0x000FFFFFFFFFFFFFULL) != 0ULL);
}

// IEEE 754 bit-check for Infinity
static inline int local_isinf(double x) {
    union { double d; uint64_t i; } u;
    u.d = x;
    return ((u.i & 0x7FF0000000000000ULL) == 0x7FF0000000000000ULL) &&
           ((u.i & 0x000FFFFFFFFFFFFFULL) == 0ULL);
}

// Local snprintf using project's format_string
static int local_snprintf(char* buf, size_t size, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    size_t len = format_string_va(buf, size, fmt, args);
    va_end(args);
    return (int)len;
}

// Compute a "nice" step size for grid lines (1, 2, 5 * 10^n)
static double nice_step(double range, uint32_t target_ticks) {
    if (range <= 0.0) return 1.0;
    double rough_step = range / (double)target_ticks;
    double magnitude = local_pow10((int)local_floor(local_log10_approx(rough_step)));
    double normalized = rough_step / magnitude;

    double nice;
    if (normalized < 1.5)       nice = 1.0;
    else if (normalized < 3.5)  nice = 2.0;
    else if (normalized < 7.5)  nice = 5.0;
    else                        nice = 10.0;

    return nice * magnitude;
}

// ============================================================
// FALLBACK BRESENHAM LINE (if driver doesn't have draw_line)
// ============================================================
static void plot_draw_line_fallback(const DisplayDriver* disp,
                                    int32_t x0, int32_t y0,
                                    int32_t x1, int32_t y1,
                                    uint32_t color) {
    if (!disp || !disp->put_pixel) return;

    int32_t dx = x1 - x0;
    int32_t dy = y1 - y0;
    int32_t abs_dx = (dx < 0) ? -dx : dx;
    int32_t abs_dy = (dy < 0) ? -dy : dy;
    int32_t sx = (dx < 0) ? -1 : 1;
    int32_t sy = (dy < 0) ? -1 : 1;
    int32_t err = abs_dx - abs_dy;
    uint8_t r = UIColor_R(color), g = UIColor_G(color), b = UIColor_B(color);

    while (1) {
        disp->put_pixel(disp, (uint32_t)x0, (uint32_t)y0, r, g, b);
        if (x0 == x1 && y0 == y1) break;
        int32_t e2 = 2 * err;
        if (e2 > -abs_dy) { err -= abs_dy; x0 += sx; }
        if (e2 < abs_dx)  { err += abs_dx; y0 += sy; }
    }
}

// ============================================================
// COORDINATE HELPERS
// ============================================================

bool plot_world_to_screen(const PlotConfig* cfg, double wx, double wy,
                          uint32_t* sx, uint32_t* sy) {
    if (!cfg || !sx || !sy) return false;
    if (cfg->inv_x_range == 0.0 || cfg->inv_y_range == 0.0) return false;

    double inv_w = (double)cfg->width * cfg->inv_x_range;
    double px = (double)cfg->x_offset + (wx - cfg->x_min) * inv_w;
    double py = (double)cfg->y_offset + (cfg->y_max - wy) * (double)cfg->height * cfg->inv_y_range;

    *sx = (uint32_t)(px + 0.5);
    *sy = (uint32_t)(py + 0.5);
    return true;
}

void plot_screen_to_world(const PlotConfig* cfg, uint32_t sx, uint32_t sy,
                          double* wx, double* wy) {
    if (!cfg || !wx || !wy) return;
    double x_range = 1.0 / cfg->inv_x_range;
    double y_range = 1.0 / cfg->inv_y_range;
    *wx = cfg->x_min + ((double)sx - (double)cfg->x_offset) / (double)cfg->width * x_range;
    *wy = cfg->y_max - ((double)sy - (double)cfg->y_offset) / (double)cfg->height * y_range;
}

// ============================================================
// DRAWING routed through DisplayDriver! NO VESA DIRECT ACCESS!
// ============================================================

void plot_clear_area(const DisplayDriver* disp, const PlotConfig* cfg) {
    if (!disp || !cfg) return;
    uint8_t r = UIColor_R(cfg->bg_color);
    uint8_t g = UIColor_G(cfg->bg_color);
    uint8_t b = UIColor_B(cfg->bg_color);

    // Use fill_rect if available much faster!
    if (disp->fill_rect) {
        disp->fill_rect(disp, cfg->x_offset, cfg->y_offset,
                        cfg->width, cfg->height, cfg->bg_color);
        return;
    }

    // Fallback: pixel-by-pixel
    for (uint32_t y = cfg->y_offset; y < cfg->y_offset + cfg->height; y++) {
        for (uint32_t x = cfg->x_offset; x < cfg->x_offset + cfg->width; x++) {
            if (disp->put_pixel) disp->put_pixel(disp, x, y, r, g, b);
        }
    }
}

// ============================================================
// GRID & AXES
// ============================================================

void plot_draw_grid(const DisplayDriver* disp, const PlotConfig* cfg) {
    if (!disp || !cfg) return;

    // Vertical grid lines
    double x_step = nice_step(cfg->x_max - cfg->x_min, cfg->width / 80);
    double x_start = local_ceil(cfg->x_min / x_step) * x_step;

    for (double x = x_start; x <= cfg->x_max; x += x_step) {
        uint32_t sx;
        sx = (uint32_t)((double)cfg->x_offset + (x - cfg->x_min) / (cfg->x_max - cfg->x_min) * (double)cfg->width + 0.5);

        if (sx >= cfg->x_offset && sx < cfg->x_offset + cfg->width) {
            if (disp->draw_line) {
                disp->draw_line(disp, (int32_t)sx, (int32_t)cfg->y_offset,
                                (int32_t)sx, (int32_t)(cfg->y_offset + cfg->height),
                                cfg->grid_color);
            } else {
                plot_draw_line_fallback(disp, (int32_t)sx, (int32_t)cfg->y_offset,
                                        (int32_t)sx, (int32_t)(cfg->y_offset + cfg->height),
                                        cfg->grid_color);
            }
        }
    }

    // Horizontal grid lines
    double y_step = nice_step(cfg->y_max - cfg->y_min, cfg->height / 60);
    double y_start = local_ceil(cfg->y_min / y_step) * y_step;

    for (double y = y_start; y <= cfg->y_max; y += y_step) {
        uint32_t sy = (uint32_t)((double)cfg->y_offset + (cfg->y_max - y) * (double)cfg->height * cfg->inv_y_range + 0.5);

        if (sy >= cfg->y_offset && sy < cfg->y_offset + cfg->height) {
            if (disp->draw_line) {
                disp->draw_line(disp, (int32_t)cfg->x_offset, (int32_t)sy,
                                (int32_t)(cfg->x_offset + cfg->width), (int32_t)sy,
                                cfg->grid_color);
            } else {
                plot_draw_line_fallback(disp, (int32_t)cfg->x_offset, (int32_t)sy,
                                        (int32_t)(cfg->x_offset + cfg->width), (int32_t)sy,
                                        cfg->grid_color);
            }
        }
    }
}

void plot_draw_axes(const DisplayDriver* disp, const PlotConfig* cfg) {
    if (!disp || !cfg) return;

    // X-axis (y=0)
    if (cfg->y_min <= 0.0 && cfg->y_max >= 0.0) {
        uint32_t sy;
        sy = (uint32_t)((double)cfg->y_offset + (cfg->y_max - 0.0) * (double)cfg->height * cfg->inv_y_range + 0.5);
        if (disp->draw_line) {
            disp->draw_line(disp, (int32_t)cfg->x_offset, (int32_t)sy,
                            (int32_t)(cfg->x_offset + cfg->width), (int32_t)sy,
                            cfg->axis_color);
        } else {
            plot_draw_line_fallback(disp, (int32_t)cfg->x_offset, (int32_t)sy,
                                    (int32_t)(cfg->x_offset + cfg->width), (int32_t)sy,
                                    cfg->axis_color);
        }
    }

    // Y-axis (x=0)
    if (cfg->x_min <= 0.0 && cfg->x_max >= 0.0) {
        uint32_t sx;
        sx = (uint32_t)((double)cfg->x_offset + (0.0 - cfg->x_min) * (double)cfg->width * cfg->inv_x_range + 0.5);
        if (disp->draw_line) {
            disp->draw_line(disp, (int32_t)sx, (int32_t)cfg->y_offset,
                            (int32_t)sx, (int32_t)(cfg->y_offset + cfg->height),
                            cfg->axis_color);
        } else {
            plot_draw_line_fallback(disp, (int32_t)sx, (int32_t)cfg->y_offset,
                                    (int32_t)sx, (int32_t)(cfg->y_offset + cfg->height),
                                    cfg->axis_color);
        }
    }
}

void plot_draw_labels(const DisplayDriver* disp, const PlotConfig* cfg) {
    if (!disp || !cfg) return;

    // X-axis tick labels
    double x_step = nice_step(cfg->x_max - cfg->x_min, cfg->width / 80);
    double x_start = local_ceil(cfg->x_min / x_step) * x_step;

    char label[16];
    for (double x = x_start; x <= cfg->x_max; x += x_step) {
        uint32_t sx;
        sx = (uint32_t)((double)cfg->x_offset + (x - cfg->x_min) / (cfg->x_max - cfg->x_min) * (double)cfg->width + 0.5);
        uint32_t sy = cfg->y_offset + cfg->height + 4;

        if (sx >= cfg->x_offset && sx < cfg->x_offset + cfg->width) {
            int n = local_snprintf(label, sizeof(label), "%.1f", x);
            if (disp->write_str) {
                disp->write_str(disp, label, sx - (uint32_t)(n * 4), sy,
                                RGB_TO_UICOLOR(180, 180, 180),
                                RGB_TO_UICOLOR(0, 0, 0));
            }
        }
    }

    // Y-axis tick labels
    double y_step = nice_step(cfg->y_max - cfg->y_min, cfg->height / 60);
    double y_start = local_ceil(cfg->y_min / y_step) * y_step;

    for (double y = y_start; y <= cfg->y_max; y += y_step) {
        uint32_t sy = (uint32_t)((double)cfg->y_offset + (cfg->y_max - y) * (double)cfg->height * cfg->inv_y_range + 0.5);

        if (sy >= cfg->y_offset && sy < cfg->y_offset + cfg->height) {
            int n = local_snprintf(label, sizeof(label), "%.1f", y);
            if (disp->write_str) {
                disp->write_str(disp, label, cfg->x_offset - (uint32_t)(n * 8) - 4, sy,
                                RGB_TO_UICOLOR(180, 180, 180),
                                RGB_TO_UICOLOR(0, 0, 0));
            }
        }
    }
}

// ============================================================
// FUNCTION PLOTTING ALL THROUGH DisplayDriver!
// ============================================================

void plot_function(const DisplayDriver* disp, const PlotConfig* cfg,
                   double (*f)(double),
                   uint32_t color,
                   uint32_t samples) {
    if (!disp || !cfg || !f) return;
    if (samples == 0) samples = cfg->width;

    double step = (cfg->x_max - cfg->x_min) / (double)(samples - 1);
    uint32_t max_samples = samples > 2048 ? 2048 : samples;

    int32_t prev_x = -1, prev_y = -1;

    for (uint32_t i = 0; i < max_samples; i++) {
        double wx = cfg->x_min + (double)i * step;
        double wy = f(wx);

        if (local_isnan(wy) || local_isinf(wy)) {
            prev_x = -1; prev_y = -1;
            continue;
        }

        uint32_t sx, sy;
        if (!plot_world_to_screen(cfg, wx, wy, &sx, &sy)) {
            prev_x = -1; prev_y = -1;
            continue;
        }

        if (sx < cfg->x_offset || sx >= cfg->x_offset + cfg->width) {
            prev_x = -1; prev_y = -1;
            continue;
        }

        if (prev_x >= 0 && prev_y >= 0) {
            int32_t cy = (int32_t)sy;
            int32_t cpy = (int32_t)prev_y;
            if (cy < (int32_t)cfg->y_offset) cy = (int32_t)cfg->y_offset;
            if (cy > (int32_t)(cfg->y_offset + cfg->height)) cy = (int32_t)(cfg->y_offset + cfg->height);
            if (cpy < (int32_t)cfg->y_offset) cpy = (int32_t)cfg->y_offset;
            if (cpy > (int32_t)(cfg->y_offset + cfg->height)) cpy = (int32_t)(cfg->y_offset + cfg->height);

            if (disp->draw_line) {
                disp->draw_line(disp, prev_x, cpy, (int32_t)sx, cy, color);
            } else {
                plot_draw_line_fallback(disp, prev_x, cpy, (int32_t)sx, cy, color);
            }
        }

        prev_x = (int32_t)sx;
        prev_y = (int32_t)sy;
    }
}

void plot_functions_overlay(const DisplayDriver* disp, const PlotConfig* cfg,
                            double (**funcs)(double),
                            uint32_t* colors,
                            uint32_t num_funcs,
                            uint32_t samples) {
    if (!disp || !cfg || !funcs || !colors || num_funcs == 0) return;
    if (num_funcs > PLOT_MAX_OVERLAY) num_funcs = PLOT_MAX_OVERLAY;

    for (uint32_t i = 0; i < num_funcs; i++) {
        if (funcs[i]) {
            plot_function(disp, cfg, funcs[i], colors[i], samples);
        }
    }
}

// ============================================================
// SCATTER PLOTS
// ============================================================

void plot_scatter(const DisplayDriver* disp, const PlotConfig* cfg,
                  const double* x_data,
                  const double* y_data,
                  uint32_t count,
                  uint32_t color,
                  uint32_t point_radius) {
    if (!disp || !cfg || !x_data || !y_data || count == 0) return;
    if (!disp->put_pixel) return;

    uint8_t r = UIColor_R(color), g = UIColor_G(color), b = UIColor_B(color);

    for (uint32_t i = 0; i < count; i++) {
        uint32_t sx, sy;
        if (!plot_world_to_screen(cfg, x_data[i], y_data[i], &sx, &sy)) continue;

        int32_t radius = (int32_t)point_radius;
        for (int32_t dy = -radius; dy <= radius; dy++) {
            for (int32_t dx = -radius; dx <= radius; dx++) {
                int32_t dist_sq = dx * dx + dy * dy;
                if (dist_sq <= radius * radius) {
                    int32_t px = (int32_t)sx + dx;
                    int32_t py = (int32_t)sy + dy;
                    if (px >= (int32_t)cfg->x_offset &&
                        px < (int32_t)(cfg->x_offset + cfg->width) &&
                        py >= (int32_t)cfg->y_offset &&
                        py < (int32_t)(cfg->y_offset + cfg->height)) {
                        disp->put_pixel(disp, (uint32_t)px, (uint32_t)py, r, g, b);
                    }
                }
            }
        }
    }
}

void plot_line_graph(const DisplayDriver* disp, const PlotConfig* cfg,
                     const double* x_data,
                     const double* y_data,
                     uint32_t count,
                     uint32_t color) {
    if (!disp || !cfg || !x_data || !y_data || count < 2) return;

    int32_t prev_x = -1, prev_y = -1;

    for (uint32_t i = 0; i < count; i++) {
        if (local_isnan(y_data[i]) || local_isinf(y_data[i])) {
            prev_x = -1; prev_y = -1;
            continue;
        }

        uint32_t sx, sy;
        if (!plot_world_to_screen(cfg, x_data[i], y_data[i], &sx, &sy)) {
            prev_x = -1; prev_y = -1;
            continue;
        }

        if (prev_x >= 0 && prev_y >= 0) {
            if (disp->draw_line) {
                disp->draw_line(disp, prev_x, prev_y, (int32_t)sx, (int32_t)sy, color);
            } else {
                plot_draw_line_fallback(disp, prev_x, prev_y, (int32_t)sx, (int32_t)sy, color);
            }
        }

        prev_x = (int32_t)sx;
        prev_y = (int32_t)sy;
    }
}

// ============================================================
// HISTOGRAM
// ============================================================

void plot_histogram(const DisplayDriver* disp, const PlotConfig* cfg,
                    const double* data,
                    uint32_t data_count,
                    uint32_t num_bins,
                    uint32_t bar_color) {
    if (!disp || !cfg || !data || data_count == 0 || num_bins == 0) return;
    if (!disp->put_pixel) return;
    if (num_bins > 100) num_bins = 100;

    uint8_t r = UIColor_R(bar_color), g = UIColor_G(bar_color), b = UIColor_B(bar_color);

    double d_min = data[0], d_max = data[0];
    for (uint32_t i = 1; i < data_count; i++) {
        if (data[i] < d_min) d_min = data[i];
        if (data[i] > d_max) d_max = data[i];
    }

    if (d_max - d_min < 1e-15) {
        d_min -= 0.5;
        d_max += 0.5;
    }

    uint32_t bins[100];
    for (uint32_t i = 0; i < num_bins; i++) bins[i] = 0;

    double bin_width = (d_max - d_min) / (double)num_bins;
    for (uint32_t i = 0; i < data_count; i++) {
        uint32_t bin = (uint32_t)((data[i] - d_min) / bin_width);
        if (bin >= num_bins) bin = num_bins - 1;
        bins[bin]++;
    }

    uint32_t max_count = 1;
    for (uint32_t i = 0; i < num_bins; i++) {
        if (bins[i] > max_count) max_count = bins[i];
    }

    double bar_width_px = (double)cfg->width / (double)num_bins;

    for (uint32_t i = 0; i < num_bins; i++) {
        uint32_t bar_height = (uint32_t)((double)bins[i] / (double)max_count * (double)cfg->height);
        uint32_t bar_x = cfg->x_offset + (uint32_t)((double)i * bar_width_px + 0.5);
        uint32_t bar_x_end = cfg->x_offset + (uint32_t)((double)(i + 1) * bar_width_px + 0.5);

        for (uint32_t py = cfg->y_offset + cfg->height - bar_height; py < cfg->y_offset + cfg->height; py++) {
            for (uint32_t px = bar_x; px < bar_x_end; px++) {
                disp->put_pixel(disp, px, py, r, g, b);
            }
        }
    }
}

// ============================================================
// AUTO Y-SCALE (pure math, no DisplayDriver needed)
// ============================================================

void plot_auto_scale_y(PlotConfig* cfg,
                       const double* y_data,
                       uint32_t count,
                       double margin_percent) {
    if (!cfg || !y_data || count == 0) return;

    double ymin = y_data[0], ymax = y_data[0];
    for (uint32_t i = 1; i < count; i++) {
        if (!local_isnan(y_data[i]) && !local_isinf(y_data[i])) {
            if (y_data[i] < ymin) ymin = y_data[i];
            if (y_data[i] > ymax) ymax = y_data[i];
        }
    }

    double margin = (ymax - ymin) * margin_percent / 100.0;
    if (margin < 1e-15) margin = 0.5;

    cfg->y_min = ymin - margin;
    cfg->y_max = ymax + margin;

    plot_config_update_reciprocals(cfg);
}
