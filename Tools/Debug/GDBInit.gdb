# GDB Initialization Script for Bare Metal Calculator

# Connect to QEMU's remote gdb stub on localhost:1234
target remote localhost:1234

# Load the ELF symbols for debugging
symbol-file Build/isofiles/boot/kernel.elf

# Set architecture to 64-bit x86
set architecture i386:x86-64

# Force disassembly style to Intel
set disassembly-flavor intel

# Add helper hooks
define show_regs
    info registers rax rbx rcx rdx rsi rdi rbp rsp rip r8 r9 r10 r11 r12 r13 r14 r15
end

document show_regs
    Display standard 64-bit general purpose registers.
end

# Set a breakpoint at the kernel start
b kernel_main

echo \n=== GDB Configured and Connected ===\n
echo Use 'c' or 'continue' to start execution up to kernel_main.\n
