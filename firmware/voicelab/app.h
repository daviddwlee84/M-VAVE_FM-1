/* SPDX-License-Identifier: MIT */
/* Voice Lab: the speech decoders a vocabulary app would carry (firmware/common lpc and adpcm), on the FM-1. Each key
 * says one of host/voicelab's test words; PRESET (or OCT- / OCT+) picks LPC or ADPCM; PLAY says the last word again.
 * The words come from build/voicelab (just voice-fw writes gen/voicedata.h, git-ignored). A spike for
 * docs/dev/tts.md, not a firmware to install. Built on fm1-base (MPL-2.0, its headers). */
#pragma once
#include <stdint.h>
#include "plat.h"

#define APP_NAME "VOICELAB"
#define APP_SPLASH_RGB RGB(120, 220, 160)
#define APP_BLOCK 256u                   /* frames per audio block: 5.8 ms at 44.1 kHz */

/* nothing is saved: only the platform's panel calibration */
enum { OBJ_PANEL, OBJ_NOBJ };
#define APP_STORE_MAP {0xDC000u}

void app_init(void);
void app_render(int32_t *lr, uint32_t n, uint32_t master_q12);
void app_panic(void);
void app_ui_init(void);
void app_frame(void);
void app_input_only(void);
void app_say(const char *a, const char *b);

/* host simulator checks: playing, engine, word, nwords */
#define APP_HOST_EXPECT
int app_expect(const char *what, int *got);
