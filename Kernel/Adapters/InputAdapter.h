/*
 * InputAdapter.h InputDriver → InputAPI Adapter (header)
 *
 * Declares the factory function that wraps a platform-specific
 * InputDriver into a stable Kernel-facing InputAPI struct.
 *
 * See InputAdapter.c for the implementation.
 */

#ifndef KERNEL_ADAPTERS_INPUTADAPTER_H
#define KERNEL_ADAPTERS_INPUTADAPTER_H

#include "../API/InputAPI.h"

/* Forward declaration from include/calc/input.h */
struct InputDriver;

/*
 * Wrap an existing InputDriver into an InputAPI struct.
 * The returned InputAPI forwards every call to the underlying driver.
 *
 * @param driver  Pointer to an initialised InputDriver.
 * @return        Populated InputAPI (all function pointers mapped).
 */
InputAPI input_wrap_driver(const struct InputDriver* driver);

#endif /* KERNEL_ADAPTERS_INPUTADAPTER_H */
