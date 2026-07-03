// Infrastructure/Platform/RaspberryPi/start.S
// ARMv8 AArch64 Bare-Metal Bootloader Entry Point

.section ".text.boot"

.global _start

_start:
    // Only execute on CPU Core 0, park the other 3 cores in an infinite loop
    mrs     x0, mpidr_el1
    and     x0, x0, #0xFF        // Filter CPU ID
    cbz     x0, .run_core0       // If ID == 0, jump to bootstrapper
    
.park_core:
    wfe                          // Wait for event
    b       .park_core           // Infinite loop park for helper cores

.run_core0:
    // Initialize Stack Pointer below this entry point (stack grows downwards!)
    ldr     x0, =_start
    mov     sp, x0

    // Clear BSS (Zero out uninitialized variables so don't read garbage!)
    ldr     x0, =__bss_start
    ldr     w1, =__bss_size
.clear_bss_loop:
    cbz     w1, .jump_kernel
    str     xzr, [x0], #8        // Zero 8 bytes at a time
    sub     w1, w1, #1
    b       .clear_bss_loop

.jump_kernel:
    // Handoff directly to this unmanaged C main entry point
    bl      kernel_main

.dead_loop:
    wfe
    b       .dead_loop
