/* SPDX-License-Identifier: GPL-3.0-or-later */
/* lesson: GUITAR's LEARN mode without the screen. A course of chords taught key by key (C first, then G, D, A and E,
 * then F with barre chords), each with the shape it is taught in: an easy one first for chords that need a barre,
 * the barre itself in the last stage. Leitner boxes per chord, rounds that start from the weakest, a stage unlocked
 * when every chord before it reaches box 2.
 *
 * No platform calls, no libc, no floating point (tests/fret_test.c runs it: just guitar-unit). Needs theory.h and
 * fret.h. */
#pragma once
#include <stdint.h>
#include "fret.h"

#define LS_BOXES 5                       /* box 0 (new) .. 5 (known) */
#define LS_PASS 2                        /* every chord of the stages open at this box or higher: the next opens */
#define LS_ITEMS_MAX 32
#define LS_ROUND 8                       /* cards in a round */

typedef struct {
    int8_t root;                         /* on the line of fifths (theory.h) */
    uint8_t q;                           /* TH_* */
    char frets[FR_STR_MAX + 1];          /* the shape it is taught in, standard tuning: "x32010" */
    uint8_t stage;                       /* 0.. */
    uint8_t easy;                        /* a stand-in for a barre chord taught later */
} ls_item_t;

typedef struct {
    const char *key;                     /* "C", "G", "A E" */
    const char *what;                    /* a line for the screen */
} ls_stage_t;

typedef struct {
    uint8_t box[LS_ITEMS_MAX];
    uint8_t stage;                       /* the highest stage open */
    uint8_t rsv;
    uint16_t rounds;                     /* rounds finished */
} ls_prog_t;

enum { LS_NAME_TO_SHAPE, LS_SHAPE_TO_NOTES };

extern const ls_item_t ls_items[];
extern const int ls_nitems;
extern const ls_stage_t ls_stages[];
extern const int ls_nstages;

void ls_reset(ls_prog_t *p);
int ls_valid(const ls_prog_t *p);
uint32_t ls_rand(uint32_t *s);                       /* xorshift32; a zero seed is replaced */
int ls_open(const ls_prog_t *p, int i);              /* item i is in an open stage */
th_chord_t ls_chord(int i);
int ls_shape(int i, fr_shape_t *sh);                 /* the item's shape on a standard guitar, fingered; 0 if not */

/* a round: n cards (items), the weakest first, never the same item twice in a row; returns how many */
int ls_round(const ls_prog_t *p, uint32_t *rng, uint8_t *cards, int n);
int ls_card_kind(const ls_prog_t *p, int item, uint32_t *rng);     /* LS_NAME_TO_SHAPE or LS_SHAPE_TO_NOTES */
/* three wrong answers for item: open items with another shape, the same stage or quality first */
int ls_distractors(const ls_prog_t *p, int item, uint32_t *rng, uint8_t *out);
/* an answer: up a box when right, back to box 0 (or 1 from box 3 and up) when wrong. Returns 1 if this opened the
 * next stage */
int ls_answer(ls_prog_t *p, int item, int right);
int ls_stage_done(const ls_prog_t *p, int stage);    /* chords of that stage at LS_PASS or more */
int ls_pcs(int item);                                /* pitch classes the taught shape sounds */
