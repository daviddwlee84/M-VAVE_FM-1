/* SPDX-License-Identifier: GPL-3.0-or-later */
/* rhythm: RHYTHM's platform-free core (no fm1-base, C99, checked on this computer by tests/rhythm_test.c).
 *
 * A pattern is beats of Kodaly cells, 12 ticks a beat (so eighths, sixteenths and triplets all land on a tick).
 * rh_parse reads one from text ("q ee r q"), rh_generate makes one for a level. rh_score lines up the taps with the
 * onsets (each tap at most one onset, the alignment with the least error), grades every onset, and names the one
 * thing most worth fixing (rh_hint_text), the way Morse's RHYTHM drill does for dits and dahs. */
#pragma once
#include <stdint.h>

#define RH_TPB 12                        /* ticks a beat */
#define RH_MAX_BEATS 8                   /* two bars of 4/4 */
#define RH_MAX_ON 40                     /* onsets in a pattern (two bars of sixteenths: 32) */
#define RH_MAX_TAPS 64
#define RH_TEXT_MAX 64                   /* rh_text's longest, with its 0 */
#define RH_LEVELS 8

/* the cells, in the order they are learned (theory.md's Kodaly order) */
enum { RH_TA, RH_TITI, RH_REST, RH_TAA, RH_TIKA, RH_TITIKA, RH_TIKATI, RH_TIMKA, RH_SYNCO, RH_TRIOLA, RH_NCELLS };

/* an onset's flags */
#define RH_F_AFTER_REST 1u               /* the first sound after a rest */
#define RH_F_SIXTEENTH 2u                /* part of a TIKA-TIKA: the evenness check */
#define RH_F_SYNCO 4u                    /* off the beat with nothing on the next beat: the syncopation check */
#define RH_F_DOWNBEAT 8u                 /* on a beat */

typedef struct {
    uint8_t bpb;                         /* beats a bar: 2, 3, 4 */
    uint8_t nbeats;                      /* beats in the pattern */
    uint8_t ncells, cells[RH_MAX_BEATS]; /* the cells in order */
    uint8_t n;                           /* onsets */
    uint16_t on[RH_MAX_ON];              /* their ticks from the start */
    uint8_t flag[RH_MAX_ON];             /* RH_F_* */
    uint8_t group[RH_MAX_ON];            /* the cell each comes from (index into cells) */
} rh_pattern_t;

enum { RH_G_GOOD, RH_G_OK, RH_G_OFF, RH_G_MISS };
enum {
    RH_H_NONE,                           /* nothing tapped */
    RH_H_GOOD,
    RH_H_ALMOST,                         /* nothing in particular, just not all good */
    RH_H_MISSED,                         /* MISSED NOTES */
    RH_H_EXTRA,                          /* EXTRA TAPS */
    RH_H_SYNCO,                          /* SYNCOPATION SQUARED: an off-beat pulled toward a beat */
    RH_H_LATE_REST,                      /* LATE AFTER RESTS */
    RH_H_UNEVEN16,                       /* UNEVEN 16THS */
    RH_H_RUSHING,
    RH_H_DRAGGING,
    RH_H_N
};

typedef struct {
    uint8_t n;                           /* onsets (the pattern's) */
    int16_t err[RH_MAX_ON];              /* tap - onset, ms (0 when missed) */
    uint8_t grade[RH_MAX_ON];            /* RH_G_* */
    int8_t tap_of[RH_MAX_ON];            /* the tap that answered it, -1 none */
    uint8_t good, ok, off, missed, extra;
    int16_t mean;                        /* mean error of the answered onsets, ms (+ late) */
    uint8_t score;                       /* 0..100 */
    uint8_t hint;                        /* RH_H_* */
    uint16_t tol_good, tol_ok;           /* the windows used, ms */
} rh_result_t;

const char *rh_cell_token(int cell);     /* "q", "ee", ... (rh_parse's words) */
const char *rh_cell_name(int cell);      /* "TA", "TI-TI", ... */
int rh_cell_beats(int cell);             /* 1, or 2 (TA-A, SYN-CO-PA) */
int rh_cell_level(int cell);             /* the level it comes in at, 1..RH_LEVELS */

/* "q ee r h": the cells by token; 0 ok, -1 a word not known or too long. bpb only marks the bars */
int rh_parse(const char *s, int bpb, rh_pattern_t *p);
void rh_text(const rh_pattern_t *p, char *out);   /* back to tokens, at most RH_TEXT_MAX */
/* a pattern for a level (1..RH_LEVELS): bars of bpb beats, the level's newest cell more likely, never starting
 * with a rest, at least two sounds. *seed moves on (any nonzero start) */
void rh_generate(uint32_t *seed, int level, int bpb, int bars, rh_pattern_t *p);

uint32_t rh_tick_ms_x1000(int bpm);      /* one tick in microseconds */
int32_t rh_onset_ms(const rh_pattern_t *p, int i, int bpm);   /* onset i from the pattern's start, ms */
int rh_tol_good(int bpm);                /* +- ms that count as GOOD: min(35, 7 % of a beat) (OK: twice that) */

/* taps: ms from the pattern's start, in order (latency already taken off) */
void rh_score(const rh_pattern_t *p, int bpm, const int32_t *taps, int ntaps, rh_result_t *r);
const char *rh_hint_text(int hint);      /* "RUSHING", ... ("" for none) */
const char *rh_hint_more(int hint);      /* a second line: what to try */
