/*
 * InputAdapter.c InputDriver → InputAPI Adapter
 *
 * Wraps a platform-specific InputDriver (from include/calc/input.h)
 * into the stable Kernel-facing InputAPI interface.
 *
 * Usage (in a boot sequence):
 *
 *     InputDriver  my_driver = ...;   // e.g. host_input_create_terminal()
 *     InputAPI     api = input_wrap_driver(&my_driver);
 *     kernel_register(kernel, KRN_INPUT, "input", &api);
 */

#include "Kernel/API/InputAPI.h"
#include "Kernel/Adapters/InputAdapter.h"
#include "include/calc/input.h"

/* ================================================================= */
/*  Trampoline callbacks                                              */
/* ================================================================= */

static int wrapped_key_available(void* context) {
    const InputDriver* drv = (const InputDriver*)context;
    if (drv && drv->key_available) {
        return drv->key_available(drv);
    }
    return 0;
}

static uint32_t wrapped_read_key(void* context) {
    const InputDriver* drv = (const InputDriver*)context;
    if (drv && drv->read_key) {
        return drv->read_key(drv);
    }
    return 0;
}

static int wrapped_init(void* context) {
    const InputDriver* drv = (const InputDriver*)context;
    if (drv && drv->init) {
        return drv->init(drv);
    }
    return 0;
}

static void wrapped_deinit(void* context) {
    const InputDriver* drv = (const InputDriver*)context;
    if (drv && drv->deinit) {
        drv->deinit(drv);
    }
}

/* ================================================================= */
/*  Factory                                                           */
/* ================================================================= */

InputAPI input_wrap_driver(const InputDriver* driver) {
    InputAPI api;
    api.context       = (void*)driver;
    api.key_available = wrapped_key_available;
    api.read_key      = wrapped_read_key;
    api.init          = wrapped_init;
    api.deinit        = wrapped_deinit;
    return api;
}
