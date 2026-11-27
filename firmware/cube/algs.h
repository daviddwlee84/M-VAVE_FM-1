/* SPDX-License-Identifier: GPL-3.0-or-later */
/* algs: the formula book (algs.c). Each entry: its set, its name, the formula, maybe a second one. The case a
 * formula solves is the formula's inverse done on a solved cube (yellow on top in the pictures), so no picture is
 * drawn by hand. Entries are append only within a set: flash-card progress is kept by position. */
#pragma once
#include <stdint.h>
#include "cube.h"

enum { ALG_LBL, ALG_OLL2, ALG_PLL2, ALG_OLL, ALG_PLL, ALG_F2L, ALG_SETS };
#define ALG_MAX 48                       /* moves in one formula at most */

typedef struct {
    uint8_t set;
    const char *name;
    const char *alg;
    const char *alt;                     /* more formulas for the same case, "|" between them; 0: none */
} alg_t;

int alg_total(void);
#define ALG_N alg_total()
const alg_t *alg_get(int i);
int alg_count(int set);
int alg_index(int set, int k);           /* the k-th entry of a set: its index, -1 none */
int alg_pos(int i);                      /* its place in its set */
const char *alg_set_name(int set);       /* "LBL", "2-LOOK OLL", ... */
const char *alg_set_short(int set);
int alg_moves(int i, uint8_t *out);      /* the formula parsed (-1: it does not parse) */
#define ALG_FORMS 6                      /* formulas for one case at most */
int alg_forms(int i);                    /* how many formulas the case has (the first and its alternatives) */
int alg_form_text(int i, int k, char *out, int max);   /* formula k as text (0: the first) */
int alg_form(int i, int k, uint8_t *out);  /* formula k parsed (-1: none, or it does not parse) */
/* the case: the formula's inverse on a solved cube, then U^auf (the pieces of the last layer turned, as a real
 * cube comes) */
int alg_case(int i, cube_t *c, int auf);
/* the case's scramble on a real cube: the inverse, then U^auf; returns the moves */
int alg_setup(int i, uint8_t *out, int auf);

/* checks (tests, and the drills) */
int alg_f2l_kept(const cube_t *c);       /* the first two layers solved */
int alg_cross_kept(const cube_t *c);     /* the cross and the FL, BL, BR pairs solved */
int alg_ll_oriented(const cube_t *c);    /* the last layer's top all one colour */
int alg_solved_auf(const cube_t *c);     /* solved but for a turn of one layer */
int alg_same_case(int a, int b);         /* one case (any side, any last U turn) */
/* what entry i's formulas reach, on cube c after one of them (any whole-cube rotation undone): OLL the last layer
 * oriented, PLL solved, F2L the first two layers, 2-look PLL's corner cases the corners placed, LBL solved (all up
 * to a last turn of one layer) */
int alg_goal_met(int i, const cube_t *c);
