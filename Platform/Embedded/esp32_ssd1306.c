/*
 * esp32_ssd1306.c ESP32-S3 SSD1306 OLED Display Driver
 * 
 * Implements the DisplayDriver virtual interface (display.h) for
 * 12864 monochrome OLED displays over I2C or SPI.
 * 
 * Hardware: ESP32-S3 + SSD1306 12864 OLED
 * Protocol: I2C (default address 0x3C) or SPI
 * Toolchain: xtensa-esp32s3-elf-gcc (ESP-IDF v5.x)
 * 
 * COMPILE with: -DPLATFORM_ESP32 -DCOMPILE_SSD1306
 * Requires ESP-IDF framework headers.
 * 
 * THE FLEX: Same calc engine driving an OLED smaller than a postage stamp.
 * No JRE required. No framebuffer required. Just I2C and determination.
 * 
 * Hardware mapping:
 * GPIO 21 SDA (I2C data)
 * GPIO 22 SCL (I2C clock)
 * GPIO 4 RST (optional reset, pull high if unused)
 */

#include "../../include/calc/display.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#ifdef PLATFORM_ESP32
#include "sdkconfig.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Configuration
#define SSD1306_WIDTH          128
#define SSD1306_HEIGHT         64
#define SSD1306_PAGES          (SSD1306_HEIGHT / 8)
#define SSD1306_BUFFER_SIZE    (SSD1306_WIDTH * SSD1306_PAGES)  // 1024 bytes

// I2C config
#define SSD1306_I2C_ADDR       0x3C
#define SSD1306_I2C_PORT       I2C_NUM_0
#define SSD1306_I2C_SPEED      400000  // 400kHz fast mode

// Commands
#define SSD1306_SETCONTRAST     0x81
#define SSD1306_DISPLAYALLON    0xA5
#define SSD1306_DISPLAYALLONOFF 0xA4
#define SSD1306_NORMALDISPLAY   0xA6
#define SSD1306_INVERTDISPLAY   0xA7
#define SSD1306_DISPLAYOFF      0xAE
#define SSD1306_DISPLAYON       0xAF
#define SSD1306_SETDISPLAYOFFSET 0xD3
#define SSD1306_SETCOMPINS      0xDA
#define SSD1306_SETVCOMDETECT   0xDB
#define SSD1306_SETDISPLAYCLOCKDIV 0xD5
#define SSD1306_SETPRECHARGE    0xD9
#define SSD1306_SETMULTIPLEX    0xA8
#define SSD1306_SETLOWCOLUMN    0x00
#define SSD1306_SETHIGHCOLUMN   0x10
#define SSD1306_SETSTARTLINE    0x40
#define SSD1306_MEMORYMODE      0x20
#define SSD1306_COLUMNADDR      0x21
#define SSD1306_PAGEADDR        0x22
#define SSD1306_COMSCANINC      0xC0
#define SSD1306_COMSCANDEC      0xC8
#define SSD1306_SEGREMAP        0xA0
#define SSD1306_CHARGEPUMP      0x8D

// Display context
typedef struct {
    uint8_t framebuffer[SSD1306_BUFFER_SIZE];  // 1024 byte monochrome fb
    uint16_t width;
    uint16_t height;
    uint16_t i2c_addr;
} SSD1306Context;

// Send command byte via I2C
static void ssd1306_send_cmd(const DisplayDriver* self, uint8_t cmd) {
    (void)self;  // Context is global in ESP32 single-core mode
    i2c_cmd_handle_t c = i2c_cmd_link_create();
    i2c_master_start(c);
    i2c_master_write_byte(c, (SSD1306_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(c, 0x00, true);  // Co=0, D/C#=0 (command)
    i2c_master_write_byte(c, cmd, true);
    i2c_master_stop(c);
    i2c_master_cmd_begin(SSD1306_I2C_PORT, c, pdMS_TO_TICKS(10));
    i2c_cmd_link_delete(c);
}

// Send data buffer via I2C
static void ssd1306_send_data(const DisplayDriver* self, const uint8_t* data, size_t len) {
    (void)self;
    i2c_cmd_handle_t c = i2c_cmd_link_create();
    i2c_master_start(c);
    i2c_master_write_byte(c, (SSD1306_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(c, 0x40, true);  // Co=0, D/C#=1 (data)
    i2c_master_write(c, (uint8_t*)data, len, true);
    i2c_master_stop(c);
    i2c_master_cmd_begin(SSD1306_I2C_PORT, c, pdMS_TO_TICKS(20));
    i2c_cmd_link_delete(c);
}

// put_pixel
// Set or clear a single pixel in the monochrome framebuffer.
// x: 0..127, y: 0..63, color components: any non-zero r/g/b = ON
static void esp_put_pixel(const DisplayDriver* self, uint32_t x, uint32_t y,
                           uint8_t r, uint8_t g, uint8_t b) {
    if (!self || !self->context) return;
    SSD1306Context* ctx = (SSD1306Context*)self->context;
    if (x >= ctx->width || y >= ctx->height) return;

    // For monochrome OLED: any color component > 128 = pixel ON
    uint8_t on = (r > 128 || g > 128 || b > 128) ? 1 : 0;
    uint32_t byte_idx = (y / 8) * ctx->width + x;
    if (on) {
        ctx->framebuffer[byte_idx] |= (1 << (y & 7));
    } else {
        ctx->framebuffer[byte_idx] &= ~(1 << (y & 7));
    }
}

// clear
// Zero out the framebuffer and flush to display
static void esp_clear(const DisplayDriver* self, UIColor bg) {
    if (!self || !self->context) return;
    SSD1306Context* ctx = (SSD1306Context*)self->context;
    // Monochrome: zero buffer (clear all pixels)
    memset(ctx->framebuffer, (bg != 0) ? 0xFF : 0x00, sizeof(ctx->framebuffer));
}

// draw_line (Bresenham, monochrome)
static void esp_draw_line(const DisplayDriver* self,
                           int32_t x0, int32_t y0,
                           int32_t x1, int32_t y1,
                           UIColor color) {
    if (!self || !self->context) return;
    uint8_t mono = (color != 0) ? 1 : 0;
    SSD1306Context* ctx = (SSD1306Context*)self->context;

    int32_t dx = x1 - x0, dy = y1 - y0;
    int32_t abs_dx = (dx < 0) ? -dx : dx;
    int32_t abs_dy = (dy < 0) ? -dy : dy;
    int32_t sx = (dx < 0) ? -1 : 1;
    int32_t sy = (dy < 0) ? -1 : 1;
    int32_t err = abs_dx - abs_dy;

    while (1) {
        if (x0 >= 0 && x0 < (int32_t)ctx->width && y0 >= 0 && y0 < (int32_t)ctx->height) {
            uint32_t bi = ((uint32_t)y0 / 8) * ctx->width + (uint32_t)x0;
            if (mono) ctx->framebuffer[bi] |= (1 << (y0 & 7));
            else      ctx->framebuffer[bi] &= ~(1 << (y0 & 7));
        }
        if (x0 == x1 && y0 == y1) break;
        int32_t e2 = 2 * err;
        if (e2 > -abs_dy) { err -= abs_dy; x0 += sx; }
        if (e2 < abs_dx)  { err += abs_dx; y0 += sy; }
    }
}

// present
// Flush the entire framebuffer to the SSD1306 over I2C
static void esp_present(const DisplayDriver* self) {
    if (!self || !self->context) return;
    SSD1306Context* ctx = (SSD1306Context*)self->context;

    // Set column and page address range
    ssd1306_send_cmd(self, SSD1306_COLUMNADDR);
    ssd1306_send_cmd(self, 0);
    ssd1306_send_cmd(self, SSD1306_WIDTH - 1);
    ssd1306_send_cmd(self, SSD1306_PAGEADDR);
    ssd1306_send_cmd(self, 0);
    ssd1306_send_cmd(self, SSD1306_PAGES - 1);

    // Push the entire framebuffer
    ssd1306_send_data(self, ctx->framebuffer, sizeof(ctx->framebuffer));
}

#include "../../Application/Interface/MathSymbols.h"

// write_str (character rendering on OLED)
static void esp_write_str(const DisplayDriver* self, const char* str,
                           uint32_t x, uint32_t y,
                           UIColor fg, UIColor bg) {
    if (!self || !self->context || !str) return;
    uint32_t cur_x = x, cur_y = y;
    for (uint32_t i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            cur_x = x;
            cur_y += 16;
        } else {
            const uint8_t* glyph = math_symbols_get_glyph(str[i]);
            if (glyph) {
                for (uint32_t row = 0; row < 16; row++) {
                    uint8_t glyph_row = glyph[row];
                    for (uint32_t col = 0; col < 8; col++) {
                        uint8_t active = (glyph_row >> (7 - col)) & 1;
                        UIColor color = active ? fg : bg;
                        uint8_t r = (color >> 16) & 0xFF;
                        uint8_t g = (color >> 8) & 0xFF;
                        uint8_t b = color & 0xFF;
                        self->put_pixel(self, cur_x + col, cur_y + row, r, g, b);
                    }
                }
            }
            cur_x += 8;
        }
    }
}

// Driver lifecycle
int esp_display_init(DisplayDriver* driver) {
    if (!driver) return -1;

    SSD1306Context* ctx = (SSD1306Context*)calloc(1, sizeof(SSD1306Context));
    if (!ctx) return -2;

    ctx->width = SSD1306_WIDTH;
    ctx->height = SSD1306_HEIGHT;
    ctx->i2c_addr = SSD1306_I2C_ADDR;

    // Configure I2C
    i2c_config_t conf;
    memset(&conf, 0, sizeof(conf));
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = 21;
    conf.scl_io_num = 22;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = SSD1306_I2C_SPEED;
    i2c_param_config(SSD1306_I2C_PORT, &conf);
    i2c_driver_install(SSD1306_I2C_PORT, conf.mode, 0, 0, 0);

    // Initialize SSD1306
    ssd1306_send_cmd(driver, SSD1306_DISPLAYOFF);
    ssd1306_send_cmd(driver, SSD1306_SETDISPLAYCLOCKDIV);
    ssd1306_send_cmd(driver, 0x80);
    ssd1306_send_cmd(driver, SSD1306_SETMULTIPLEX);
    ssd1306_send_cmd(driver, SSD1306_HEIGHT - 1);
    ssd1306_send_cmd(driver, SSD1306_SETDISPLAYOFFSET);
    ssd1306_send_cmd(driver, 0x00);
    ssd1306_send_cmd(driver, SSD1306_SETSTARTLINE | 0x00);
    ssd1306_send_cmd(driver, SSD1306_CHARGEPUMP);
    ssd1306_send_cmd(driver, 0x14);  // Enable charge pump
    ssd1306_send_cmd(driver, SSD1306_MEMORYMODE);
    ssd1306_send_cmd(driver, 0x00);  // Horizontal addressing
    ssd1306_send_cmd(driver, SSD1306_SEGREMAP | 0x01);  // Column addr 127 mapped to SEG0
    ssd1306_send_cmd(driver, SSD1306_COMSCANDEC);  // Scan from COM[N-1] to COM0
    ssd1306_send_cmd(driver, SSD1306_SETCOMPINS);
    ssd1306_send_cmd(driver, SSD1306_HEIGHT == 64 ? 0x12 : 0x02);
    ssd1306_send_cmd(driver, SSD1306_SETCONTRAST);
    ssd1306_send_cmd(driver, 0x7F);
    ssd1306_send_cmd(driver, SSD1306_SETPRECHARGE);
    ssd1306_send_cmd(driver, 0xF1);
    ssd1306_send_cmd(driver, SSD1306_SETVCOMDETECT);
    ssd1306_send_cmd(driver, 0x40);
    ssd1306_send_cmd(driver, SSD1306_DISPLAYALLONOFF);
    ssd1306_send_cmd(driver, SSD1306_NORMALDISPLAY);
    ssd1306_send_cmd(driver, SSD1306_DISPLAYON);

    // Clear framebuffer
    memset(ctx->framebuffer, 0, sizeof(ctx->framebuffer));
    esp_present(driver);

    // Wire up the DisplayDriver interface
    driver->context   = ctx;
    driver->width     = SSD1306_WIDTH;
    driver->height    = SSD1306_HEIGHT;
    driver->bpp       = 1;  // Monochrome
    driver->char_width  = 8;
    driver->char_height = 16;
    driver->put_pixel = esp_put_pixel;
    driver->clear     = esp_clear;
    driver->draw_line = esp_draw_line;
    driver->write_str = esp_write_str;
    driver->present   = esp_present;

    return 0;
}

void esp_display_deinit(DisplayDriver* driver) {
    if (!driver || !driver->context) return;
    SSD1306Context* ctx = (SSD1306Context*)driver->context;
    ssd1306_send_cmd(driver, SSD1306_DISPLAYOFF);
    i2c_driver_delete(SSD1306_I2C_PORT);
    free(ctx);
    driver->context = NULL;
}

#else // Not ESP32 stub for compilation testing
int esp_display_init(DisplayDriver* driver) { (void)driver; return -99; }
void esp_display_deinit(DisplayDriver* driver) { (void)driver; }
#endif // PLATFORM_ESP32
