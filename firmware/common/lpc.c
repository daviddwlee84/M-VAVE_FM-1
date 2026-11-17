/* SPDX-License-Identifier: MIT */
/* lpc: TMS5220-style LPC speech decoder. See lpc.h. */
#include "lpc.h"

#define LPC_RATE 8000u

static const uint8_t lpc_kbits[10] = {5, 5, 4, 4, 4, 4, 4, 3, 3, 3};
static const int16_t lpc_energy[16] = {0, 1, 2, 3, 4, 6, 8, 11, 16, 23, 33, 47, 63, 85, 114, 0};
static const uint8_t lpc_pitch[64] = {
    0, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
    30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 44, 46, 48,
    50, 52, 53, 56, 58, 60, 62, 65, 68, 70, 72, 76, 78, 80, 84, 86,
    91, 94, 98, 101, 105, 109, 114, 118, 122, 127, 132, 137, 142, 148, 153, 159};
static const int16_t lpc_k1[32] = {
    -501, -498, -497, -495, -493, -491, -488, -482, -478, -474, -469, -464, -459, -452, -445, -437,
    -412, -380, -339, -288, -227, -158, -81, -1, 80, 157, 226, 287, 337, 379, 411, 436};
static const int16_t lpc_k2[32] = {
    -328, -303, -274, -244, -211, -175, -138, -99, -59, -18, 24, 64, 105, 143, 180, 215,
    248, 278, 306, 331, 354, 374, 392, 408, 422, 435, 445, 455, 463, 470, 476, 506};
static const int16_t lpc_k3_7[5][16] = {
    {-441, -387, -333, -279, -225, -171, -117, -63, -9, 45, 98, 152, 206, 260, 314, 368},
    {-328, -273, -217, -161, -106, -50, 5, 61, 116, 172, 228, 283, 339, 394, 450, 506},
    {-328, -282, -235, -189, -142, -96, -50, -3, 43, 90, 136, 182, 229, 275, 322, 368},
    {-256, -212, -168, -123, -79, -35, 10, 54, 98, 143, 187, 232, 276, 320, 365, 409},
    {-308, -260, -212, -164, -117, -69, -21, 27, 75, 122, 170, 218, 266, 314, 361, 409}};
static const int16_t lpc_k8_10[3][8] = {
    {-256, -161, -66, 29, 124, 219, 314, 409},
    {-256, -176, -96, -15, 65, 146, 226, 307},
    {-205, -132, -59, 14, 87, 160, 234, 307}};
static const int8_t lpc_chirp[52] = {
    0x00, 0x03, 0x0f, 0x28, 0x4c, 0x6c, 0x71, 0x50, 0x25, 0x26, 0x4c, 0x44, 0x1a, 0x32, 0x3b, 0x13,
    0x37, 0x1a, 0x25, 0x1f, 0x1d};
static const uint8_t lpc_interp[8] = {0, 3, 3, 3, 2, 2, 1, 1};

static int32_t lpc_bits(lpc_t *s, uint32_t k)
{
    int32_t v = 0;
    if (s->bit + k > s->n * 8u)
        return -1;
    while (k--) {
        v = (v << 1) | ((s->p[s->bit >> 3] >> (s->bit & 7u)) & 1);
        s->bit++;
    }
    return v;
}

static int32_t lpc_k(uint32_t i, int32_t code)
{
    if (i == 0) return lpc_k1[code];
    if (i == 1) return lpc_k2[code];
    if (i < 7) return lpc_k3_7[i - 2][code];
    return lpc_k8_10[i - 7][code];
}

/* the next frame into tgt; 0 = the stream ended (stop frame or no more bits) */
static int lpc_frame(lpc_t *s)
{
    int32_t e = lpc_bits(s, 4), rep, per, i;
    int was_voiced = s->tgt[1] > 0, was_silent = s->tgt[0] == 0;
    if (e < 0 || e == 15)
        return 0;
    if (e == 0) {
        s->tgt[0] = 0;
        s->jump = 0;
        return 1;
    }
    rep = lpc_bits(s, 1);
    per = lpc_bits(s, 6);
    if (rep < 0 || per < 0)
        return 0;
    s->tgt[0] = lpc_energy[e];
    s->tgt[1] = lpc_pitch[per];
    if (!rep) {
        uint32_t nk = per ? 10u : 4u;
        for (i = 0; i < (int32_t)nk; i++) {
            int32_t c = lpc_bits(s, lpc_kbits[i]);
            if (c < 0)
                return 0;
            s->tgt[2 + i] = lpc_k((uint32_t)i, c);
        }
        if (!per)
            for (i = 4; i < 10; i++)
                s->tgt[2 + i] = 0;
    }
    /* as the chip: no gliding across a change of voicing or out of silence */
    s->jump = was_silent || (was_voiced != (per > 0));
    return 1;
}

void lpc_begin(lpc_t *s, const uint8_t *data, uint32_t n, uint32_t frame_ms)
{
    uint32_t i;
    s->p = data;
    s->n = n;
    s->bit = 0;
    s->spf = (uint16_t)(LPC_RATE * (frame_ms ? frame_ms : 25u) / 1000u);
    s->step = (uint16_t)(s->spf / 8u ? s->spf / 8u : 1u);
    s->pos = 0;
    for (i = 0; i < 12; i++)
        s->cur[i] = s->tgt[i] = 0;
    for (i = 0; i < 10; i++)
        s->x[i] = 0;
    s->pc = 0;
    s->rng = 0x1FFF;
    s->done = 0;
    s->started = 0;
    s->jump = 1;
}

uint32_t lpc_render(lpc_t *s, int16_t *out, uint32_t n)
{
    uint32_t w = 0, i;
    while (w < n && !s->done) {
        int32_t u, k;
        if (s->pos == 0) {
            if (!lpc_frame(s)) {
                s->done = 1;
                break;
            }
            if (!s->started || s->jump)
                for (i = 0; i < 12; i++)
                    s->cur[i] = s->tgt[i];
            s->started = 1;
        } else if (s->pos % s->step == 0) {
            uint32_t ip = s->pos / s->step;
            if (ip < 8)
                for (i = 0; i < 12; i++)
                    s->cur[i] += (s->tgt[i] - s->cur[i]) >> lpc_interp[ip];
        }
        /* excitation */
        if (s->cur[0] == 0) {
            u = 0;
        } else if (s->cur[1] > 0) {
            if (++s->pc >= s->cur[1])
                s->pc = 0;
            u = s->pc < 52 ? (lpc_chirp[s->pc] * s->cur[0]) >> 6 : 0;
        } else {
            s->rng = (uint16_t)((s->rng >> 1) ^ ((s->rng & 1u) ? 0xB800u : 0u));
            u = (s->rng & 1u) ? s->cur[0] : -s->cur[0];
        }
        /* lattice: forward path u[10] (the excitation) down to u[0], then the backward one. K(k+1) is cur[2 + k] */
        {
            int32_t uu[11];
            uu[10] = u;
            for (k = 9; k >= 0; k--)
                uu[k] = uu[k + 1] - ((s->cur[2 + k] * s->x[k]) >> 9);
            if (uu[0] > 511) uu[0] = 511;
            if (uu[0] < -512) uu[0] = -512;
            for (k = 9; k >= 1; k--)
                s->x[k] = s->x[k - 1] + ((s->cur[2 + k - 1] * uu[k - 1]) >> 9);
            s->x[0] = uu[0];
            u = uu[0];
        }
        out[w++] = (int16_t)(u * 64);
        if (++s->pos >= s->spf)
            s->pos = 0;
    }
    return w;
}

uint32_t lpc_frames(const uint8_t *data, uint32_t n)
{
    lpc_t s;
    uint32_t frames = 0;
    lpc_begin(&s, data, n, 25);
    while (lpc_frame(&s))
        frames++;
    return frames;
}
