/* tetris.h - Game constants, piece data, and function declarations */

#ifndef _TETRIS_H
#define _TETRIS_H

/* Playfield dimensions */
#define PF_W    10
#define PF_H    20

/* Playfield position on nametable (top-left of inner area) */
#define PF_X    3
#define PF_Y    2

/* Nametable address for playfield cell */
#define PF_NTADR(col,row) NTADR_A((col)+PF_X, (row)+PF_Y)

/* Game states */
#define STATE_TITLE     0
#define STATE_PLAYING   1
#define STATE_LINECLEAR 2
#define STATE_GAMEOVER  3

/* Piece types */
#define PIECE_I  0
#define PIECE_O  1
#define PIECE_T  2
#define PIECE_S  3
#define PIECE_Z  4
#define PIECE_J  5
#define PIECE_L  6
#define NUM_PIECES 7

/* DAS (Delayed Auto Shift) timings in frames */
#define DAS_DELAY   16
#define DAS_REPEAT  6

/* Line clear animation frames */
#define LINECLEAR_FRAMES 20

/* Score display position on nametable */
#define SCORE_X  16
#define SCORE_Y  1
#define LINES_X  16
#define LINES_Y  4
#define LEVEL_X  16
#define LEVEL_Y  7
#define NEXT_X   17
#define NEXT_Y   10

/* Piece data tables (112 bytes each): piece*16 + rot*4 + block */
extern const unsigned char piece_x[];
extern const unsigned char piece_y[];

/* Sprite palette index per piece type (0-3) */
extern const unsigned char piece_pal[];

/* Speed table: frames per drop for each level */
extern const unsigned char speed_table[];

/* Playfield state: PF_W * PF_H bytes, 0=empty, nonzero=filled */
extern unsigned char playfield[PF_H * PF_W];

/* Game state variables */
extern unsigned char game_state;
extern unsigned char cur_piece;
extern unsigned char cur_rot;
extern signed char cur_x;
extern signed char cur_y;
extern unsigned char next_piece;
extern unsigned char level;
extern unsigned char drop_timer;
extern unsigned char lineclear_timer;
extern unsigned char lines_to_clear[4];
extern unsigned char num_lines_clearing;

/* Score: 3 bytes BCD (6 digits) */
extern unsigned char score[3];
/* Lines: 2 bytes BCD (4 digits) */
extern unsigned char lines[2];

/* Input state */
extern unsigned char pad_cur;
extern unsigned char pad_prev;
extern unsigned char pad_new;

/* DAS state */
extern unsigned char das_dir;
extern unsigned char das_timer;

/* RNG seed */
extern unsigned int rng_seed;

/* ── tetris.c functions ── */
void start_game(void);
unsigned char check_collision(unsigned char piece, unsigned char rot,
                              signed char x, signed char y);
void lock_piece(void);
unsigned char check_lines(void);
void collapse_lines(void);
void add_score(unsigned char num_lines);
void spawn_piece(void);
unsigned char next_random_piece(void);
void do_gravity(void);
void do_input(void);

/* ── render.c functions ── */
void vbuf_put(unsigned int adr, unsigned char tile);
void update_sprites(void);
void hide_sprites(void);
void draw_playfield(void);
void draw_border(void);
void draw_hud_labels(void);
void draw_score(void);
void draw_next_piece(void);
void draw_title_screen(void);
void flash_lines(unsigned char phase);
void redraw_playfield_full(void);

#endif /* _TETRIS_H */
