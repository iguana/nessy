/* main.c - NESsy Tetris game state machine */

#include "neslib.h"
#include "tetris.h"

/* BG palette: black background with multiple piece colors */
static const unsigned char bg_pal[16] = {
    0x0F, 0x30, 0x10, 0x00,   /* BG 0: black, white, lt gray, dk gray (text + borders) */
    0x0F, 0x2C, 0x1C, 0x0C,   /* BG 1: cyan shades (I, S pieces on BG) */
    0x0F, 0x28, 0x18, 0x08,   /* BG 2: yellow shades (O, Z pieces on BG) */
    0x0F, 0x24, 0x14, 0x04,   /* BG 3: purple shades (T, J pieces on BG) */
};

/* Sprite palette: piece colors */
static const unsigned char spr_pal[16] = {
    0x0F, 0x27, 0x17, 0x07,   /* Spr 0: orange (L piece) */
    0x0F, 0x2C, 0x1C, 0x0C,   /* Spr 1: cyan (I, S pieces) */
    0x0F, 0x28, 0x18, 0x08,   /* Spr 2: yellow (O, Z pieces) */
    0x0F, 0x24, 0x14, 0x04,   /* Spr 3: purple (T, J pieces) */
};

/* Draw the full game screen (rendering must be off) */
static void draw_game_screen(void)
{
    /* Clear entire nametable */
    vram_adr(NTADR_A(0, 0));
    vram_fill(TILE_BLANK, 960);

    draw_border();
    draw_playfield();
    draw_hud_labels();
}

void main(void)
{
    /* Initial setup */
    ppu_off();
    pal_bg(bg_pal);
    pal_spr(spr_pal);

    game_state = STATE_TITLE;
    rng_seed = 0;

    draw_title_screen();
    scroll(0, 0);
    ppu_on_all();

    /* ── Main loop ── */
    while (1) {
        ppu_wait_nmi();

        switch (game_state) {

        case STATE_TITLE:
            /* Tick RNG seed while on title screen */
            ++rng_seed;
            pad_prev = pad_cur;
            pad_cur = pad_poll(0);
            pad_new = pad_cur & ~pad_prev;

            if (pad_new & PAD_START) {
                ppu_off();
                start_game();
                draw_game_screen();
                draw_score();
                draw_next_piece();
                scroll(0, 0);
                ppu_on_all();
            }
            break;

        case STATE_PLAYING:
            do_input();
            do_gravity();
            if (game_state == STATE_PLAYING)
                update_sprites();
            break;

        case STATE_LINECLEAR:
            ++lineclear_timer;
            /* Flash every 4 frames */
            if ((lineclear_timer & 3) == 0) {
                flash_lines(lineclear_timer >> 2);
            }
            if (lineclear_timer >= LINECLEAR_FRAMES) {
                add_score(num_lines_clearing);
                collapse_lines();
                redraw_playfield_full();
                spawn_piece();
                draw_next_piece();
                draw_score();
                game_state = STATE_PLAYING;
            }
            break;

        case STATE_GAMEOVER:
            /* Show "GAME OVER" via vbuf */
            if (lineclear_timer == 0) {
                /* Reuse lineclear_timer as "did we draw" flag */
                lineclear_timer = 1;
                vbuf_put(NTADR_A(PF_X + 1, PF_Y + 9), 0x27); /* G */
                vbuf_put(NTADR_A(PF_X + 2, PF_Y + 9), 0x21); /* A */
                vbuf_put(NTADR_A(PF_X + 3, PF_Y + 9), 0x2D); /* M */
                vbuf_put(NTADR_A(PF_X + 4, PF_Y + 9), 0x25); /* E */
                vbuf_put(NTADR_A(PF_X + 5, PF_Y + 9), 0x00); /*   */
                vbuf_put(NTADR_A(PF_X + 6, PF_Y + 9), 0x2F); /* O */
                vbuf_put(NTADR_A(PF_X + 7, PF_Y + 9), 0x36); /* V */
                vbuf_put(NTADR_A(PF_X + 8, PF_Y + 9), 0x25); /* E */
                vbuf_put(NTADR_A(PF_X + 9, PF_Y + 9), 0x32); /* R */
            }

            pad_prev = pad_cur;
            pad_cur = pad_poll(0);
            pad_new = pad_cur & ~pad_prev;

            if (pad_new & PAD_START) {
                lineclear_timer = 0;
                ppu_off();
                draw_title_screen();
                scroll(0, 0);
                game_state = STATE_TITLE;
                ppu_on_all();
            }
            break;
        }
    }
}
