/* main.c - NESsy hello world
 * Displays text on screen to verify the build chain works.
 */

#include "neslib.h"

/* BG palette: black bg, white/light gray/dark gray text */
static const unsigned char palette[16] = {
    0x0F, 0x30, 0x10, 0x00,   /* BG palette 0: black, white, light gray, dark gray */
    0x0F, 0x30, 0x10, 0x00,   /* BG palette 1 */
    0x0F, 0x30, 0x10, 0x00,   /* BG palette 2 */
    0x0F, 0x30, 0x10, 0x00    /* BG palette 3 */
};

/* Write a null-terminated string to VRAM at current PPU address.
 * Tile index = ASCII code - 0x20 (matching our CHR layout). */
static void write_string(const char *str)
{
    while (*str) {
        vram_put((unsigned char)(*str - 0x20));
        ++str;
    }
}

void main(void)
{
    /* Turn off rendering to safely write VRAM */
    ppu_off();

    /* Set BG palette */
    pal_bg(palette);

    /* Write text to nametable A */
    /*            "NESSY" centered on row 10 (col 13) */
    vram_adr(NTADR_A(13, 10));
    write_string("NESSY");

    /*            "NES BUILD CHAIN ACTIVE" centered on row 14 (col 5) */
    vram_adr(NTADR_A(5, 14));
    write_string("NES BUILD CHAIN ACTIVE");

    /*            "HELLO FROM CC65!" centered on row 16 (col 8) */
    vram_adr(NTADR_A(8, 16));
    write_string("HELLO FROM CC65!");

    /* Reset scroll to top-left */
    scroll(0, 0);

    /* Enable BG rendering (also enables NMI) */
    ppu_on_bg();

    /* Main loop: just wait for vblank forever */
    while (1) {
        ppu_wait_nmi();
    }
}
