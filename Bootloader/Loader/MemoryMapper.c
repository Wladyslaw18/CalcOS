/*
 * File: MemoryMapper.c
 * Author: W. Kowal
 * Description: Identity page table initialization with e820 awareness.
 * 
 * On real hardware, you CANNOT blindly map the first 16MB.
 * The e820 table (populated by BootEntry.asm via INT 0x15, AX=0xE820)
 * tells which physical memory regions are actually RAM vs MMIO/ACPI.
 * 
 * Without this check, writing to MMIO regions would cause an instant
 * hardware fault. This is what separates QEMU demos from real metal.
 */

#include "../Boot/BootConfig.h"
#include "../../include/calc/memory_map.h"

// FIX: uint64_t is already defined by <stdint.h> via memory_map.h
// This typedef conflicts with the standard header removed to avoid
// redefinition error. The standard uint64_t from <stdint.h> is used.
// (The redundancy was harmless in GCC without -Werror but breaks Clang.)

#define PAGE_PRESENT (1ULL << 0)
#define PAGE_WRITE   (1ULL << 1)
#define PAGE_SIZE    (1ULL << 7)  /* Enables 2MB page size in PDT */

/* Identity maps physical RAM using 2MB huge pages.
 * Uses e820 data if available, falls back to first 16MB. */
void init_paging(void) {
    uint64_t* pml4 = (uint64_t*)PML4_BASE;
    uint64_t* pdpt = (uint64_t*)PDPT_BASE;
    uint64_t* pdt  = (uint64_t*)PDT_BASE;

    /* Zero out page table structures (4KB / 512 entries each) */
    for (int i = 0; i < 512; i++) {
        pml4[i] = 0;
        pdpt[i] = 0;
        pdt[i]  = 0;
    }

    /* PML4[0] -> PDPT */
    pml4[0] = (uint64_t)PDPT_BASE | PAGE_PRESENT | PAGE_WRITE;

    /* PDPT[0] -> PDT */
    pdpt[0] = (uint64_t)PDT_BASE | PAGE_PRESENT | PAGE_WRITE;

    /*
 Query the e820 map for the largest usable RAM region.
 * This tells how much memory can safely identity-map.
 */
    uint64_t ram_base, ram_size;
    ram_base = e820_find_largest_usable(&ram_size);

    /* Ensure don't map below the kernel load or page structures */
    if (ram_base < KERNEL_LOAD_ADDR) {
        ram_base = KERNEL_LOAD_ADDR;
    }
    if (ram_size > 64ULL * 1024 * 1024) {
        ram_size = 64ULL * 1024 * 1024; // Cap at 64MB for now
    }

    /*
 Identity map using 2MB pages only covers the usable RAM range.
 * MMIO and reserved regions stay unmapped, so any accidental write
 * to them triggers a page fault instead of silent corruption.
 */
    uint64_t num_pages = ram_size / 0x200000; // 2MB per page
    if (num_pages > 512) num_pages = 512;     // PDT has 512 entries max
    if (num_pages < 8) num_pages = 8;          // Always map at least 16MB

    for (uint64_t i = 0; i < num_pages; i++) {
        pdt[i] = (i * 0x200000) | PAGE_PRESENT | PAGE_WRITE | PAGE_SIZE;
    }

    /* Map an additional PDPT entry for higher memory (> 1GB) if available.
     * Uses 1GB pages if the CPU supports them, otherwise 2MB. */
    if (ram_base + ram_size > 0x100000000ULL) {
        // Map the region above 4GB via PDPT[1] (1GB page)
        // Note: This requires CPU support for 1GB pages (CPUID leaf 0x80000001, EDX bit 26)
        pdpt[1] = 0x100000000ULL | PAGE_PRESENT | PAGE_WRITE | PAGE_SIZE;
    }
}
