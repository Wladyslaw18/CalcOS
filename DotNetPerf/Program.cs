using System;
using System.Diagnostics;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Intrinsics;
using System.Runtime.Intrinsics.X86;
using System.Runtime.Intrinsics.Arm;
using System.Threading;

namespace DotNetPerf
{
    // 64-byte cache-aligned state matching core C kernel memory layout
    [StructLayout(LayoutKind.Sequential, Pack = 64)]
    public unsafe struct CalculatorState
    {
        public fixed double Operands[4];
        public byte OpCount;
        public byte CurrentOp;
        public byte Flags;
        public byte Mode;
        public uint HistoryIdx;
        private fixed byte _pad[24];
    }

    // 64-byte render vertex point compatible with GPU VBO layout
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
        private fixed byte _pad[24];
    }

    // Lock-free atomic ring buffer with 64-byte padded head/tail pointers to prevent false sharing
    public unsafe struct LockFreeRingBuffer
    {
        public const int Capacity = 1024;

        private fixed long _head[8];
        private fixed long _tail[8];
        private RenderPoint* _buffer;

        public static LockFreeRingBuffer Create()
        {
            LockFreeRingBuffer q = default;
            q._buffer = (RenderPoint*)NativeMemory.AlignedAlloc((nuint)(Capacity * sizeof(RenderPoint)), 64);
            Unsafe.InitBlock(q._buffer, 0, (uint)(Capacity * sizeof(RenderPoint)));
            return q;
        }

        public void Free()
        {
            if (_buffer != null)
            {
                NativeMemory.AlignedFree(_buffer);
                _buffer = null;
            }
        }

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

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool TryPop(out RenderPoint point)
        {
            long currentHead = Volatile.Read(ref _head[0]);
            long currentTail = Volatile.Read(ref _tail[0]);
            if (currentHead >= currentTail)
            {
                point = default;
                return false;
            }

            point = _buffer[currentHead % Capacity];
            Volatile.Write(ref _head[0], currentHead + 1);
            return true;
        }
    }

    public static unsafe class Program
    {
        private const int ArraySize = 4096;
        private const int Iterations = 50000;

        public static void Main()
        {
            Console.WriteLine("=== SIMD ENGINE & BENCHMARK HARNESS ===");
            Console.WriteLine($"Vector512 Supported : {Vector512.IsHardwareAccelerated}");
            Console.WriteLine($"AVX2 Supported      : {Avx2.IsSupported}");
            Console.WriteLine($"SSE2 Supported      : {Sse2.IsSupported}");
            Console.WriteLine($"ARM NEON Supported  : {AdvSimd.IsSupported}");
            Console.WriteLine($"Is64BitProcess      : {Environment.Is64BitProcess}");

            double* inputA = (double*)NativeMemory.AlignedAlloc((nuint)(ArraySize * sizeof(double)), 64);
            double* inputB = (double*)NativeMemory.AlignedAlloc((nuint)(ArraySize * sizeof(double)), 64);
            double* outputRes = (double*)NativeMemory.AlignedAlloc((nuint)(ArraySize * sizeof(double)), 64);

            LockFreeRingBuffer ringBuffer = LockFreeRingBuffer.Create();

            try
            {
                for (int i = 0; i < ArraySize; i++)
                {
                    inputA[i] = i * 0.1;
                    inputB[i] = i * 0.2;
                }

                CalculatorState state = default;
                state.Mode = 1;

                AddScalar(inputA, inputB, outputRes, ArraySize);
                if (Sse2.IsSupported) AddSse2(inputA, inputB, outputRes, ArraySize);
                if (Avx2.IsSupported) AddAvx2Unrolled(inputA, inputB, outputRes, ArraySize);
                if (AdvSimd.IsSupported) AddArmNeon(inputA, inputB, outputRes, ArraySize);

                var sw = Stopwatch.StartNew();
                for (int i = 0; i < Iterations; i++)
                {
                    AddScalar(inputA, inputB, outputRes, ArraySize);
                }
                sw.Stop();
                long scalarTicks = sw.ElapsedTicks;
                Console.WriteLine($"\nScalar Fallback Time       : {sw.ElapsedMilliseconds} ms ({scalarTicks} ticks)");

                if (Sse2.IsSupported)
                {
                    sw = Stopwatch.StartNew();
                    for (int i = 0; i < Iterations; i++)
                    {
                        AddSse2(inputA, inputB, outputRes, ArraySize);
                    }
                    sw.Stop();
                    long sseTicks = sw.ElapsedTicks;
                    Console.WriteLine($"SSE2 Vectorized Time       : {sw.ElapsedMilliseconds} ms ({sseTicks} ticks) - Speedup: {(double)scalarTicks / sseTicks:F2}x");
                }

                if (Avx2.IsSupported)
                {
                    sw = Stopwatch.StartNew();
                    for (int i = 0; i < Iterations; i++)
                    {
                        AddAvx2Unrolled(inputA, inputB, outputRes, ArraySize);
                    }
                    sw.Stop();
                    long avx2Ticks = sw.ElapsedTicks;
                    Console.WriteLine($"AVX2 Unrolled (4x) Time     : {sw.ElapsedMilliseconds} ms ({avx2Ticks} ticks) - Speedup: {(double)scalarTicks / avx2Ticks:F2}x");
                }

                if (AdvSimd.IsSupported)
                {
                    sw = Stopwatch.StartNew();
                    for (int i = 0; i < Iterations; i++)
                    {
                        AddArmNeon(inputA, inputB, outputRes, ArraySize);
                    }
                    sw.Stop();
                    long neonTicks = sw.ElapsedTicks;
                    Console.WriteLine($"ARM NEON Vectorized Time    : {sw.ElapsedMilliseconds} ms ({neonTicks} ticks) - Speedup: {(double)scalarTicks / neonTicks:F2}x");
                }

                Console.WriteLine("\n=== ZERO-ALLOCATION RPN EXPRESSION ENGINE ===");
                ReadOnlySpan<char> expr = "(3 + 5) * (10 - 2) / 4.0";
                Span<RpnToken> rpnQueue = stackalloc RpnToken[32];

                sw = Stopwatch.StartNew();
                double exprResult = 0.0;
                for (int i = 0; i < Iterations; i++)
                {
                    if (InfixToRpn(expr, rpnQueue, out int tokenCount))
                    {
                        exprResult = EvaluateRpn(rpnQueue, tokenCount);
                    }
                }
                sw.Stop();
                Console.WriteLine($"Expression: \"{expr.ToString()}\" => Result: {exprResult}");
                Console.WriteLine($"RPN Parse + Eval ({Iterations} runs): {sw.ElapsedMilliseconds} ms ({sw.ElapsedTicks} ticks)");

                Console.WriteLine("\n=== LOCK-FREE RING BUFFER PIPELINE STREAM ===");
                sw = Stopwatch.StartNew();
                int pushedCount = 0;
                for (int i = 0; i < LockFreeRingBuffer.Capacity; i++)
                {
                    RenderPoint pt = new RenderPoint { X = i * 0.01, Y = outputRes[i], Z = 0.0, ColorR = 0.0f, ColorG = 0.95f, ColorB = 1.0f, ColorA = 1.0f };
                    if (ringBuffer.TryPush(in pt)) pushedCount++;
                }
                sw.Stop();
                Console.WriteLine($"Lock-Free Atomic Push ({pushedCount} points) : {sw.ElapsedMilliseconds} ms ({sw.ElapsedTicks} ticks)");
            }
            finally
            {
                ringBuffer.Free();
                NativeMemory.AlignedFree(inputA);
                NativeMemory.AlignedFree(inputB);
                NativeMemory.AlignedFree(outputRes);
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void AddScalar(double* a, double* b, double* result, int count)
        {
            for (int i = 0; i < count; i++)
            {
                result[i] = a[i] + b[i];
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void AddSse2(double* a, double* b, double* result, int count)
        {
            int i = 0;
            for (; i + 1 < count; i += 2)
            {
                Vector128<double> va = Vector128.Load(a + i);
                Vector128<double> vb = Vector128.Load(b + i);
                Vector128<double> vr = va + vb;
                vr.Store(result + i);
            }
            for (; i < count; i++)
            {
                result[i] = a[i] + b[i];
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void AddAvx2Unrolled(double* a, double* b, double* result, int count)
        {
            int i = 0;
            for (; i + 15 < count; i += 16)
            {
                Vector256<double> va0 = Vector256.Load(a + i);
                Vector256<double> vb0 = Vector256.Load(b + i);
                Vector256<double> va1 = Vector256.Load(a + i + 4);
                Vector256<double> vb1 = Vector256.Load(b + i + 4);
                Vector256<double> va2 = Vector256.Load(a + i + 8);
                Vector256<double> vb2 = Vector256.Load(b + i + 8);
                Vector256<double> va3 = Vector256.Load(a + i + 12);
                Vector256<double> vb3 = Vector256.Load(b + i + 12);

                Vector256.Store(va0 + vb0, result + i);
                Vector256.Store(va1 + vb1, result + i + 4);
                Vector256.Store(va2 + vb2, result + i + 8);
                Vector256.Store(va3 + vb3, result + i + 12);
            }

            for (; i + 3 < count; i += 4)
            {
                Vector256<double> va = Vector256.Load(a + i);
                Vector256<double> vb = Vector256.Load(b + i);
                Vector256.Store(va + vb, result + i);
            }

            for (; i < count; i++)
            {
                result[i] = a[i] + b[i];
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static void AddArmNeon(double* a, double* b, double* result, int count)
        {
            int i = 0;
            for (; i + 1 < count; i += 2)
            {
                Vector128<double> va = Vector128.Load(a + i);
                Vector128<double> vb = Vector128.Load(b + i);
                Vector128<double> vr = va + vb;
                vr.Store(result + i);
            }
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

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        public struct RpnToken
        {
            public RpnTokenData Data;
            public RpnTokenType Type;
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
                    int start = i;
                    while (i < infix.Length && (char.IsDigit(infix[i]) || infix[i] == '.'))
                    {
                        i++;
                    }
                    int length = i - start;
                    i--;

                    if (!double.TryParse(infix.Slice(start, length), out double val)) return false;

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
                    if (opTop + 1 >= opStack.Length) return false;
                    opStack[++opTop] = '(';
                    continue;
                }

                if (c == ')')
                {
                    while (opTop >= 0 && opStack[opTop] != '(')
                    {
                        if (tokenCount >= rpnQueue.Length) return false;
                        rpnQueue[tokenCount++] = new RpnToken 
                        { 
                            Type = RpnTokenType.Operator, 
                            Data = new RpnTokenData { Op = opStack[opTop--] } 
                        };
                    }
                    if (opTop < 0) return false;
                    opTop--;
                    continue;
                }

                if (c == '+' || c == '-' || c == '*' || c == '/')
                {
                    int prec = (c == '*' || c == '/') ? 2 : 1;
                    while (opTop >= 0 && opStack[opTop] != '(')
                    {
                        char topOp = opStack[opTop];
                        int topPrec = (topOp == '*' || topOp == '/') ? 2 : 1;
                        if (topPrec >= prec)
                        {
                            if (tokenCount >= rpnQueue.Length) return false;
                            rpnQueue[tokenCount++] = new RpnToken 
                            { 
                                Type = RpnTokenType.Operator, 
                                Data = new RpnTokenData { Op = opStack[opTop--] } 
                            };
                        }
                        else break;
                    }
                    if (opTop + 1 >= opStack.Length) return false;
                    opStack[++opTop] = c;
                }
            }

            while (opTop >= 0)
            {
                if (opStack[opTop] == '(') return false;
                if (tokenCount >= rpnQueue.Length) return false;
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
                ref readonly var token = ref rpnQueue[i];
                if (token.Type == RpnTokenType.Number)
                {
                    if (evalTop + 1 >= evalStack.Length) return double.NaN;
                    evalStack[++evalTop] = token.Data.Value;
                }
                else
                {
                    if (evalTop < 1) return double.NaN;
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

            return evalTop == 0 ? evalStack[0] : double.NaN;
        }
    }
}
