## Security Policy

### Supported Versions

CalcOS is a research / hobby bare-metal OS project. There is one active branch:

| Version | Supported |
| --- | --- |
| `main` | Yes |

### Reporting a Vulnerability

If you find a security issue (buffer overflow, stack smash, parser escape, plugin
sandbox bypass), open a **private GitHub Security Advisory** instead of a public
issue.

What qualifies:
- Stack buffer overflows in the expression parser (`Parser.c`)
- Plugin sandbox escapes (cross-namespace DB reads via `PluginContext`)
- Double-free or use-after-free in the plugin DLL loader lifecycle
- Bootloader memory corruption (pre-protected-mode segment register bugs)

What does NOT qualify:
- Intentional behavior (e.g. triple fault on corrupt IDT - that is the reset mechanism)
- Performance regressions
- Feature requests

Response time: best-effort. This is a one-person project.
