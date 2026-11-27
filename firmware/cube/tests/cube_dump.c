/* SPDX-License-Identifier: GPL-3.0-or-later */
/* N random-move scrambles and the stickers cube.c makes of them, one "scramble<TAB>facelets" line each, for
 * host/cube/make_vectors.py to hand to Kociemba's own solver (just cube-vectors). */
#include <stdio.h>
#include <stdlib.h>
#include "../cube.c"

int main(int argc, char **argv)
{
    uint32_t rng = 2026u;
    int n = argc > 1 ? atoi(argv[1]) : 40, i;
    for (i = 0; i < n; i++) {
        uint8_t m[25];
        char s[128], f[55];
        int len = i < 5 ? 1 + i : 25;
        cube_t c;
        cube_scramble_moves(m, len, &rng);
        cube_print(m, len, s, sizeof s);
        cube_reset(&c);
        cube_apply(&c, m, len);
        cube_facelets(&c, f);
        printf("%s\t%s\n", s, f);
    }
    return 0;
}
