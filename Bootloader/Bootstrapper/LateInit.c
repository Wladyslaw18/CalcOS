#include "../Boot/BootConfig.h"
#include "../../include/calc/idt.h"

extern void boot_log_string(const char* str);
extern void boot_log_hex(unsigned long long val);
extern void boot_error(const char* message);

typedef void (*kernel_entry_func)(void);

/* Verifies features such as SSE3 or AVX */
static void query_cpu_features(void) {
    unsigned int eax, ebx, ecx, edx;

    __asm__ volatile (
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(1)
    );

    boot_log_string("CPUID Leaf 1: ECX=");
    boot_log_hex(ecx);
    boot_log_string(" EDX=");
    boot_log_hex(edx);
    boot_log_string("\n");

    if (ecx & (1U << 0)) {
        boot_log_string("SSE3 Instructions verified.\n");
    }
    if (ecx & (1U << 28)) {
        boot_log_string("AVX Instructions verified.\n");
    }
}

/* Performs final diagnostics and executes the kernel */
void late_init(void) {
    boot_log_string("Entering Late Bootstrapper Initialization...\n");

    query_cpu_features();

    /* Install IDT exception handlers (prevents triple fault on real hw!) */
    idt_init();
    boot_log_string("IDT exception handlers installed.\n");

    boot_log_string("Handing off execution to kernel at: ");
    boot_log_hex(KERNEL_LOAD_ADDR);
    boot_log_string("\n");

    kernel_entry_func kernel_entry = (kernel_entry_func)KERNEL_LOAD_ADDR;

    /* Execute the Kernel */
    kernel_entry();

    /* Fallback if Kernel exits */
    boot_error("Kernel returned control to bootstrapper!");
}
