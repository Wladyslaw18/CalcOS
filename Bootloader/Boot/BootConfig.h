#ifndef BOOT_CONFIG_H
#define BOOT_CONFIG_H

/* Constants for bootloader stages and kernel loading */
#define BOOT_SECTOR_ORIGIN  0x7C00
#define PROTECTED_MODE_ESP  0x90000
#define KERNEL_LOAD_ADDR    0x100000  /* 1MB physical address */
#define STACK_ADDR          0x7FFF0   /* Top of low memory stack */

/* Paging structure locations in low memory */
#define PML4_BASE           0x1000    /* 0x1000 - 0x1FFF */
#define PDPT_BASE           0x2000    /* 0x2000 - 0x2FFF */
#define PDT_BASE            0x3000    /* 0x3000 - 0x3FFF */
#define PT_BASE             0x4000    /* 0x4000 - 0x4FFF */

/* VGA Constants */
#define VGA_MEM_TEXT        0xB8000
#define VGA_WIDTH           80
#define VGA_HEIGHT          25
#define VGA_COLOR_WHITE_ON_BLACK 0x07

/* I/O Port Constants */
#define PORT_SPEAKER        0x61
#define PORT_PIT_COMMAND    0x43
#define PORT_PIT_DATA2      0x42

#endif /* BOOT_CONFIG_H */
