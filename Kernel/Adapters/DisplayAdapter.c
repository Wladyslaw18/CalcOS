/*
 * DisplayAdapter.c DisplayDriver → DisplayAPI Adapter
 *
 * Wraps a platform-specific DisplayDriver (from include/calc/display.h)
 * into the stable Kernel-facing DisplayAPI interface.
 *
 * Usage (in a boot sequence):
 *
 *     DisplayDriver  my_driver = ...;   // e.g. host_term_create()
 *     DisplayAPI     api = display_wrap_driver(&my_driver);
 *     kernel_register(kernel, KRN_DISPLAY, "display", &api);
 */

#include "Kernel/API/DisplayAPI.h"
#include "include/calc/display.h"

/* ================================================================= */
/*  Trampoline callbacks                                              */
/* ================================================================= */

static void wrapped_clear(void* context, uint32_t bg) {
    const DisplayDriver* drv = (const DisplayDriver*)context;
    if (drv && drv->clear) {
        drv->clear(drv, bg);
    }
}

static void wrapped_write_str(void* context, const char* str,
                               uint32_t x, uint32_t y,
                               uint32_t fg, uint32_t bg) {
    const DisplayDriver* drv = (const DisplayDriver*)context;
    if (drv && drv->write_str) {
        drv->write_str(drv, str, x, y, fg, bg);
    }
}

static void wrapped_put_char(void* context, char c,
                              uint32_t x, uint32_t y,
                              uint32_t fg, uint32_t bg) {
    const DisplayDriver* drv = (const DisplayDriver*)context;
    if (drv && drv->put_char) {
        drv->put_char(drv, c, x, y, fg, bg);
    }
}

static void wrapped_set_cursor(void* context, uint32_t x, uint32_t y) {
    const DisplayDriver* drv = (const DisplayDriver*)context;
    if (drv && drv->set_cursor) {
        drv->set_cursor(drv, x, y);
    }
}

static void wrapped_scroll(void* context, int32_t lines) {
    const DisplayDriver* drv = (const DisplayDriver*)context;
    if (drv && drv->scroll) {
        drv->scroll(drv, lines);
    }
}

static void wrapped_present(void* context) {
    const DisplayDriver* drv = (const DisplayDriver*)context;
    if (drv && drv->present) {
        drv->present(drv);
    }
}

/* ================================================================= */
/*  Factory                                                           */
/* ================================================================= */

DisplayAPI display_wrap_driver(const DisplayDriver* driver) {
    DisplayAPI api;
    api.context   = (void*)driver;
    api.width     = driver ? driver->width     : 0;
    api.height    = driver ? driver->height    : 0;
    api.text_cols = driver ? driver->text_cols : 0;
    api.text_rows = driver ? driver->text_rows : 0;

    api.clear      = wrapped_clear;
    api.write_str  = wrapped_write_str;
    api.put_char   = wrapped_put_char;
    api.set_cursor = wrapped_set_cursor;
    api.scroll     = wrapped_scroll;
    api.present    = wrapped_present;

    return api;
}
