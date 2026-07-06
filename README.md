<div align="center">
  <h1>⚜︎ CALC OS</h1>
  <p><i>"Built at 3am. Booted on bare metal. Regretted nothing."</i></p>

  <code>[ Conventional Memory: 640KB ]</code> &nbsp; <code>[ Core Size: 64 Bytes ]</code> &nbsp; <code>[ Target: Freestanding x86_64 ]</code>
</div>

---

##  EMERGENCY DISCLAIMER (READ BEFORE THE LKML FINDS THIS)

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
receipts. We ball. 💀

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
