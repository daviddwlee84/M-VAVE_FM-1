/* SPDX-License-Identifier: GPL-3.0-or-later */
/* fret: a fretted instrument's neck, worked out from theory.c. Tunings, where a key's notes lie on the frets,
 * scale positions (CAGED windows, three or two notes per string) and chord shapes found by search.
 *
 * Strings are numbered from the lowest-pitched one up (string 0 is a guitar's low E), whatever the screen draws on
 * top. A re-entrant tuning (ukulele GCEA) keeps its order on the neck: string 0 is its high G.
 *
 * No platform calls, no libc, no floating point (tests/fret_test.c runs it on a computer: just guitar-unit).
 * Needs theory.h (and theory.c in the same build). */
#pragma once
#include <stdint.h>
#include "theory.h"

#define FR_STR_MAX 6                     /* strings at most */
#define FR_FRETS 24                      /* the highest fret */
#define FR_SPAN 4                        /* frets a chord shape may cover (index to pinky) */
#define FR_SHAPES_MAX 8                  /* chord shapes kept per chord */

enum { FR_STANDARD, FR_DROPD, FR_DADGAD, FR_OPENG, FR_BASS4, FR_BASS5, FR_UKULELE, FR_MANDOLIN, FR_NTUNINGS };

typedef struct {
    const char *name;
    uint8_t n;                           /* strings */
    uint8_t reentrant;                   /* string 0 is not the lowest note: a chord's root need not be in the bass */
    uint8_t open[FR_STR_MAX];            /* MIDI notes of the open strings, string 0 first */
} fr_tuning_t;

extern const fr_tuning_t fr_tunings[FR_NTUNINGS];

/* where the notes are */
int fr_midi(const fr_tuning_t *t, int s, int fret);          /* the note at that string and fret */
int fr_scale_pcs(th_key_t key);                              /* pitch classes of the key's scale, bit 0 = C */
uint32_t fr_pc_frets(const fr_tuning_t *t, int s, int pc, int lo, int hi);   /* frets lo..hi of string s with pc */

/* scale positions: a set of frets on each string (mask[s] bit f = fret f) */
enum { FR_ALL, FR_CAGED, FR_NPS, FR_NSYSTEMS };
int fr_positions(th_key_t key, int system);  /* how many: ALL 1, CAGED 5, NPS 7 (7-note scales), 5 (pentatonic), 0 */
int fr_position(const fr_tuning_t *t, th_key_t key, int system, int pos, uint32_t *mask);   /* 1, or 0 if none */
int fr_nps(th_key_t key);                    /* notes per string in an NPS position: 3, 2, or 0 */
extern const char fr_caged_name[5];          /* "CAGED": the shape each CAGED position is named after */

/* chord shapes */
typedef struct {
    int8_t fret[FR_STR_MAX];             /* -1 muted, 0 open */
    uint8_t finger[FR_STR_MAX];          /* 0 none (open or muted), 1 index .. 4 pinky */
    uint8_t barre;                       /* the barre's fret, 0 none */
    uint8_t bfrom, bto;                  /* the strings it covers */
    uint8_t no5;                         /* the fifth left out */
    int16_t score;                       /* lower is easier */
} fr_shape_t;

/* The chord's shapes on this instrument, easiest first: every sounding note a chord tone, every tone there (a
 * perfect fifth may be left out), the root in the bass (unless the tuning is re-entrant), within FR_SPAN frets,
 * at most four fingers (one barre counts as one). Returns how many (at most max). */
int fr_shapes(const fr_tuning_t *t, th_chord_t c, fr_shape_t *out, int max);
int fr_shape_lo(const fr_shape_t *sh, int n);    /* the lowest fretted fret (0 if none) */
int fr_shape_hi(const fr_shape_t *sh, int n);    /* the highest */
int fr_shape_midi(const fr_tuning_t *t, const fr_shape_t *sh, uint8_t *out);   /* sounding notes, low string first */
