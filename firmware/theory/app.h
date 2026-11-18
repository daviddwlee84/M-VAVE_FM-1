/* SPDX-License-Identifier: GPL-3.0-or-later */
/* THEORY: the FM-1 as a pocket music theory reference and trainer (design: docs/dev/theory.md in
 * github.com/daviddwlee84/M-VAVE_FM-1). v0.1 is the WHEEL: the circle of fifths with the key's chords marked; play
 * a chord and it is named, with its Roman numeral in the key; or let the white keys play the key's own chords.
 * Built on fm1-base (MPL-2.0, its headers). This app is GPL-3.0-or-later; ../common (ledfx) is MIT. */
#pragma once
#include <stdint.h>
#include "plat.h"

#define APP_NAME "THEORY"
#define APP_SPLASH_RGB RGB(120, 200, 255)
#define APP_PALETTE 2                    /* cyan */
#define APP_BLOCK 256u                   /* frames per audio block: 5.8 ms at 44.1 kHz */
#define APP_TOUCH                        /* velocity from the keys (lib/touch.h): soft and loud chords */

/* the settings, one record (app.c rec_t), at 0xD8000: a sector pair no firmware on this unit has written
 * (docs/hardware/flash-map.md), below Morse's 0xDA000 and away from the 0xFC000 settings sector SLOOP, Agent Deck
 * and CTL-1 share. The panel calibration is the platform's */
enum { OBJ_PANEL, OBJ_THEORY, OBJ_NOBJ };
#define APP_STORE_MAP {0xDC000u, 0xD8000u}

void app_init(void);
void app_render(int32_t *lr, uint32_t n, uint32_t master_q12);
void app_panic(void);
void app_ui_init(void);
void app_frame(void);
void app_input_only(void);
void app_say(const char *a, const char *b);

/* host simulator: checks (tonic, mode, keymode, voices, lit_keys, ...), commands (expect_text) and the failures of
 * those commands */
#define APP_HOST_EXPECT
int app_expect(const char *what, int *got);
#define APP_HOST_CMD
int app_host_cmd(const char *cmd, const char *a, const char *b, const char *c, const char *d);
#define APP_HOST_FAILS
int app_host_fails(void);
