# CalcOS Physical Memory Map

Layout when booted on a standard 640KB conventional PC:

| Address Range | Size | Contents |
|---|---|---|
| `0x0000 - 0x003FF` | 1 KB | Real Mode IVT (BIOS) |
| `0x0400 - 0x004FF` | 256 B | BIOS Data Area |
| `0x0500 - 0x7BFF` | ~30 KB | Free (used for early stack) |
| `0x7C00 - 0x7DFF` | 512 B | Boot sector loaded by BIOS |
| `0x7E00 - 0x7FFF` | 512 B | Boot sector overflow / stage buffer |
| `0x8000 - 0xFFFF` | 32 KB | CalcOS kernel and engine code |
| `0x10000 - 0x8FFFF` | ~512 KB | Free conventional memory |
| `0x90000 - 0x9FFFF` | 64 KB | Protected Mode stack |
| `0xA0000 - 0xAFFFF` | 64 KB | VGA framebuffer (graphics mode) |
| `0xB8000 - 0xBFFFF` | 32 KB | VGA text buffer (80x25 chars) |
| `0xC0000 - 0xFFFFF` | 256 KB | ROM / BIOS |

The kernel is linked to load at `0x8000`. The bootloader copies 32 disk
sectors (16KB) from the floppy to that address before switching to Protected Mode.
