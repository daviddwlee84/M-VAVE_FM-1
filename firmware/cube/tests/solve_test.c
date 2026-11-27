/* SPDX-License-Identifier: GPL-3.0-or-later */
/* solve.c on this computer: the tables, then random states solved and checked (just cube-unit).
 * Built twice: with SOLVE_GEN (tables made in RAM) and against gen/cube_tables.h (what the firmware carries). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../cube.c"
#include "../solve.c"

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

int main(int argc, char **argv)
{
    int i, m, n = argc > 1 ? atoi(argv[1]) : 1000, r, hist[SOLVE_MAX + 1] = {0}, worst = 0;
    uint32_t rng = 12345u, maxnodes = 0;
    double total_nodes = 0;
    clock_t t0;
    char a[55], b[55];

    t0 = clock();
    r = solve_init();
    CHECK(r == 0, "solve_init %d (twist classes %d, perm classes %d)", r, solve_twist_classes(), solve_perm_classes());
    if (r) return 1;
    printf("tables: %.2f s\n", (double)(clock() - t0) / CLOCKS_PER_SEC);
    CHECK(solve_twist_classes() == 291, "twist classes %d", solve_twist_classes());
    CHECK(solve_perm_classes() == 2768, "perm classes %d", solve_perm_classes());

    /* the cubie moves and cube.c's sticker moves agree */
    for (m = 0; m < 18; m++) {
        cc_t c;
        cube_t k;
        cc_ident(&c);
        cc_move(&c, m);
        cc_facelets(&c, a);
        cube_reset(&k);
        cube_move(&k, m);
        cube_facelets(&k, b);
        CHECK(!strcmp(a, b), "move %d: cubie %s stickers %s", m, a, b);
    }
    /* facelets round trip, and random states are valid */
    for (i = 0; i < 200; i++) {
        cc_t c, d;
        solve_random_cc(&c, &rng);
        CHECK(cc_valid(&c), "random cube %d not valid", i);
        cc_facelets(&c, a);
        CHECK(cc_from_facelets(&d, a) && cc_same(&c, &d), "facelets round trip %d", i);
    }
    /* solve random states: the solution solves, its inverse scrambles to the same stickers */
    t0 = clock();
    for (i = 0; i < n; i++) {
        static solve_t s;
        cc_t c;
        cube_t k;
        int j;
        solve_random_cc(&c, &rng);
        solve_start(&s, &c, 21);
        while (solve_step(&s, 50000u) == 0) {}
        CHECK(s.done, "state %d not solved", i);
        if (!s.done) continue;
        cc_facelets(&c, a);
        cube_from_facelets(&k, a);
        cube_apply(&k, s.sol, s.len);
        CHECK(cube_solved(&k), "solution %d does not solve", i);
        for (j = 1; j < s.len; j++) {
            int f = s.sol[j] / 3, pf = s.sol[j - 1] / 3;
            CHECK(f != pf, "solution %d: one face twice at %d", i, j);
        }
        hist[s.len]++;
        if (s.len > worst) worst = s.len;
        total_nodes += s.nodes;
        if (s.nodes > maxnodes) maxnodes = s.nodes;
    }
    printf("%d states: %.3f s, nodes mean %.0f max %u; lengths:", n, (double)(clock() - t0) / CLOCKS_PER_SEC,
           total_nodes / n, maxnodes);
    for (i = 0; i <= SOLVE_MAX; i++)
        if (hist[i]) printf(" %d:%d", i, hist[i]);
    printf("\n");
    CHECK(worst <= 24, "longest %d", worst);
    printf("%d checks, %d failed\n", checks, fails);
    return fails != 0;
}
