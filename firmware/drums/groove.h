/* SPDX-License-Identifier: GPL-3.0-or-later */
/* groove: DRUMS' player, platform-free (no float, no 64-bit division, no libc beyond <stdint.h>).
 *
 * A style is a few parts (MAIN A, MAIN B, FILL A, FILL B, ENDING), each one bar of steps per lane, written as text
 * (styles.h). The player walks a bar at a time: the part a bar plays is decided at its first tick, from the requests
 * that came in (a fill, a MAIN change, a transition held, the ending) and FILL AT:
 *   NOW     the fill starts on the next step and plays to the bar's end (the fill's tail)
 *   BAR     the next bar is the fill
 *   PHRASE  the fill waits for the last bar of the phrase (4, 8 or 16 bars); pressed in that bar, it starts now
 * A bar after a fill opens with a crash. Time is ticks, GV_TPB a beat; the app calls groove_tick for every tick.
 * Design: docs/dev/drums.md. */
#pragma once
#include <stdint.h>

#define GV_TPB 96                        /* ticks a beat: 16ths are 24, triplet 8ths 32; MIDI clock is every 4th */
#define GV_MAX_EV 16                     /* events one tick can give */

enum { L_KICK, L_SNARE, L_RIM, L_CLAP, L_CHH, L_OHH, L_RIDE, L_CRASH, L_TOMH, L_TOMM, L_TOML, L_BELL, L_SHAKE,
       GV_LANES };
enum { P_MAIN_A, P_MAIN_B, P_FILL_A, P_FILL_B, P_END, GV_PARTS };
enum { FA_NOW, FA_BAR, FA_PHRASE, FA_N };
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
    uint8_t beats, spb, bpm, steps;
    uint16_t off[GV_PARTS], n[GV_PARTS]; /* each part's events in the pool, sorted by step */
} gv_style_t;

typedef struct {
    gv_ev_t e[GV_MAX_EV];
    uint8_t n;
} gv_out_t;

typedef struct {
    const gv_style_t *st;
    uint8_t state;                       /* GS_* */
    uint8_t sec;                         /* 0 MAIN A, 1 MAIN B */
    uint8_t part;                        /* P_*: what this bar plays */
    uint8_t fill_from;                   /* a fill started mid-bar (NOW): its steps from this one */
    uint8_t crash;                       /* the next bar opens with a crash */
    uint32_t bar_start, bar_ticks;       /* tick of this bar's start, its length */
    uint32_t bars;                       /* bars played since the start */
    uint16_t bar_in_sec;                 /* bars since this section began (the phrase counts from it) */
    uint32_t pos;                        /* the last tick's place in the bar */
    /* requests (the app writes them under its lock) */
    uint8_t req_fill, req_sec, req_end, switch_now, hold;
    const gv_style_t *req_style;
    /* settings */
    uint8_t fill_at, phrase, auto_fill, swing;   /* swing: ticks the off 16ths are late (0..12) */
    uint16_t mutes;                      /* bit per lane: the parts leave it out (the pads still play) */
} groove_t;

/* the styles: parse every source once (returns the number parsed, or -(index+1) of the first that fails) */
int gv_load(const gv_style_src_t *src, int n);
int gv_count(void);
const gv_style_t *gv_style(int i);
const gv_ev_t *gv_events(const gv_style_t *st, int part, uint32_t *n);
const char *gv_error(void);              /* why gv_load failed */

void groove_init(groove_t *g, const gv_style_t *st);
void groove_start(groove_t *g, int sec);   /* from tick 0 */
void groove_stop(groove_t *g);
int groove_tick(groove_t *g, uint32_t t, gv_out_t *out);   /* the events of tick t (t counts up by one) */

/* requests: what the player answers depends on FILL AT, AUTO FILL and the phrase */
void groove_fill(groove_t *g);
void groove_main(groove_t *g, int sec);  /* the section it plays now: a fill that returns to it (as a PSR's) */
void groove_hold(groove_t *g, int on);   /* a transition: the fill loops while held, the other MAIN after */
void groove_end(groove_t *g);
void groove_set_style(groove_t *g, const gv_style_t *st);   /* stopped: now; playing: at the next bar */

int groove_step(const groove_t *g);      /* the step playing now */
int groove_last_of_phrase(const groove_t *g);
int groove_fill_pending(const groove_t *g);   /* a fill is coming (this bar or a later one) */
const char *gv_lane_name(int lane);
