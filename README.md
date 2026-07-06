<div align="center">
  <h1>⚜︎ CALC OS</h1>
  <p><i>"Built at 3am. Booted on bare metal. Regretted nothing."</i></p>

  <code>[ Conventional Memory: 640KB ]</code> &nbsp; <code>[ Core Size: 64 Bytes ]</code> &nbsp; <code>[ Target: Freestanding x86_64 ]</code>
</div>

---

##  DISCLAIMER (READ BEFORE THE LKML FINDS THIS)

**BEFORE ANYONE FROM `lkml.org` SHOWS UP:** this is not an operating system.
I need that in the H2 header, bolded, italicized, notarized, and possibly
tattooed on my arm before some Geek™ with 40,000 kernel commits opens this
repo and has a medical event.

An OS has a scheduler. Mine has `while(1) { read_keyboard(); }` and the
sheer audacity to call that "the calculator loop." An OS has process
isolation. Mine has one (1) global 64-byte struct and a prayer. An OS has
a filesystem. Mine has a `PluginContext` that namespaces keys like
`plugin:PluginId:Key` for a storage layer that **evaporates the second you
pull the power cord**, because there is no disk driver, because I ran out
of sanity around hour 6.

Calling this an "OS" is like calling a lemonade stand "a Federal Reserve
regional bank." Sure, money changes hands at both. One of them sets
interest rates for a nation. The other one is a folding table and my
cousin's cash box.

**TO THE GEEKS, THE COMMITTEE, THE COUNCIL OF ELDERS, WHOEVER YOU ARE:**
I know. I know I remapped the PIC. I know I queried the memory map with
`INT 15h, E820` like I had ANY business doing that. I know I set `CR0`'s
PE bit and did a far-jump to flush the prefetch queue like a man who has
seen things. None of that makes this Linux. None of that makes this
*anything*. It makes it a calculator wearing a bootloader as a Halloween
costume, and the costume is held on with duct tape and unchecked BIOS
interrupts.

**TO LINUS SPECIFICALLY, IF THIS SOMEHOW REACHES YOU:** sir I am begging
you, do not rebase your entire week onto roasting my `BootEntry.asm`. I
will preemptively `rm -rf` the disk image myself. I will disable A20 out
of shame alone. I will go live in a cabin and do arithmetic with rocks.

Consider this README the wanted poster before you have to draw one
yourselves:

> **WANTED — DEAD OR ALIVE (mostly dead, the code has seen things)**
> **Suspect:** WladyslawKW
> **Charges:** Impersonating an Operating System (1st degree), Reckless
> Endangerment of a 64-Byte Cache Line, Conspiracy to Commit
> `solve(sin(x) - cos(x), x, 0, pi)`, Criminal Overuse of the Word "Kernel"
> **Last known location:** QEMU, `-m 640k`, screaming into `-serial stdio`
> **Reward:** absolutely nothing, this project doesn't even have a
> filesystem to store a bounty in

**What this actually is:** a bare-metal-capable calculator with a
genuinely over-engineered, topologically-sorted plugin system, built
because "write a basic calculator" triggered a full systems-engineering
psychotic episode and the only survivors were a Pratt parser and my
will to live.

**What this is not:** an operating system, a threat to Linux, a threat
to anyone's CS degree, or, in retrospect, a good use of a Tuesday night.

Anyway `solve()` still converges to 9 decimal places so we're keeping the
receipts. We ball. 

---

Everyone starts their coding journey by writing a basic calculator. A couple of variables, a basic parser, and a simple screen output. It's the universal rite of passage. 

Years later, we build complex systems, design operating systems, and compile low-level drivers. But sometimes, you want to go back to where it all began. I wanted to take that simple, nostalgic calculator concept and apply **every single low-level optimization and systems-engineering trick I learned along the way** directly to it.

The result is a unified engine that runs **anywhere**:
*   **100% Freestanding Bare-Metal**: Boots directly on physical x86_64 CPU silicon or QEMU with exactly **24 Kilobytes of RAM** (zero OS, zero libc).
*   **Host OS Native**: Compiles natively as a standard user-space app on Windows, Linux, macOS, WebAssembly (WASM), or ESP32 microcontrollers. 

Same core math engine. Same parser. Zero code duplication. Just swap the driver. ⚜︎ 

---

## ✦﹎ WHY INFLICT THIS ON MYSELF?

- **Zero-Bypass Performance** - No OS. No Windows. No Linux. No libc. The code owns the CPU registers and writes characters directly to the VGA text-mode buffer at `0xB8000`. Ring 0 or bust, baby.
- **Zero-Allocation Policy** - Strictly no `malloc`, no `free`, and no heap. Everything is statically allocated or processed on the stack. If it doesn't fit, it doesn't compile. The heap was never invited to this party.
- **L1 Cache line Sympathy** - The entire global state machine (`CalculatorState`) is compiled to be exactly **64 bytes** and cache-aligned. The CPU loads the whole OS state in a single memory transaction. One (1) cache line to rule them all.
- **Topological Plugin Mesh** - A decoupled C++17 runtime plugin loader that does topological sorting, circular dependency blocking, and Namespaced DB sandboxing without a kernel filesystem. Yes, a filesystem-less DB. No I will not be elaborating on how that survives a reboot (it doesn't).

---

## ☨ THE BARE-METAL ARCHITECTURE (Target B)

When booted on real hardware or emulated in QEMU, the system runs a flat, unsegmented physical memory model:
HARDWARE  →  Bootloader (BootEntry.asm)  →  Protected Mode Switch  →  C-Kernel (0x8000)

The boot sequence runs in **3 phases** to transition the CPU from 16-bit legacy real mode to full execution, which is a very fancy way of saying "we convince a 40-year-old CPU design to trust us":

| Phase | Files | What happens |
|---|---|---|
| **Phase 0** | [BootEntry.asm](Bootloader/Boot/BootEntry.asm) | Disables interrupts. Sets `DS=0`. Queries the physical memory map via BIOS `INT 15h, E820` loops. Enables the A20 Gate. |
| **Phase 1** | [BootEntry.asm](Bootloader/Boot/BootEntry.asm) | Loads 32 sectors of C code from disk to address `0x8000`. Sets the PE bit in `CR0` to switch to 32-bit Protected Mode. Far-jumps to flush the prefetch queue. |
| **Phase 2** | [KernelLoader.c](Bootloader/Loader/KernelLoader.c) | Establishes the Protected Mode stack pointer at `0x90000`. Remaps the PIC to route PS/2 keyboard interrupts, loads the IDT, and jumps to the calculator loop. |

> ⚡︎ **The 24KB Stack Ceiling**: If you emulated this on a strict `-m 24k` QEMU layout, setting the stack to `0x90000` (576KB) would cause a GPF. We stick to `-m 640k` (conventional IBM PC memory) so the calculator has plenty of safety margin to run. "Plenty of safety margin" is doing a LOT of heavy lifting in that sentence.

---

## ⚡︎ THE ENGINE

No standard libraries. No `libm`. Every mathematical operation implemented from scratch, because apparently I have a personal vendetta against `#include <math.h>`:

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
- **BigInt Division**: Knuth's Algorithm D implemented for multi-precision division on raw uint64 arrays without heap allocations. Yes I read the actual TAOCP chapter for a calculator. No I do not accept feedback on this life choice.
- **Memory Safety**: Hardened stack buffer copying loops and double-to-int64 precision checks to ensure NaN expressions or long inputs never trigger a stack smash. This is the one part of the project that's actually responsible. Cherish it.

---

## ⚙︎ THE PLUGIN PROTOCOL *(Don't Touch The Core)*

A modular, decoupled plugin system so external developers can write plugins without compile-time coupling — an unreasonable amount of enterprise architecture for a thing that adds numbers:
PLUGIN DLL  →  ExportAPI()  →  Service Mesh  →  Global Parser Registry

Follow the **Isolation Protocol** to write a plugin:

1. **Self-Describing Dependencies**: Upgraded the `IPlugin` interface to list its dependencies. The manager reads them directly from the plugin instance.
2. **Topological Staggered Loader**: Dynamically resolves and loads plugins in dependency order. Blocks circular dependency loops:
```cpp
   // CoreA depends on nothing, CoreB depends on CoreA
   // Sort resolves order: CoreA -> CoreB
```
3. **Namespaced Sandboxing**: The `PluginContext` prefixes database keys (`plugin:PluginId:Key`) so plugins cannot pollute each other's storage. A DB with governance and zero persistence. It's basically a country with a constitution and no land.
4. **VTable Teardown Protection**: Safe double-unload guards prevent dangling pointer access violations when unmapping DLL libraries from memory. Because "just don't call it after freeing it" was apparently not going to happen on the honor system.

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

These aren't design patterns. They are hard-won lessons written in triple-fault hardware reboots and compiler register spilling:

1. **Zero-Bypass Pointers** - Don't bypass the VGA/serial print loops. Going direct is fun until you hit real hardware and corrupt the screen memory. Ask me how I know. Actually don't, it's a whole story and it ends with a hard reset.
2. **No dynamic casting on unmapped memory** - If you unload a plugin, clear its hooks. Calling a virtual method on a freed DLL is an instant ticket to `0xC0000005` (Access Violation), and also a instant ticket to me screaming into a pillow.
3. **Keep CalculatorState under 64 bytes** - Adding fields is permitted, but you must reduce the `_pad` padding array to keep the struct size exactly at a single cache line. Protect the L1 cache like it's the last slice of pizza.
4. **CANARY VIGILANCE** - The IST stack canary (`0xDEADC0DE`) is active. If the Double Fault handler overflows, the canary gets smashed, and the system prints a warning before halting the CPU. RIP to that canary, it died so the register state could live.

---

**Calc OS: Built because adding two numbers should be a matter of physical copper cycles, not operating system overhead.**

*(Not an OS. Repeat: NOT an OS. Please don't sue me, Torvalds.)*

*- WladyslawKW, somewhere at 3am, probably being hunted*

Signed by Ty Coon, Vice of President (and undisputed President of Vice)
