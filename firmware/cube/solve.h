/* SPDX-License-Identifier: GPL-3.0-or-later */
/* solve: random-state scrambles for the 3x3x3, the WCA way: a state picked uniformly from all 43 quintillion, then a
 * short way there (usually 19 to 21 moves), found by Kociemba's two-phase algorithm.
 *
 * Phase 1 takes the cube into <U, D, R2, L2, F2, B2> (corners and edges oriented, the middle-layer edges in the
 * middle layer), phase 2 solves it with those moves. The coordinates and the symmetry reduction follow Herbert
 * Kociemba's description (kociemba.org/cube.htm) and min2phase's choice of tables (Chen Shuang): phase 1 prunes on
 * twist x slice, twist reduced by the 8 rotations that keep the U-D axis, and on the flip alone; phase 2 on corner and
 * on edge permutation, each reduced by the 16 symmetries of the U-D axis, times the middle-layer permutation. Our own
 * implementation, written from those descriptions.
 *
 * The tables (about 310 KB) are built by tools/gen_tables.c into gen/cube_tables.h and kept in flash; built with
 * SOLVE_GEN, this file makes them in RAM instead (the generator and the tests).
 *
 * The search runs in slices (solve_step), so the main loop keeps drawing: each call does about `budget` nodes.
 * Platform-free: no platform calls, no libc. */
#pragma once
#include <stdint.h>

typedef struct {
    uint8_t cp[8], co[8], ep[12], eo[12];
} cc_t;

#define SOLVE_MAX 30                     /* moves in a solution at most */

typedef struct {                         /* one of the six searches (solve.c) */
    /* the cube being solved */
    cc_t cc;
    int8_t maxlen;                       /* the length being looked for now (it grows when a level takes too long) */
    int8_t d1;                           /* phase 1's depth being searched */
    int8_t n;                            /* phase 1: the level of the node being expanded (-1: start the depth) */
    uint8_t done, failed;
    uint32_t nodes, nodes_level;         /* all nodes; nodes since maxlen last grew */
    uint32_t level_budget;               /* nodes allowed at a length before it grows by one */
    /* phase 1 per level */
    uint16_t tw[SOLVE_MAX + 1], fl[SOLVE_MAX + 1], sl[SOLVE_MAX + 1];
    int8_t mv[SOLVE_MAX + 1];
    uint8_t mv1[SOLVE_MAX];
    /* the solution */
    uint8_t sol[SOLVE_MAX];
    int8_t len;
} sv_dir_t;

#define SOLVE_DIRS 6                     /* the cube along U-D, R-L, F-B, and its inverse along each */
typedef struct {
    sv_dir_t d[SOLVE_DIRS];
    uint8_t turn, done, failed, dir;
    uint32_t nodes;
    uint8_t sol[SOLVE_MAX];              /* moves that solve the cube (face move ids 0..17) */
    int8_t len;
} solve_t;

int solve_init(void);                    /* SOLVE_GEN: build the tables (returns 0); otherwise checks them */
void solve_random_cc(cc_t *c, uint32_t *rng);   /* a uniformly random reachable cube */
int cc_valid(const cc_t *c);
void cc_ident(cc_t *c);
void cc_move(cc_t *c, int m);            /* one of the 18 face moves */
int cc_from_facelets(cc_t *c, const char *s);   /* 54 letters URFDLB (cube_facelets); 1 = taken */
void cc_facelets(const cc_t *c, char *out);     /* 54 letters and a 0 */

void solve_start(solve_t *s, const cc_t *c, int maxlen);
int solve_step(solve_t *s, uint32_t budget);    /* 1 done (s->sol, s->len), 0 more to do, -1 failed */

/* a random-state scramble in one go (host, tests): returns the length, moves in out (face move ids 0..17) */
int solve_scramble(uint8_t *out, uint32_t *rng, int maxlen);
int solve_bank(uint32_t i, uint8_t *out);   /* a random-state scramble made on the computer (SOLVE_GEN: -1) */

/* table statistics, for the tests */
int solve_twist_classes(void);
int solve_perm_classes(void);
