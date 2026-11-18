/* SPDX-License-Identifier: GPL-3.0-or-later */
/* RHYTHM: the FM-1 as a pocket rhythm trainer (design: docs/dev/rhythm.md in github.com/daviddwlee84/M-VAVE_FM-1).
 * v0.1 has two pages. METRO: a metronome with subdivisions, tap tempo and a gap click (it goes quiet for some bars
 * and shows how far you drifted). ECHO: hear one or two bars after a count-in, tap them back on any key, and see each
 * note early or late in ms, with a hint for the one thing most worth fixing (RUSHING, DRAGGING, LATE AFTER RESTS...).
 * The rhythms and the scoring are rhythm.c (platform-free). Built on fm1-base (MPL-2.0, its headers). This app is
 * GPL-3.0-or-later; ../common (ledfx) is MIT. */
#pragma once
#include <stdint.h>
#include "plat.h"

#define APP_NAME "RHYTHM"
#define APP_SPLASH_RGB RGB(255, 170, 80)
#define APP_PALETTE 1
#define APP_BLOCK 256u                   /* frames per audio block: 5.8 ms at 44.1 kHz */
#define APP_TICK                         /* 1 kHz: the keys' onsets, timed against the audio's own sample count */

/* the settings, one record (app.c rec_t), at 0xD6000: below THEORY's 0xD8000 and Morse's 0xDA000, a sector pair
 * empty in this unit's backups (docs/hardware/flash-map.md; rehearse with just emu-rehearse before a flash). GUITAR
 * takes 0xD4000. The panel calibration is the platform's */
enum { OBJ_PANEL, OBJ_RHYTHM, OBJ_NOBJ };
#define APP_STORE_MAP {0xDC000u, 0xD6000u}

void app_init(void);
void app_render(int32_t *lr, uint32_t n, uint32_t master_q12);
void app_panic(void);
void app_ui_init(void);
void app_frame(void);
void app_input_only(void);
void app_say(const char *a, const char *b);
void app_tick(void);

/* host simulator: checks (page, bpm, echo, score, hint, ...), commands (pattern, autotap, metrotap) */
#define APP_HOST_EXPECT
int app_expect(const char *what, int *got);
#define APP_HOST_CMD
int app_host_cmd(const char *cmd, const char *a, const char *b, const char *c, const char *d);
#define APP_HOST_FAILS
int app_host_fails(void);
