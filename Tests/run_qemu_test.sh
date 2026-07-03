#!/bin/bash
# QEMU Bare-Metal 16KB RAM Test Script
# Runs the freestanding binary with exactly 16KB of RAM

echo "=== Running QEMU Bare-Metal 16KB Hunger Games ==="
qemu-system-x86_64 -m 16k -kernel build/bin/calc.bin -nographic -serial stdio
