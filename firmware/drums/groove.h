/* SPDX-License-Identifier: GPL-3.0-or-later */
/* groove: DRUMS' player, platform-free (no float, no 64-bit division, no libc beyond <stdint.h>).
 *
 * A style is a set of parts written as text (styles.h): four MAINs (A-D), a fill for each, a tom fill, a two-bar
 * fill, an intro, a break, a stop-time hit and an ending. A part is one bar, or two (the two-bar fill, an intro).
 * The player walks a bar at a time: the part a bar plays is decided at its first tick, from the requests that came
 * in (a fill, a MAIN change, a transition held, a break, the ending) and the settings:
 *   FILL AT   NOW     the fill starts on the next step and plays to the bar's end (the fill's tail)
 *             BAR     the next bar is the fill
 *             PHRASE  the fill waits for the end of the phrase (4, 8 or 16 bars); pressed in its last bar, now
 *   FILL LEN  HALF    the fill's second half only (the main plays the first)
 *             ONE     one bar
 *             TWO     the two-bar fill (PHRASE puts it on the phrase's last two bars)
 * A bar after a fill (or the intro, a break) opens with a crash. Time is ticks, GV_TPB a beat; the app calls
 * groove_tick for every tick. Design: docs/dev/drums.md. */
#pragma once
#include <stdint.h>

#define GV_TPB 96                        /* ticks a beat: 16ths are 24, triplet 8ths 32; MIDI clock is every 4th */
#define GV_MAX_EV 16                     /* events one tick can give */
#define GV_MAINS 4

enum { L_KICK, L_SNARE, L_RIM, L_CLAP, L_CHH, L_OHH, L_RIDE, L_CRASH, L_TOMH, L_TOMM, L_TOML, L_BELL, L_SHAKE,
       GV_LANES };
/* the parts; their letters in styles.h: A B C D, a b c d, t, w, I, K, P, E */
enum { P_MAIN_A, P_MAIN_B, P_MAIN_C, P_MAIN_D, P_FILL_A, P_FILL_B, P_FILL_C, P_FILL_D, P_TOM, P_TWO, P_INTRO,
       P_BREAK, P_STOP, P_END, GV_PARTS };
enum { FA_NOW, FA_BAR, FA_PHRASE, FA_N };
enum { FL_HALF, FL_ONE, FL_TWO, FL_N };
enum { FK_SEC, FK_TOM };                 /* which fill: the MAIN's own, or the tom fill */
enum { GS_STOP, GS_PLAY, GS_END };
#define GV_NONE 0xFFu

typedef struct {
    uint8_t step, lane, vel;             /* vel 1..9 */
} gv_ev_t;

typedef struct {
    const char *name;
    uint8_t beats, spb, bpm;             /* beats a bar, steps a beat (4: 16ths, 3: triplets), the style's tempo */
    const char *src;                     /* the parts as text (styles.h) */
} gv_style_src_t;

typedef struct {
    const char *name;
    uint8_t beats, spb, bpm, steps;      /* steps: a bar's */
    uint16_t off[GV_PARTS], n[GV_PARTS]; /* each part's events in the pool, sorted by step */
    uint8_t bars[GV_PARTS];              /* each part's length: 1 or 2 bars */
} gv_style_t;

typedef struct {
    gv_ev_t e[GV_MAX_EV];
    uint8_t n;
} gv_out_t;

typedef struct {
    const gv_style_t *st;
    uint8_t state;                       /* GS_* */
    uint8_t sec;                         /* the MAIN: 0..3 (A..D) */
    uint8_t part;                        /* P_*: what this bar plays */
    uint8_t part_bar;                    /* a two-bar part: which of its bars */
    uint8_t fill_from;                   /* a fill's steps from this one (NOW, HALF); the MAIN before it */
    uint8_t crash;                       /* this bar opens with a crash */
    uint32_t bar_start, bar_ticks;       /* tick of this bar's start, its length */
    uint32_t bars;                       /* bars played since the start */
    uint16_t bar_in_sec;                 /* bars since this MAIN began (the phrase counts from it) */
    uint32_t pos;                        /* the last tick's place in the bar */
    /* requests (the app writes them under its lock) */
    uint8_t req_fill, req_kind, req_len, req_sec, req_end, req_break, req_stop, switch_now, hold;
    const gv_style_t *req_style;
    /* settings */
    uint8_t fill_at, fill_len, phrase, auto_fill, swing;   /* swing: ticks the off 16ths are late (0..12) */
    uint16_t mutes;                      /* bit per lane: the parts leave it out (the pads still play) */
} groove_t;

/* the styles: parse every source once (returns the number parsed, or -(index+1) of the first that fails) */
int gv_load(const gv_style_src_t *src, int n);
int gv_count(void);
const gv_style_t *gv_style(int i);
const gv_ev_t *gv_events(const gv_style_t *st, int part, uint32_t *n);
const char *gv_error(void);              /* why gv_load failed */
const char *gv_lane_name(int lane);
const char *gv_part_name(int part);
int gv_is_fill(int part);                /* a MAIN's fill, the tom fill or the two-bar fill */

void groove_init(groove_t *g, const gv_style_t *st);
void groove_start(groove_t *g, int sec);         /* from tick 0, in that MAIN */
void groove_start_intro(groove_t *g, int sec);   /* the intro first, then that MAIN */
void groove_stop(groove_t *g);
int groove_tick(groove_t *g, uint32_t t, gv_out_t *out);   /* the events of tick t (t counts up by one) */

/* requests: what the player answers depends on FILL AT, FILL LEN, AUTO FILL and the phrase */
void groove_fill(groove_t *g);           /* the MAIN's fill at FILL LEN */
void groove_fill_ex(groove_t *g, int kind, int len);
void groove_main(groove_t *g, int sec);  /* the MAIN it plays: a fill that returns to it (as a PSR's) */
void groove_hold(groove_t *g, int on);   /* a transition: the fill loops while held, the next MAIN after */
void groove_break(groove_t *g);          /* the next bar: the break, then the MAIN again */
void groove_stop_time(groove_t *g);      /* the next bar: a hit on the one, then silence; then the MAIN */
void groove_end(groove_t *g);
void groove_set_style(groove_t *g, const gv_style_t *st);   /* stopped: now; playing: at the next bar */

int groove_step(const groove_t *g);      /* the step playing now (in the bar) */
int groove_last_of_phrase(const groove_t *g);
int groove_fill_pending(const groove_t *g);   /* a fill is coming (this bar or a later one) */
