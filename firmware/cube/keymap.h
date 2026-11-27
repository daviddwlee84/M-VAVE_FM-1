/* SPDX-License-Identifier: GPL-3.0-or-later */
/* keymap: which key turns what on the virtual cube (keymap.c). Key k is the FM-1's key k (0 = F3 .. 26 = G5). An
 * action is a move id (cube.h MV) or KA_*. Four layouts: BLACK (a white key turns clockwise, the black key to its
 * right counter-clockwise), TWO HANDS (after the classic computer-keyboard layout of cstimer and qqtimer: the right
 * hand turns R, U, F, the left hand L, D, B), SEL (16 white keys turn clockwise, with SEL held counter-clockwise; the
 * black keys turn twice) and CUSTOM (any of them, changed key by key). In every layout SEL + a key turns the other
 * way. Platform-free. */
#pragma once
#include <stdint.h>

#define NKEYS_CUBE 27
#define KA_NONE 0xFFu
#define KA_UNDO 0xFEu
enum { KM_BLACK, KM_TWOHANDS, KM_SEL, KM_PRESETS, KM_CUSTOM = KM_PRESETS };

void km_preset(int preset, uint8_t *map);
const char *km_name(int layout);         /* "BLACK KEYS", "TWO HANDS", "SEL", "CUSTOM" */
static inline int km_move(int action) { return action; }
/* the key to press for move mv: the key whose action is it; else its quarter turn (twice for a half turn, *sel set
 * for the other direction). -1: no key */
int km_key_for(const uint8_t *map, int mv, int *sel, int *twice);
int km_action_next(int action, int dir); /* the action after (dir +1) or before (-1) it, for editing a key */
