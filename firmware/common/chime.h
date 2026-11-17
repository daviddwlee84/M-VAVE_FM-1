/* SPDX-License-Identifier: MIT */
/* chime: a small two-operator FM voice for notification sounds, with a queue of a few notes.
 *
 *   chime_play(CHIME_UP);                         main loop: queue a figure (drops it if one is playing)
 *   chime_render(lr, n, gain);                    audio interrupt: adds the chime into lr (24-bit stereo)
 *
 * Figures are short note lists (MIDI note, length in ms); add your own with chime_notes(). */
#pragma once
#include <stdint.h>

enum { CHIME_UP, CHIME_DONE, CHIME_ERROR, CHIME_TICK, CHIME_N };

void chime_play(int figure);
void chime_notes(const uint8_t *notes, const uint16_t *ms, uint32_t n);
void chime_render(int32_t *lr, uint32_t n, float gain);
int chime_busy(void);
void chime_stop(void);
