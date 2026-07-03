# QEMU Bare-Metal 16KB RAM Test Script for PowerShell
# Runs the freestanding binary with exactly 16KB of RAM

Write-Host "=== Running QEMU Bare-Metal 16KB Hunger Games ===" -ForegroundColor Cyan
qemu-system-x86_64 -m 16k -kernel build/bin/calc.bin -nographic -serial stdio
