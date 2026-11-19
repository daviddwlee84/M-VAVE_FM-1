/* SPDX-License-Identifier: GPL-3.0-or-later */
/* DRUMS: the FM-1 as a live drum machine (design: docs/dev/drums.md in github.com/daviddwlee84/M-VAVE_FM-1).
 * Pick a style and a kit, press PLAY, and ask for fills and changes as you go: FILL AT says when a fill comes (now,
 * the next bar, or the last bar of the phrase), REC is one button for all of it (tap: a fill, hold: the other MAIN,
 * twice: the ending), and the right half of the keys are pads. The player is groove.c (platform-free), the sounds
 * kit.c (synthesised). Built on fm1-base (MPL-2.0, its headers). This app is GPL-3.0-or-later; ../common (ledfx) is
 * MIT. */
#pragma once
#include <stdint.h>
#include "plat.h"

#define APP_NAME "DRUMS"
#define APP_SPLASH_RGB RGB(240, 120, 60)
#define APP_PALETTE 1
#define APP_BLOCK 256u                   /* frames per audio block: 5.8 ms at 44.1 kHz */
#define APP_TICK                         /* 1 kHz: the pads, timed against the audio's own sample count */

/* the settings, one record (app.c rec_t), at 0xD0000: the next sector below RHYTHM's 0xD2000 (GUITAR 0xD4000,
 * THEORY 0xD{6,8}000, Morse 0xDA000, the panel 0xDC000; docs/hardware/flash-map.md). Check it is blank in the latest
 * backup with just emu-rehearse before a flash. The panel calibration is the platform's */
enum { OBJ_PANEL, OBJ_DRUMS, OBJ_NOBJ };
#define APP_STORE_MAP {0xDC000u, 0xD0000u}

void app_init(void);
void app_render(int32_t *lr, uint32_t n, uint32_t master_q12);
void app_panic(void);
void app_ui_init(void);
void app_frame(void);
void app_input_only(void);
void app_say(const char *a, const char *b);
void app_tick(void);

/* host simulator: checks (state, sec, part, bar, style, kit, bpm, hits_K ...) */
#define APP_HOST_EXPECT
int app_expect(const char *what, int *got);
#define APP_HOST_FAILS
int app_host_fails(void);
