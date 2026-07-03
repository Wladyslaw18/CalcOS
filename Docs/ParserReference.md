# Parser Reference

CalcOS uses a hand-rolled single-pass Pratt parser. All expressions are
evaluated left-to-right with standard mathematical precedence.

## Operator Precedence (high to low)

| Precedence | Operators | Associativity |
|---|---|---|
| 7 | `!` (factorial postfix), `v` | Right |
| 6 | `^` (power) | Right |
| 5 | Unary `-`, `+` | Right |
| 4 | `*`, `/`, implicit mul | Left |
| 3 | `+`, `-` | Left |
| 2 | `=` (assignment) | Right |

## Built-in Functions

| Function | Description | Example |
|---|---|---|
| `sin(x)` | Sine (radians) | `sin(pi/2)` = 1 |
| `cos(x)` | Cosine | `cos(0)` = 1 |
| `tan(x)` | Tangent | `tan(pi/4)` = 1 |
| `asin(x)` | Arc sine | `asin(1)` = p/2 |
| `acos(x)` | Arc cosine | |
| `atan(x)` | Arc tangent | |
| `ln(x)` | Natural log | `ln(e)` = 1 |
| `log10(x)` | Log base 10 | `log10(100)` = 2 |
| `sqrt(x)` | Square root | `sqrt(-4)` = 2i |
| `fact(n)` | Factorial | `fact(5)` = 120 |
| `abs(x)` | Absolute value | |
| `rand()` | Uniform [0,1) via xoshiro256++ | |
| `randn()` | Normal distribution | |
| `mean(...)` | Arithmetic mean | `mean(1,2,3)` = 2 |
| `median(...)` | Median | |
| `var(...)` | Variance | |
| `solve(expr, var, lo, hi)` | Bisection root finder | `solve(x^2-4,x,0,5)` = 2 |
| `rk4(expr, x, y, x0, y0, x1, n)` | Runge-Kutta ODE | |

## Built-in Constants

| Symbol | Value |
|---|---|
| `pi` | 3.14159265358979... |
| `e` | 2.71828182845904... |
| `phi` (golden ratio) | 1.61803398874989... |
| `Ans` | Result of last expression |

## Variables

Assign with `=`. Variables persist across expressions in the same session.
```
x = 5
x^2 + 2*x + 1   -> 36
```

Complex number literals: append `i` to a number: `3 + 4i`, `2i`, `-1i`.
