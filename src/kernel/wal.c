// src/kernel/wal.c
#include <stdbool.h>
#include "../../include/kernel/wal.h"
#include "../../Infrastructure/Utils/MemoryUtils.h"
#include "../../include/calc/ui.h"
#include "../../Application/Output/Formatter.h"

// MSVC Linker segment boundary references
#if defined(_MSC_VER)
    extern const Application* const __appreg_start;
    extern const Application* const __appreg_end;
#else
    // GCC / Linker script symbols for custom sections
    extern const Application* const __start_appreg[];
    extern const Application* const __stop_appreg[];
#endif

// Zero-allocation Levenshtein distance with stack-allocated buffers to prevent heap fragmentation
int32_t wal_levenshtein_distance(const char* s1, const char* s2) {
    if (!s1 || !s2) return 999;
    
    uint32_t len1 = 0;
    while (s1[len1] != '\0' && len1 < 32) len1++;
    uint32_t len2 = 0;
    while (s2[len2] != '\0' && len2 < 32) len2++;

    if (len1 == 0) return len2;
    if (len2 == 0) return len1;

    int32_t dp0[33];
    int32_t dp1[33];

    for (uint32_t j = 0; j <= len2; j++) {
        dp0[j] = j;
    }

    for (uint32_t i = 1; i <= len1; i++) {
        dp1[0] = i;
        for (uint32_t j = 1; j <= len2; j++) {
            char c1 = s1[i - 1];
            char c2 = s2[j - 1];
            if (c1 >= 'A' && c1 <= 'Z') c1 = c1 - 'A' + 'a';
            if (c2 >= 'A' && c2 <= 'Z') c2 = c2 - 'A' + 'a';

            int32_t cost = (c1 == c2) ? 0 : 1;
            
            int32_t del = dp0[j] + 1;
            int32_t ins = dp1[j - 1] + 1;
            int32_t sub = dp0[j - 1] + cost;

            int32_t min = del;
            if (ins < min) min = ins;
            if (sub < min) min = sub;
            dp1[j] = min;
        }
        for (uint32_t j = 0; j <= len2; j++) {
            dp0[j] = dp1[j];
        }
    }
    return dp0[len2];
}

void wal_init(WindowManager* wm) {
    fast_memset(wm, 0, sizeof(WindowManager));
    wm->focused_app_idx = -1; // -1 = show launcher
    wm->selected_menu_idx = 0;
    wm->page_offset = 0;
    wm->search_len = 0;
    fast_memset(wm->search_buffer, 0, sizeof(wm->search_buffer));

    // Resolve boundaries of the link-time registry section
#if defined(_MSC_VER)
    // MSVC puts segments between start ($a), mid ($b), and end ($z)
    const Application* const* start = &__appreg_start + 1;
    const Application* const* end = &__appreg_end;
    wm->registered_apps = start;
    
    // Calculate size (pointers between start and end)
    uint32_t count = 0;
    for (const Application* const* p = start; p < end; p++) {
        if (*p != (void*)0) {
            count++;
        }
    }
    wm->app_count = count;
#else
    // GCC sets __start_SECTION and __stop_SECTION automatically
    wm->registered_apps = (const Application* const*)__start_appreg;
    wm->app_count = (uint32_t)(__stop_appreg - __start_appreg);
#endif

    // Run init hooks on all registered applications
    for (uint32_t i = 0; i < wm->app_count; i++) {
        const Application* app = wm->registered_apps[i];
        if (app && app->init) {
            app->init(app->context);
        }
    }
}

static void search_find_best_match(WindowManager* wm) {
    if (wm->search_len == 0 || wm->app_count == 0) {
        return;
    }
    
    int32_t best_idx = 0;
    int32_t min_dist = 999;
    
    for (uint32_t i = 0; i < wm->app_count; i++) {
        const Application* app = wm->registered_apps[i];
        if (!app) continue;
        int32_t dist = wal_levenshtein_distance(wm->search_buffer, app->name);
        if (dist < min_dist) {
            min_dist = dist;
            best_idx = (int32_t)i;
        }
    }
    
    wm->selected_menu_idx = best_idx;
    if (wm->selected_menu_idx >= wm->page_offset + LAUNCHER_PAGE_SIZE) {
        wm->page_offset = wm->selected_menu_idx - LAUNCHER_PAGE_SIZE + 1;
    } else if (wm->selected_menu_idx < wm->page_offset) {
        wm->page_offset = wm->selected_menu_idx;
    }
}

void wal_dispatch_input(WindowManager* wm, uint32_t key) {
    if (wm->focused_app_idx != -1) {
        if (key == UI_KEY_F12) {
            wm->focused_app_idx = -1;
            wm->search_len = 0;
            fast_memset(wm->search_buffer, 0, sizeof(wm->search_buffer));
            return;
        }
        
        const Application* active = wm->registered_apps[wm->focused_app_idx];
        if (active && active->update) {
            active->update(active->context, key);
        }
        return;
    }
    
    // Launcher Menu Input logic
    if (key == UI_KEY_UP) {
        if (wm->selected_menu_idx > 0) {
            wm->selected_menu_idx--;
            if (wm->selected_menu_idx < wm->page_offset) {
                wm->page_offset = wm->selected_menu_idx;
            }
        }
    } else if (key == UI_KEY_DOWN) {
        if (wm->selected_menu_idx < (int32_t)wm->app_count - 1) {
            wm->selected_menu_idx++;
            if (wm->selected_menu_idx >= wm->page_offset + LAUNCHER_PAGE_SIZE) {
                wm->page_offset = wm->selected_menu_idx - LAUNCHER_PAGE_SIZE + 1;
            }
        }
    } else if (key == UI_KEY_ENTER) {
        if (wm->app_count > 0 && wm->selected_menu_idx >= 0 && wm->selected_menu_idx < (int32_t)wm->app_count) {
            wm->focused_app_idx = wm->selected_menu_idx;
        }
    } else if (key == UI_KEY_BACKSPACE) {
        if (wm->search_len > 0) {
            wm->search_len--;
            wm->search_buffer[wm->search_len] = '\0';
            search_find_best_match(wm);
        }
    } else {
        if (key >= 32 && key <= 126 && wm->search_len < 31) {
            wm->search_buffer[wm->search_len++] = (char)key;
            wm->search_buffer[wm->search_len] = '\0';
            search_find_best_match(wm);
        }
    }
}

void wal_render_all(WindowManager* wm, const DisplayDriver* disp) {
    if (!disp) return;

    if (wm->focused_app_idx != -1) {
        const Application* app = wm->registered_apps[wm->focused_app_idx];
        if (app && app->draw) {
            ClipRect clip;
            clip.x = 0;
            clip.y = 0;
            clip.w = disp->text_cols;
            clip.h = disp->text_rows;
            app->draw(app->context, disp, clip);
        }
        return;
    }

    // Render system Launcher Menu
    if (disp->clear) {
        disp->clear(disp, UI_COLOR_BLACK);
    }

    if (disp->write_str) {
        disp->write_str(disp, "╔══════════════════════════════════════════════════════╗", 0, 0, UI_COLOR_CYAN, UI_COLOR_BLACK);
        disp->write_str(disp, "║               CALC OS EXOKERNEL LAUNCHER             ║", 0, 1, UI_COLOR_CYAN, UI_COLOR_BLACK);
        disp->write_str(disp, "╚══════════════════════════════════════════════════════╝", 0, 2, UI_COLOR_CYAN, UI_COLOR_BLACK);
        
        char search_line[64];
        format_string(search_line, sizeof(search_line), "Search (Fuzzy matches typos): %s_", wm->search_buffer);
        disp->write_str(disp, search_line, 2, 4, UI_COLOR_YELLOW, UI_COLOR_BLACK);
        disp->write_str(disp, "Use UP/DOWN arrows to navigate. ENTER to run. F12 to exit app.", 2, 5, UI_COLOR_GRAY, UI_COLOR_BLACK);
        disp->write_str(disp, "────────────────────────────────────────────────────────", 0, 6, UI_COLOR_GRAY, UI_COLOR_BLACK);
        
        // Print paginated apps listing
        int start_y = 8;
        int list_count = (int)wm->app_count - wm->page_offset;
        if (list_count > LAUNCHER_PAGE_SIZE) list_count = LAUNCHER_PAGE_SIZE;
        
        for (int i = 0; i < list_count; i++) {
            int app_idx = wm->page_offset + i;
            const Application* app = wm->registered_apps[app_idx];
            if (!app) continue;
            
            char item_line[80];
            bool is_selected = (app_idx == wm->selected_menu_idx);
            
            format_string(item_line, sizeof(item_line), "  %s %d. [%s]", 
                          is_selected ? "=>" : "  ",
                          app_idx + 1,
                          app->name);
            
            disp->write_str(disp, item_line, 2, start_y + i, 
                           is_selected ? UI_COLOR_GREEN : UI_COLOR_WHITE,
                           UI_COLOR_BLACK);
        }
        
        char page_info[64];
        int total_pages = (wm->app_count + LAUNCHER_PAGE_SIZE - 1) / LAUNCHER_PAGE_SIZE;
        int current_page = (wm->page_offset / LAUNCHER_PAGE_SIZE) + 1;
        if (total_pages == 0) total_pages = 1;
        
        format_string(page_info, sizeof(page_info), "Page %d of %d (Total Registered Apps: %d)", 
                      current_page, total_pages, wm->app_count);
        disp->write_str(disp, page_info, 2, start_y + LAUNCHER_PAGE_SIZE + 1, UI_COLOR_LIGHT_GRAY, UI_COLOR_BLACK);
    }
}
