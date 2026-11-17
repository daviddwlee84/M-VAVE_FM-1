/* SPDX-License-Identifier: MIT */
/* lpc: a TMS5220-style LPC speech decoder (the Speak & Spell's format, as the Talkie library and python_wizard
 * write it): ten-pole lattice filter, chirp or noise excitation, 8 kHz out. No platform calls, no allocation: the
 * same file builds on the computer (host/voicelab, tests/) and in an fm1-base app.
 *
 *   lpc_t s;
 *   lpc_begin(&s, word, word_len, 25);           a stream of frames, 25 ms each (python_wizard -F 25; 10 for -F 10)
 *   n = lpc_render(&s, pcm, 256);                up to 256 samples at 8 kHz (int16); fewer once the word ends
 *   lpc_done(&s)                                 the stop frame (energy 15) or the end of the data was reached
 *
 * The stream: per frame energy (4 bits; 0 silence, 15 stop), repeat (1), pitch (6; 0 unvoiced), then K1..K10
 * (5 5 4 4 4 4 4 3 3 3 bits) unless repeated, only K1..K4 when unvoiced. Bits are taken from each byte's lowest
 * first, each field assembled highest bit first: python_wizard's "nibble swapped, reversed" bytes. Tables: the
 * TMS5220's, from python_wizard's lpcplayer/tables.py (MIT, ptwz), which has them from MAME. */
#pragma once
#include <stdint.h>

typedef struct {
    const uint8_t *p;
    uint32_t n, bit;                 /* the stream, its length in bytes, the next bit */
    uint16_t spf, pos, step;         /* samples a frame, the sample in the frame, samples between interpolations */
    int32_t cur[12], tgt[12];        /* energy, period, K1..K10 (Q9): now, and where this frame goes */
    int32_t x[10];                   /* the lattice's backward path */
    uint16_t pc, rng;                /* pitch period counter, the noise generator */
    uint8_t done, started, jump;
} lpc_t;

void lpc_begin(lpc_t *s, const uint8_t *data, uint32_t n, uint32_t frame_ms);
uint32_t lpc_render(lpc_t *s, int16_t *out, uint32_t n);
static inline int lpc_done(const lpc_t *s) { return s->done; }
/* the frames' total bit count, for sizes (a dry run over the stream: no synthesis) */
uint32_t lpc_frames(const uint8_t *data, uint32_t n);
