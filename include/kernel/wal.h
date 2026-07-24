// include/kernel/wal.h
#pragma once
#include "app.h"

#define LAUNCHER_PAGE_SIZE 10

typedef struct {
    // We now reference applications by pointer from the link-time registry!
    // No copying Application structs to RAM. RAM overhead is literally ZERO.
    const Application* const* registered_apps;
    uint32_t                  app_count;
    
    // Application Focus State
    int32_t     focused_app_idx; // -1 represents the Launcher menu is active
    
    // Launcher Menu navigation state
    int32_t     selected_menu_idx; // Currently highlighted app index in the menu list
    int32_t     page_offset;       // Scrolling offset for multi-page menu
    
    // Typo-tolerant search
    char        search_buffer[32];
    uint32_t    search_len;
    
} WindowManager;

void wal_init(WindowManager* wm);
void wal_dispatch_input(WindowManager* wm, uint32_t key);
void wal_render_all(WindowManager* wm, const DisplayDriver* disp);
int32_t wal_levenshtein_distance(const char* s1, const char* s2);
