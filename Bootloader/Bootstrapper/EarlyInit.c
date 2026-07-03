#include "../Boot/BootConfig.h"

#define EARLY_STACK_SIZE 4096

/* Aligned stack for early 64-bit execution */
__attribute__((aligned(16))) static char early_stack[EARLY_STACK_SIZE];

extern void late_init(void);
extern void boot_log_string(const char* str);

/* Entry point in 64-bit Long Mode */
void early_init(void) {
    /* Set stack pointer to the top of this aligned stack */
    __asm__ volatile (
        "mov rsp, %0\n\t"
        "mov rbp, rsp"
        :
        : "r"(&early_stack[EARLY_STACK_SIZE])
        : "rsp", "rbp"
    );

    boot_log_string("Successfully entered 64-bit Long Mode!\n");
    boot_log_string("64-bit Execution stack initialized.\n");

    /* Enable coprocessor monitoring & SSE instructions */
    unsigned long long cr0_val;
    unsigned long long cr4_val;

    __asm__ volatile ("mov %%cr0, %0" : "=r"(cr0_val));
    cr0_val &= ~(1ULL << 2);  /* Clear CR0.EM (Emulation) */
    cr0_val |= (1ULL << 1);   /* Set CR0.MP (Monitor Coprocessor) */
    __asm__ volatile ("mov %0, %%cr0" : : "r"(cr0_val));

    __asm__ volatile ("mov %%cr4, %0" : "=r"(cr4_val));
    cr4_val |= (1ULL << 9);   /* Set CR4.OSFXSR (FXSAVE/FXRSTOR support) */
    cr4_val |= (1ULL << 10);  /* Set CR4.OSXMMEXCPT (SIMD Floating-Point Exception) */
    __asm__ volatile ("mov %0, %%cr4" : : "r"(cr4_val));

    boot_log_string("FPU/SSE context extensions configured.\n");

    /* Transition to Late Initialization */
    late_init();
}
