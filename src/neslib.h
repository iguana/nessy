/* neslib.h - NES library for cc65
 * Minimal PPU/controller interface for NES homebrew
 */

#ifndef _NESLIB_H
#define _NESLIB_H

/* Nametable address macro: x=0..31, y=0..29 */
#define NTADR_A(x,y) ((unsigned int)(0x2000 | ((y) << 5) | (x)))
#define NTADR_B(x,y) ((unsigned int)(0x2400 | ((y) << 5) | (x)))
#define NTADR_C(x,y) ((unsigned int)(0x2800 | ((y) << 5) | (x)))
#define NTADR_D(x,y) ((unsigned int)(0x2C00 | ((y) << 5) | (x)))

/* Controller button masks */
#define PAD_A       0x80
#define PAD_B       0x40
#define PAD_SELECT  0x20
#define PAD_START   0x10
#define PAD_UP      0x08
#define PAD_DOWN    0x04
#define PAD_LEFT    0x02
#define PAD_RIGHT   0x01

/* PPU mask bits */
#define MASK_BG     0x08
#define MASK_SPR    0x10
#define MASK_BG_L8  0x02
#define MASK_SPR_L8 0x04

/* Wait for next NMI (vblank). Requires NMI to be enabled. */
void __fastcall__ ppu_wait_nmi(void);

/* Turn on BG rendering */
void __fastcall__ ppu_on_bg(void);

/* Turn on sprite rendering */
void __fastcall__ ppu_on_spr(void);

/* Turn on both BG and sprites */
void __fastcall__ ppu_on_all(void);

/* Turn off all rendering */
void __fastcall__ ppu_off(void);

/* Set PPU mask directly */
void __fastcall__ ppu_mask(unsigned char mask);

/* Set VRAM address for subsequent writes */
void __fastcall__ vram_adr(unsigned int adr);

/* Write a byte to VRAM */
void __fastcall__ vram_put(unsigned char val);

/* Write a buffer to VRAM. len bytes from data. */
void __fastcall__ vram_write(const unsigned char *data, unsigned int len);

/* Fill VRAM with a value for len bytes */
void __fastcall__ vram_fill(unsigned char val, unsigned int len);

/* Set all 32 palette bytes from a 32-byte array */
void __fastcall__ pal_all(const unsigned char *data);

/* Set BG palette (16 bytes) */
void __fastcall__ pal_bg(const unsigned char *data);

/* Set sprite palette (16 bytes) */
void __fastcall__ pal_spr(const unsigned char *data);

/* Set a single palette color. index=0..31, color=NES color byte */
void __fastcall__ pal_col(unsigned char index, unsigned char color);

/* Read controller 1. Returns button bitmask. */
unsigned char __fastcall__ pad_poll(unsigned char pad);

/* Set scroll position */
void __fastcall__ scroll(unsigned int x, unsigned int y);

#endif /* _NESLIB_H */
