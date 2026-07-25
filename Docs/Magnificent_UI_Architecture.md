# ARCHITECTURE SPECIFICATION: CALCOS ZERO-LATENCY GRAPHICAL SYSTEM

> **"Built at 3 AM. Zero GC. 120 FPS. Bare-metal hardware performance."**

---

## 1. ARCHITECTURAL VISION & PHILOSOPHY

CalcOS-Graphical is a zero-allocation, lock-free, GPU-accelerated mathematical visualization engine. It bridges a 64-byte cache-aligned C11 kernel with a C# (.NET 10) AVX2 SIMD vectorization pipeline and a hardware-accelerated UI layer.

### Design Principles:
1. **Zero-Copy Memory Pipeline**: SIMD-generated math data streams directly to GPU vertex shaders from shared native memory pointers.
2. **Zero GC Pressure**: Zero heap allocations occur during calculation or rendering frames.
3. **Lock-Free Concurrency**: Producer/consumer communication between the math thread and render thread uses an SPMC atomic ring buffer.
4. **Mechanical Sympathy**: Core data structures enforce 64-byte L1 cache line alignment and 256-bit SIMD alignment.

---

## 2. SYSTEM LAYERING

```
┌────────────────────────────────────────────────────────────────────────┐
│                        LAYER 4: GRAPHICAL UI                           │
│   Avalonia / SkiaSharp / WebGL Shaders (120Hz Dynamic Rendering)       │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │ Direct Native Pointer Access
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                    LAYER 3: ZERO-COPY GPU BRIDGE                       │
│   NativeMemory.AlignedAlloc (64-byte cache-line aligned)               │
│   Vulkan / DirectX 12 / Metal VBO Shared Pointer Mapping              │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │ SIMD Vector Instructions
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                LAYER 2: C# MANAGED MATH ENGINE (.NET 10)               │
│   4x Unrolled AVX2 / Vector512 Vector Math                             │
│   ReadOnlySpan<char> Zero-Alloc RPN Expression Parser                  │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │ C-ABI Native Calls
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│               LAYER 1: BARE-METAL C11 KERNEL / BOOTLOADER              │
│   64-byte Cache-Aligned CalculatorState                                │
│   Direct VGA / VESA LFB Framebuffer Drivers                            │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 3. MEMORY & DATA PIPELINE SPECIFICATION

### 3.1 Cache-Aligned Memory Layout

Core data structures align to 64-byte cache lines to eliminate cache line splits and false sharing across CPU cores:

```csharp
[StructLayout(LayoutKind.Sequential, Pack = 64)]
public unsafe struct RenderPoint
{
    public double X;
    public double Y;
    public double Z;
    public float ColorR;
    public float ColorG;
    public float ColorB;
    public float ColorA;
    private fixed byte Pad[24];
}
```

### 3.2 Lock-Free Atomic Ring Buffer

```csharp
public unsafe struct LockFreeRingBuffer
{
    private const int Capacity = 1024;
    private fixed long _head[8];
    private fixed long _tail[8];
    private RenderPoint* _buffer;

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public bool TryPush(in RenderPoint point)
    {
        long currentTail = Volatile.Read(ref _tail[0]);
        long currentHead = Volatile.Read(ref _head[0]);
        if (currentTail - currentHead >= Capacity) return false;

        _buffer[currentTail % Capacity] = point;
        Volatile.Write(ref _tail[0], currentTail + 1);
        return true;
    }
}
```

---

## 4. GLASSMORPHIC UI & SHADER SPECIFICATION

- **Background**: Obsidian Gray (`#0B0E14`) with a 20px Gaussian Backdrop Blur.
- **Surface**: Semi-transparent glass (`rgba(255, 255, 255, 0.04)`) with a 1px subtle border gradient.
- **Plot Curves**: Glowing neon shaders (Cyan `#00F3FF` for primary function, Magenta `#FF007A` for derivatives).
- **Typography**: Google `Outfit` / `Fira Code` for precision layout.

```glsl
#version 450
in vec2 uv;
out vec4 FragColor;

uniform vec3 u_CurveColor;
uniform float u_GlowIntensity;

void main() {
    float dist = abs(uv.y - sin(uv.x * 10.0));
    float glow = exp(-dist * 40.0) * u_GlowIntensity;
    vec3 col = u_CurveColor * glow;
    FragColor = vec4(col, clamp(glow, 0.0, 1.0));
}
```

---

## 5. PERFORMANCE METRICS TARGETS

| Metric | Target Standard | Sub-System Responsible |
| :--- | :--- | :--- |
| **Expression Evaluation** | $< 0.001 \text{ ms}$ | `InfixToRpn` (Span-based Parsing) |
| **SIMD Point Math Generation** | $< 0.05 \text{ ms}$ (4,096 points) | `AddAvx2Unrolled` / Vector512 |
| **GC Allocations Per Frame** | **0.00 Bytes** | Zero-Allocation Engine Architecture |
| **Frame Render Time** | $8.33 \text{ ms}$ ($120 \text{ FPS}$) | SkiaSharp / Vulkan Shared VBO Pointer |
| **Memory Footprint** | $< 16 \text{ MB}$ Total | Native Heap Allocation Policy |
