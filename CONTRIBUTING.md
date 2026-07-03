## Contributing

This is a solo research project but contributions that align with the core philosophy
are welcome.

### The Core Philosophy

The entire project is built around one constraint: **going back to the roots of what
a calculator is** and applying every systems-engineering concept along the way.
Every line of code must justify its existence in that context.

### Before You Open a PR

1. **Read the Rules of the Forge** in `README.md`. Seriously.
2. Make sure your change compiles with zero warnings under both MSVC and GCC.
3. Run the full test suite:
   ```powershell
   cmake --build build --config Release
   .\build\bin\Release\test_parse.exe
   .\build\bin\Release\test_plugin_system.exe
   .\build\bin\Release\test_complex_calc.exe
   ```
4. If you touch `CalculatorState`, verify it is still exactly 64 bytes after your
   change. Add a `static_assert` if it isn't already there.

### What Will Be Accepted

- Bug fixes with a regression test
- Performance improvements with benchmark numbers (before/after)
- New plugins in `Tests/Unit/` that demonstrate a new use case
- Documentation improvements
- New QEMU/platform targets

### What Will Not Be Accepted

- Heap allocations (`malloc`/`free`) in the core engine
- Standard library (`<stdio.h>`, `<string.h>`, `<math.h>`) includes in freestanding
  compilation units
- Dependencies on third-party libraries
- Breaking the zero-allocation policy

### Commit Message Convention

```
type: short description (max 72 chars)
```

Types: `feat`, `fix`, `perf`, `refactor`, `docs`, `test`, `chore`, `boot`, `kernel`,
`platform`, `plugin`, `infra`

Examples:
```
perf: unroll AVX2 tail loop in add_avx2 for odd-count arrays
fix: clamp expr_str copy_len to PARSER_EXPR_STR_SIZE - 1
docs: add QEMU memory stress configuration table
plugin: add example matrix determinant plugin
```
