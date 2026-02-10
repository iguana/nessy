# NESsy

NES game development toolchain and build system. Produces valid iNES ROMs from C and 6502 assembly using the cc65 toolchain.

## Quick Start

```bash
make        # builds build/nessy.nes (installs cc65 via Homebrew if missing)
make run    # builds and opens ROM in default emulator
make clean  # removes build artifacts
```

## Prerequisites

- **cc65** — C compiler, assembler, and linker for 6502 (`brew install cc65`)
- **Python 3** — generates CHR font data
- **NES emulator** — [Mesen](https://www.mesen.ca/) or [FCEUX](https://fceux.com/) recommended

## Project Structure

```
nessy/
├── Makefile               Build orchestration + cc65 auto-install
├── cfg/
│   └── nes.cfg            ld65 linker config (NROM mapper 0)
├── src/
│   ├── crt0.s             Startup asm: iNES header, reset/NMI/IRQ, vectors
│   ├── neslib.h           C API: PPU, palette, VRAM, controller functions
│   ├── neslib.s           Assembly implementation of neslib (cc65 fastcall)
│   └── main.c             Hello world: palette + text on screen
├── chr/
│   └── ascii.chr          Generated 8KB font (NES 2bpp planar format)
├── tools/
│   └── chr_gen.py         Generates ascii.chr with 96 printable ASCII glyphs
└── build/
    └── nessy.nes          Output ROM (24,592 bytes)
```

## Architecture

**Target**: NROM-128 (mapper 0) — 16KB PRG-ROM + 8KB CHR-ROM

**Build flow**: `cc65` (.c → .s) → `ca65` (.s → .o) → `ld65` (.o + none.lib → .nes)

**Memory map**:
| Region | Address | Size | Purpose |
|--------|---------|------|---------|
| Zero Page | $0010-$00FF | 240 B | Fast variables |
| OAM Buffer | $0200-$02FF | 256 B | Sprite data (DMA source) |
| RAM | $0300-$07FF | 1.25 KB | General purpose |
| PRG-ROM | $C000-$FFFF | 16 KB | Code + data |
| CHR-ROM | PPU $0000-$1FFF | 8 KB | Tile graphics |

**NMI handler** runs every vblank: OAM DMA, palette upload, scroll/PPU register apply.

## Emulator Testing

Open `build/nessy.nes` in Mesen or FCEUX. You should see three lines of white text on a black background:
- NESSY
- NES BUILD CHAIN ACTIVE
- HELLO FROM CC65!
