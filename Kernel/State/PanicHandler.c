#include "PanicHandler.h"

#if defined(_WIN32) || defined(__linux__) || defined(__APPLE__)
#include <stdio.h>
#include <stdlib.h>
// hosted: just use printf. no serial port needed.
static void serial_write_str(const char* str) {
    printf("%s", str);
}
#else
#include "../../Application/Interface/SerialIO.h"
#endif

// hex dump without printf or stdlib. writes 16 hex chars + null into out_buf.
// out_buf must be at least 17 bytes. no bounds check -- caller's responsibility.
static void raw_u64_to_hex(uint64_t val, char* out_buf) {
    const char hex_chars[] = "0123456789ABCDEF";
    for (int i = 15; i >= 0; i--) {
        out_buf[i] = hex_chars[val & 0xF];
        val >>= 4;
    }
    out_buf[16] = '\0';
}

void panic_abort(const char* message, const CpuRegisters* regs) {
    // step 1: print the reason. don't die quietly.
    serial_write_str("\r\n!!! KERNEL PANIC !!!\r\n");
    if (message) {
        serial_write_str("Reason: ");
        serial_write_str(message);
        serial_write_str("\r\n");
    }

    // step 2: dump registers if caller provided them. otherwise we're flying blind.
    if (regs) {
        char val_buf[17];

        serial_write_str("Register Dump:\r\n");

        serial_write_str("RAX: "); raw_u64_to_hex(regs->rax, val_buf); serial_write_str(val_buf);
        serial_write_str(" RBX: "); raw_u64_to_hex(regs->rbx, val_buf); serial_write_str(val_buf);
        serial_write_str(" RCX: "); raw_u64_to_hex(regs->rcx, val_buf); serial_write_str(val_buf);
        serial_write_str(" RDX: "); raw_u64_to_hex(regs->rdx, val_buf); serial_write_str(val_buf);
        serial_write_str("\r\n");

        serial_write_str("RSI: "); raw_u64_to_hex(regs->rsi, val_buf); serial_write_str(val_buf);
        serial_write_str(" RDI: "); raw_u64_to_hex(regs->rdi, val_buf); serial_write_str(val_buf);
        serial_write_str(" RBP: "); raw_u64_to_hex(regs->rbp, val_buf); serial_write_str(val_buf);
        serial_write_str(" RSP: "); raw_u64_to_hex(regs->rsp, val_buf); serial_write_str(val_buf);
        serial_write_str("\r\n");

        serial_write_str("RIP: "); raw_u64_to_hex(regs->rip, val_buf); serial_write_str(val_buf);
        serial_write_str(" RFLAGS: "); raw_u64_to_hex(regs->rflags, val_buf); serial_write_str(val_buf);
        serial_write_str("\r\n");
    }

    // step 3: reset the machine.
    serial_write_str("Triple faulting CPU now...\r\n");

#if defined(__GNUC__) || defined(__clang__)
    // ACPI reset via port 0xCF9: more reliable than triple-fault.
    // triple-fault behavior is chipset-dependent -- some modern CPUs just hang.
    // ACPI reset is universal. bit 1 = full reset, bit 2 = CPU reset.
    //
    // port 0xCF9 > 0xFF so it can't use the immediate form of outb.
    // `outb %%al, $0xCF9` silently truncates to 0xF9 -- wrong port. use DX.
    __asm__ volatile (
        "movl $0xCF9, %%edx\n\t"
        "movb $0x0E, %%al\n\t"
        "outb %%al, %%dx\n\t"
        :
        :
        : "eax", "edx"
    );

    // brief delay -- let the reset propagate before trying the triple-fault fallback
    for (volatile int i = 0; i < 1000; i++) {
        __asm__ volatile ("nop");
    }

    // triple-fault fallback: zero-length IDT + int3 = double fault + triple fault = reset.
    // on some hardware this just hangs instead. hence the ACPI attempt first.
    volatile uint16_t idt_limit_zero = 0;
    volatile uint64_t idt_base_zero = 0;
    struct __attribute__((packed)) { uint16_t limit; uint64_t base; } idtr;

    idtr.limit = idt_limit_zero;
    idtr.base = idt_base_zero;

    __asm__ volatile (
        "cli\n\t"
        "lidt %0\n\t"
        "int $3"
        :
        : "m"(idtr)
    );

    // CPU somehow survived the triple fault. just halt forever.
    while (1) {
        __asm__ volatile ("cli; hlt");
    }
#else
    // Windows/MSVC hosted fallback: nothing to triple-fault, just exit.
    #ifdef _WIN32
    exit(1);
    #else
    while(1);
    #endif
#endif
}