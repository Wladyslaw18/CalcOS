# QEMU Testing Guide

This document covers running CalcOS under QEMU for both rapid iteration and
regression testing. All commands assume you have QEMU installed and NASM
available on your path.

---

## Prerequisites

- QEMU `qemu-system-x86_64` (any version >= 5.0)
- NASM >= 2.14
- CMake >= 3.16 + MSVC / GCC toolchain for the host test suite

---

## 1. Assemble the Boot Sector

```powershell
# Windows (NASM installed to default path)
& "C:\Program Files\NASM\nasm.exe" -f bin Bootloader/Boot/BootEntry.asm -o build/boot.bin
```

```bash
# Linux / WSL
nasm -f bin Bootloader/Boot/BootEntry.asm -o build/boot.bin
```

The output is a flat 512-byte binary with the `0x55AA` boot signature at offset 510.

---

## 2. Boot Headless (Serial Only)

No window, COM1 redirected to host stdout. Fastest iteration loop.

```powershell
qemu-system-x86_64 -cpu 486 -m 640k `
  -drive format=raw,file=build/boot.bin `
  -nographic -serial stdio
```

Expected serial output:
```
CalcOS boot OK
DS=0x0000 CS=0x0008 SS=0x0010
IDT loaded. VGA mapped at 0xB8000.
```

---

## 3. Boot with VGA Window

Launches a 640x480 VGA window for visual validation of the text-mode UI.

```powershell
qemu-system-x86_64 -cpu 486 -m 640k `
  -drive format=raw,file=build/boot.bin `
  -serial stdio
```

---

## 4. Boot with GDB Attached

Halts at boot entry and waits for a GDB connection on port 1234.

```powershell
qemu-system-x86_64 -cpu 486 -m 640k `
  -drive format=raw,file=build/boot.bin `
  -nographic -serial stdio `
  -S -gdb tcp::1234
```

In a second terminal:
```bash
gdb -x Tools/Debug/GDBInit.gdb
```

---

## 5. Memory Stress Configurations

| RAM Flag | Conventional equivalent | Notes |
|---|---|---|
| `-m 640k` | IBM PC 5150 max | Recommended. Stack at `0x90000`. |
| `-m 256k` | Early XT machines | Stack must be moved to `0x3F000`. |
| `-m 1m` | 386 baseline | Full extended memory available. |

Do not run with `-m 24k`. The BIOS loads the boot sector to `0x7C00` (31KB).
A 24KB ceiling means the bootloader address is in non-existent memory.

---

## 6. CPU Emulation Targets

| Flag | Era | Notes |
|---|---|---|
| `-cpu 486` | 1989-1993 | No SSE. x87 FPU only. AVX paths disabled. |
| `-cpu pentium` | 1993-1997 | No SSE. Slightly faster pipeline. |
| `-cpu pentium2` | 1997-1999 | MMX available. No SSE. |
| `-cpu qemu64` | Modern x86_64 | Full SSE4/AVX2 paths active. |

The engine auto-detects capabilities at boot via `CPUID` and selects the
appropriate SIMD path. Running `-cpu 486` will silently fall back to scalar math.
