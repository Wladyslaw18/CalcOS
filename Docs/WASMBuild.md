# WebAssembly Build (Emscripten)

The CalcOS engine compiles to WASM via Emscripten, exposing the full
Pratt parser and math engine to the browser with zero runtime dependencies.

## Prerequisites

- Emscripten SDK (`emsdk`) installed and activated
- CMake >= 3.16

## Build

```bash
source /path/to/emsdk/emsdk_env.sh
emcmake cmake -B build_wasm -DCMAKE_BUILD_TYPE=Release
cmake --build build_wasm
```

Output files in `build_wasm/`:
- `calc_web.js` - Emscripten loader
- `calc_web.wasm` - compiled engine
- `calc_web.html` - minimal test harness

## Calling from JavaScript

```javascript
const calc = await CalcModule();
const result = calc.ccall('calc_evaluate_str', 'number', ['string'], ['sin(pi/4)']);
console.log(result); // 0.7071...
```

## Limitations

- Plugin system is disabled in WASM builds (no `dlopen`)
- Thread-local storage uses `_Thread_local` (supported in Clang/Emscripten)
- SIMD: Emscripten targets WASM SIMD128 (maps SSE2 intrinsics)
