;=============================================================================
; BootEntry.asm - 16-bit Real Mode Boot Sector
;=============================================================================
[org 0x7C00]
[bits 16]

section .text
global _start

_start:
    ; Disable interrupts
    cli
    xor ax, ax
    mov ds, ax

    ; Flex the screen: Write "CALC OS" directly to VGA text memory at 0xB800:0000 (physical 0xB8000)
    mov ax, 0xB800
    mov es, ax
    mov word [es:0],  0x0F43  ; 'C' (Bright White on Black)
    mov word [es:2],  0x0F41  ; 'A'
    mov word [es:4],  0x0F4C  ; 'L'
    mov word [es:6],  0x0F43  ; 'C'
    mov word [es:8],  0x0F20  ; ' '
    mov word [es:10], 0x0F4F  ; 'O'
    mov word [es:12], 0x0F53  ; 'S'

    ; Initialize COM1 serial port
    mov dx, 0x3F9          ; Disable all interrupts
    mov al, 0x00
    out dx, al
    mov dx, 0x3FB          ; Enable DLAB (set baud rate divisor)
    mov al, 0x80
    out dx, al
    mov dx, 0x3F8          ; Set divisor to 3 (38400 baud)
    mov al, 0x03
    out dx, al
    mov dx, 0x3F9          ; Divisor high byte
    mov al, 0x00
    out dx, al
    mov dx, 0x3FB          ; 8 bits, no parity, one stop bit
    mov al, 0x03
    out dx, al
    mov dx, 0x3FC          ; Enable FIFO, clear them
    mov al, 0xC7
    out dx, al

    ; Write boot message to serial port COM1 (0x3F8)
    mov si, msg_boot
.print_loop:
    lodsb
    test al, al
    jz .print_done
    mov bl, al
.wait_tx:
    mov dx, 0x3FD          ; Line Status Register
    in al, dx
    test al, 0x20          ; Transmitter Holding Register Empty (bit 5)
    jz .wait_tx
    mov dx, 0x3F8          ; Data register
    mov al, bl
    out dx, al
    jmp .print_loop
.print_done:



    ; ═══════════════════════════════════════════════════════════════
    ; e820 MEMORY MAP QUERY — must loop until EBX=0!
    ; INT 0x15, AX=0xE820 only writes ONE entry per call.
    ; Without looping, you get only the first region (640KB).
    ; The list is stored at 0x5000, terminated by a zeroed entry.
    ; ═══════════════════════════════════════════════════════════════
    xor ax, ax
    mov es, ax              ; ES = 0 for the e820 buffer
    mov di, 0x5000          ; ES:DI = buffer location
    xor ebx, ebx            ; Continuation = 0 for first call
    mov edx, 0x534D4150     ; 'SMAP' signature

.e820_loop:
    mov eax, 0xE820         ; EAX gets clobbered by the BIOS call!
    mov ecx, 24             ; Request 24 bytes per entry
    int 0x15
    jc .e820_done           ; Carry flag set -> end of map or error
    cmp eax, 0x534D4150     ; Verify BIOS returned 'SMAP' signature
    jne .e820_done          ; Signature mismatch -> abort

    test ebx, ebx           ; If ebx is 0, this was the last entry
    jz .e820_last_entry

    add di, 24              ; Move buffer destination to next entry slot
    jmp .e820_loop

.e820_last_entry:
    add di, 24              ; Step past the final written entry
.e820_done:
    ; Zero out the subsequent entry to act as a clean terminator
    ; for memory_map.h's e820_get_entry_count() loop!
    push ax
    xor ax, ax
    mov cx, 12              ; 24 bytes = 12 words
    rep stosw               ; Zero out the terminator entry
    pop ax

    ; Zero-initialize segment registers
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00      ; Stack grows down from boot origin

    ; Save boot drive index passed by BIOS in DL
    mov [boot_drive], dl

    ; Enable A20 Gate via BIOS (INT 15h, AX=2401h)
    mov ax, 0x2401
    int 0x15
    jc .a20_keyboard_fallback
    jmp .a20_enabled

.a20_keyboard_fallback:
    ; Fast A20 attempt via Port 0x92
    in al, 0x92
    or al, 2
    out 0x92, al

.a20_enabled:
    ; Load the loader stage from disk (address 0x8000)
    ; DL has the boot drive index.
    mov bx, 0x8000      ; ES:BX = 0x0000:0x8000
    mov ah, 0x02        ; Read sectors from drive function
    mov al, 32          ; Number of sectors to read
    mov ch, 0x00        ; Cylinder 0
    mov dh, 0x00        ; Head 0
    mov cl, 0x02        ; Sector 2 (Sector 1 is boot sector)
    mov dl, [boot_drive]
    int 0x13
    jnc .load_success

    ; BIOS Disk read error: halt
    hlt

.load_success:
    ; Load temporary GDT
    lgdt [gdt_descriptor]

    ; Switch to Protected Mode by setting CR0.PE (bit 0)
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; Far jump to 32-bit protected mode descriptor (0x08 is code selector)
    jmp 0x08:init_32bit

[bits 32]
init_32bit:
    ; Set up 32-bit segment registers (0x10 is data selector)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000    ; Top of stack in protected mode

    ; Jump directly to the loaded C-entry / loader stage at 0x8000
    jmp 0x8000

; Global variables
boot_drive db 0
msg_boot db "CALC OS Bootloader Active (24KB RAM limit)", 10, 13, 0

; GDT Definitions
align 4
gdt_start:
    ; Null descriptor
    dq 0x0

gdt_code:
    ; 32-bit Code descriptor: Base=0, Limit=0xFFFFF, Access=0x9A (present, ring 0, exec/read), Flags=0xC (32-bit, 4KB granularity)
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x9A
    db 0xCF
    db 0x00

gdt_data:
    ; 32-bit Data descriptor: Base=0, Limit=0xFFFFF, Access=0x92 (present, ring 0, read/write), Flags=0xC (32-bit, 4KB granularity)
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x92
    db 0xCF
    db 0x00
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

; Constants for e820 map location
E820_BASE       equ 0x5000   ; Base address for memory map table
E820_MAGIC      equ 0x534D4150 ; 'SMAP'

; Boot sector padding & signature
times 510-($-$$) db 0
dw 0xAA55