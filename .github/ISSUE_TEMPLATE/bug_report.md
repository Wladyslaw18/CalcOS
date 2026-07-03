---
name: Bug Report
about: Parser crash, wrong result, or boot failure
title: "[BUG] "
labels: bug
---

**Describe the bug**
A clear description of what went wrong.

**Expression / Input**
If a parser bug, paste the exact expression:
```
solve(sin(x) - cos(x), x, 0, pi)
```

**Expected result**

**Actual result**

**Target**
- [ ] Host terminal (`calc_terminal`)
- [ ] Bare-metal / QEMU
- [ ] ESP32
- [ ] WASM

**CPU / OS**
- OS: Windows 11 / Ubuntu 24.04 / bare-metal x86_64
- CPU: Intel Core i7 / AMD Ryzen / QEMU `-cpu 486`

**Additional context**
