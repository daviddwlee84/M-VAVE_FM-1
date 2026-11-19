/* SPDX-License-Identifier: GPL-3.0-or-later */
/* kit: DRUMS' drum sounds: one voice a lane (groove.h L_*), a hit restarts it.
 * Kick and toms are a sine with a falling pitch, snare and rim add filtered noise, the clap is noise in three quick
 * bursts, hats and cymbals six square waves at the 808's ratios (or noise) through a high-pass, the cowbell two
 * squares; RUSTY plays recordings (samples.h) for most lanes. A kit is a table of each lane's settings. Compiled at -O2 (fm1.toml o2), platform-free. */
#pragma once
#include <stdint.h>

enum { KIT_808, KIT_STUDIO, KIT_RUSTY, KIT_N };

void kit_init(float sr);
const char *kit_name(int kit);
void kit_trigger(int lane, int vel, int kit);   /* vel 1..9; the render calls it, between kit_render's */
void kit_render(float *out, uint32_t n);        /* the voices into out (written, not added), mono */
void kit_silence(void);
void kit_set_level(int lane, float gain);       /* the MIX page: a lane's gain (1 = as the kit has it) */
int kit_active(void);                           /* voices sounding */
