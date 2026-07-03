/*
 * host_sdl.c SDL2 Display Driver
 * 
 * Implements the DisplayDriver virtual interface from include/calc/display.h
 * using SDL2 for windowed rendering. Provides:
 * - Pixel-perfect framebuffer via uint32_t surface
 * - Hardware-accelerated line/rect rendering via SDL2
 * - Full present() cycle with texture streaming
 * 
 * This is the Target A (Host) high-resolution display backend.
 * COMPILE with: -DPLATFORM_HOST -DCOMPILE_SDL `sdl2-config --cflags`
 * 
 * THE FLEX: SDL2 on desktop, SDL2 on embedded, SDL2 on anything.
 * The calc engine doesn't know the difference.
 */

#include "../../include/calc/display.h"
#include <stdlib.h>
#include <string.h>

#ifdef COMPILE_SDL
#include <SDL2/SDL.h>

// Driver context
typedef struct {
    SDL_Window*   window;
    SDL_Renderer* renderer;
    SDL_Texture*  texture;
    uint32_t*     pixels;       // Backing pixel buffer (width height)
    uint32_t      width;
    uint32_t      height;
} SDLContext;

// Forward declarations of driver functions
static void sdl_put_pixel(const DisplayDriver* self, uint32_t x, uint32_t y,
                           uint8_t r, uint8_t g, uint8_t b);
static void sdl_clear(const DisplayDriver* self, UIColor bg);
static void sdl_present(const DisplayDriver* self);
static void sdl_draw_line(const DisplayDriver* self,
                           int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                           UIColor color);

// put_pixel
// Write directly to the backing pixel buffer (mimics a flat-mapped VESA LFB).
// The buffer is blitted to the screen on present().
static void sdl_put_pixel(const DisplayDriver* self, uint32_t x, uint32_t y,
                           uint8_t r, uint8_t g, uint8_t b) {
    if (!self || !self->context) return;
    SDLContext* ctx = (SDLContext*)self->context;
    if (x >= ctx->width || y >= ctx->height) return;
    ctx->pixels[y * ctx->width + x] = (uint32_t)(0xFF << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

// fill_rect
// Fast rectangular fill using SDL2 native rendering
static void sdl_fill_rect(const DisplayDriver* self,
                           uint32_t x, uint32_t y,
                           uint32_t w, uint32_t h,
                           UIColor color) {
    if (!self || !self->context) return;
    SDLContext* ctx = (SDLContext*)self->context;
    SDL_Rect rect = { (int)x, (int)y, (int)w, (int)h };
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    SDL_SetRenderDrawColor(ctx->renderer, r, g, b, 255);
    SDL_RenderFillRect(ctx->renderer, &rect);
}

// draw_line
// Hardware-accelerated line via SDL2
static void sdl_draw_line(const DisplayDriver* self,
                           int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                           UIColor color) {
    if (!self || !self->context) return;
    SDLContext* ctx = (SDLContext*)self->context;
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    SDL_SetRenderDrawColor(ctx->renderer, r, g, b, 255);
    SDL_RenderDrawLine(ctx->renderer, x0, y0, x1, y1);
}

// clear
static void sdl_clear(const DisplayDriver* self, UIColor bg) {
    if (!self || !self->context) return;
    SDLContext* ctx = (SDLContext*)self->context;
    uint8_t r = (bg >> 16) & 0xFF;
    uint8_t g = (bg >> 8) & 0xFF;
    uint8_t b = bg & 0xFF;
    SDL_SetRenderDrawColor(ctx->renderer, r, g, b, 255);
    SDL_RenderClear(ctx->renderer);
    // Also clear the pixel buffer
    uint32_t color = (0xFF << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
    for (uint32_t i = 0; i < ctx->width * ctx->height; i++) {
        ctx->pixels[i] = color;
    }
}

// present
// Stream the pixel buffer to the SDL texture and present it.
// This is the VESA "vsync" equivalent without the actual vsync.
static void sdl_present(const DisplayDriver* self) {
    if (!self || !self->context) return;
    SDLContext* ctx = (SDLContext*)self->context;

    SDL_UpdateTexture(ctx->texture, NULL, ctx->pixels, ctx->width * sizeof(uint32_t));
    SDL_RenderCopy(ctx->renderer, ctx->texture, NULL, NULL);
    SDL_RenderPresent(ctx->renderer);
}

// Driver lifecycle
static int sdl_init(DisplayDriver* driver, uint32_t width, uint32_t height, const char* title) {
    if (!driver) return -1;

    SDLContext* ctx = (SDLContext*)calloc(1, sizeof(SDLContext));
    if (!ctx) return -2;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        free(ctx);
        return -3;
    }

    ctx->width = width;
    ctx->height = height;

    ctx->window = SDL_CreateWindow(title ? title : "Calc Engine",
                                    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                    (int)width, (int)height,
                                    SDL_WINDOW_SHOWN);
    if (!ctx->window) {
        SDL_Quit();
        free(ctx);
        return -4;
    }

    ctx->renderer = SDL_CreateRenderer(ctx->window, -1,
                                        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ctx->renderer) {
        SDL_DestroyWindow(ctx->window);
        SDL_Quit();
        free(ctx);
        return -5;
    }

    ctx->texture = SDL_CreateTexture(ctx->renderer,
                                      SDL_PIXELFORMAT_ARGB8888,
                                      SDL_TEXTUREACCESS_STREAMING,
                                      (int)width, (int)height);
    if (!ctx->texture) {
        SDL_DestroyRenderer(ctx->renderer);
        SDL_DestroyWindow(ctx->window);
        SDL_Quit();
        free(ctx);
        return -6;
    }

    // Allocate the backing pixel buffer
    ctx->pixels = (uint32_t*)calloc(width * height, sizeof(uint32_t));
    if (!ctx->pixels) {
        SDL_DestroyTexture(ctx->texture);
        SDL_DestroyRenderer(ctx->renderer);
        SDL_DestroyWindow(ctx->window);
        SDL_Quit();
        free(ctx);
        return -7;
    }

    // Wire up the DisplayDriver function pointers
    driver->context    = ctx;
    driver->width      = width;
    driver->height     = height;
    driver->bpp        = 32;
    driver->put_pixel  = sdl_put_pixel;
    driver->clear      = sdl_clear;
    driver->draw_line  = sdl_draw_line;
    driver->fill_rect  = sdl_fill_rect;
    driver->present    = sdl_present;

    return 0;
}

static void sdl_shutdown(DisplayDriver* driver) {
    if (!driver || !driver->context) return;
    SDLContext* ctx = (SDLContext*)driver->context;
    if (ctx->pixels)   free(ctx->pixels);
    if (ctx->texture)  SDL_DestroyTexture(ctx->texture);
    if (ctx->renderer) SDL_DestroyRenderer(ctx->renderer);
    if (ctx->window)   SDL_DestroyWindow(ctx->window);
    SDL_Quit();
    free(ctx);
    driver->context = NULL;
}

// Public API
int host_sdl_create(DisplayDriver* driver, uint32_t width, uint32_t height, const char* title) {
    return sdl_init(driver, width, height, title);
}

void host_sdl_destroy(DisplayDriver* driver) {
    sdl_shutdown(driver);
}

// Get SDL window for event handling (host_input.c needs this)
SDL_Window* host_sdl_get_window(const DisplayDriver* driver) {
    if (!driver || !driver->context) return NULL;
    return ((SDLContext*)driver->context)->window;
}

#else // COMPILE_SDL not defined stub
int host_sdl_create(DisplayDriver* driver, uint32_t width, uint32_t height, const char* title) {
    (void)driver; (void)width; (void)height; (void)title;
    return -99; // SDL not compiled in
}
void host_sdl_destroy(DisplayDriver* driver) { (void)driver; }
// FIX: Use opaque struct pointer instead of SDL_Window* (unknown type without SDL.h)
void* host_sdl_get_window(const DisplayDriver* driver) { (void)driver; return NULL; }
#endif // COMPILE_SDL
