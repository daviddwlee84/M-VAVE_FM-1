/* SPDX-License-Identifier: MIT */
/* adpcm: IMA ADPCM decoder. See adpcm.h. */
#include "adpcm.h"

static const int8_t adpcm_index[8] = {-1, -1, -1, -1, 2, 4, 6, 8};
static const int16_t adpcm_steps[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97,
    107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871,
    5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623,
    27086, 29794, 32767};

void adpcm_begin(adpcm_t *a, const uint8_t *data, uint32_t nbytes, uint32_t nsamples)
{
    a->p = data;
    a->n = nbytes;
    a->left = nsamples;
    a->at = 0;
    a->k = 0;
    a->pred = 0;
    a->idx = 0;
}

uint32_t adpcm_render(adpcm_t *a, int16_t *out, uint32_t n)
{
    uint32_t w = 0;
    while (w < n && a->left) {
        if (a->k == 0) {                                  /* a block's header: its first sample as is */
            if (a->at + 4u > a->n) {
                a->left = 0;
                break;
            }
            a->pred = (int16_t)(a->p[a->at] | a->p[a->at + 1] << 8);
            a->idx = a->p[a->at + 2] > 88 ? 88 : a->p[a->at + 2];
            out[w++] = (int16_t)a->pred;
        } else {
            uint32_t byte = a->at + 4u + (a->k - 1u) / 2u;
            int32_t code, step, diff;
            if (byte >= a->n) {
                a->left = 0;
                break;
            }
            code = (a->k - 1u) & 1u ? a->p[byte] >> 4 : a->p[byte] & 15;
            step = adpcm_steps[a->idx];
            diff = step >> 3;
            if (code & 4) diff += step;
            if (code & 2) diff += step >> 1;
            if (code & 1) diff += step >> 2;
            a->pred += code & 8 ? -diff : diff;
            a->pred = a->pred > 32767 ? 32767 : a->pred < -32768 ? -32768 : a->pred;
            a->idx += adpcm_index[code & 7];
            a->idx = a->idx < 0 ? 0 : a->idx > 88 ? 88 : a->idx;
            out[w++] = (int16_t)a->pred;
        }
        a->left--;
        if (++a->k == ADPCM_BLOCK_SAMPLES) {
            a->k = 0;
            a->at += ADPCM_BLOCK_BYTES;
        }
    }
    return w;
}
