/* SPDX-License-Identifier: GPL-3.0-or-later */
/* rhythm.c on this computer: just rhythm-unit. Parsing, the generator's rules, timing, the alignment and grades,
 * and that each hint comes from the mistake it names. */
#include <stdio.h>
#include <string.h>
#include "../rhythm.c"

static int fails, checks;

#define CHECK(c)                                                                                                       \
    do {                                                                                                               \
        checks++;                                                                                                      \
        if (!(c)) {                                                                                                    \
            fails++;                                                                                                   \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c);                                                        \
        }                                                                                                              \
    } while (0)

/* taps at the onsets, each moved by off[i % noff] ms (a missing onset where skip is set) */
static int taps_for(const rh_pattern_t *p, int bpm, const int *off, int noff, uint64_t skip, int32_t *t)
{
    int i, m = 0;
    for (i = 0; i < p->n; i++)
        if (!(skip >> i & 1u)) t[m++] = rh_onset_ms(p, i, bpm) + off[i % noff];
    return m;
}

static void test_parse(void)
{
    rh_pattern_t p;
    char s[RH_TEXT_MAX];
    CHECK(rh_parse("q ee r q", 4, &p) == 0);
    CHECK(p.n == 4 && p.nbeats == 4);
    CHECK(p.on[0] == 0 && p.on[1] == 12 && p.on[2] == 18 && p.on[3] == 36);
    CHECK(p.flag[3] & RH_F_AFTER_REST);
    CHECK(!(p.flag[1] & RH_F_AFTER_REST));
    CHECK(p.flag[0] & RH_F_DOWNBEAT && !(p.flag[2] & RH_F_DOWNBEAT));
    rh_text(&p, s);
    CHECK(!strcmp(s, "q ee r q"));
    CHECK(rh_parse("h syn | ssss trip q ds", 4, &p) == 0);
    CHECK(p.nbeats == 8);
    rh_text(&p, s);
    CHECK(!strcmp(s, "h syn | ssss trip q ds"));
    CHECK(p.flag[2] & RH_F_SYNCO);       /* SYN-CO-PA's middle: tick 2 * 12 + 6 */
    CHECK(p.on[2] == 30);
    CHECK(rh_parse("q x", 4, &p) < 0);
    CHECK(rh_parse("", 4, &p) < 0);
    CHECK(rh_parse("h h h h h", 4, &p) < 0);   /* longer than two bars */
}

static void test_generate(void)
{
    uint32_t seed = 12345;
    int lv, k, i, used[RH_NCELLS];
    memset(used, 0, sizeof used);
    for (lv = 1; lv <= RH_LEVELS; lv++)
        for (k = 0; k < 400; k++) {
            rh_pattern_t p;
            int bpb = 2 + k % 3, bars = 1 + k % 2, beats = 0;
            rh_generate(&seed, lv, bpb, bars, &p);
            CHECK(p.nbeats == bpb * bars);
            CHECK(p.n >= 2);
            CHECK(p.cells[0] != RH_REST);
            for (i = 0; i < p.ncells; i++) {
                CHECK(rh_cell_level(p.cells[i]) <= lv);
                if (i) CHECK(!(p.cells[i] == RH_REST && p.cells[i - 1] == RH_REST));
                /* a two-beat cell never across a bar line */
                CHECK(beats / bpb == (beats + rh_cell_beats(p.cells[i]) - 1) / bpb);
                beats += rh_cell_beats(p.cells[i]);
                if (lv == RH_LEVELS) used[p.cells[i]]++;
            }
        }
    for (i = 0; i < RH_NCELLS; i++) CHECK(used[i] > 0);   /* every cell turns up at the top level */
}

static void test_timing(void)
{
    rh_pattern_t p;
    rh_parse("q ee ssss trip", 4, &p);
    CHECK(rh_onset_ms(&p, 1, 120) == 500);
    CHECK(rh_onset_ms(&p, 2, 120) == 750);
    CHECK(rh_onset_ms(&p, 4, 120) == 1125);   /* the second sixteenth of beat 3 */
    CHECK(rh_onset_ms(&p, 8, 120) == 1667);   /* triplet: 1500 + 166.7 */
    CHECK(rh_tol_good(60) == 35);
    CHECK(rh_tol_good(120) == 35);
    CHECK(rh_tol_good(240) == 17);
}

static void test_score(void)
{
    rh_pattern_t p;
    rh_result_t r;
    int32_t t[RH_MAX_TAPS];
    int z[1] = { 0 }, early[1] = { -40 }, late[1] = { 40 }, small[2] = { 5, -5 }, m, i;

    rh_parse("q ee r q | ssss q h", 4, &p);
    m = taps_for(&p, 100, z, 1, 0, t);
    rh_score(&p, 100, t, m, &r);
    CHECK(r.good == p.n && r.score == 100 && r.hint == RH_H_GOOD && r.mean == 0);

    m = taps_for(&p, 100, small, 2, 0, t);
    rh_score(&p, 100, t, m, &r);
    CHECK(r.good == p.n && r.hint == RH_H_GOOD);

    m = taps_for(&p, 100, early, 1, 0, t);
    rh_score(&p, 100, t, m, &r);
    CHECK(r.hint == RH_H_RUSHING && r.mean == -40 && r.ok == p.n);

    m = taps_for(&p, 100, late, 1, 0, t);
    rh_score(&p, 100, t, m, &r);
    CHECK(r.hint == RH_H_DRAGGING && r.mean == 40);

    /* one note missed: the rest still line up */
    m = taps_for(&p, 100, z, 1, 1u << 2, t);
    rh_score(&p, 100, t, m, &r);
    CHECK(r.missed == 1 && r.grade[2] == RH_G_MISS && r.good == p.n - 1 && r.hint == RH_H_MISSED);
    for (i = 0; i < p.n; i++)
        if (i != 2) CHECK(r.err[i] == 0);

    /* an extra tap between two notes */
    m = taps_for(&p, 100, z, 1, 0, t);
    for (i = m; i > 2; i--) t[i] = t[i - 1];
    t[2] = t[1] + 290;                   /* between 600 and 900 ms: closest to neither */
    rh_score(&p, 100, t, m + 1, &r);
    CHECK(r.extra == 1 && r.good == p.n && r.hint == RH_H_EXTRA);

    /* late after the rest only */
    m = taps_for(&p, 100, z, 1, 0, t);
    t[3] += 50;                          /* the q after r */
    rh_score(&p, 100, t, m, &r);
    CHECK(r.hint == RH_H_LATE_REST);

    /* the sixteenths galloped: long short long */
    {
        int g = 4;                       /* first of the ssss */
        m = taps_for(&p, 100, z, 1, 0, t);
        t[g + 1] += 25;
        t[g + 2] -= 0;
        t[g + 3] += 25;
        rh_score(&p, 100, t, m, &r);
        CHECK(r.hint == RH_H_UNEVEN16);
    }

    /* syncopation pulled onto the beat */
    rh_parse("q syn q", 4, &p);
    m = taps_for(&p, 90, z, 1, 0, t);
    t[2] += 140;                         /* the CO tapped near the next beat (beat 667 ms: off-beat at +333) */
    rh_score(&p, 90, t, m, &r);
    CHECK(r.hint == RH_H_SYNCO);

    /* nothing tapped */
    rh_score(&p, 90, t, 0, &r);
    CHECK(r.hint == RH_H_NONE && r.missed == p.n && r.score == 0);

    /* every tap twice */
    rh_parse("q q q q", 4, &p);
    m = taps_for(&p, 120, z, 1, 0, t);
    for (i = 0; i < 4; i++) t[4 + i] = t[i] + 120;
    for (i = 0; i < 8; i++) {            /* sort */
        int j;
        for (j = i + 1; j < 8; j++)
            if (t[j] < t[i]) {
                int32_t x = t[i];
                t[i] = t[j];
                t[j] = x;
            }
    }
    rh_score(&p, 120, t, 8, &r);
    CHECK(r.extra == 4 && r.good == 4 && r.hint == RH_H_EXTRA && r.score == 50);
}

int main(void)
{
    test_parse();
    test_generate();
    test_timing();
    test_score();
    printf("rhythm: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
