using System;
using System.Diagnostics;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Intrinsics;
using System.Runtime.Intrinsics.X86;
using System.Runtime.Intrinsics.Arm;

namespace DotNetPerf
{
    // STACK-ALLOCATED, CACHE-ALIGNED MEMORY BLOCK. NO GC HEAP PRESSURE
    [StructLayout(LayoutKind.Sequential, Pack = 64)]
    public unsafe struct CalculatorState
    {
        public fixed double Operands[4]; // 32 bytes
        public byte OpCount;            // 1 byte
        public byte CurrentOp;          // 1 byte
        public byte Flags;              // 1 byte
        public byte Mode;               // 1 byte
        public uint HistoryIdx;         // 4 bytes
        private fixed byte _pad[24];    // 24 bytes - PAD TO 64 BYTES OR DIE
    }

    public static unsafe class Program
    {
        private const int ArraySize = 4096;
        private const int Iterations = 50000;

        public static void Main()
        {
            Console.WriteLine("=== DOTNET 10 UNMANAGED SIMD CALCULATOR ENGINE ===");
            Console.WriteLine($"AVX2 Supported: {Avx2.IsSupported}");
            Console.WriteLine($"SSE2 Supported: {Sse2.IsSupported}");
            Console.WriteLine($"ARM NEON Supported: {AdvSimd.IsSupported}");
            Console.WriteLine($"Is64BitProcess: {Environment.Is64BitProcess}");

            // Statically allocate buffers on the stack or native heap. Zero GC allocation!
            double* inputA = (double*)NativeMemory.AlignedAlloc((nuint)(ArraySize * sizeof(double)), 64);
            double* inputB = (double*)NativeMemory.AlignedAlloc((nuint)(ArraySize * sizeof(double)), 64);
            double* outputRes = (double*)NativeMemory.AlignedAlloc((nuint)(ArraySize * sizeof(double)), 64);

            try
            {
                // Init data
                for (int i = 0; i < ArraySize; i++)
                {
                    inputA[i] = i * 0.1;
                    inputB[i] = i * 0.2;
                }

                // Stack-allocate the CalculatorState
                CalculatorState state = default;

                // Warm-up to JIT compile hot paths
                AddScalar(&state, inputA, inputB, outputRes, ArraySize);
                if (Avx2.IsSupported) AddAvx2(&state, inputA, inputB, outputRes, ArraySize);
                if (Sse2.IsSupported) AddSse2(&state, inputA, inputB, outputRes, ArraySize);
                if (AdvSimd.IsSupported) AddArmNeon(&state, inputA, inputB, outputRes, ArraySize);

                // Benchmark Scalar
                var sw = Stopwatch.StartNew();
                for (int i = 0; i < Iterations; i++)
                {
                    AddScalar(&state, inputA, inputB, outputRes, ArraySize);
                }
                sw.Stop();
                long scalarTicks = sw.ElapsedTicks;
                Console.WriteLine($"\nScalar Fallback Time: {sw.ElapsedMilliseconds} ms ({scalarTicks} ticks)");

                if (Sse2.IsSupported)
                {
                    // Benchmark SSE2
                    sw = Stopwatch.StartNew();
                    for (int i = 0; i < Iterations; i++)
                    {
                        AddSse2(&state, inputA, inputB, outputRes, ArraySize);
                    }
                    sw.Stop();
                    long sseTicks = sw.ElapsedTicks;
                    Console.WriteLine($"SSE2 Vectorized Time: {sw.ElapsedMilliseconds} ms ({sseTicks} ticks) - Speedup: {(double)scalarTicks / sseTicks:F2}x");
                }

                if (Avx2.IsSupported)
                {
                    // Benchmark AVX2
                    sw = Stopwatch.StartNew();
                    for (int i = 0; i < Iterations; i++)
                    {
                        AddAvx2(&state, inputA, inputB, outputRes, ArraySize);
                    }
                    sw.Stop();
                    long avx2Ticks = sw.ElapsedTicks;
                    Console.WriteLine($"AVX2 Vectorized Time: {sw.ElapsedMilliseconds} ms ({avx2Ticks} ticks) - Speedup: {(double)scalarTicks / avx2Ticks:F2}x 🔥");
                }

                if (AdvSimd.IsSupported)
                {
                    // Benchmark ARM NEON
                    sw = Stopwatch.StartNew();
                    for (int i = 0; i < Iterations; i++)
                    {
                        AddArmNeon(&state, inputA, inputB, outputRes, ArraySize);
                    }
                    sw.Stop();
                    long neonTicks = sw.ElapsedTicks;
                    Console.WriteLine($"ARM NEON Vectorized Time: {sw.ElapsedMilliseconds} ms ({neonTicks} ticks) - Speedup: {(double)scalarTicks / neonTicks:F2}x 🔥");
                }
            }
            finally
            {
                // Free native memory. Keep memory usage at flat 0 bytes leaked!
                NativeMemory.AlignedFree(inputA);
                NativeMemory.AlignedFree(inputB);
                NativeMemory.AlignedFree(outputRes);
            }
        }

        // [MethodImpl(MethodImplOptions.AggressiveInlining)] prevents call instruction jump overheads!
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void AddScalar(CalculatorState* state, double* a, double* b, double* result, int count)
        {
            for (int i = 0; i < count; i++)
            {
                result[i] = a[i] + b[i];
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void AddSse2(CalculatorState* state, double* a, double* b, double* result, int count)
        {
            int i = 0;
            // 128-bit vector = 2 doubles at a time
            for (; i + 1 < count; i += 2)
            {
                Vector128<double> va = Vector128.Load(a + i);
                Vector128<double> vb = Vector128.Load(b + i);
                Vector128<double> vr = va + vb;
                vr.Store(result + i);
            }
            // Tail cleanup
            for (; i < count; i++)
            {
                result[i] = a[i] + b[i];
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void AddAvx2(CalculatorState* state, double* a, double* b, double* result, int count)
        {
            int i = 0;
            // 256-bit vector = 4 doubles at a time
            for (; i + 3 < count; i += 4)
            {
                Vector256<double> va = Vector256.Load(a + i);
                Vector256<double> vb = Vector256.Load(b + i);
                Vector256<double> vr = va + vb;
                vr.Store(result + i);
            }
            // SSE Cleanup
            if (i + 1 < count)
            {
                Vector128<double> va = Vector128.Load(a + i);
                Vector128<double> vb = Vector128.Load(b + i);
                Vector128<double> vr = va + vb;
                vr.Store(result + i);
                i += 2;
            }
            // Tail cleanup
            for (; i < count; i++)
            {
                result[i] = a[i] + b[i];
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void AddArmNeon(CalculatorState* state, double* a, double* b, double* result, int count)
        {
            int i = 0;
            // ARM NEON registers are 128-bit = 2 doubles at a time
            for (; i + 1 < count; i += 2)
            {
                Vector128<double> va = Vector128.Load(a + i);
                Vector128<double> vb = Vector128.Load(b + i);
                Vector128<double> vr = va + vb; // Emits NEON instructions on ARM64!
                vr.Store(result + i);
            }
            // Tail cleanup
            for (; i < count; i++)
            {
                result[i] = a[i] + b[i];
            }
        }

        public enum RpnTokenType : byte
        {
            Number,
            Operator
        }

        [StructLayout(LayoutKind.Explicit)]
        public struct RpnTokenData
        {
            [FieldOffset(0)] public double Value;
            [FieldOffset(0)] public char Op;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct RpnToken
        {
            public RpnTokenType Type;
            public RpnTokenData Data;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool InfixToRpn(ReadOnlySpan<char> infix, Span<RpnToken> rpnQueue, out int tokenCount)
        {
            tokenCount = 0;
            Span<char> opStack = stackalloc char[32];
            int opTop = -1;

            for (int i = 0; i < infix.Length; i++)
            {
                char c = infix[i];
                if (c == ' ') continue;

                if (char.IsDigit(c) || c == '.')
                {
                    // Parse number block
                    double val = 0.0;
                    double divisor = 1.0;
                    bool decimalPoint = false;
                    while (i < infix.Length && (char.IsDigit(infix[i]) || infix[i] == '.'))
                    {
                        if (infix[i] == '.') decimalPoint = true;
                        else
                        {
                            if (!decimalPoint) val = val * 10.0 + (infix[i] - '0');
                            else
                            {
                                divisor *= 10.0;
                                val += (infix[i] - '0') / divisor;
                            }
                        }
                        i++;
                    }
                    i--; // Backtrack index

                    if (tokenCount >= rpnQueue.Length) return false;
                    rpnQueue[tokenCount++] = new RpnToken 
                    { 
                        Type = RpnTokenType.Number, 
                        Data = new RpnTokenData { Value = val } 
                    };
                    continue;
                }

                if (c == '(')
                {
                    opStack[++opTop] = '(';
                    continue;
                }

                if (c == ')')
                {
                    while (opTop >= 0 && opStack[opTop] != '(')
                    {
                        rpnQueue[tokenCount++] = new RpnToken 
                        { 
                            Type = RpnTokenType.Operator, 
                            Data = new RpnTokenData { Op = opStack[opTop--] } 
                        };
                    }
                    opTop--; // pop '('
                    continue;
                }

                if (c == '+' || c == '-' || c == '*' || c == '/')
                {
                    int prec = c == '*' || c == '/' ? 2 : 1;
                    while (opTop >= 0 && opStack[opTop] != '(')
                    {
                        char topOp = opStack[opTop];
                        int topPrec = topOp == '*' || topOp == '/' ? 2 : 1;
                        if (topPrec >= prec)
                        {
                            rpnQueue[tokenCount++] = new RpnToken 
                            { 
                                Type = RpnTokenType.Operator, 
                                Data = new RpnTokenData { Op = opStack[opTop--] } 
                            };
                        }
                        else break;
                    }
                    opStack[++opTop] = c;
                }
            }

            while (opTop >= 0)
            {
                rpnQueue[tokenCount++] = new RpnToken 
                { 
                    Type = RpnTokenType.Operator, 
                    Data = new RpnTokenData { Op = opStack[opTop--] } 
                };
            }

            return true;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static double EvaluateRpn(ReadOnlySpan<RpnToken> rpnQueue, int tokenCount)
        {
            Span<double> evalStack = stackalloc double[32];
            int evalTop = -1;

            for (int i = 0; i < tokenCount; i++)
            {
                var token = rpnQueue[i];
                if (token.Type == RpnTokenType.Number)
                {
                    evalStack[++evalTop] = token.Data.Value;
                }
                else
                {
                    double v2 = evalStack[evalTop--];
                    double v1 = evalStack[evalTop--];
                    double res = token.Data.Op switch
                    {
                        '+' => v1 + v2,
                        '-' => v1 - v2,
                        '*' => v1 * v2,
                        '/' => v1 / v2,
                        _ => 0.0
                    };
                    evalStack[++evalTop] = res;
                }
            }

            return evalStack[0];
        }
    }
}
