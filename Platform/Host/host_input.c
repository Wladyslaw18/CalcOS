/*
 * host_input.c Host Platform Input Translation Layer
 * 
 * Implements the InputDriver interface from include/calc/input.h.
 * Supports both SDL2 keyboard events and ANSI terminal raw input.
 * 
 * For SDL2: Maps SDL_Scancode values to UI_KEY_* constants.
 * For Terminal: Reads stdin in raw mode and parses escape sequences.
 * 
 * COMPILE with: -DPLATFORM_HOST
 * SDL2 path needs: -DCOMPILE_SDL and SDL2 linkage
 * 
 * THE FLEX: The calc engine doesn't know if you typed on a keyboard,
 * clicked a button, or sent Morse code. It just reads UI_KEY_*.
 */

#include "../../include/calc/input.h"
#include "../../include/calc/display.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/select.h>
#endif

#ifdef COMPILE_SDL
#include <SDL2/SDL.h>
#endif

// Input context
typedef struct {
    int use_sdl;           // Non-zero if using SDL input
#ifdef COMPILE_SDL
    SDL_Event event;       // Current SDL event buffer
#endif
    // Raw terminal mode state (for ANSI terminal input)
    uint8_t  escape_buf[8]; // Buffer for parsing escape sequences
    int      escape_len;
    int      raw_mode_enabled;
} HostInputContext;

// Forward declarations
static int  host_key_available(const InputDriver* self);
static uint32_t host_read_key(const InputDriver* self);
static int  host_input_init(const InputDriver* self);
static void host_input_deinit(const InputDriver* self);

#ifdef COMPILE_SDL
// SDL2 key mapping
static uint32_t sdl_to_ui_key(SDL_Keycode sym) {
    // ASCII-range keys pass through directly
    if (sym >= ' ' && sym <= '~') return (uint32_t)sym;

    switch (sym) {
        case SDLK_RETURN:    return UI_KEY_ENTER;
        case SDLK_BACKSPACE: return UI_KEY_BACKSPACE;
        case SDLK_TAB:       return UI_KEY_TAB;
        case SDLK_ESCAPE:    return UI_KEY_ESCAPE;
        case SDLK_DELETE:    return UI_KEY_DELETE;
        case SDLK_UP:        return UI_KEY_UP;
        case SDLK_DOWN:      return UI_KEY_DOWN;
        case SDLK_LEFT:      return UI_KEY_LEFT;
        case SDLK_RIGHT:     return UI_KEY_RIGHT;
        case SDLK_HOME:      return UI_KEY_HOME;
        case SDLK_END:       return UI_KEY_END;
        case SDLK_PAGEUP:    return UI_KEY_PAGE_UP;
        case SDLK_PAGEDOWN:  return UI_KEY_PAGE_DOWN;
        case SDLK_F1:        return UI_KEY_F1;
        case SDLK_F2:        return UI_KEY_F2;
        case SDLK_F3:        return UI_KEY_F3;
        case SDLK_F4:        return UI_KEY_F4;
        case SDLK_F5:        return UI_KEY_F5;
        case SDLK_F6:        return UI_KEY_F6;
        case SDLK_F7:        return UI_KEY_F7;
        case SDLK_F8:        return UI_KEY_F8;
        case SDLK_F9:        return UI_KEY_F9;
        case SDLK_F10:       return UI_KEY_F10;
        case SDLK_F11:       return UI_KEY_F11;
        case SDLK_F12:       return UI_KEY_F12;
        default:             return UI_KEY_NONE;
    }
}
#endif

// Terminal raw mode
// FIX: Save AND restore the original console mode (#6)
// On POSIX uses a saved struct termios. On Windows save the
// original console mode instead of hardcoding a replacement.
#ifndef _WIN32
static struct termios g_original_termios;
#else
static DWORD g_original_console_mode = 0;
#endif

static void term_enable_raw_mode(void) {
#ifndef _WIN32
    struct termios raw;
    tcgetattr(STDIN_FILENO, &g_original_termios);
    raw = g_original_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
#else
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hStdin, &mode);
    g_original_console_mode = mode;  // Save original!
    mode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
    SetConsoleMode(hStdin, mode);
#endif
}

static void term_disable_raw_mode(void) {
#ifndef _WIN32
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_original_termios);
#else
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    SetConsoleMode(hStdin, g_original_console_mode);  // Restore original!
#endif
}

// Terminal escape sequence parsing
// Reads ANSI escape codes like \033[A (up arrow) and maps to UI_KEY_*
static uint32_t term_parse_escape(const uint8_t* buf, int len) {
    if (len == 1 && buf[0] == '\033') return UI_KEY_ESCAPE;
    if (len < 2 || buf[0] != '\033') return UI_KEY_NONE;
    if (buf[1] != '[') {
        return UI_KEY_ESCAPE;
    }
    if (len < 3) return UI_KEY_NONE;

    // CSI sequences: \033[...
    switch (buf[2]) {
        case 'A': return UI_KEY_UP;
        case 'B': return UI_KEY_DOWN;
        case 'C': return UI_KEY_RIGHT;
        case 'D': return UI_KEY_LEFT;
        case 'H': return UI_KEY_HOME;
        case 'F': return UI_KEY_END;
        case '~': {
            // Extended codes: \033[1~, \033[3~, etc.
            if (len >= 4) {
                switch (buf[3]) {
                    case '1': return buf[4] == '~' ? UI_KEY_HOME : UI_KEY_NONE;
                    case '2': return buf[4] == '~' ? UI_KEY_F1 : UI_KEY_NONE;  // Actually INS but close enough
                    case '3': return buf[4] == '~' ? UI_KEY_DELETE : UI_KEY_NONE;
                    case '4': return buf[4] == '~' ? UI_KEY_END : UI_KEY_NONE;
                    case '5': return buf[4] == '~' ? UI_KEY_PAGE_UP : UI_KEY_NONE;
                    case '6': return buf[4] == '~' ? UI_KEY_PAGE_DOWN : UI_KEY_NONE;
                    default: return UI_KEY_NONE;
                }
            }
            return UI_KEY_NONE;
        }
        default: return UI_KEY_NONE;
    }
}

// key_available
static int host_key_available(const InputDriver* self) {
    if (!self || !self->context) return 0;
    HostInputContext* ctx = (HostInputContext*)self->context;

#ifdef COMPILE_SDL
    if (ctx->use_sdl) {
        // Poll SDL event queue for keyboard events (non-blocking)
        SDL_PumpEvents();
        // Check if there's a key event pending (peek without removing)
        int pending = SDL_PeepEvents(&ctx->event, 1, SDL_PEEKEVENT, SDL_KEYDOWN, SDL_KEYDOWN);
        return pending > 0;
    }
#endif

    // Terminal stdin check (non-blocking)
#ifdef _WIN32
    return _kbhit() ? 1 : 0;
#else
    struct timeval tv = {0, 0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
#endif
}

// read_key
static uint32_t host_read_key(const InputDriver* self) {
    if (!self || !self->context) return UI_KEY_NONE;
    HostInputContext* ctx = (HostInputContext*)self->context;

#ifdef COMPILE_SDL
    if (ctx->use_sdl) {
        // Process ALL pending SDL events, return first keypress
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                return UI_KEY_ESCAPE; // Treat window close as ESC
            }
            if (e.type == SDL_KEYDOWN) {
                uint32_t code = sdl_to_ui_key(e.key.keysym.sym);
                if (code != UI_KEY_NONE) return code;
            }
        }
        return UI_KEY_NONE;
    }
#endif

    // Terminal input: read raw bytes and parse
    char c;
#ifndef _WIN32
    if (read(STDIN_FILENO, &c, 1) <= 0) return UI_KEY_NONE;
#else
    if (!_kbhit()) return UI_KEY_NONE;
    c = (char)_getch();
    if ((unsigned char)c == 0 || (unsigned char)c == 0xE0) {
        char next = (char)_getch();
        switch ((unsigned char)next) {
            case 72: return UI_KEY_UP;
            case 80: return UI_KEY_DOWN;
            case 75: return UI_KEY_LEFT;
            case 77: return UI_KEY_RIGHT;
            case 83: return UI_KEY_DELETE;
            case 71: return UI_KEY_HOME;
            case 79: return UI_KEY_END;
            case 73: return UI_KEY_PAGE_UP;
            case 81: return UI_KEY_PAGE_DOWN;
            default: return UI_KEY_NONE;
        }
    }
#endif

    // Start of escape sequence?
    if (c == '\033') {
        ctx->escape_len = 1;
        ctx->escape_buf[0] = (uint8_t)'\033';
        // Try to read more bytes for the escape sequence (non-blocking)
        uint8_t next;
#ifndef _WIN32
        while (ctx->escape_len < 7 && read(STDIN_FILENO, (char*)&next, 1) > 0) {
#else
        while (ctx->escape_len < 7 && _kbhit()) {
            next = (uint8_t)_getch();
#endif
            ctx->escape_buf[ctx->escape_len++] = next;
            if (next == '~' || (next >= 'A' && next <= 'Z')) break;
        }
        return term_parse_escape(ctx->escape_buf, ctx->escape_len);
    }

    // FIX: Terminal backspace and Enter handling
    // Backspace: most terminals send 0x7F (DEL), some send 0x08 (BS)
    // Enter: most terminals send 0x0D (CR), UI_KEY_ENTER is 0x0A (LF)
    // Map BOTH to the correct UI_KEY_* constants.
    if ((uint8_t)c == 0x7F || (uint8_t)c == 0x08) {
        return UI_KEY_BACKSPACE;
    }
    if ((uint8_t)c == 0x0D) {
        return UI_KEY_ENTER;  // Map carriage return Enter key
    }
    
    // FIX: Filter UTF-8 continuation bytes
    // Bytes 0x80-0xBF are UTF-8 continuation/trail bytes. If read
    // them individually, they produce garbage characters. Skip them
    // to avoid encoding noise from partial UTF-8 sequences.
    if (((uint8_t)c & 0xC0) == 0x80) {
        return UI_KEY_NONE; // Skip UTF-8 continuation byte
    }
    
    // Regular ASCII character (0x00-0x7F, minus control chars)
    return (uint32_t)(unsigned char)c;
}

// init
static int host_input_init(const InputDriver* self) {
    if (!self || !self->context) return -1;
    HostInputContext* ctx = (HostInputContext*)self->context;

#ifdef COMPILE_SDL
    if (ctx->use_sdl) {
        // SDL input initialization is handled by SDL_Init in the display driver
        return 0;
    }
#endif

    // Terminal input: enable raw mode
    if (!ctx->raw_mode_enabled) {
        term_enable_raw_mode();
        ctx->raw_mode_enabled = 1;
    }
    return 0;
}

// deinit
static void host_input_deinit(const InputDriver* self) {
    if (!self || !self->context) return;
    HostInputContext* ctx = (HostInputContext*)self->context;

    if (ctx->raw_mode_enabled) {
        term_disable_raw_mode();
        ctx->raw_mode_enabled = 0;
    }
}

// Public API

// Create an SDL2-based input driver (requires SDL window from host_sdl.c)
int host_input_create_sdl(InputDriver* driver) {
    if (!driver) return -1;
    HostInputContext* ctx = (HostInputContext*)calloc(1, sizeof(HostInputContext));
    if (!ctx) return -2;

#ifdef COMPILE_SDL
    ctx->use_sdl = 1;
#else
    free(ctx);
    return -3; // SDL not compiled in
#endif

    driver->context = ctx;
    driver->name = "SDL2 Keyboard";
    driver->key_available = host_key_available;
    driver->read_key = host_read_key;
    driver->init = host_input_init;
    driver->deinit = host_input_deinit;
    return 0;
}

// Create a terminal-based input driver (stdin raw mode)
int host_input_create_terminal(InputDriver* driver) {
    if (!driver) return -1;
    HostInputContext* ctx = (HostInputContext*)calloc(1, sizeof(HostInputContext));
    if (!ctx) return -2;

    ctx->use_sdl = 0;
    ctx->raw_mode_enabled = 0;

    driver->context = ctx;
    driver->name = "Terminal Keyboard";
    driver->key_available = host_key_available;
    driver->read_key = host_read_key;
    driver->init = host_input_init;
    driver->deinit = host_input_deinit;
    return 0;
}

void host_input_destroy(InputDriver* driver) {
    if (!driver || !driver->context) return;
    HostInputContext* ctx = (HostInputContext*)driver->context;
    if (ctx->raw_mode_enabled) term_disable_raw_mode();
    free(ctx);
    driver->context = NULL;
}
