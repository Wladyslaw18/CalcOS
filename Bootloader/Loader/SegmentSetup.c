#include "../Boot/BootConfig.h"

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

struct gdt_tss_descriptor {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
    uint32_t base_highest;
    uint32_t reserved;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

struct tss_entry {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
} __attribute__((packed));

/* Static GDT for Long Mode */
static struct {
    struct gdt_entry null_desc;
    struct gdt_entry code_desc;
    struct gdt_entry data_desc;
    struct gdt_tss_descriptor tss_desc;
} __attribute__((packed)) gdt_64;

static struct gdt_ptr gdt_ptr_64;
struct tss_entry tss_64; // Non-static so IDT module can access IST fields

void init_gdt(void) {
    /* 1. Null Descriptor */
    gdt_64.null_desc.limit_low = 0;
    gdt_64.null_desc.base_low = 0;
    gdt_64.null_desc.base_middle = 0;
    gdt_64.null_desc.access = 0;
    gdt_64.null_desc.granularity = 0;
    gdt_64.null_desc.base_high = 0;

    /* 2. Code Descriptor (64-bit Kernel Code) */
    /* Access: 0x9A (Present, Ring 0, Executable, Readable) */
    /* Flags: 0x20 (Long Mode / L-bit set, Size flag cleared) */
    gdt_64.code_desc.limit_low = 0;
    gdt_64.code_desc.base_low = 0;
    gdt_64.code_desc.base_middle = 0;
    gdt_64.code_desc.access = 0x9A;
    gdt_64.code_desc.granularity = 0x20;
    gdt_64.code_desc.base_high = 0;

    /* 3. Data Descriptor (64-bit Kernel Data) */
    /* Access: 0x92 (Present, Ring 0, Writeable, Readable) */
    gdt_64.data_desc.limit_low = 0;
    gdt_64.data_desc.base_low = 0;
    gdt_64.data_desc.base_middle = 0;
    gdt_64.data_desc.access = 0x92;
    gdt_64.data_desc.granularity = 0;
    gdt_64.data_desc.base_high = 0;

    /* 4. TSS Configuration */
    volatile uint8_t* tss_bytes = (volatile uint8_t*)&tss_64;
    for (uint32_t i = 0; i < sizeof(struct tss_entry); i++) {
        tss_bytes[i] = 0;
    }
    tss_64.rsp0 = PROTECTED_MODE_ESP;
    tss_64.iomap_base = sizeof(struct tss_entry);

    /* TSS Descriptor in GDT */
    uint64_t tss_base = (uint64_t)&tss_64;
    uint32_t tss_limit = sizeof(struct tss_entry) - 1;

    gdt_64.tss_desc.limit_low = (uint16_t)(tss_limit & 0xFFFF);
    gdt_64.tss_desc.base_low = (uint16_t)(tss_base & 0xFFFF);
    gdt_64.tss_desc.base_middle = (uint8_t)((tss_base >> 16) & 0xFF);
    gdt_64.tss_desc.access = 0x89; /* Present, Ring 0, Available 64-bit TSS */
    gdt_64.tss_desc.granularity = 0;
    gdt_64.tss_desc.base_high = (uint8_t)((tss_base >> 24) & 0xFF);
    gdt_64.tss_desc.base_highest = (uint32_t)((tss_base >> 32) & 0xFFFFFFFF);
    gdt_64.tss_desc.reserved = 0;

    /* Configure GDT Pointer */
    gdt_ptr_64.limit = sizeof(gdt_64) - 1;
    gdt_ptr_64.base = (uint32_t)&gdt_64;

    /* Load the GDT */
    __asm__ volatile ("lgdt %0" : : "m"(gdt_ptr_64));
}
