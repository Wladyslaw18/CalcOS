## What does this PR do?

## Checklist
- [ ] Compiles with zero warnings on MSVC Release
- [ ] Compiles with zero warnings on GCC -Wall -Wextra
- [ ] `test_parse.exe` passes 100%
- [ ] `test_plugin_system.exe` passes 100%
- [ ] `test_complex_calc.exe` passes 100%
- [ ] No `malloc`/`free` added to core engine
- [ ] No standard library includes in freestanding compilation units
- [ ] `CalculatorState` is still 64 bytes (check `StateSizeCheck.h`)
- [ ] Commit messages follow the convention in `CONTRIBUTING.md`

## Benchmark numbers (if perf change)
Before:
After:
