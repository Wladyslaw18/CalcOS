#!/bin/bash
# Script to launch QEMU for the Bare Metal Calculator

set -e

KERNEL_BIN="Build/isofiles/boot/kernel.elf"
ISO_IMAGE="Build/calculator.iso"

# Check if kernel bin exists
if [ ! -f "${KERNEL_BIN}" ]; then
    echo "Error: ${KERNEL_BIN} not found. Please build the project first."
    exit 1
fi

echo "=== Launching QEMU ==="
echo "Press Ctrl+A then X to exit QEMU monitor."

# Run QEMU with serial/console redirections and debug support enabled
# -s: shorthand for -gdb tcp::1234
# -S: freeze CPU at startup
qemu-system-x86_64 \
    -kernel "${KERNEL_BIN}" \
    -m 512M \
    -no-reboot \
    -no-shutdown \
    -d guest_errors,cpu_reset \
    -s -S
