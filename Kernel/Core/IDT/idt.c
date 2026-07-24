/*
 * File: idt.c
 * Author: W. Kowal
 * Description: IDT setup with bare-metal exception handlers.
 * 
 * Real hardware WILL fault on unexpected conditions (misaligned SSE access,
 * null pointer dereference, ring-level violation, etc.).
 * Without these handlers, every fault becomes a Triple Fault instant reboot.
 * With these handlers, get a register dump + clear error message.
 */

#include "../../../include/calc/idt.h"

#ifndef _MSC_VER

#ifdef _MSC_VER
#define ALIGN16 __declspec(align(16))
#else
#define ALIGN16 __attribute__((aligned(16)))
#endif

#pragma pack(push, 1)
typedef struct {
    uint16_t offset_low;   // Handler address bits 0-15
    uint16_t selector;     // Code segment selector
    uint8_t  ist;          // Interrupt Stack Table offset (0-7, 0 = disabled)
    uint8_t  type_attr;    // Gate type + DPL + Present
    uint16_t offset_mid;   // Handler address bits 16-31
    uint32_t offset_high;  // Handler address bits 32-63
    uint32_t reserved;     // Reserved, must be 0
} IDTEntry;

typedef struct {
    uint16_t limit;
    uint64_t base;
} IDTPtr;
#pragma pack(pop)

// IDT size: 256 entries, 16 bytes each
#define IDT_SIZE 256
static ALIGN16 IDTEntry idt[IDT_SIZE];

// IST stacks for Double Fault handling
// #DF needs its own stack so it doesn't fault again when it switches stacks!
#define IST_STACK_SIZE 4096 /* 4KB - one page. Enough for #DF handler. Do NOT increase without adjusting _pad. */
static ALIGN16 uint8_t ist_stack1[IST_STACK_SIZE];
// FIX: Stack guard sentinel for IST (#18)
// The IST stack has no guard page below it (bare-metal, no MMU page for that).
// If the double fault handler recurses deeply, it will write past the stack
// and corrupt adjacent global variables. Write a canary pattern to detect this.
#define STACK_CANARY_VALUE 0xDEADC0DE

// Register dump structure
// Matches the push order in the assembly exception stub
typedef struct {
    uint64_t rax, rbx, rcx, rdx;
    uint64_t rsi, rdi, rbp, rsp;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
    uint64_t rip, cs, rflags, rsp_user, ss_user;
    uint64_t error_code;
    uint64_t vector;
} RegDump;

// Forward declarations for assembly stubs
extern void exception_stub_0(void);   // #DE
extern void exception_stub_6(void);   // #UD
extern void exception_stub_8(void);   // #DF
extern void exception_stub_13(void);  // #GP
extern void exception_stub_14(void);  // #PF

// Helper: write character to VGA text buffer
static void vga_putc(char c, int x, int y, uint8_t color) {
    volatile uint16_t* vga = (volatile uint16_t*)0xB8000;
    if (x >= 0 && x < 80 && y >= 0 && y < 25) {
        vga[y * 80 + x] = (uint16_t)((uint16_t)c | ((uint16_t)color << 8));
    }
}

static void vga_puts(const char* str, int x, int y, uint8_t color) {
    int cx = x, cy = y;
    for (int i = 0; str[i]; i++) {
        if (str[i] == '\n') { cx = x; cy++; continue; }
        vga_putc(str[i], cx, cy, color);
        if (++cx >= 80) { cx = x; cy++; }
        if (cy >= 25) break;
    }
}

// Port I/O for serial
static inline void outb(uint16_t port, uint8_t val) {
#if defined(__i386__) || defined(__x86_64__)
    __asm__ volatile ("outb %b0, %w1" : : "a"(val), "Nd"(port));
#else
    (void)port; (void)val;
#endif
}
static inline uint8_t inb(uint16_t port) {
#if defined(__i386__) || defined(__x86_64__)
    uint8_t ret;
    __asm__ volatile ("inb %w1, %b0" : "=a"(ret) : "Nd"(port));
    return ret;
#else
    (void)port; return 0;
#endif
}

// Serial COM1 helpers
#define COM1_PORT 0x3F8
static void serial_out(char c) {
    for (int i = 0; i < 100000; i++) {
        if (inb(COM1_PORT + 5) & 0x20) break;
    }
    outb(COM1_PORT, (uint8_t)c);
}

static void serial_str(const char* s) {
    for (; *s; s++) serial_out(*s);
}

// Hex dump helper
static void print_hex64(uint64_t val, int col, int row) {
    const char hex[] = "0123456789ABCDEF";
    char buf[19] = "0x0000000000000000";
    for (int i = 17; i >= 2; i--) {
        buf[i] = hex[val & 0xF];
        val >>= 4;
    }
    for (int i = 0; i < 18; i++) vga_putc(buf[i], col + i, row, 0x4F);
}

static void serial_hex64(uint64_t val) {
    const char hex[] = "0123456789ABCDEF";
    serial_str("0x");
    for (int i = 60; i >= 0; i -= 4) serial_out(hex[(val >> i) & 0xF]);
}

// C exception handler
// Called from the assembly stub after saving registers.
// Prints full register dump to VGA + COM1, then halts forever.
void exception_handler(RegDump* regs) {
    // Disable interrupts immediately
    __asm__ volatile ("cli");

    vga_puts("\n\n!!! KERNEL EXCEPTION !!!\n\n", 0, 0, 0x4F);

    serial_str("\r\n!!! KERNEL EXCEPTION !!!\r\n");
    serial_str("Vector: "); serial_hex64(regs->vector); serial_str("\r\n");
    serial_str("Error:  "); serial_hex64(regs->error_code); serial_str("\r\n");

    const char* names[] = {
        "#DE - Division Error",   // 0
        "#UD - Invalid Opcode",   // 6
        "#DF - Double Fault",     // 8
        "#GP - Protection Fault",  // 13
        "#PF - Page Fault"        // 14
    };
    int name_idx = -1;
    switch (regs->vector) {
        case 0:  name_idx = 0; break;
        case 6:  name_idx = 1; break;
        case 8:  name_idx = 2; break;
        case 13: name_idx = 3; break;
        case 14: name_idx = 4; break;
    }
    if (name_idx >= 0) {
        vga_puts(names[name_idx], 0, 2, 0x4F);
        serial_str(names[name_idx]); serial_str("\r\n");
    }

    // Print registers in a 2-column layout
    struct { const char* name; uint64_t val; int reg_idx; } regs_out[] = {
        {"RAX", regs->rax, 0}, {"RBX", regs->rbx, 0},
        {"RCX", regs->rcx, 0}, {"RDX", regs->rdx, 0},
        {"RSI", regs->rsi, 0}, {"RDI", regs->rdi, 0},
        {"RBP", regs->rbp, 0}, {"RSP", regs->rsp_user, 0},
        {"R8 ", regs->r8, 0}, {"R9 ", regs->r9, 0},
        {"R10", regs->r10, 0}, {"R11", regs->r11, 0},
        {"R12", regs->r12, 0}, {"R13", regs->r13, 0},
        {"R14", regs->r14, 0}, {"R15", regs->r15, 0},
        {"RIP", regs->rip, 0}, {"CS", regs->cs, 0},
        {"RFL", regs->rflags, 0}, {"ERR", regs->error_code, 0}
    };

    int row = 4;
    for (int i = 0; i < 20; i += 2) {
        vga_puts(regs_out[i].name, 0, row, 0x4F);
        vga_putc('=', 3, row, 0x4F);
        print_hex64(regs_out[i].val, 5, row);

        vga_puts(regs_out[i+1].name, 30, row, 0x4F);
        vga_putc('=', 33, row, 0x4F);
        print_hex64(regs_out[i+1].val, 35, row);
        row++;
    }

    // Serial dump
    for (int i = 0; i < 20; i++) {
        serial_str(regs_out[i].name);
        serial_str("=");
        serial_hex64(regs_out[i].val);
        serial_str(i % 2 == 0 ? "  " : "\r\n");
    }

    // Page fault specific info
    if (regs->vector == 14) {
        uint64_t cr2;
        __asm__ volatile ("mov %%cr2, %0" : "=r"(cr2));
        vga_puts("CR2 (fault address)=", 0, row, 0x4C);
        print_hex64(cr2, 20, row);
        serial_str("CR2="); serial_hex64(cr2); serial_str("\r\n");
    }

    // Check IST stack canary integrity
    if (*(volatile uint32_t*)(ist_stack1 + IST_STACK_SIZE - 4) != 0xA110CA7E) {
        vga_puts("WARNING: IST Stack Top Sentinel Clobbered! (Overflow)", 0, row, 0x4E);
        serial_str("WARNING: IST Stack Top Sentinel Clobbered! (Overflow)\r\n");
        row++;
    }
    if (((volatile uint32_t*)ist_stack1)[0] != STACK_CANARY_VALUE) {
        vga_puts("WARNING: IST Stack Base Canary Clobbered! (Deep overflow)", 0, row, 0x4E);
        serial_str("WARNING: IST Stack Base Canary Clobbered! (Deep overflow)\r\n");
        row++;
    }

    vga_puts("SYSTEM HALTED - Press reset button", 0, 24, 0x4F);
    serial_str("\r\n!!! SYSTEM HALTED !!!\r\n");

    // Infinite halt CPU consumes near-zero power
    while (1) {
        __asm__ volatile ("cli; hlt");
    }
}

// Helper: set a single IDT entry
static void set_idt_entry(int vec, void* handler, uint8_t type_attr, uint8_t ist) {
    uint64_t addr = (uint64_t)handler;
    idt[vec].offset_low  = (uint16_t)(addr & 0xFFFF);
    idt[vec].selector    = 0x08; // 64-bit code segment
    idt[vec].ist         = ist;
    idt[vec].type_attr   = type_attr;
    idt[vec].offset_mid  = (uint16_t)((addr >> 16) & 0xFFFF);
    idt[vec].offset_high = (uint32_t)((addr >> 32) & 0xFFFFFFFF);
    idt[vec].reserved    = 0;
}

// Initialize IDT
void idt_init(void) {
    // Zero out the entire IDT (all entries start as "not present")
    for (int i = 0; i < IDT_SIZE; i++) {
        *(uint64_t*)&idt[i] = 0;
        *(uint64_t*)((char*)&idt[i] + 8) = 0;
    }

    // Install exception handlers (type_attr = 0x8E: 32-bit trap gate, present, ring 0)
    // In 64-bit mode: Interrupt Gate (DPL=0, Present, 0-size for 64-bit)
    // 0x8E: present, DPL=0, interrupt gate
    uint8_t attr = 0x8E; // 64-bit interrupt gate, present, DPL=0

    set_idt_entry(0,  exception_stub_0,  attr, 0);  // #DE
    set_idt_entry(6,  exception_stub_6,  attr, 0);  // #UD
    set_idt_entry(8,  exception_stub_8,  attr, 1);  // #DF with IST=1
    set_idt_entry(13, exception_stub_13, attr, 0);  // #GP
    set_idt_entry(14, exception_stub_14, attr, 0);  // #PF

    // Set up IST for Double Fault (stack switch to known-good stack)
    // The IST address is stored in the TSS, which configured in SegmentSetup.c.
    // Here ensure the TSS.ist1 field points to this dedicated stack.
    //
    // The TSS was set up with base at &tss_64 in SegmentSetup.c.
    // rsp0 was set to PROTECTED_MODE_ESP.
    // must update ist1 to point to this DF stack.
    //
    // Write the IST1 entry: TSS offset 36 (rsp0) + 8 (rsp1) + 8 (rsp2) + 8 (reserved1)
    // Actually: TSS layout: reserved0(4), rsp0(8), rsp1(8), rsp2(8), reserved1(8),
    //           ist1(8), ist2(8), ...
    // IST1 is at offset 36 (4 + 8*3 + 8 = 36) from TSS base.
    // But must know the TSS address. make it simple:
    // write the IST entries directly using the TSS location.
    //
    // For now, the TSS was set up in SegmentSetup.c with ist1=0. update it:
    // TSS base is stored in gdt_64's TSS descriptor. can't easily get it.
    // Alternative: use MSR to set IST1 for vector 8.
    //
    // The simplest approach: use `ltr` to load a TSS with IST set up.
    // Since the existing TSS in SegmentSetup.c already has rsp0 set,
    // can just update the IST1 field if know the TSS address.
    //
    // Reference the TSS from SegmentSetup.c (made non-static for this)
    extern struct tss_entry { uint32_t reserved0; uint64_t rsp0, rsp1, rsp2, reserved1;
        uint64_t ist1, ist2, ist3, ist4, ist5, ist6, ist7;
        uint64_t reserved2; uint16_t reserved3; uint16_t iomap_base; } tss_64;
    // Set IST1 to this dedicated stack top (for Double Fault recovery)
    tss_64.ist1 = (uint64_t)(ist_stack1 + IST_STACK_SIZE);

    // FIX: Paint the IST stack with a canary pattern (#18)
    // Fill the entire IST stack area with a known pattern, then write the
    // canary at the bottom. After a double fault, check if the canary is
    // intact if overwritten, the handler recursed too deeply.
    for (uint32_t i = 0; i < IST_STACK_SIZE / sizeof(uint32_t); i++) {
        ((volatile uint32_t*)ist_stack1)[i] = STACK_CANARY_VALUE;
    }
    // Set a unique sentinel at the very top of the stack (first to be clobbered)
    *(volatile uint32_t*)(ist_stack1 + IST_STACK_SIZE - 4) = 0xA110CA7E;

    // Load the IDT
    IDTPtr idtr;
    idtr.limit = sizeof(idt) - 1;
    idtr.base  = (uint64_t)idt;
    __asm__ volatile ("lidt %0" : : "m"(idtr));

    // Log success
    volatile uint16_t* vga = (volatile uint16_t*)0xB8000;
    vga[0] = (uint16_t)'I' | (0x02 << 8);
    vga[1] = (uint16_t)'D' | (0x02 << 8);
    vga[2] = (uint16_t)'T' | (0x02 << 8);
    vga[3] = (uint16_t)'=' | (0x02 << 8);
    vga[4] = (uint16_t)'O' | (0x02 << 8);
    vga[5] = (uint16_t)'K' | (0x02 << 8);
}
#else
void idt_init(void) {
    // No-op under host simulation
}
#endif
