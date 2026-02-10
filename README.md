# NESsy

A Tetris game for the NES, built from scratch in C and 6502 assembly using the cc65 toolchain.

## Quick Start

```bash
make        # builds build/nessy.nes (installs cc65 via Homebrew if missing)
make run    # builds and opens ROM in default emulator
make clean  # removes build artifacts
```

## Prerequisites

- **cc65** — C compiler, assembler, and linker for 6502 (`brew install cc65`)
- **Python 3** — generates CHR tile data
- **NES emulator** — [Mesen](https://www.mesen.ca/) or [FCEUX](https://fceux.com/) recommended

## Controls

| Button | Action |
|--------|--------|
| D-Pad Left/Right | Move piece (with DAS auto-repeat) |
| D-Pad Down | Soft drop |
| D-Pad Up | Hard drop |
| A | Rotate clockwise |
| B | Rotate counter-clockwise |
| Start | Start game / return to title |

## Gameplay

- 10x20 playfield with 7 standard tetrominoes (I, O, T, S, Z, J, L)
- Next piece preview
- Score, lines, and level display
- Level increases every 10 lines, speeding up gravity
- Line clear flash animation
- Scoring: 1 line = 40, 2 = 100, 3 = 300, Tetris = 1200 (multiplied by level+1)

## Project Structure

```
nessy/
├── Makefile               Build orchestration + cc65 auto-install
├── cfg/
│   └── nes.cfg            ld65 linker config (NROM mapper 0)
├── src/
│   ├── crt0.s             Startup: iNES header, reset/NMI/IRQ, VRAM buffer drain
│   ├── neslib.h           C API: PPU, palette, VRAM, controller, tile constants
│   ├── neslib.s           Assembly implementation of neslib (cc65 fastcall)
│   ├── tetris.h           Game constants, piece data externs, function declarations
│   ├── tetris.c           Core logic: collision, rotation, line clear, scoring, DAS, RNG
│   ├── render.c           Rendering: VRAM buffer, sprites, screen drawing, score display
│   └── main.c             Game state machine (title/playing/lineclear/gameover)
├── chr/
│   └── ascii.chr          Generated 8KB CHR (ASCII font + game tiles, NES 2bpp planar)
├── tools/
│   └── chr_gen.py         Generates ascii.chr with font glyphs + block/border tiles
└── build/
    └── nessy.nes          Output ROM (24,592 bytes)
```

## Architecture

**Target**: NROM-128 (mapper 0) — 16KB PRG-ROM + 8KB CHR-ROM

**Build flow**: `cc65` (.c → .s) → `ca65` (.s → .o) → `ld65` (.o + none.lib → .nes)

**Memory map**:
| Region | Address | Size | Purpose |
|--------|---------|------|---------|
| Zero Page | $0010-$00FF | 240 B | Fast variables, VRAM buffer length |
| OAM Buffer | $0200-$02FF | 256 B | Sprite data (DMA source) |
| RAM | $0300-$07FF | 1.25 KB | Palette, VRAM buffer, playfield, game state |
| PRG-ROM | $C000-$FFFF | 16 KB | Code + data |
| CHR-ROM | PPU $0000-$1FFF | 8 KB | Tile graphics |

**Rendering**: The active falling piece uses sprites (4 OAM entries). Placed blocks and UI are background tiles. A VRAM update buffer queues nametable changes during gameplay; the NMI handler drains it during vblank.
