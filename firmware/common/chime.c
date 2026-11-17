/* SPDX-License-Identifier: MIT */
/* chime: see chime.h. One voice: a sine carrier, a sine modulator at 2x, the index falling with the note's
 * envelope (a bell). Needs fastmath.h (fm_sinf, fm_exp2f) from fm1-base, in scope in the unity build. */
#include "chime.h"

#define CHIME_MAX 6u
#define CHIME_SR 44100.0f

static volatile uint8_t ch_note[CHIME_MAX];
static volatile uint16_t ch_ms[CHIME_MAX];
static volatile uint32_t ch_n, ch_i;      /* notes queued, the one playing (ch_i == ch_n: silent) */
static uint32_t ch_left;                 /* samples left of the note playing */
static float ch_pc, ch_pm, ch_env, ch_inc;

static const uint8_t FIG_NOTES[CHIME_N][3] = {
    {76, 81, 0},                         /* UP: E5 A5, someone needs you */
    {72, 76, 79},                        /* DONE: C5 E5 G5 */
    {57, 53, 0},                         /* ERROR: A3 F3 */
    {84, 0, 0},                          /* TICK */
};
static const uint16_t FIG_MS[CHIME_N][3] = {{110, 220, 0}, {90, 90, 260}, {180, 320, 0}, {40, 0, 0}};

void chime_notes(const uint8_t *notes, const uint16_t *ms, uint32_t n)
{
    uint32_t i;
    if (ch_i < ch_n)
        return;                          /* one figure at a time: no pile-up when many agents change at once */
    if (n > CHIME_MAX)
        n = CHIME_MAX;
    ch_n = 0;                            /* silent while the queue is refilled (the render reads it) */
    ch_i = 0;
    for (i = 0; i < n; i++) {
        ch_note[i] = notes[i];
        ch_ms[i] = ms[i];
    }
    ch_left = 0;
    ch_n = n;
}

void chime_play(int f)
{
    uint32_t n = 0;
    if (f < 0 || f >= CHIME_N)
        return;
    while (n < 3u && FIG_NOTES[f][n])
        n++;
    chime_notes(FIG_NOTES[f], FIG_MS[f], n);
}

int chime_busy(void) { return ch_i < ch_n; }
void chime_stop(void) { ch_i = ch_n; }

void chime_render(int32_t *lr, uint32_t n, float gain)
{
    uint32_t i;
    for (i = 0; i < n; i++) {
        float m, s;
        if (!ch_left) {
            if (ch_i >= ch_n) {
                ch_env = 0.0f;
                return;
            }
            ch_inc = 440.0f * fm_exp2f(((float)ch_note[ch_i] - 69.0f) / 12.0f) / CHIME_SR;
            ch_left = (uint32_t)ch_ms[ch_i] * 44u;
            ch_env = 1.0f;
            ch_i++;
        }
        ch_left--;
        ch_env *= 0.99985f;              /* ~150 ms to a third */
        ch_pc += ch_inc;
        if (ch_pc >= 1.0f)
            ch_pc -= 1.0f;
        ch_pm += ch_inc * 2.0f;
        if (ch_pm >= 1.0f)
            ch_pm -= 1.0f;
        m = fm_sinf(ch_pm * 6.2831853f) * 1.5f * ch_env;
        s = fm_sinf((ch_pc + m * 0.159f) * 6.2831853f) * ch_env * 0.2f * gain;
        if (ch_left < 220u)              /* 5 ms out: no click at the note's end */
            s *= (float)ch_left / 220.0f;
        lr[2 * i] += (int32_t)s;
        lr[2 * i + 1] += (int32_t)s;
    }
}
