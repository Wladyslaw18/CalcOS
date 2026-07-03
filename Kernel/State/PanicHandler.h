#ifndef PANIC_HANDLER_H
#define PANIC_HANDLER_H

#include <stdint.h>

// CPU register structure filled during panic invocation
typedef struct {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t rip, cs, rflags, rsp, ss;
} CpuRegisters;

// Gracefully handles a fatal error by dumping registers to screen and COM1, then triple-faulting
void panic_abort(const char* message, const CpuRegisters* regs);

#endif // PANIC_HANDLER_H
