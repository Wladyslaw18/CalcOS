SECTIONS
{
    // Raspberry Pi loads bare-metal kernels at address 0x80000 (AArch64 mode) 🏎️
    . = 0x80000;
    
    .text : {
        KEEP(*(.text.boot))
        *(.text .text.*)
    }

    .rodata : {
        *(.rodata .rodata.*)
    }

    .data : {
        *(.data .data.*)
    }

    .align 16;
    __bss_start = .;
    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    }
    .align 16;
    __bss_end = .;
    
    __bss_size = (__bss_end - __bss_start) >> 3; // Size in double words

    . = ALIGN(16);
    __stack_start = .;
    .stack : {
        . += 2048; /* Explicitly reserve 2KB stack size limit! */
    }
    __stack_end = .;
}
