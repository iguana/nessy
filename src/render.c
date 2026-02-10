/* render.c - Rendering: VRAM buffer, sprites, screen drawing, score display */

#include "neslib.h"
#include "tetris.h"

/* ASCII tile offset: tile_index = char - 0x20 */
#define CHR(c) ((unsigned char)((c) - 0x20))

/* Queue one VRAM update: addr_hi, addr_lo, tile */
void vbuf_put(unsigned int adr, unsigned char tile)
{
    unsigned char i;
    i = vbuf_len * 3;
    vram_buf[i]   = (unsigned char)(adr >> 8);
    vram_buf[i+1] = (unsigned char)(adr);
    vram_buf[i+2] = tile;
    ++vbuf_len;
}

/* Write a string directly to VRAM at current PPU address (rendering must be off) */
static void write_str(const char *s)
{
    while (*s) {
        vram_put(CHR(*s));
        ++s;
    }
}

/* Update 4 OAM sprites for the active falling piece */
void update_sprites(void)
{
    unsigned char i, idx, px, py, pal;
    idx = (unsigned char)(cur_piece << 4) | (unsigned char)(cur_rot << 2);
    pal = piece_pal[cur_piece];

    for (i = 0; i < 4; ++i) {
        px = (unsigned char)((signed char)piece_x[idx + i] + cur_x);
        py = (unsigned char)((signed char)piece_y[idx + i] + cur_y);

        /* Skip blocks above visible area */
        if (py >= PF_H) {
            oam_buf[i * 4]     = 0xFF; /* hide */
            continue;
        }

        /* OAM: y, tile, attr, x */
        oam_buf[i * 4]     = (unsigned char)((py + PF_Y) * 8 - 1); /* Y pixel (-1 for NES OAM quirk) */
        oam_buf[i * 4 + 1] = TILE_BLOCK;       /* tile */
        oam_buf[i * 4 + 2] = pal;              /* palette + no flip */
        oam_buf[i * 4 + 3] = (unsigned char)((px + PF_X) * 8); /* X pixel */
    }
}

/* Hide all 4 piece sprites off-screen */
void hide_sprites(void)
{
    unsigned char i;
    for (i = 0; i < 4; ++i) {
        oam_buf[i * 4] = 0xFF;
    }
}

/* Draw the playfield border (rendering must be off) */
void draw_border(void)
{
    unsigned char i;

    /* Top border: row PF_Y-1, cols PF_X-1 to PF_X+PF_W */
    vram_adr(NTADR_A(PF_X - 1, PF_Y - 1));
    vram_put(TILE_BRD_TL);
    for (i = 0; i < PF_W; ++i)
        vram_put(TILE_BRD_H);
    vram_put(TILE_BRD_TR);

    /* Side borders: rows PF_Y to PF_Y+PF_H-1 */
    for (i = 0; i < PF_H; ++i) {
        vram_adr(NTADR_A(PF_X - 1, PF_Y + i));
        vram_put(TILE_BRD_V);
        vram_adr(NTADR_A(PF_X + PF_W, PF_Y + i));
        vram_put(TILE_BRD_V);
    }

    /* Bottom border */
    vram_adr(NTADR_A(PF_X - 1, PF_Y + PF_H));
    vram_put(TILE_BRD_BL);
    for (i = 0; i < PF_W; ++i)
        vram_put(TILE_BRD_H);
    vram_put(TILE_BRD_BR);
}

/* Draw all playfield cells from the playfield[] array (rendering must be off) */
void draw_playfield(void)
{
    unsigned char r, c;
    for (r = 0; r < PF_H; ++r) {
        vram_adr(PF_NTADR(0, r));
        for (c = 0; c < PF_W; ++c) {
            if (playfield[r * PF_W + c])
                vram_put(TILE_BLOCK);
            else
                vram_put(TILE_EMPTY);
        }
    }
}

/* Draw HUD labels (rendering must be off) */
void draw_hud_labels(void)
{
    /* "SCORE" */
    vram_adr(NTADR_A(SCORE_X, SCORE_Y));
    write_str("SCORE");

    /* "LINES" */
    vram_adr(NTADR_A(LINES_X, LINES_Y));
    write_str("LINES");

    /* "LEVEL" */
    vram_adr(NTADR_A(LEVEL_X, LEVEL_Y));
    write_str("LEVEL");

    /* "NEXT" */
    vram_adr(NTADR_A(NEXT_X, NEXT_Y));
    write_str("NEXT");

    /* Next piece box border */
    vram_adr(NTADR_A(NEXT_X, NEXT_Y + 1));
    vram_put(TILE_BRD_TL);
    vram_put(TILE_BRD_H);
    vram_put(TILE_BRD_H);
    vram_put(TILE_BRD_H);
    vram_put(TILE_BRD_H);
    vram_put(TILE_BRD_TR);

    vram_adr(NTADR_A(NEXT_X, NEXT_Y + 2));
    vram_put(TILE_BRD_V);
    vram_put(TILE_EMPTY);
    vram_put(TILE_EMPTY);
    vram_put(TILE_EMPTY);
    vram_put(TILE_EMPTY);
    vram_put(TILE_BRD_V);

    vram_adr(NTADR_A(NEXT_X, NEXT_Y + 3));
    vram_put(TILE_BRD_V);
    vram_put(TILE_EMPTY);
    vram_put(TILE_EMPTY);
    vram_put(TILE_EMPTY);
    vram_put(TILE_EMPTY);
    vram_put(TILE_BRD_V);

    vram_adr(NTADR_A(NEXT_X, NEXT_Y + 4));
    vram_put(TILE_BRD_BL);
    vram_put(TILE_BRD_H);
    vram_put(TILE_BRD_H);
    vram_put(TILE_BRD_H);
    vram_put(TILE_BRD_H);
    vram_put(TILE_BRD_BR);
}

/* Draw score digits via VRAM buffer (6 BCD digits) */
void draw_score(void)
{
    unsigned char i;
    unsigned int adr;

    /* Score: 3 bytes BCD = 6 digits */
    adr = NTADR_A(SCORE_X, SCORE_Y + 1);
    for (i = 0; i < 3; ++i) {
        vbuf_put(adr, CHR('0') + (score[i] >> 4));
        ++adr;
        vbuf_put(adr, CHR('0') + (score[i] & 0x0F));
        ++adr;
    }

    /* Lines: 2 bytes BCD = 4 digits */
    adr = NTADR_A(LINES_X, LINES_Y + 1);
    for (i = 0; i < 2; ++i) {
        vbuf_put(adr, CHR('0') + (lines[i] >> 4));
        ++adr;
        vbuf_put(adr, CHR('0') + (lines[i] & 0x0F));
        ++adr;
    }

    /* Level: 2 digits from level byte */
    adr = NTADR_A(LEVEL_X, LEVEL_Y + 1);
    vbuf_put(adr, CHR('0') + (level / 10));
    vbuf_put(adr + 1, CHR('0') + (level % 10));
}

/* Draw next piece preview inside the box (via VRAM buffer) */
void draw_next_piece(void)
{
    unsigned char i, idx, bx, by;
    unsigned int adr;

    /* Clear the 4x2 preview area first */
    for (by = 0; by < 2; ++by) {
        adr = NTADR_A(NEXT_X + 1, NEXT_Y + 2 + by);
        for (bx = 0; bx < 4; ++bx) {
            vbuf_put(adr + bx, TILE_EMPTY);
        }
    }

    /* Draw next piece blocks */
    idx = (unsigned char)(next_piece << 4); /* rotation 0 */
    for (i = 0; i < 4; ++i) {
        bx = piece_x[idx + i];
        by = piece_y[idx + i];
        adr = NTADR_A(NEXT_X + 1 + bx, NEXT_Y + 2 + by);
        vbuf_put(adr, TILE_BLOCK);
    }
}

/* Draw title screen (rendering must be off) */
void draw_title_screen(void)
{
    /* Clear nametable */
    vram_adr(NTADR_A(0, 0));
    vram_fill(TILE_BLANK, 960);

    /* Title */
    vram_adr(NTADR_A(9, 10));
    write_str("NESSY TETRIS");

    /* Prompt */
    vram_adr(NTADR_A(9, 16));
    write_str("PRESS START");
}

/* Flash clearing lines: phase toggles block/empty */
void flash_lines(unsigned char phase)
{
    unsigned char i, c;
    unsigned char tile;
    unsigned int adr;

    tile = (phase & 1) ? TILE_EMPTY : TILE_BLOCK;

    for (i = 0; i < num_lines_clearing; ++i) {
        adr = PF_NTADR(0, lines_to_clear[i]);
        for (c = 0; c < PF_W; ++c) {
            vbuf_put(adr + c, tile);
        }
    }
}

/* Full playfield redraw after line collapse (rendering off) */
void redraw_playfield_full(void)
{
    ppu_off();
    draw_playfield();
    scroll(0, 0);
    ppu_on_all();
}
