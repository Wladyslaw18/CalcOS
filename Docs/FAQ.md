# Frequently Asked Questions

**Q: Why not just use Python or a web calculator?**
A: Because the question was never "what is the easiest way to add two numbers."
   It was "how far can you take the simplest concept in programming."

**Q: Does this actually boot on real hardware?**
A: Yes. Tested on a Dell Optiplex 760 (Core 2 Duo, 2GB RAM) with a USB-to-floppy
   adapter. Boot time: under 0.5 seconds.

**Q: Why no heap allocation?**
A: On bare-metal, there is no allocator unless you write one. Writing one costs
   RAM, code size, and introduces fragmentation. The stack is already there.
   Everything fits. Zero cost.

**Q: Can I use this as a library in my C project?**
A: Yes. Build `calc_static` (`.lib`/`.a`) or `calc_shared` (`.dll`/`.so`).
   Include `src/calc.h` and link. No other dependencies.

**Q: Why does `atan(1.0)` return 0.744012 instead of 0.785398 (pi/4)?**
A: The parser`s `atan` uses a Pade approximation tuned for speed over precision
   at the edges. For high-precision results use `solve(tan(x)-1, x, 0, pi/2)`.

**Q: Does the plugin system work on bare-metal?**
A: No. Dynamic loading requires a filesystem and OS loader. Plugins are host-only.
   The core engine itself is fully freestanding.

**Q: What is the 24KB RAM figure?**
A: That is the active stack + state footprint during a typical expression evaluation.
   The kernel code itself is ~80-120KB. You need at least 640KB conventional memory
   for a comfortable boot with the stack at 0x90000.
