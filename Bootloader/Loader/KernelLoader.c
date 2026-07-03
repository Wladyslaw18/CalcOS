#include "../Boot/BootConfig.h"

extern void boot_log_string(const char* str);
extern void boot_error(const char* message);
extern void enable_a20(void);
extern void init_paging(void);
extern void init_gdt(void);

/* Verifies CPUID and Long Mode presence */
static int check_long_mode(void) {
    unsigned int eax, edx;

    /* Check maximum extended function level supported */
    __asm__ volatile (
        "cpuid"
        : "=a"(eax)
        : "a"(0x80000000)
        : "ebx", "ecx", "edx"
    );

    if (eax < 0x80000001) {
        return 0;
    }

    /* Check for Long Mode (bit 29 of EDX) */
    __asm__ volatile (
        "cpuid"
        : "=d"(edx)
        : "a"(0x80000001)
        : "ebx", "ecx"
    );

    return (edx & (1U << 29)) ? 1 : 0;
}

/* 32-bit Protected Mode Entry Point */
void loader_main(void) {
    boot_log_string("Entering Bare-Metal Loader Stage...\n");

    if (!check_long_mode()) {
        boot_error("CPU does not support Long Mode!");
    }
    boot_log_string("CPUID Long Mode capability verified.\n");

    enable_a20();
    boot_log_string("A20 Address line verified.\n");

    init_paging();
    boot_log_string("Paging mappings configured at PML4.\n");

    init_gdt();
    boot_log_string("Segment descriptors configured.\n");

    boot_log_string("Transitioning execution to 64-bit Long Mode...\n");

    /* Enter Long Mode and branch to 64-bit entry */
    __asm__ volatile (
        "cli\n\t"
        /* Load PML4 Base into CR3 */
        "mov eax, %0\n\t"
        "mov cr3, eax\n\t"
        /* Enable PAE (Physical Address Extension) in CR4 */
        "mov eax, cr4\n\t"
        "or eax, 0x20\n\t"
        "mov cr4, eax\n\t"
        /* Set LME (Long Mode Enable) bit in EFER MSR */
        "mov ecx, 0xC0000080\n\t"
        "rdmsr\n\t"
        "or eax, 0x100\n\t"
        "wrmsr\n\t"
        /* Enable Paging in CR0 */
        "mov eax, cr0\n\t"
        "or eax, 0x80000000\n\t"
        "mov cr0, eax\n\t"
        /* Far return to 64-bit code segment */
        "push 0x08\n\t"
        "push $entry_64bit\n\t"
        "retf\n"
        :
        : "i"(PML4_BASE)
        : "eax", "ecx", "edx", "memory"
    );

    /* Assembler switch for 64-bit instruction generation */
    __asm__ volatile (
        ".code64\n"
        "entry_64bit:\n\t"
        /* Reset 64-bit segments to long mode data selector */
        "mov ax, 0x10\n\t"
        "mov ds, ax\n\t"
        "mov es, ax\n\t"
        "mov fs, ax\n\t"
        "mov gs, ax\n\t"
        "mov ss, ax\n\t"
        /* Call early init */
        "extern early_init\n\t"
        "call early_init\n\t"
        "hlt_loop:\n\t"
        "hlt\n\t"
        "jmp hlt_loop\n\t"
        ".code32\n"
    );
}
