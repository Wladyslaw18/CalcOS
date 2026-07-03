<div align="center">
  <h1>⚜︎ CALC OS</h1>
  <p><i>"Built at 3am. Booted on bare metal. Regretted nothing."</i></p>

  <code>[ Conventional Memory: 640KB ]</code> &nbsp; <code>[ Core Size: 64 Bytes ]</code> &nbsp; <code>[ Target: Freestanding x86_64 ]</code>
</div>

Everyone starts their coding journey by writing a basic calculator. A couple of variables, a basic parser, and a simple screen output. It's the universal rite of passage. 

Years later, we build complex systems, design operating systems, and compile low-level drivers. But sometimes, you want to go back to where it all began. I wanted to take that simple, nostalgic calculator concept and apply **every single low-level optimization and systems-engineering trick I learned along the way** directly to it.

The result is a unified engine that runs **anywhere**:
*   **100% Freestanding Bare-Metal**: Boots directly on physical x86_64 CPU silicon or QEMU with exactly **24 Kilobytes of RAM** (zero OS, zero libc).
*   **Host OS Native**: Compiles natively as a standard user-space app on Windows, Linux, macOS, WebAssembly (WASM), or ESP32 microcontrollers. 

Same core math engine. Same parser. Zero code duplication. Just swap the driver. ⚜︎ 

---

## ✦&#xFE0E; WHY INFLICT THIS ON MYSELF?

- **Zero-Bypass Performance** - No OS. No Windows. No Linux. No libc. The code owns the CPU registers and writes characters directly to the VGA text-mode buffer at `0xB8000`.
- **Zero-Allocation Policy** - Strictly no `malloc`, no `free`, and no heap. Everything is statically allocated or processed on the stack. If it doesn't fit, it doesn't compile. 
- **L1 Cache line Sympathy** - The entire global state machine (`CalculatorState`) is compiled to be exactly **64 bytes** and cache-aligned. The CPU loads the whole OS state in a single memory transaction.
- **Topological Plugin Mesh** - A decoupled C++17 runtime plugin loader that does topological sorting, circular dependency blocking, and Namespaced DB sandboxing without a kernel filesystem.

---

## ☨ THE BARE-METAL ARCHITECTURE (Target B)

When booted on real hardware or emulated in QEMU, the system runs a flat, unsegmented physical memory model:

```
HARDWARE  →  Bootloader (BootEntry.asm)  →  Protected Mode Switch  →  C-Kernel (0x8000)
```

The boot sequence runs in **3 phases** to transition the CPU from 16-bit legacy real mode to full execution:

| Phase | Files | What happens |
|---|---|---|
| **Phase 0** | [BootEntry.asm](Bootloader/Boot/BootEntry.asm) | Disables interrupts. Sets `DS=0`. Queries the physical memory map via BIOS `INT 15h, E820` loops. Enables the A20 Gate. |
| **Phase 1** | [BootEntry.asm](Bootloader/Boot/BootEntry.asm) | Loads 32 sectors of C code from disk to address `0x8000`. Sets the PE bit in `CR0` to switch to 32-bit Protected Mode. Far-jumps to flush the prefetch queue. |
| **Phase 2** | [KernelLoader.c](Bootloader/Loader/KernelLoader.c) | Establishes the Protected Mode stack pointer at `0x90000`. Remaps the PIC to route PS/2 keyboard interrupts, loads the IDT, and jumps to the calculator loop. |

> ⚡︎ **The 24KB Stack Ceiling**: If you emulated this on a strict `-m 24k` QEMU layout, setting the stack to `0x90000` (576KB) would cause a GPF. We stick to `-m 640k` (conventional IBM PC memory) so the calculator has plenty of safety margin to run.

---

## ⚡︎ THE ENGINE

No standard libraries. No `libm`. Every mathematical operation implemented from scratch:

- **Root Finder**: A built-in `solve()` bisection and Newton-Raphson solver that converges to 9-decimal precision inside the parser:
  ```text
  >> solve(sin(x) - cos(x), x, 0, pi)
  = 0.785398 (exactly π/4)
  ```
- **ODE Solver**: 4th-order Runge-Kutta ODE solver (`rk4()`) executed on a stack-allocated state machine:
  ```text
  >> rk4(x + y, x, y, 0, 1, 10, 1000)
  ```
- **Complex Numbers**: Full imaginary parsing using `i` syntax (e.g., `sqrt(-4) = 2i`), complex multiplication, and complex trigonometry.
- **BigInt Division**: Knuth's Algorithm D implemented for multi-precision division on raw uint64 arrays without heap allocations.
- **Memory Safety**: Hardened stack buffer copying loops and double-to-int64 precision checks to ensure NaN expressions or long inputs never trigger a stack smash.

---

## ⚙︎ THE PLUGIN PROTOCOL *(Don't Touch The Core)*

A modular, decoupled plugin system so external developers can write plugins without compile-time coupling:

```
PLUGIN DLL  →  ExportAPI()  →  Service Mesh  →  Global Parser Registry
```

Follow the **Isolation Protocol** to write a plugin:

1. **Self-Describing Dependencies**: Upgraded the `IPlugin` interface to list its dependencies. The manager reads them directly from the plugin instance.
2. **Topological Staggered Loader**: Dynamically resolves and loads plugins in dependency order. Blocks circular dependency loops:
   ```cpp
   // CoreA depends on nothing, CoreB depends on CoreA
   // Sort resolves order: CoreA -> CoreB
   ```
3. **Namespaced Sandboxing**: The `PluginContext` prefixes database keys (`plugin:PluginId:Key`) so plugins cannot pollute each other's storage.
4. **VTable Teardown Protection**: Safe double-unload guards prevent dangling pointer access violations when unmapping DLL libraries from memory.

---

## ☿ HOW TO RUN THE CHAOS

### 1. Compile the Simulator (Target A)
Compile the engine wrapped in a desktop terminal/GUI driver for rapid testing:
```powershell
# Configures CMake and compiles all test targets in Release mode
cmake -B build
cmake --build build --config Release
```

### 2. Run the Test Suite
Execute the unit tests, Pratt parser stress checks (250-level nested parentheses), and plugin service mesh tests:
```powershell
.\build\bin\Release\test_parse.exe
.\build\bin\Release\test_plugin_system.exe
```

### 3. Emulate on Bare-Metal (Target B)
Assemble the boot sector using NASM and boot the raw disk image under QEMU:
```powershell
# Assemble BootEntry
& "C:\Program Files\NASM\nasm.exe" -f bin Bootloader/Boot/BootEntry.asm -o build/boot.bin

# Run in QEMU with serial COM1 redirected to host console
& "C:\Program Files\qemu\qemu-system-x86_64.exe" -cpu 486 -m 640k -drive format=raw,file=build/boot.bin -nographic -serial stdio
```

---

## ☲ THE RULES OF THE FORGE

These aren't design patterns. They are hard-won lessons written in triple-fault hardware reboots and compiler register spilling.

1. **Zero-Bypass Pointers** - Don't bypass the VGA/serial print loops. Going direct is fun until you hit real hardware and corrupt the screen memory. 
2. **No dynamic casting on unmapped memory** - If you unload a plugin, clear its hooks. Calling a virtual method on a freed DLL is an instant ticket to `0xC0000005` (Access Violation).
3. **Keep CalculatorState under 64 bytes** - Adding fields is permitted, but you must reduce the `_pad` padding array to keep the struct size exactly at a single cache line. Protect the L1 cache.
4. **CANARY VIGILANCE** - The IST stack canary (`0xDEADC0DE`) is active. If the Double Fault handler overflows, the canary gets smashed, and the system prints a warning before halting the CPU.

---

**Calc OS: Built because adding two numbers should be a matter of physical copper cycles, not operating system overhead.**

*- WladyslawKW, somewhere at 3am, probably*
