/* SPDX-License-Identifier: GPL-3.0-or-later */
/* cube.c, stats.c, keymap.c and the formulas (algs.h) on this computer (just cube-unit).
 * The reference vectors (tests/vectors.h) come from host/cube/make_vectors.py (just cube-vectors). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../cube.c"
#include "../stats.c"
#include "../keymap.c"
#include "../algs.c"
#include "vectors.h"

static int fails, checks;
#define CHECK(c, ...)                                                                                                 \
    do {                                                                                                          \
        checks++;                                                                                                 \
        if (!(c)) {                                                                                               \
            fails++;                                                                                              \
            printf("FAIL %s:%d: ", __FILE__, __LINE__);                                                           \
            printf(__VA_ARGS__);                                                                                  \
            printf("\n");                                                                                         \
        }                                                                                                         \
    } while (0)

static int same_after(const char *a, const char *b)   /* two sequences do the same from solved */
{
    cube_t x, y;
    uint8_t m[64];
    int n;
    cube_reset(&x);
    cube_reset(&y);
    if ((n = cube_parse(a, m, 64)) < 0) return 0;
    cube_apply(&x, m, n);
    if ((n = cube_parse(b, m, 64)) < 0) return 0;
    cube_apply(&y, m, n);
    return cube_equal(&x, &y);
}

static int identity(const char *a) { return same_after(a, ""); }

static void test_notation(void)
{
    uint8_t m[64], inv[64];
    char t[256];
    int n, b, k;
    n = cube_parse("R U R' U' (R2 F) Rw' [x y2] M2 E' S r3 u2'", m, 64);
    CHECK(n == 14, "parse count %d", n);
    cube_print(m, n, t, sizeof t);
    CHECK(!strcmp(t, "R U R' U' R2 F r' x y2 M2 E' S r' u2"), "print: %s", t);
    CHECK(cube_parse("R Q", m, 64) < 0, "unknown move accepted");
    CHECK(cube_parse("Mw", m, 64) < 0, "Mw accepted");
    CHECK(cube_parse("R U R U R", m, 4) < 0, "too many moves accepted");
    CHECK(cube_parse("R4 U", m, 64) == 1, "R4 is nothing");
    /* invert: a then its inverse is nothing */
    n = cube_parse("R U2 F' x M' r2 E S'", m, 64);
    cube_invert(m, n, inv);
    {
        cube_t c;
        cube_reset(&c);
        cube_apply(&c, m, n);
        cube_apply(&c, inv, n);
        CHECK(cube_solved(&c) && c.f[4] == CF_U && c.f[22] == CF_F, "inverse undoes");
    }
    n = cube_parse("R R U U' U F2 F2 L", m, 64);
    n = cube_merge(m, n);
    cube_print(m, n, t, sizeof t);
    CHECK(!strcmp(t, "R2 U L"), "merge: %s", t);
    for (b = 0; b < MB_N; b++) {         /* every move four times is nothing */
        cube_t c;
        cube_reset(&c);
        for (k = 0; k < 4; k++) cube_move(&c, MV(b, 1));
        CHECK(cube_solved(&c) && c.f[4] == CF_U && c.f[22] == CF_F, "base %d four times", b);
        cube_reset(&c);
        cube_move(&c, MV(b, 1));
        cube_move(&c, MV(b, 3));
        CHECK(cube_solved(&c) && c.f[4] == CF_U, "base %d and prime", b);
    }
}

static void test_moves(void)
{
    CHECK(identity("R U R' U' R U R' U' R U R' U' R U R' U' R U R' U' R U R' U'"), "sexy move six times");
    CHECK(identity("M2 U M2 U2 M2 U M2 M2 U M2 U2 M2 U M2"), "H perm twice");
    CHECK(identity("R2 U R U R' U' R' U' R' U R' R2 U R U R' U' R' U' R' U R' R2 U R U R' U' R' U' R' U R'"),
          "Ub three times");
    /* slices follow L, D, F; rotations follow R, U, F; wide moves are a face and its slice */
    CHECK(same_after("x", "R M' L'"), "x = R M' L'");
    CHECK(same_after("y", "U E' D'"), "y = U E' D'");
    CHECK(same_after("z", "F S B'"), "z = F S B'");
    CHECK(same_after("r", "R M'"), "r = R M'");
    CHECK(same_after("l", "L M"), "l = L M");
    CHECK(same_after("u", "U E'"), "u = U E'");
    CHECK(same_after("d", "D E"), "d = D E");
    CHECK(same_after("f", "F S"), "f = F S");
    CHECK(same_after("b", "B S'"), "b = B S'");
    CHECK(same_after("x U x'", "F"), "x U x' = F (x brings the front up)");
    CHECK(same_after("y R y'", "B"), "y R y' = B");
    CHECK(same_after("z U z'", "L"), "z U z' = L");
    {                                    /* one look at the stickers: U turns the front row to the left */
        cube_t c;
        cube_reset(&c);
        cube_move(&c, MV(MB_U, 1));
        CHECK(c.f[CF_L * 9 + 0] == CF_F && c.f[CF_F * 9 + 0] == CF_R && c.f[CF_F * 9 + 3] == CF_F, "U moves F to L");
        cube_reset(&c);
        cube_move(&c, MV(MB_R, 1));
        CHECK(c.f[CF_U * 9 + 2] == CF_F && c.f[CF_B * 9 + 6] == CF_U, "R moves F to U and U to B");
    }
}

static void test_vectors(void)           /* Kociemba's own solver (host/cube/make_vectors.py) solves our stickers */
{
    int i;
    for (i = 0; i < (int)(sizeof CUBE_VEC / sizeof CUBE_VEC[0]); i++) {
        cube_t c, d;
        uint8_t m[64];
        int n;
        char t[55];
        cube_reset(&c);
        n = cube_parse(CUBE_VEC[i].scramble, m, 64);
        cube_apply(&c, m, n);
        cube_facelets(&c, t);
        CHECK(!strcmp(t, CUBE_VEC[i].facelets), "vector %d facelets %s want %s", i, t, CUBE_VEC[i].facelets);
        CHECK(cube_from_facelets(&d, CUBE_VEC[i].facelets) && cube_equal(&c, &d), "vector %d from facelets", i);
        n = cube_parse(CUBE_VEC[i].solution, m, 64);
        CHECK(n > 0, "vector %d solution parses", i);
        cube_apply(&c, m, n);
        CHECK(cube_solved(&c), "vector %d: kociemba's solution does not solve our cube", i);
    }
    CHECK(i >= 20, "only %d vectors", i);
}

static void test_scramble(void)
{
    uint32_t rng = 42;
    int t, i;
    for (t = 0; t < 2000; t++) {
        uint8_t m[30];
        int n = cube_scramble_moves(m, 25, &rng);
        CHECK(n == 25, "length");
        for (i = 1; i < n; i++) {
            int f = MV_BASE(m[i]), p = MV_BASE(m[i - 1]);
            CHECK(f < 6 && f != p, "scramble %d move %d", t, i);
            if (i >= 2) CHECK(!(f % 3 == p % 3 && f % 3 == MV_BASE(m[i - 2]) % 3), "three on one axis %d", t);
        }
    }
}

static void test_checks(void)
{
    cube_t c;
    uint8_t m[64];
    int n;
    cube_reset(&c);
    CHECK(cube_solved(&c) && cube_f2l_solved(&c) && cube_solved_turn(&c), "solved");
    cube_move(&c, MV(MB_y, 1));
    CHECK(cube_solved(&c), "a rotated cube is solved");
    cube_move(&c, MV(MB_U, 1));
    CHECK(!cube_solved(&c) && cube_solved_turn(&c), "one turn off");
    cube_reset(&c);
    n = cube_parse("R U R' U'", m, 64);
    cube_apply(&c, m, n);
    CHECK(!cube_solved_turn(&c) && !cube_f2l_solved(&c), "sexy move is not solved");
    cube_reset(&c);
    n = cube_parse("R U R' U R U2 R'", m, 64);   /* Sune: only the last layer changes */
    cube_apply(&c, m, n);
    CHECK(cube_f2l_solved(&c) && !cube_solved_turn(&c), "Sune keeps the first two layers");
}

static void test_stats(void)
{
    int32_t t5[5] = {10000, 12000, 11000, 9000, 13000};
    int32_t t5d[5] = {10000, ST_DNF, 11000, 9000, 13000};
    int32_t t5dd[5] = {10000, ST_DNF, 11000, ST_DNF, 13000};
    int32_t t3[3] = {10000, 12000, 11000};
    CHECK(st_average(t5, 5) == 11000, "ao5 %d", (int)st_average(t5, 5));
    CHECK(st_average(t5d, 5) == 11333, "ao5 with a DNF %d", (int)st_average(t5d, 5));
    CHECK(st_average(t5dd, 5) == ST_DNF, "ao5 with two DNFs");
    CHECK(st_mean(t3, 3) == 11000, "mo3");
    CHECK(st_average(t5, 4) == ST_NONE, "ao5 of 4");
    {                                    /* ao12: drop one best and one worst */
        int32_t t[12];
        int i;
        for (i = 0; i < 12; i++) t[i] = 10000 + i * 1000;
        CHECK(st_average(t, 12) == 15500, "ao12 %d", (int)st_average(t, 12));
        t[3] = ST_DNF;
        CHECK(st_average(t, 12) == 16300, "ao12 with a DNF %d", (int)st_average(t, 12));
    }
    {                                    /* ao100 drops 5 each side */
        static int32_t t[100];
        int i;
        for (i = 0; i < 100; i++) t[i] = 1000 * (i + 1);
        CHECK(st_average(t, 100) == 50500, "ao100 %d", (int)st_average(t, 100));
    }
    {
        char b[16];
        st_format(83456, b);
        CHECK(!strcmp(b, "1:23.45"), "format %s", b);
        st_format(9876, b);
        CHECK(!strcmp(b, "9.87"), "format %s", b);
        st_format(ST_DNF, b);
        CHECK(!strcmp(b, "DNF"), "format %s", b);
    }
    CHECK(st_with_penalty(10000, ST_PLUS2) == 12000 && st_with_penalty(10000, ST_DNF_PEN) == ST_DNF, "penalties");
}

static void test_keymap(void)
{
    int p, k, seen;
    for (p = 0; p < KM_PRESETS; p++) {
        uint8_t map[NKEYS_CUBE];
        km_preset(p, map);
        seen = 0;
        for (k = 0; k < NKEYS_CUBE; k++) {
            int a = map[k];
            CHECK(a == KA_NONE || a == KA_UNDO || km_move(a) < MV_N, "preset %d key %d action %d", p, k, a);
            if (a == KA_UNDO) seen |= 1;
            if (a != KA_NONE && a != KA_UNDO && MV_BASE(km_move(a)) == MB_R) seen |= 2;
            if (a != KA_NONE && a != KA_UNDO && MV_BASE(km_move(a)) == MB_U) seen |= 4;
        }
        CHECK(seen == 7, "preset %d lacks undo, R or U (%d)", p, seen);
    }
    {
        uint8_t map[NKEYS_CUBE];
        km_preset(KM_BLACK, map);
        CHECK(km_move(map[0]) == MV(MB_L, 1) && km_move(map[1]) == MV(MB_L, 3), "BLACK: F3 L, F#3 L'");
        CHECK(km_move(map[24]) == MV(MB_R, 1) && km_move(map[25]) == MV(MB_R, 3), "BLACK: F5 R, F#5 R'");
        CHECK(map[23] == KA_UNDO, "BLACK: E5 undo");
        CHECK(km_key_for(map, MV(MB_U, 1), 0, 0) == 12, "BLACK: U is F4 (key %d)", km_key_for(map, MV(MB_U, 1), 0, 0));
    }
}

static void test_algs(void)
{
    int i, set, counts[ALG_SETS] = {0};
    for (i = 0; i < ALG_N; i++) {
        const alg_t *a = alg_get(i);
        uint8_t m[ALG_MAX], inv[ALG_MAX];
        int n = cube_parse(a->alg, m, ALG_MAX), j;
        cube_t c;
        counts[a->set]++;
        CHECK(n > 0, "%s: '%s' does not parse", a->name, a->alg);
        if (n <= 0) continue;
        if (a->alt && a->alt[0]) {
            int n2 = cube_parse(a->alt, inv, ALG_MAX);
            cube_t c1, c2;
            CHECK(n2 > 0, "%s: alternative does not parse", a->name);
            /* the alternative solves the same case (up to a last U turn and a rotation) */
            cube_reset(&c1);
            cube_invert(m, n, inv + 32);
            cube_apply(&c1, inv + 32, n);
            c2 = c1;
            n2 = cube_parse(a->alt, inv, ALG_MAX);
            cube_apply(&c2, inv, n2);
            CHECK(cube_solved_turn(&c2) || alg_solved_auf(&c2), "%s: alternative does not solve the case", a->name);
            (void)c1;
        }
        /* the case: the inverse on a solved cube; it keeps what the set keeps */
        cube_reset(&c);
        cube_invert(m, n, inv);
        cube_apply(&c, inv, n);
        CHECK(!cube_solved(&c), "%s does nothing", a->name);
        if (a->set == ALG_OLL || a->set == ALG_PLL || a->set == ALG_OLL2 || a->set == ALG_PLL2)
            CHECK(alg_f2l_kept(&c), "%s breaks the first two layers", a->name);
        if (a->set == ALG_PLL || a->set == ALG_PLL2) CHECK(alg_ll_oriented(&c), "%s turns last-layer pieces", a->name);
        if (a->set == ALG_F2L) CHECK(alg_cross_kept(&c), "%s breaks the cross", a->name);
        for (j = 0; j < n; j++) {}
    }
    /* each set's cases differ from each other (seen from any side, with any last U turn) */
    for (set = 0; set < ALG_SETS; set++) {
        int a, b;
        for (a = 0; a < ALG_N; a++) {
            if (alg_get(a)->set != set) continue;
            for (b = a + 1; b < ALG_N; b++)
                if (alg_get(b)->set == set) CHECK(!alg_same_case(a, b), "%s and %s are one case", alg_get(a)->name,
                                                  alg_get(b)->name);
        }
    }
    CHECK(counts[ALG_OLL] == 57, "OLL %d", counts[ALG_OLL]);
    CHECK(counts[ALG_PLL] == 21, "PLL %d", counts[ALG_PLL]);
    CHECK(counts[ALG_F2L] == 41, "F2L %d", counts[ALG_F2L]);
    CHECK(counts[ALG_OLL2] == 10, "2-look OLL %d", counts[ALG_OLL2]);
    CHECK(counts[ALG_PLL2] == 6, "2-look PLL %d", counts[ALG_PLL2]);
    CHECK(counts[ALG_LBL] >= 5, "LBL %d", counts[ALG_LBL]);
}

int main(void)
{
    test_notation();
    test_moves();
    test_vectors();
    test_scramble();
    test_checks();
    test_stats();
    test_keymap();
    test_algs();
    printf("cube: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
