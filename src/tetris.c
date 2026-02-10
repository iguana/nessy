/* tetris.c - Core game logic: pieces, collision, movement, rotation,
 *            line clearing, scoring, RNG, DAS input
 */

#include "neslib.h"
#include "tetris.h"

/* ── Piece data ──
 * 7 pieces × 4 rotations × 4 blocks = 112 entries
 * Index: piece*16 + rot*4 + block
 * Coordinates are relative to piece origin (0,0 = top-left of bounding box)
 */

/* Piece X offsets */
const unsigned char piece_x[] = {
    /* I piece - rotations 0,1,2,3 */
    0,1,2,3,  2,2,2,2,  0,1,2,3,  1,1,1,1,
    /* O piece */
    1,2,1,2,  1,2,1,2,  1,2,1,2,  1,2,1,2,
    /* T piece */
    0,1,2,1,  1,0,1,1,  1,0,1,2,  0,0,1,0,
    /* S piece */
    1,2,0,1,  0,0,1,1,  1,2,0,1,  0,0,1,1,
    /* Z piece */
    0,1,1,2,  1,1,0,0,  0,1,1,2,  1,1,0,0,
    /* J piece */
    0,0,1,2,  1,2,1,1,  0,1,2,2,  1,1,1,0,
    /* L piece */
    2,0,1,2,  0,1,1,1,  0,1,2,0,  1,1,0,1,
};

/* Piece Y offsets */
const unsigned char piece_y[] = {
    /* I piece - rotations 0,1,2,3 */
    0,0,0,0,  0,1,2,3,  2,2,2,2,  0,1,2,3,
    /* O piece */
    0,0,1,1,  0,0,1,1,  0,0,1,1,  0,0,1,1,
    /* T piece */
    0,0,0,1,  0,1,1,2,  0,1,1,1,  0,1,1,2,
    /* S piece */
    0,0,1,1,  0,1,1,2,  0,0,1,1,  0,1,1,2,
    /* Z piece */
    0,0,1,1,  0,1,1,2,  0,0,1,1,  0,1,1,2,
    /* J piece */
    0,1,1,1,  0,0,1,2,  1,1,1,2,  0,1,2,2,
    /* L piece */
    0,1,1,1,  0,0,1,2,  1,1,1,2,  0,1,2,2,
};

/* Sprite palette per piece type (maps to sprite palette 0-3) */
const unsigned char piece_pal[] = {
    1,  /* I - cyan */
    2,  /* O - yellow */
    3,  /* T - purple */
    1,  /* S - green (reuse cyan pal) */
    2,  /* Z - red (reuse yellow pal) */
    3,  /* J - blue (reuse purple pal) */
    0,  /* L - orange (use pal 0) */
};

/* Speed table: frames per gravity drop, indexed by level (0-29) */
const unsigned char speed_table[] = {
    48, 43, 38, 33, 28, 23, 18, 13, 8, 6,  /* levels 0-9 */
     5,  5,  5,  4,  4,  4,  3,  3,  3, 2,  /* levels 10-19 */
     2,  2,  2,  2,  2,  2,  2,  2,  2, 1,  /* levels 20-29 */
};

/* ── Game state variables ── */
unsigned char playfield[PF_H * PF_W];
unsigned char game_state;
unsigned char cur_piece;
unsigned char cur_rot;
signed char cur_x;
signed char cur_y;
unsigned char next_piece;
unsigned char level;
unsigned char drop_timer;
unsigned char lineclear_timer;
unsigned char lines_to_clear[4];
unsigned char num_lines_clearing;

unsigned char score[3];
unsigned char lines[2];

unsigned char pad_cur;
unsigned char pad_prev;
unsigned char pad_new;

unsigned char das_dir;
unsigned char das_timer;

unsigned int rng_seed;

/* ── RNG: 16-bit Galois LFSR ── */
static unsigned char raw_random(void)
{
    unsigned char bit;
    bit = (unsigned char)(rng_seed & 1);
    rng_seed >>= 1;
    if (bit)
        rng_seed ^= 0xB400;
    return (unsigned char)(rng_seed & 0xFF);
}

unsigned char next_random_piece(void)
{
    unsigned char p;
    p = raw_random() % NUM_PIECES;
    /* Re-roll once if same as current (reduces repeats) */
    if (p == cur_piece)
        p = raw_random() % NUM_PIECES;
    return p;
}

/* ── Collision detection ──
 * Returns 1 if piece at (x,y) with given rotation collides, 0 if OK
 */
unsigned char check_collision(unsigned char piece, unsigned char rot,
                              signed char x, signed char y)
{
    unsigned char i, idx;
    signed char bx, by;

    idx = (piece << 4) | (rot << 2);

    for (i = 0; i < 4; ++i) {
        bx = x + (signed char)piece_x[idx + i];
        by = y + (signed char)piece_y[idx + i];

        /* Wall/floor bounds */
        if (bx < 0 || bx >= PF_W || by >= PF_H)
            return 1;

        /* Above playfield is OK (spawning) */
        if (by < 0)
            continue;

        /* Check playfield */
        if (playfield[(unsigned char)by * PF_W + (unsigned char)bx])
            return 1;
    }
    return 0;
}

/* ── Lock the current piece into the playfield ── */
void lock_piece(void)
{
    unsigned char i, idx, bx, by;

    idx = (unsigned char)(cur_piece << 4) | (unsigned char)(cur_rot << 2);

    for (i = 0; i < 4; ++i) {
        bx = (unsigned char)((signed char)piece_x[idx + i] + cur_x);
        by = (unsigned char)((signed char)piece_y[idx + i] + cur_y);

        if (by < PF_H && bx < PF_W) {
            playfield[by * PF_W + bx] = cur_piece + 1; /* nonzero = filled */

            /* Queue VRAM update for this cell */
            vbuf_put(PF_NTADR(bx, by), TILE_BLOCK);
        }
    }
}

/* ── Check for completed lines ──
 * Returns number of completed lines (0-4), fills lines_to_clear[]
 */
unsigned char check_lines(void)
{
    unsigned char r, c, full, count;

    count = 0;
    for (r = 0; r < PF_H; ++r) {
        full = 1;
        for (c = 0; c < PF_W; ++c) {
            if (!playfield[r * PF_W + c]) {
                full = 0;
                break;
            }
        }
        if (full) {
            lines_to_clear[count] = r;
            ++count;
            if (count >= 4) break;
        }
    }
    num_lines_clearing = count;
    return count;
}

/* ── Collapse cleared lines ── */
void collapse_lines(void)
{
    unsigned char i, r, c, dst;

    /* Process from bottom line to top */
    for (i = num_lines_clearing; i > 0; --i) {
        dst = lines_to_clear[i - 1];
        /* Shift everything above down by one */
        for (r = dst; r > 0; --r) {
            for (c = 0; c < PF_W; ++c) {
                playfield[r * PF_W + c] = playfield[(r - 1) * PF_W + c];
            }
        }
        /* Clear top row */
        for (c = 0; c < PF_W; ++c) {
            playfield[c] = 0;
        }
        /* Adjust remaining line indices (they shifted down) */
        {
            unsigned char j;
            for (j = 0; j < i - 1; ++j) {
                if (lines_to_clear[j] < dst)
                    ++lines_to_clear[j];
            }
        }
    }
}

/* ── BCD addition helper ──
 * Add a 16-bit value to 3-byte BCD score
 * NES doesn't have decimal mode, so we do it manually
 */
static void bcd_add(unsigned char *bcd, unsigned long val)
{
    unsigned long current;
    unsigned long result;
    unsigned char d;
    unsigned char i;

    /* Convert BCD to binary */
    current = 0;
    for (i = 0; i < 3; ++i) {
        current = current * 100UL + (unsigned long)((bcd[i] >> 4) * 10 + (bcd[i] & 0x0F));
    }

    result = current + val;
    if (result > 999999UL) result = 999999UL;

    /* Convert back to BCD */
    for (i = 3; i > 0; --i) {
        d = (unsigned char)(result % 100UL);
        bcd[i - 1] = (unsigned char)(((d / 10) << 4) | (d % 10));
        result /= 100UL;
    }
}

/* ── Add score for cleared lines ── */
void add_score(unsigned char num_lines)
{
    static const unsigned int points[] = { 0, 40, 100, 300, 1200 };
    unsigned long pts;

    pts = (unsigned long)points[num_lines] * (unsigned long)(level + 1);
    bcd_add(score, pts);

    /* Add to line counter (BCD) */
    bcd_add(lines, (unsigned long)num_lines);

    /* Level up every 10 lines: convert lines BCD to binary */
    {
        unsigned int total_lines;
        total_lines = (unsigned int)((lines[0] >> 4) * 10 + (lines[0] & 0x0F)) * 100
                    + (unsigned int)((lines[1] >> 4) * 10 + (lines[1] & 0x0F));
        level = (unsigned char)(total_lines / 10);
        if (level > 29) level = 29;
    }
}

/* ── Spawn a new piece ── */
void spawn_piece(void)
{
    cur_piece = next_piece;
    next_piece = next_random_piece();
    cur_rot = 0;
    cur_x = 3;
    cur_y = -1; /* Start partially above screen */

    /* If spawn position collides, game over */
    if (check_collision(cur_piece, cur_rot, cur_x, cur_y + 1)) {
        game_state = STATE_GAMEOVER;
        lineclear_timer = 0;
        hide_sprites();
    }
}

/* ── Gravity: drop piece by one row ── */
void do_gravity(void)
{
    unsigned char spd;
    spd = (level < 30) ? speed_table[level] : 1;

    ++drop_timer;
    if (drop_timer < spd)
        return;
    drop_timer = 0;

    if (!check_collision(cur_piece, cur_rot, cur_x, cur_y + 1)) {
        ++cur_y;
    } else {
        /* Lock piece */
        lock_piece();
        if (check_lines()) {
            game_state = STATE_LINECLEAR;
            lineclear_timer = 0;
            hide_sprites();
        } else {
            spawn_piece();
            draw_next_piece();
            draw_score();
        }
    }
}

/* ── Input handling with DAS ── */
void do_input(void)
{
    unsigned char new_rot;
    signed char new_x;

    pad_prev = pad_cur;
    pad_cur = pad_poll(0);
    pad_new = pad_cur & ~pad_prev;

    /* Tick RNG on every input poll */
    raw_random();

    /* Rotate: A = clockwise, B = counter-clockwise */
    if (pad_new & PAD_A) {
        new_rot = (cur_rot + 1) & 3;
        if (!check_collision(cur_piece, new_rot, cur_x, cur_y))
            cur_rot = new_rot;
    }
    if (pad_new & PAD_B) {
        new_rot = (cur_rot + 3) & 3; /* -1 mod 4 */
        if (!check_collision(cur_piece, new_rot, cur_x, cur_y))
            cur_rot = new_rot;
    }

    /* Left/Right with DAS */
    if (pad_new & PAD_LEFT) {
        if (!check_collision(cur_piece, cur_rot, cur_x - 1, cur_y))
            --cur_x;
        das_dir = PAD_LEFT;
        das_timer = 0;
    } else if (pad_new & PAD_RIGHT) {
        if (!check_collision(cur_piece, cur_rot, cur_x + 1, cur_y))
            ++cur_x;
        das_dir = PAD_RIGHT;
        das_timer = 0;
    } else if (pad_cur & das_dir) {
        ++das_timer;
        if (das_timer >= DAS_DELAY) {
            das_timer = DAS_DELAY - DAS_REPEAT;
            new_x = cur_x + ((das_dir == PAD_LEFT) ? -1 : 1);
            if (!check_collision(cur_piece, cur_rot, new_x, cur_y))
                cur_x = new_x;
        }
    } else {
        das_dir = 0;
        das_timer = 0;
    }

    /* Soft drop: Down */
    if (pad_cur & PAD_DOWN) {
        if (!check_collision(cur_piece, cur_rot, cur_x, cur_y + 1)) {
            ++cur_y;
            drop_timer = 0;
        }
    }

    /* Hard drop: Up */
    if (pad_new & PAD_UP) {
        while (!check_collision(cur_piece, cur_rot, cur_x, cur_y + 1))
            ++cur_y;
        drop_timer = 255; /* Force immediate lock on next gravity tick */
    }
}

/* ── Initialize game state ── */
void start_game(void)
{
    unsigned int i;

    /* Clear playfield */
    for (i = 0; i < PF_H * PF_W; ++i)
        playfield[i] = 0;

    /* Reset score/lines/level */
    score[0] = 0; score[1] = 0; score[2] = 0;
    lines[0] = 0; lines[1] = 0;
    level = 0;
    drop_timer = 0;
    das_dir = 0;
    das_timer = 0;

    /* Seed RNG (use whatever is in nmi_flag count from title screen) */
    if (rng_seed == 0) rng_seed = 0x1234;

    /* Pick first two pieces */
    next_piece = next_random_piece();
    spawn_piece();

    game_state = STATE_PLAYING;
}
