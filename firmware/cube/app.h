/* SPDX-License-Identifier: GPL-3.0-or-later */
/* CUBE: the FM-1 as a Rubik's cube companion (design: docs/dev/cube.md in github.com/daviddwlee84/M-VAVE_FM-1).
 * TIMER (WCA inspection, hold and let go to start, ao5 / ao12 / ao100) with random-state scrambles found on the FM-1
 * itself (Kociemba's two-phase algorithm, solve.c) or random-move ones; CUBE, a virtual cube turned from the keys and
 * seen from any side (K1 .. K4); ALGS, the formula book (beginner's, 2-look, OLL, PLL, F2L) with each case drawn
 * from its formula; DRILL, the cases as flash cards, on a real cube, or on the virtual one. Built on fm1-base
 * (MPL-2.0, its headers). This app is GPL-3.0-or-later; ../common (ledfx, chime) is MIT; ../theory's srs.c and
 * gtext.c are GPL-3.0-or-later. */
#pragma once
#include <stdint.h>
#include "plat.h"

#define APP_NAME "CUBE"
#define APP_SPLASH_RGB RGB(250, 215, 30)
#define APP_PALETTE 2                    /* cyan */
#define APP_BLOCK 256u
#define APP_TICK                         /* 1 kHz: the keys' edges timed to the millisecond (the timer) */

/* the store: the panel calibration (the platform's), then the settings and own key layout, the session's solves,
 * the drills' progress, each two sectors below METRONOME's 0xCE000 (DRUMS 0xD0000, RHYTHM 0xD2000, GUITAR 0xD4000,
 * THEORY 0xD{6,8}000, Morse 0xDA000, the panel 0xDC000; docs/hardware/flash-map.md). 0xC8000 .. 0xCDFFF are blank in
 * the backup of 2026-11-25; check again with just emu-rehearse before a flash */
enum { OBJ_PANEL, OBJ_CUBE, OBJ_SOLVES, OBJ_DRILL, OBJ_NOBJ };
#define APP_STORE_MAP {0xDC000u, 0xCC000u, 0xCA000u, 0xC8000u}

void app_init(void);
void app_render(int32_t *lr, uint32_t n, uint32_t master_q12);
void app_panic(void);
void app_ui_init(void);
void app_frame(void);
void app_input_only(void);
void app_say(const char *a, const char *b);
void app_tick(void);

/* host simulator: checks (page, timer, solves, last_ms, pen, solved, moves, ...) */
#define APP_HOST_EXPECT
int app_expect(const char *what, int *got);
#define APP_HOST_FAILS
int app_host_fails(void);
