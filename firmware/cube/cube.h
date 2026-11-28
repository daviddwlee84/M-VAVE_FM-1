/* SPDX-License-Identifier: GPL-3.0-or-later */
/* cube: a 3x3x3 Rubik's cube as 54 stickers, every move in WCA / SiGN notation, and the checks the app needs.
 *
 * Stickers are numbered as Kociemba's facelet string: faces U R F D L B, nine each, row by row as the face is seen
 * from outside (U with B at the top, D with F at the top, the four sides with U at the top). A sticker holds the
 * face it belongs to when solved (CF_U .. CF_B), so a facelet string is cube_facelets' letters.
 *
 * Moves: a base (MB_*) and a turn count 1, 2, 3 (3 = prime). Faces U R F D L B, slices M E S (M follows L, E follows
 * D, S follows F), wide moves u r f d l b (two layers, also written Uw ...), rotations x y z. A move id is
 * MV(base, turns); the 18 face moves are ids 0..17 in Kociemba's order (U U2 U' R R2 R' ...).
 *
 * Platform-free: no platform calls, no libc (tests/cube_test.c). */
#pragma once
#include <stdint.h>

enum { CF_U, CF_R, CF_F, CF_D, CF_L, CF_B, CF_N };
enum { MB_U, MB_R, MB_F, MB_D, MB_L, MB_B, MB_M, MB_E, MB_S, MB_u, MB_r, MB_f, MB_d, MB_l, MB_b, MB_x, MB_y, MB_z,
       MB_N };
#define MV(b, t) ((uint8_t)((b) * 3 + (t) - 1))
#define MV_BASE(m) ((m) / 3)
#define MV_TURNS(m) ((m) % 3 + 1)
#define MV_N (MB_N * 3)
#define MV_NONE 0xFFu
#define MV_INV(m) ((uint8_t)((m) - (m) % 3 + 2 - (m) % 3))   /* R <-> R', R2 stays */

typedef struct {
    uint8_t f[54];
} cube_t;

void cube_reset(cube_t *c);
void cube_move(cube_t *c, int mv);
void cube_apply(cube_t *c, const uint8_t *mv, int n);
int cube_equal(const cube_t *a, const cube_t *b);

/* notation: parse returns the number of moves, or -1 (a token it does not know, or more than max). Spaces, commas,
 * brackets and parentheses are skipped; R2' is R2, R3 is R'. print writes moves with one space between them and
 * returns the length (stops before max - 1 characters) */
int cube_parse(const char *s, uint8_t *out, int max);
int cube_print(const uint8_t *mv, int n, char *out, int max);
int cube_mv_name(int mv, char *out);     /* "R", "R2", "R'", "r", "x'": returns the length (1..2) */
void cube_invert(const uint8_t *in, int n, uint8_t *out);   /* out may be in */
int cube_merge(uint8_t *mv, int n);      /* same base side by side merged (R R -> R2, R R' -> nothing): new n */

/* checks */
int cube_solved(const cube_t *c);        /* every face one colour (any orientation of the whole cube) */
int cube_solved_turn(const cube_t *c);   /* solved, or one outer-layer turn from it (the AUF a case ends with) */
int cube_f2l_solved(const cube_t *c);    /* D face and the two lower layers solved, centres where they belong */
void cube_facelets(const cube_t *c, char *out);   /* 54 letters URFDLB and a 0 */
int cube_from_facelets(cube_t *c, const char *s); /* 54 letters; 1 = taken */

/* random-move scramble: n face moves, no face twice in a row, no three on one axis (R L R) */
uint32_t cube_rand(uint32_t *x);         /* xorshift32 */
int cube_scramble_moves(uint8_t *out, int n, uint32_t *rng);

/* where a sticker sits (for drawing): its centre in {-1, 0, 1}^3 (x right, y up, z front) and its face's normal */
void cube_sticker_pos(int i, int8_t *pos, int8_t *normal);
int cube_sticker_at(const int8_t *pos, const int8_t *normal);   /* -1 if none */
/* which stickers a move turns (bit i of 64), for drawing a turn: in mask, and the axis (0 x, 1 y, 2 z) and the sign
 * of a quarter turn about it (+1 counter-clockwise looking from +axis) */
uint64_t cube_move_layer(int base, int *axis, int *sign);
