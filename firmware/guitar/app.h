/* SPDX-License-Identifier: GPL-3.0-or-later */
/* GUITAR: the FM-1 as a pocket guitar neck and chord book (design: docs/dev/guitar.md in
 * github.com/daviddwlee84/M-VAVE_FM-1). Three pages: NECK, a key's scale on the frets, whole or one position at a
 * time; SHAPE, chord diagrams found by search, easiest first; LEARN (0.2), chords taught key by key on Leitner cards
 * (lesson.c). The neck is fret.c; the theory, the voice
 * and the music signs are THEORY's (../theory). Built on fm1-base (MPL-2.0, its headers). This app is
 * GPL-3.0-or-later; ../common (ledfx) is MIT. */
#pragma once
#include <stdint.h>
#include "plat.h"

#define APP_NAME "GUITAR"
#define APP_SPLASH_RGB RGB(255, 170, 80)
#define APP_PALETTE 1                    /* amber */
#define APP_BLOCK 256u                   /* frames per audio block: 5.8 ms at 44.1 kHz */
#define APP_TOUCH                        /* velocity from the keys (lib/touch.h) */

/* the settings, one record (app.c rec_t), at 0xD4000: below THEORY's 0xD6000 (0.2 progress) and 0xD8000 and Morse's 0xDA000,
 * away from the 0xFC000 settings sector SLOOP, Agent Deck and CTL-1 share. Not yet rehearsed over this unit's
 * backups (docs/dev/guitar.md). The panel calibration is the platform's */
enum { OBJ_PANEL, OBJ_GUITAR, OBJ_NOBJ };
#define APP_STORE_MAP {0xDC000u, 0xD4000u}

void app_init(void);
void app_render(int32_t *lr, uint32_t n, uint32_t master_q12);
void app_panic(void);
void app_ui_init(void);
void app_frame(void);
void app_input_only(void);
void app_say(const char *a, const char *b);

/* host simulator: checks (tonic, scale, page, pos, shape, ...), commands (expect_text) and their failures */
#define APP_HOST_EXPECT
int app_expect(const char *what, int *got);
#define APP_HOST_CMD
int app_host_cmd(const char *cmd, const char *a, const char *b, const char *c, const char *d);
#define APP_HOST_FAILS
int app_host_fails(void);
