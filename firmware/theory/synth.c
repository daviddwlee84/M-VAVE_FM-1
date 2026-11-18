/* SPDX-License-Identifier: GPL-3.0-or-later */
/* synth: see synth.h. Needs fastmath.h (fm_exp2f, fm_sin_turns, fm_tanhf) included before it. */
#include "synth.h"

#define SY_RING 64u                      /* events between two blocks, at most */
#define SY_FS 44100.0f

enum { SY_FREE, SY_HELD, SY_REL };
enum { SY_EV_ON, SY_EV_OFF, SY_EV_ALL, SY_EV_HOLD };

typedef struct {
    uint8_t type, note, vel;
} sy_ev_t;

typedef struct {
    float inc, pc, pm;                   /* phase increment and phases, in turns */
    float amp;                           /* the note's loudness, from its velocity */
    float env;                           /* the slow envelope: held, then released */
    float bright;                        /* the fast one: the attack's modulation depth */
    float att;                           /* 0..1 over the first 3 ms (no click) */
    float floor;                         /* held: the envelope settles here (0: it dies away like a piano's) */
    float low;                           /* below E4: kept modulation, so the small speaker hears its overtones */
    uint32_t age;                        /* when it started: the oldest is taken first */
    uint8_t note, stage;
} sy_voice_t;

static sy_ev_t sy_ring[SY_RING];
static volatile uint32_t sy_w, sy_r;     /* sy_w: the main loop's; sy_r: the audio interrupt's */
static float sy_inc[128];
static sy_voice_t sy_v[SY_VOICES];
static uint32_t sy_clock;
static volatile int sy_count;
static volatile uint8_t sy_silence;      /* synth_stop asked: clear everything at the next block */

/* the score: two buffers, the main loop fills the one not playing, then names it */
static sy_note_t sy_sc[2][SY_SCORE_MAX];
static uint8_t sy_sc_n[2];
static volatile uint8_t sy_sc_go, sy_sc_seen, sy_sc_buf, sy_sc_stop;
static volatile uint8_t sy_sc_busy;
static uint8_t sy_sc_play;               /* the audio interrupt's: the buffer playing */
static uint32_t sy_sc_pos;               /* samples since it started */
static uint8_t sy_sc_state[SY_SCORE_MAX];   /* 0 waiting, 1 sounding, 2 done */
static uint8_t sy_sc_active;

void synth_init(void)
{
    int i;
    for (i = 0; i < 128; i++) sy_inc[i] = 440.0f * fm_exp2f((float)(i - 69) / 12.0f) / SY_FS;
}

static void sy_push(uint8_t type, int note, int vel)
{
    uint32_t w = sy_w;
    if (w - sy_r >= SY_RING) return;     /* full: a burst this big is dropped rather than block */
    sy_ring[w % SY_RING].type = type;
    sy_ring[w % SY_RING].note = (uint8_t)(note & 127);
    sy_ring[w % SY_RING].vel = (uint8_t)(vel < 1 ? 1 : vel > 127 ? 127 : vel);
    sy_w = w + 1u;
}

void synth_note_on(int note, int vel) { if (note >= 0 && note < 128) sy_push(SY_EV_ON, note, vel); }
void synth_note_hold(int note, int vel) { if (note >= 0 && note < 128) sy_push(SY_EV_HOLD, note, vel); }
void synth_note_off(int note) { if (note >= 0 && note < 128) sy_push(SY_EV_OFF, note, 0); }
void synth_all_off(void) { sy_push(SY_EV_ALL, 0, 0); }
void synth_stop(void) { sy_silence = 1; sy_r = sy_w; sy_sc_stop = 1; }

void synth_score(const sy_note_t *notes, uint32_t n)
{
    uint8_t b = (uint8_t)(sy_sc_buf ^ 1u);
    uint32_t i;
    if (n > SY_SCORE_MAX) n = SY_SCORE_MAX;
    for (i = 0; i < n; i++) sy_sc[b][i] = notes[i];
    sy_sc_n[b] = (uint8_t)n;
    sy_sc_buf = b;
    sy_sc_busy = n > 0;
    sy_sc_go = (uint8_t)(sy_sc_go + 1u);
}

void synth_score_stop(void) { sy_sc_stop = 1; sy_sc_busy = 0; }
int synth_score_busy(void) { return sy_sc_busy; }
int synth_active(void) { return sy_count; }

static void sy_start(int note, int vel, float floor)
{
    sy_voice_t *v = 0;
    uint32_t i, oldest = 0;
    float x = (float)vel / 127.0f;
    for (i = 0; i < SY_VOICES && !v; i++)            /* the same note again: that voice, struck anew */
        if (sy_v[i].stage != SY_FREE && sy_v[i].note == note) v = &sy_v[i];
    for (i = 0; i < SY_VOICES && !v; i++)
        if (sy_v[i].stage == SY_FREE) v = &sy_v[i];
    if (!v) {                                        /* all busy: the oldest released one, else the oldest */
        for (i = 1; i < SY_VOICES; i++) {
            int rel_i = sy_v[i].stage == SY_REL, rel_o = sy_v[oldest].stage == SY_REL;
            if (rel_i > rel_o || (rel_i == rel_o && sy_v[i].age < sy_v[oldest].age)) oldest = i;
        }
        v = &sy_v[oldest];
    }
    v->inc = sy_inc[note];
    v->pc = v->pm = 0.0f;
    v->amp = 0.035f + 0.11f * x * x;                 /* soft keys quiet, hard ones up to ~0.15 */
    v->low = note < 64 ? (float)(64 - note) / 24.0f : 0.0f;   /* 0 at E4, 1 at E2 */
    if (v->low > 1.2f) v->low = 1.2f;
    v->amp *= 1.0f + 0.6f * v->low;                  /* the speaker hardly plays them: louder, and brighter below */
    v->env = 1.0f;
    v->bright = 0.6f + 0.9f * x;                     /* harder: brighter */
    v->att = 0.0f;
    v->floor = floor;
    v->age = sy_clock++;
    v->note = (uint8_t)note;
    v->stage = SY_HELD;
}

static void sy_release(int note)
{
    uint32_t i;
    for (i = 0; i < SY_VOICES; i++)
        if (sy_v[i].stage == SY_HELD && sy_v[i].note == note) sy_v[i].stage = SY_REL;
}

/* the score's notes that start or end in this block (main loop: synth_score; here: the audio interrupt) */
static void sy_score_step(uint32_t n)
{
    uint32_t i, left = 0;
    if (sy_sc_stop) {
        sy_sc_stop = 0;
        if (sy_sc_active)
            for (i = 0; i < sy_sc_n[sy_sc_play]; i++)
                if (sy_sc_state[i] == 1u) sy_release(sy_sc[sy_sc_play][i].note);
        sy_sc_active = 0;                /* (a score named since starts below) */
    }
    if (sy_sc_go != sy_sc_seen) {        /* a new score: the old one's notes let go */
        if (sy_sc_active)
            for (i = 0; i < sy_sc_n[sy_sc_play]; i++)
                if (sy_sc_state[i] == 1u) sy_release(sy_sc[sy_sc_play][i].note);
        sy_sc_seen = sy_sc_go;
        sy_sc_play = sy_sc_buf;
        sy_sc_pos = 0;
        sy_sc_active = 1;
        for (i = 0; i < SY_SCORE_MAX; i++) sy_sc_state[i] = 0;
    }
    if (!sy_sc_active) return;
    for (i = 0; i < sy_sc_n[sy_sc_play]; i++) {
        const sy_note_t *e = &sy_sc[sy_sc_play][i];
        uint32_t on = (uint32_t)e->at * 44u + (uint32_t)e->at / 10u, off = on + (uint32_t)e->dur * 44u;
        if (sy_sc_state[i] == 0u && on <= sy_sc_pos + n) {
            sy_start(e->note, e->vel, 0.0f);
            sy_sc_state[i] = 1;
        }
        if (sy_sc_state[i] == 1u && off <= sy_sc_pos + n) {
            sy_release(e->note);
            sy_sc_state[i] = 2;
        }
        if (sy_sc_state[i] != 2u) left++;
    }
    sy_sc_pos += n;
    if (!left) {
        sy_sc_active = 0;
        sy_sc_busy = 0;
    }
}

static void sy_events(void)
{
    while (sy_r != sy_w) {
        sy_ev_t e = sy_ring[sy_r % SY_RING];
        uint32_t i;
        sy_r = sy_r + 1u;
        if (e.type == SY_EV_ON || e.type == SY_EV_HOLD) sy_start(e.note, e.vel, e.type == SY_EV_HOLD ? 0.3f : 0.0f);
        else
            for (i = 0; i < SY_VOICES; i++)
                if (sy_v[i].stage == SY_HELD && (e.type == SY_EV_ALL || sy_v[i].note == e.note)) sy_v[i].stage = SY_REL;
    }
}

void synth_render(int32_t *lr, uint32_t n, float gain)
{
    uint32_t i, k;
    int count = 0;
    if (sy_silence) {
        for (k = 0; k < SY_VOICES; k++) sy_v[k].stage = SY_FREE;
        sy_silence = 0;
    }
    sy_events();
    sy_score_step(n);
    for (i = 0; i < 2u * n; i++) lr[i] = 0;
    for (k = 0; k < SY_VOICES; k++) {
        sy_voice_t *v = &sy_v[k];
        float out;
        if (v->stage == SY_FREE) continue;
        count++;
        for (i = 0; i < n; i++) {
            float m, s;
            if (v->att < 1.0f) v->att += 1.0f / 132.0f;              /* 3 ms */
            if (v->stage == SY_HELD) v->env = v->floor + (v->env - v->floor) * 0.99998f;   /* ~1.1 s to 1/e */
            else v->env *= 0.99955f;                                 /* released: ~50 ms */
            v->bright *= 0.99985f;                                   /* the bite fades in ~150 ms */
            m = fm_sin_turns(v->pm) * (v->bright + 0.25f + 0.9f * v->low) * v->env;
            s = fm_sin_turns(v->pc + m * 0.159f) * v->env * v->att * v->amp;
            v->pc += v->inc;
            v->pm += v->inc;
            if (v->pc >= 1.0f) v->pc -= 1.0f;
            if (v->pm >= 1.0f) v->pm -= 1.0f;
            out = (float)lr[2 * i] + s * 8388608.0f;                 /* summed in the buffer, limited below */
            lr[2 * i] = (int32_t)out;
        }
        if (v->env < 0.0005f) v->stage = SY_FREE;
    }
    for (i = 0; i < n; i++) {                                        /* a soft limit, then MASTER */
        float s = fm_tanhf((float)lr[2 * i] * (1.0f / 8388608.0f)) * gain;
        lr[2 * i] = (int32_t)s;
        lr[2 * i + 1] = (int32_t)s;
    }
    sy_count = count;
}
