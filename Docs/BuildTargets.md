# Build Targets

CalcOS compiles to six distinct deployment targets from the same core source tree.

| Target | Command | Output | Notes |
|---|---|---|---|
| **Host Terminal** | `cmake --build build --target calc_terminal` | `calc_terminal.exe` | ANSI escape sequences, Windows/Linux |
| **Host SDL2** | `cmake --build build --target calc_app` | `calc_app.exe` | Hardware-accelerated GUI |
| **Static Library** | `cmake --build build --target calc_static` | `calc.lib` | Link into your own project |
| **Shared Library** | `cmake --build build --target calc_shared` | `calc.dll` | Plugin hosts |
| **WebAssembly** | `emcmake cmake ... && cmake --build` | `calc_web.wasm` | Browser deployment |
| **Bare-Metal ISO** | `cmake --build build --target kernel_iso` | `calculator.iso` | Boot from QEMU or physical disk |
| **ESP32 Firmware** | `idf.py build` | `firmware.bin` | Flash to ESP32-S3 |
| **Unikernel** | `cmake --build build --target unikernel_build` | `solo5.exe` | Solo5 MirageOS |

All targets share `Kernel/`, `Application/Input/`, `Application/Output/`, and `src/`.
Only the `Platform/` driver layer changes between them.
