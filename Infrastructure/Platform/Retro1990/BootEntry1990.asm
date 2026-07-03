; Infrastructure/Platform/Retro1990/BootEntry1990.asm
; Legacy 16-bit Bootloader targeting Intel 80386/80486 (32-bit Protected Mode) 🏎️

[org 0x7c00]
[bits 16]

boot_entry:
    ; Setup segment registers
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00          ; Stack grows down from boot address

    ; Enable A20 Line via BIOS (Standard for early 90s machines)
    mov ax, 0x2401
    int 0x15

    ; Switch to 32-bit Protected Mode
    cli                     ; Disable interrupts 💀
    lgdt [gdt_descriptor]   ; Load 32-bit GDT

    mov eax, cr0
    or eax, 1               ; Set PE (Protected Mode Enable) bit
    mov cr0, eax

    ; Far jump to flush instruction prefetch queue and load CS segment
    jmp CODE_SEG:init_pm

[bits 32]
init_pm:
    ; Initialize Protected Mode segments
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000        ; Establish stack pointer well above code

    ; Boot directly to our 32-bit kernel entry point (no 64-bit long mode jump!)
    call kernel32_main

.halt_loop:
    hlt
    jmp .halt_loop

; =====================================================================
; GLOBAL DESCRIPTOR TABLE (GDT) FOR 32-BIT FLAT MEMORY MODEL
; =====================================================================
align 4
gdt_start:
    dd 0x0, 0x0             ; Null descriptor

gdt_code:
    dw 0xffff               ; Limit (0-15) = 4GB
    dw 0x0                  ; Base (0-15)
    db 0x0                  ; Base (16-23)
    db 10011010b            ; Access byte (Code, Exec/Read, Ring 0)
    db 11001111b            ; Granularity flag (32-bit, 4KB blocks)
    db 0x0                  ; Base (24-31)

gdt_data:
    dw 0xffff               ; Limit (0-15)
    dw 0x0                  ; Base (0-15)
    db 0x0                  ; Base (16-23)
    db 10010010b            ; Access byte (Data, Read/Write, Ring 0)
    db 11001111b            ; Granularity flag (32-bit)
    db 0x0                  ; Base (24-31)
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

times 510-($-$$) db 0
dw 0xaa55
