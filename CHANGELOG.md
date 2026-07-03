# Changelog

All notable changes to CalcOS are documented here.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

---

## [Unreleased]

### Added
- GitHub Actions CI pipeline: MSVC Windows build, GCC Linux build, NASM boot signature verification
- QEMU testing guide covering headless boot, GDB attach, and CPU era emulation targets
- Plugin development guide with full `PluginContext` API reference
- Security policy covering parser, plugin sandbox, and bootloader attack surfaces
- Contributing guide with commit convention and zero-allocation policy enforcement

---

## [0.1.0] - 2026-07-03

### Added
- Freestanding x86_64 bootloader: 16-bit real mode -> A20 gate -> 32-bit protected mode
- Custom IDT with Double Fault (#DF) IST stack and active canary verification (`0xDEADC0DE`)
- Single-pass Pratt parser with operator precedence, implicit multiplication, and FNV-1a variable hashing
- `solve()` bisection + Newton-Raphson root finder converging to 9-decimal precision
- `rk4()` 4th-order Runge-Kutta ODE solver on stack-allocated state machine
- Complex number support: full imaginary `i` syntax, complex trig, `sqrt(-n)`
- BigInt multi-precision arithmetic via Knuth's Algorithm D (zero heap)
- SIMD-accelerated arithmetic: AVX2 / SSE2 / ARM NEON / scalar fallback paths
- NaN-safe quicksort in statistics module (NaN partitioning fix)
- Thread-local variable symbol table (`THREAD_LOCAL`) for WASM/multi-instance isolation
- Stack buffer overflow guards on `expr_str` copy loops in `solve()` and `rk4()` blocks
- C++17 topological plugin loader: circular dependency detection, namespaced DB sandboxing
- Dynamic DLL/SO file loader with double-unload lifecycle guards
- Custom function registration from plugins into the Pratt parser at runtime
- Host OS target: SDL2 GUI + ANSI terminal drivers sharing the same core engine
- ESP32-S3 target: SSD1306 OLED I2C display driver
- WebAssembly target via Emscripten
- Solo5 unikernel target
- Raspberry Pi framebuffer driver
- VGA text-mode driver (direct `0xB8000` writes, 80x25 characters)
- PS/2 keyboard interrupt handler
- Serial COM1 UART driver (used for QEMU headless output)
- x87 FPU fallback math for pre-SSE CPUs (486/Pentium era)
- Full unit test suite: parser stress (250-level nesting), complex math, plugin mesh, SIMD correctness
- Performance benchmarks: AVX2 ~16 GFlops, SSE ~10 GFlops, scalar ~5 GFlops at 3GHz

### Fixed
- `local_floor`/`local_ceil` undefined behavior for doubles >= 2^53 (int64 cast guard)
- Complex division zero-check aligned to sum-of-squares denominator (`denom == 0.0`)
- `__builtin_memcpy` replaced with freestanding unrolled SSE/AVX/64-bit copy to prevent libc linkage
- `cli` guard added before zero-limit IDT load in triple-fault reset path
