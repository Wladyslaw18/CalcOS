/*
 * DisplayAdapter.h DisplayDriver → DisplayAPI Adapter (header)
 *
 * Declares the factory function that wraps a platform-specific
 * DisplayDriver into a stable Kernel-facing DisplayAPI struct.
 *
 * See DisplayAdapter.c for the implementation.
 */

#ifndef KERNEL_ADAPTERS_DISPLAYADAPTER_H
#define KERNEL_ADAPTERS_DISPLAYADAPTER_H

#include "../API/DisplayAPI.h"

/* Forward declaration from include/calc/display.h */
struct DisplayDriver;

/*
 * Wrap an existing DisplayDriver into a DisplayAPI struct.
 * The returned DisplayAPI forwards every call to the underlying driver.
 *
 * @param driver  Pointer to an initialised DisplayDriver.
 * @return        Populated DisplayAPI (all function pointers mapped).
 */
DisplayAPI display_wrap_driver(const struct DisplayDriver* driver);

#endif /* KERNEL_ADAPTERS_DISPLAYADAPTER_H */
