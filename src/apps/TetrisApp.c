// src/apps/TetrisApp.c
#include <stdbool.h>
#include "../../include/kernel/app.h"
#include "../../Infrastructure/Utils/MemoryUtils.h"
#include "../../include/calc/ui.h"
#include "../../Application/Output/Formatter.h"

#define BOARD_WIDTH  10
#define BOARD_HEIGHT 20

// Contiguous application state (fits in BSS, zero heap!)
typedef struct {
    uint8_t  board[BOARD_HEIGHT][BOARD_WIDTH];
    int32_t  current_x;
    int32_t  current_y;
    uint32_t current_piece;
    uint32_t current_rotation;
    uint32_t score;
    uint32_t ticks;
    bool     game_over;
} TetrisAppContext;

static TetrisAppContext g_tetris_state;

// Tetromino configurations (4 rotations x 4 blocks x 2 coordinates)
static const int8_t TETROMINOES[7][4][4][2] = {
    // I Piece
    {
        {{0,1}, {1,1}, {2,1}, {3,1}},
        {{2,0}, {2,1}, {2,2}, {2,3}},
        {{0,2}, {1,2}, {2,2}, {3,2}},
        {{1,0}, {1,1}, {1,2}, {1,3}}
    },
    // O Piece
    {
        {{1,1}, {1,2}, {2,1}, {2,2}},
        {{1,1}, {1,2}, {2,1}, {2,2}},
        {{1,1}, {1,2}, {2,1}, {2,2}},
        {{1,1}, {1,2}, {2,1}, {2,2}}
    },
    // T Piece
    {
        {{1,0}, {0,1}, {1,1}, {2,1}},
        {{1,0}, {1,1}, {2,1}, {1,2}},
        {{0,1}, {1,1}, {2,1}, {1,2}},
        {{1,0}, {0,1}, {1,1}, {1,2}}
    },
    // S Piece
    {
        {{1,1}, {2,1}, {0,2}, {1,2}},
        {{1,0}, {1,1}, {2,1}, {2,2}},
        {{1,1}, {2,1}, {0,2}, {1,2}},
        {{1,0}, {1,1}, {2,1}, {2,2}}
    },
    // Z Piece
    {
        {{0,1}, {1,1}, {1,2}, {2,2}},
        {{2,0}, {1,1}, {2,1}, {1,2}},
        {{0,1}, {1,1}, {1,2}, {2,2}},
        {{2,0}, {1,1}, {2,1}, {1,2}}
    },
    // J Piece
    {
        {{0,0}, {0,1}, {1,1}, {2,1}},
        {{1,0}, {2,0}, {1,1}, {1,2}},
        {{0,1}, {1,1}, {2,1}, {2,2}},
        {{1,0}, {1,1}, {0,2}, {1,2}}
    },
    // L Piece
    {
        {{2,0}, {0,1}, {1,1}, {2,1}},
        {{1,0}, {1,1}, {1,2}, {2,2}},
        {{0,1}, {1,1}, {2,1}, {0,2}},
        {{0,0}, {1,0}, {1,1}, {1,2}}
    }
};

static void spawn_piece(TetrisAppContext* app) {
    // Simple determinism (ticks modulo 7) instead of rand() to stay zero-alloc/no-libc
    app->current_piece = app->ticks % 7;
    app->current_rotation = 0;
    app->current_x = BOARD_WIDTH / 2 - 2;
    app->current_y = 0;

    // Check game over
    for (int i = 0; i < 4; i++) {
        int px = app->current_x + TETROMINOES[app->current_piece][app->current_rotation][i][0];
        int py = app->current_y + TETROMINOES[app->current_piece][app->current_rotation][i][1];
        if (py >= 0 && app->board[py][px]) {
            app->game_over = true;
        }
    }
}

static void tetris_app_init(void* ctx) {
    TetrisAppContext* app = (TetrisAppContext*)ctx;
    fast_memset(app, 0, sizeof(TetrisAppContext));
    spawn_piece(app);
}

static bool check_collision(TetrisAppContext* app, int32_t dx, int32_t dy, int32_t dr) {
    int32_t next_rot = (app->current_rotation + dr) % 4;
    for (int i = 0; i < 4; i++) {
        int32_t px = app->current_x + dx + TETROMINOES[app->current_piece][next_rot][i][0];
        int32_t py = app->current_y + dy + TETROMINOES[app->current_piece][next_rot][i][1];
        
        if (px < 0 || px >= BOARD_WIDTH || py >= BOARD_HEIGHT) {
            return true;
        }
        if (py >= 0 && app->board[py][px]) {
            return true;
        }
    }
    return false;
}

static void lock_piece(TetrisAppContext* app) {
    for (int i = 0; i < 4; i++) {
        int32_t px = app->current_x + TETROMINOES[app->current_piece][app->current_rotation][i][0];
        int32_t py = app->current_y + TETROMINOES[app->current_piece][app->current_rotation][i][1];
        if (py >= 0) {
            app->board[py][px] = (uint8_t)(app->current_piece + 1);
        }
    }

    // Line clearing
    for (int y = BOARD_HEIGHT - 1; y >= 0; y--) {
        bool full = true;
        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (app->board[y][x] == 0) {
                full = false;
                break;
            }
        }
        if (full) {
            app->score += 100;
            // Shift down
            for (int sy = y; sy > 0; sy--) {
                for (int sx = 0; sx < BOARD_WIDTH; sx++) {
                    app->board[sy][sx] = app->board[sy - 1][sx];
                }
            }
            // Clear top line
            for (int sx = 0; sx < BOARD_WIDTH; sx++) {
                app->board[0][sx] = 0;
            }
            y++; // Check same line index again after shift
        }
    }

    spawn_piece(app);
}

static void tetris_app_update(void* ctx, uint32_t key) {
    TetrisAppContext* app = (TetrisAppContext*)ctx;
    app->ticks++;

    if (app->game_over) {
        if (key == UI_KEY_ENTER) {
            tetris_app_init(app);
        }
        return;
    }

    // Handle user inputs
    switch (key) {
        case UI_KEY_LEFT:
            if (!check_collision(app, -1, 0, 0)) app->current_x--;
            break;
        case UI_KEY_RIGHT:
            if (!check_collision(app, 1, 0, 0)) app->current_x++;
            break;
        case UI_KEY_DOWN:
            // Drop down step
            if (!check_collision(app, 0, 1, 0)) {
                app->current_y++;
            } else {
                lock_piece(app);
            }
            break;
        case UI_KEY_UP:
            // Rotate
            if (!check_collision(app, 0, 0, 1)) {
                app->current_rotation = (app->current_rotation + 1) % 4;
            }
            break;
        default:
            // Gravity Tick step (runs on ticks count threshold for smooth play loop)
            if (app->ticks % 20 == 0) {
                if (!check_collision(app, 0, 1, 0)) {
                    app->current_y++;
                } else {
                    lock_piece(app);
                }
            }
            break;
    }
}

static void tetris_app_draw(void* ctx, const DisplayDriver* disp, ClipRect clip) {
    TetrisAppContext* app = (TetrisAppContext*)ctx;

    if (!disp || !disp->write_str || !disp->put_char) return;

    // Clear board area within ClipRect
    disp->clear(disp, UI_COLOR_BLACK);

    // Draw borders
    int offset_x = clip.x + 5;
    int offset_y = clip.y + 2;

    for (int y = 0; y <= BOARD_HEIGHT; y++) {
        disp->put_char(disp, '|', offset_x - 1, offset_y + y, UI_COLOR_GRAY, UI_COLOR_BLACK);
        disp->put_char(disp, '|', offset_x + BOARD_WIDTH, offset_y + y, UI_COLOR_GRAY, UI_COLOR_BLACK);
    }
    for (int x = 0; x < BOARD_WIDTH; x++) {
        disp->put_char(disp, '-', offset_x + x, offset_y + BOARD_HEIGHT, UI_COLOR_GRAY, UI_COLOR_BLACK);
    }

    // Draw Board contents
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            uint8_t cell = app->board[y][x];
            if (cell > 0) {
                // Color pieces differently
                UIColor color = UI_COLOR_CYAN + (cell - 1);
                disp->put_char(disp, '#', offset_x + x, offset_y + y, color, UI_COLOR_BLACK);
            }
        }
    }

    // Draw Active Falling Piece
    if (!app->game_over) {
        for (int i = 0; i < 4; i++) {
            int32_t px = app->current_x + TETROMINOES[app->current_piece][app->current_rotation][i][0];
            int32_t py = app->current_y + TETROMINOES[app->current_piece][app->current_rotation][i][1];
            if (py >= 0) {
                disp->put_char(disp, '@', offset_x + px, offset_y + py, UI_COLOR_YELLOW, UI_COLOR_BLACK);
            }
        }
    }

    // Draw Stats Dashboard
    char stats_line[64];
    format_string(stats_line, sizeof(stats_line), "SCORE: %d", app->score);
    disp->write_str(disp, stats_line, offset_x + BOARD_WIDTH + 4, offset_y + 2, UI_COLOR_GREEN, UI_COLOR_BLACK);

    if (app->game_over) {
        disp->write_str(disp, " GAME OVER ", offset_x + BOARD_WIDTH + 4, offset_y + 5, UI_COLOR_RED, UI_COLOR_BLACK);
        disp->write_str(disp, "Press Enter to Retry", offset_x + BOARD_WIDTH + 4, offset_y + 6, UI_COLOR_YELLOW, UI_COLOR_BLACK);
    }
}

const Application g_tetris_app = {
    .name = "Tetris",
    .context = &g_tetris_state,
    .init = tetris_app_init,
    .update = tetris_app_update,
    .draw = tetris_app_draw
};

REGISTER_APPLICATION(g_tetris_app)
