# Plugin Examples

## 1. Unit Conversion Plugin

Extends the parser with `to_km()`, `to_miles()`, and `to_celsius()` functions.

```cpp
static double plugin_to_km(const double* args, int argc, void*) {
    return argc >= 1 ? args[0] * 1.60934 : 0.0;
}
static double plugin_to_miles(const double* args, int argc, void*) {
    return argc >= 1 ? args[0] / 1.60934 : 0.0;
}
static double plugin_to_celsius(const double* args, int argc, void*) {
    return argc >= 1 ? (args[0] - 32.0) * 5.0 / 9.0 : 0.0;
}

void OnLoad(PluginContext* ctx) override {
    ctx->RegisterFunction("to_km",      plugin_to_km);
    ctx->RegisterFunction("to_miles",   plugin_to_miles);
    ctx->RegisterFunction("to_celsius", plugin_to_celsius);
}
```

Usage in the calculator:
```
to_km(26.2)         // marathon in km = 42.195
to_celsius(98.6)    // body temperature = 37.0
```

## 2. Financial Plugin

Adds `compound(principal, rate, years)` and `pv(fv, rate, years)`.

```cpp
static double plugin_compound(const double* args, int argc, void*) {
    if (argc < 3) return 0.0;
    double p = args[0], r = args[1], n = args[2];
    /* A = P(1 + r)^n */
    double result = p;
    for (int i = 0; i < (int)n; i++) result *= (1.0 + r);
    return result;
}
```

Usage:
```
compound(1000, 0.07, 10)   // $1000 at 7% for 10 years = $1967.15
```

## 3. Matrix Trace Plugin

Adds `mtrace(a11,a12,a21,a22)` for 2x2 matrix trace.

```cpp
static double plugin_mtrace(const double* args, int argc, void*) {
    if (argc < 4) return 0.0;
    return args[0] + args[3]; /* a11 + a22 */
}
```
