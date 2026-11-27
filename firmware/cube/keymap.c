/* SPDX-License-Identifier: GPL-3.0-or-later */
/* keymap: see keymap.h. */
#include "keymap.h"
#include "cube.h"

#define Q(b) MV(b, 1)
#define P(b) MV(b, 3)
#define H(b) MV(b, 2)

/*            F3      F#3     G3      G#3     A3      A#3     B3      C4      C#4     D4      D#4     E4 */
static const uint8_t KM[KM_PRESETS][NKEYS_CUBE] = {
    {Q(MB_L), P(MB_L), Q(MB_D), P(MB_D), Q(MB_B), P(MB_B), Q(MB_x), Q(MB_F), P(MB_F), Q(MB_M), P(MB_M), Q(MB_y),
     /* F4    F#4     G4      G#4     A4      A#4     B4      C5      C#5     D5      D#5     E5     F5      F#5 */
     Q(MB_U), P(MB_U), Q(MB_E), P(MB_E), Q(MB_S), P(MB_S), Q(MB_z), Q(MB_r), P(MB_r), Q(MB_u), P(MB_u), KA_UNDO,
     Q(MB_R), P(MB_R), /* G5 */ KA_NONE},
    /* TWO HANDS: the left hand's fingers on A3 .. D4 (B L L' D U' F'), the right hand's on F4 .. C#5 (F U R' R D' B'),
     * rotations and wide turns under the little fingers and thumbs */
    {P(MB_z), Q(MB_l), P(MB_y), Q(MB_B), Q(MB_D), P(MB_L), Q(MB_L), P(MB_U), P(MB_l), P(MB_F), P(MB_M), Q(MB_x),
     Q(MB_F), Q(MB_M), Q(MB_U), Q(MB_r), P(MB_R), Q(MB_R), P(MB_D), Q(MB_y), P(MB_B), Q(MB_z), P(MB_r), KA_UNDO,
     P(MB_x), Q(MB_S), Q(MB_E)},
    /* SEL: white keys clockwise (SEL held: the other way), black keys twice */
    {Q(MB_L), H(MB_L), Q(MB_M), H(MB_M), Q(MB_D), H(MB_D), Q(MB_B), Q(MB_F), H(MB_F), Q(MB_x), H(MB_x), Q(MB_y),
     Q(MB_z), H(MB_z), Q(MB_U), H(MB_U), Q(MB_E), H(MB_E), Q(MB_S), Q(MB_r), H(MB_r), Q(MB_u), H(MB_u), KA_UNDO,
     Q(MB_R), H(MB_R), Q(MB_f)},
};

static const char *const KM_NAME[KM_PRESETS + 1] = {"BLACK KEYS", "TWO HANDS", "SEL", "CUSTOM"};

void km_preset(int p, uint8_t *map)
{
    int k;
    if (p < 0 || p >= KM_PRESETS) p = KM_BLACK;
    for (k = 0; k < NKEYS_CUBE; k++) map[k] = KM[p][k];
}

const char *km_name(int l) { return KM_NAME[l >= 0 && l <= KM_PRESETS ? l : 0]; }

int km_key_for(const uint8_t *map, int mv, int *sel, int *twice)
{
    int k, b = MV_BASE(mv), t = MV_TURNS(mv);
    if (sel) *sel = 0;
    if (twice) *twice = 0;
    for (k = 0; k < NKEYS_CUBE; k++)
        if (map[k] == mv) return k;
    if (t == 2) {
        for (k = 0; k < NKEYS_CUBE; k++)
            if (map[k] == MV(b, 1) || map[k] == MV(b, 3)) {
                if (twice) *twice = 1;
                return k;
            }
        return -1;
    }
    for (k = 0; k < NKEYS_CUBE; k++)     /* the other direction, with SEL */
        if (map[k] == MV(b, 4 - t)) {
            if (sel) *sel = 1;
            return k;
        }
    return -1;
}

int km_action_next(int a, int dir)       /* none, undo, then every move */
{
    int i = a == KA_NONE ? 0 : a == KA_UNDO ? 1 : a + 2, n = MV_N + 2;
    i = (i + n + (dir > 0 ? 1 : -1)) % n;
    return i == 0 ? KA_NONE : i == 1 ? KA_UNDO : i - 2;
}
