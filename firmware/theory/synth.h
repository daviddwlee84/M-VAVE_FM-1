/* SPDX-License-Identifier: GPL-3.0-or-later */
/* synth: eight voices of a two-operator FM electric piano, enough to hear chords and voicings through the FM-1's
 * speaker. The voice follows firmware/common/chime (MIT): a carrier phase-modulated by a sine whose depth falls as
 * the note decays, so a note starts bright and mellows. Here the modulator runs at the carrier's frequency (an
 * electric piano, not a bell), a fast envelope gives the attack its bite, a slow one holds the note while the key is
 * down, and a release fades it when the key goes up.
 *
 * One writer: only the main loop calls synth_note_on / _off / _all_off; they queue events that the audio interrupt
 * reads at the start of each block, so no lock is needed. Pitches come from a table built by synth_init. */
#pragma once
#include <stdint.h>

#define SY_VOICES 8u

void synth_init(void);                   /* the pitch table: the main loop, before any note */
void synth_note_on(int note, int vel);   /* MIDI note 0..127, velocity 1..127 */
void synth_note_off(int note);
void synth_all_off(void);                /* every voice released */
void synth_stop(void);                   /* silence now, nothing queued (app_panic, a reboot) */
void synth_render(int32_t *lr, uint32_t n, float gain);   /* audio interrupt: writes every sample of n frames */
int synth_active(void);                  /* voices sounding, as of the last block */
