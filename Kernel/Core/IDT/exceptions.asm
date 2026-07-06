; exceptions.asm
; Assembly stubs for IDT exception handlers.
; Each stub saves all registers, pushes the vector number,
; and calls the C exception_handler function.
; 
; For exceptions with error codes (GP=13, PF=14, DF=8),
; the CPU pushes the error code automatically.
; For exceptions without (DE=0, UD=6), we push 0 as placeholder.
;
; THE FLEX: Without these stubs, any hardware exception = triple fault.
; With these stubs, we get a full register dump and an error message.
; That's the difference between a QEMU demo and real hardware. 

[BITS 64]
[SECTION .text]

; Common handler macro: saves all registers and calls handler
%macro EXCEPTION_STUB 1
    ; If the exception has an error code, CPU already pushed it.
    ; If not, we push a dummy 0 to keep stack layout consistent.
    %if %1 != 8 && %1 != 13 && %1 != 14
        push 0                  ; Dummy error code
    %endif
    
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rbp
    push rdi
    push rsi
    push rdx
    push rcx
    push rbx
    push rax
    
    ; RSP now points to saved rax = start of the GPR save area (RegDump).
    ;
    ; Alignment analysis (for error-code exceptions like #PF):
    ;   CPU pushed: error_code(8) + RIP(8) + CS(8) + RFLAGS(8) = 32 bytes
    ;   We pushed:  15 GPRs(120) + push %1(8) + push rdi(8) = 136 bytes
    ;   Total before call: 32 + 136 = 168 = 16*10 + 8 → 8 (mod 16)
    ;   After call pushes return address: +8 → 0 (mod 16) = ALIGNED! ✓
    ;
    ; For non-error-code exceptions (#DE, #UD):
    ;   CPU pushed: RIP(8) + CS(8) + RFLAGS(8) = 24 bytes
    ;   We pushed: push 0(8) + 15 GPRs(120) + push %1(8) + push rdi(8) = 144 bytes
    ;   Total before call: 24 + 144 = 168 = same as above → aligned after call! ✓
    ; No padding needed — removing the old sub rsp,8 was the real fix. 
    
    mov rdi, rsp                ; RDI = pointer to RegDump struct
    push %1                     ; Push the vector number
    push rdi                    ; Push pointer again (stack trace context)
    ; RDI still points to rax — no adjustment needed.
    ; The two pushes went BELOW the GPR save area, so RDI stays valid. 
    
    extern exception_handler
    call exception_handler
    
    ; Never returns — exception_handler halts forever
    ; But have a hlt loop just in case the CPU survives
.hang_%1:
    cli
    hlt
    jmp .hang_%1
%endmacro

; ── Individual stubs ──
global exception_stub_0
exception_stub_0:
    EXCEPTION_STUB 0            ; #DE - Division Error

global exception_stub_6
exception_stub_6:
    EXCEPTION_STUB 6            ; #UD - Invalid Opcode

global exception_stub_8
exception_stub_8:
    EXCEPTION_STUB 8            ; #DF - Double Fault (has error code)

global exception_stub_13
exception_stub_13:
    EXCEPTION_STUB 13           ; #GP - General Protection Fault (has error code)

global exception_stub_14
exception_stub_14:
    EXCEPTION_STUB 14           ; #PF - Page Fault (has error code)