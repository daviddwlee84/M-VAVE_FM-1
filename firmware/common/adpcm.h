/* SPDX-License-Identifier: MIT */
/* adpcm: an IMA ADPCM (4 bits a sample) decoder, streaming, for recorded speech or samples kept in flash. The block
 * format is host/voicelab/adpcm.py's (as IMA ADPCM in WAV): 256-byte blocks, each a header (the first sample, int16
 * little-endian; the step index; a spare byte) and 252 bytes of nibbles, low nibble first: 505 samples a block.
 * No platform calls, no allocation.
 *
 *   adpcm_t a;
 *   adpcm_begin(&a, data, nbytes, nsamples);
 *   n = adpcm_render(&a, pcm, 256);         up to 256 samples (int16); fewer at the end */
#pragma once
#include <stdint.h>

#define ADPCM_BLOCK_BYTES 256u
#define ADPCM_BLOCK_SAMPLES 505u

typedef struct {
    const uint8_t *p;
    uint32_t n, left;        /* bytes, samples still to come */
    uint32_t at, k;          /* the block's first byte, the sample within the block */
    int32_t pred, idx;
} adpcm_t;

void adpcm_begin(adpcm_t *a, const uint8_t *data, uint32_t nbytes, uint32_t nsamples);
uint32_t adpcm_render(adpcm_t *a, int16_t *out, uint32_t n);
