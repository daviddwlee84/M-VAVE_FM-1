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

/* ------------------------------------------------------------------ v0.3 --- */
/* PULSE: tap every beat with no click (after a count-in, or cold from the BPM alone). Each tap gets a beat index from
 * your own pulse (the median gap between taps), so a tempo that wanders off the click is still counted beat by beat.
 * A straight line through (index, time) gives your tempo; its two halves the trend; what is left over, the jitter */
#define RH_PULSE_MAX 64                  /* beats: 16 bars of 4/4 */
#define RH_PULSE_TAPS 96
#define RH_HIST 11                       /* error bins, each one GOOD window wide, the middle one centred on 0 */
enum {
    RH_P_NONE, RH_P_GOOD, RH_P_CLOSE, RH_P_MISSED, RH_P_EXTRA, RH_P_SPEEDING, RH_P_SLOWING, RH_P_RUSHING,
    RH_P_DRAGGING, RH_P_UNSTEADY, RH_P_N
};
typedef struct {
    uint8_t n;                           /* beats asked for */
    uint8_t answered, missed, extra, good, ok, off;
    int16_t err[RH_PULSE_MAX];           /* tap - the beat on the target grid, ms (0 when missed) */
    int8_t tap_of[RH_PULSE_MAX];         /* -1 missed */
    uint8_t grade[RH_PULSE_MAX];         /* RH_G_* */
    int16_t mean;                        /* mean error, ms (+ late) */
    int16_t bpm_x10;                     /* your tempo from the fit, BPM x10 (0: too few taps) */
    int16_t tempo_x10;                   /* its error, % x10 (+ faster than asked) */
    int16_t trend_x10;                   /* second half's tempo - first half's, % x10 (+ speeding up) */
    int16_t end_ms;                      /* where the fit ends against the target grid on the last beat, ms */
    int16_t jitter;                      /* SD of the taps around the fit, ms */
    uint8_t hist[RH_HIST];
    uint8_t score, hint;                 /* 0..100, RH_P_* */
    uint16_t tol_good, tol_ok;
} rh_pulse_t;
/* taps: ms from beat 0 of the grid (cold: from the first tap), in order, latency taken off */
void rh_pulse(int bpm, int beats, const int32_t *taps, int ntaps, rh_pulse_t *r);
const char *rh_pulse_hint(int hint);
const char *rh_pulse_more(int hint);

/* JUDGE: was it faster or slower, rushing or dragging. A 2-down 1-up staircase on how big the change is, with
 * geometric steps; it ends after RH_ST_REVS reversals or RH_ST_TRIALS trials, and the threshold is the geometric
 * mean of the last reversals (about 71 % correct) */
enum { RH_J_TEMPO, RH_J_FEEL, RH_J_DRIFT, RH_J_N };
#define RH_ST_TRIALS 40
#define RH_ST_REVS 10
typedef struct {
    uint8_t type, trials, run, revs, dir, done;   /* dir: 1 last step down, 2 up */
    float d;                             /* the change now: % (TEMPO, DRIFT), ms (FEEL) */
    float rev[RH_ST_REVS];
    uint16_t hist_x10[RH_ST_TRIALS];     /* d of each trial, x10 */
    uint8_t right[RH_ST_TRIALS];         /* trial i answered right */
} rh_stair_t;
void rh_stair_init(rh_stair_t *s, int type);
int rh_stair_answer(rh_stair_t *s, int correct);   /* 1 when the run is done */
float rh_stair_threshold(const rh_stair_t *s);
const char *rh_judge_name(int type);     /* "TEMPO", ... */
const char *rh_judge_unit(int type);     /* "%", "MS" */
float rh_judge_min(int type);            /* the smallest step the staircase goes to */

/* the sounds of a trial (and of the quiz's HEAR and METER): times in microseconds from the start, in order */
enum { RH_S_ACCENT = 1, RH_S_CLICK = 2, RH_S_SOFT = 4, RH_S_SUB = 8, RH_S_NOTE = 16, RH_S_LOW = 32 };
#define RH_SEQ_MAX 64
typedef struct {
    uint8_t n;
    uint32_t us[RH_SEQ_MAX];
    uint8_t ev[RH_SEQ_MAX];              /* RH_S_* */
} rh_seq_t;
/* sign +1 faster / rushing, -1 slower / dragging; d as in rh_stair_t */
void rh_judge_seq(int type, int bpm, int bpb, float d, int sign, rh_seq_t *q);
void rh_clicks_seq(int bpm, int bpb, int bars, rh_seq_t *q);   /* plain clicks, a downbeat accent */

/* QUIZ: tempo markings (en.wikipedia "Tempo", basic tempo markings, read 2026-11-19), tempo changes, and meters by
 * ear. Four choices; a wrong choice is never also right: a BPM inside the asked term's range, a term whose range holds
 * the asked BPM, a change term of the same kind, a meter that sounds the same are all kept out */
enum { RH_Q_TERM_BPM, RH_Q_BPM_TERM, RH_Q_HEAR, RH_Q_CHANGE, RH_Q_METER, RH_Q_N };
#define RH_NTERMS 16
#define RH_NCHANGES 11
#define RH_NMETERS 6
typedef struct {
    const char *name;
    uint16_t lo, hi;                     /* BPM; lo 0: "and under", hi 0: "and over" */
    uint8_t core;                        /* in level 1 */
} rh_term_t;
typedef struct {
    const char *name, *meaning;
    uint8_t kind;                        /* terms of one kind never stand against each other */
} rh_change_t;
typedef struct {
    const char *name;                    /* "6/8" */
    uint8_t eighths, ngroups, group[5];  /* a bar in eighths, grouped (3+3, 2+2+3) */
    uint8_t level;                       /* 1, or 2 (5/4 at 1 too) */
} rh_meter_t;
extern const rh_term_t RH_TERMS[RH_NTERMS];
extern const rh_change_t RH_CHANGES[RH_NCHANGES];
extern const rh_meter_t RH_METERS[RH_NMETERS];
int rh_term_has(int term, int bpm);      /* bpm inside the term's range */
int rh_term_draw_bpm(uint32_t *seed, int term);   /* a BPM inside it (open ends kept to 18..24, 200..230) */
void rh_term_range(int term, char *out); /* "40-66", "24-", "200+" */

typedef struct {
    uint8_t type, level;
    uint8_t ask;                         /* the term, change or meter asked about */
    uint16_t bpm;                        /* BPM_TERM, HEAR: the BPM; METER: the quarter's tempo */
    uint8_t opt[4];                      /* term / change / meter index of each choice (TERM_BPM: unused) */
    uint16_t val[4];                     /* TERM_BPM: the BPM of each choice */
    uint8_t answer;                      /* 0..3 */
    uint8_t dir;                         /* CHANGE: 0 term -> meaning, 1 meaning -> term */
} rh_quiz_t;
/* weight: per item (term, change or meter, by type) how much more often it comes (misses), 0 = 1; prev: avoid it */
void rh_quiz_make(uint32_t *seed, int type, int level, const uint8_t *weight, int prev, rh_quiz_t *q);
void rh_meter_seq(int meter, int bpm, int level, int bars, rh_seq_t *q);
int rh_quiz_items(int type, int level);  /* how many items a type has at a level */
const char *rh_quiz_name(int type);      /* "TERM>BPM", ... */
