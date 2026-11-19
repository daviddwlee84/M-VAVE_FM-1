/* SPDX-License-Identifier: GPL-3.0-or-later */
/* groove.c on this computer: just drums-unit. The styles parse, and every request lands where FILL AT says. */
#include <stdio.h>
#include <string.h>
#include "../groove.c"
#include "../styles.h"

static int fails, checks;
#define CHECK(c, ...)                                   \
    do {                                                \
        checks++;                                       \
        if (!(c)) {                                     \
            fails++;                                    \
            printf("FAIL %s:%d: ", __FILE__, __LINE__); \
            printf(__VA_ARGS__);                        \
            printf("\n");                               \
        }                                               \
    } while (0)

static groove_t g;
static uint32_t t;                       /* the next tick to play */
static gv_out_t out;
static uint32_t hits[GV_LANES];

static void tick1(void)
{
    uint32_t i;
    groove_tick(&g, t++, &out);
    for (i = 0; i < out.n; i++) hits[out.e[i].lane]++;
}

/* play until bar b has begun (its first tick played); returns its part */
static int to_bar(uint32_t b)
{
    while (g.state != GS_STOP && g.bars < b) tick1();
    return g.part;
}

/* play to step s of the current bar (its tick played) */
static void to_step(uint32_t s)
{
    uint32_t want = g.bar_start + s * (GV_TPB / g.st->spb);
    while (g.state != GS_STOP && t <= want) tick1();
}

static void fresh(int style, int fill_at, int phrase, int autof)
{
    groove_init(&g, gv_style(style));
    g.fill_at = (uint8_t)fill_at;
    g.phrase = (uint8_t)phrase;
    g.auto_fill = (uint8_t)autof;
    groove_start(&g, 0);
    t = 0;
    memset(hits, 0, sizeof hits);
    tick1();
}

static void test_styles(void)
{
    int i, n = gv_load(DR_STYLES, DR_NSTYLES);
    CHECK(n == DR_NSTYLES, "gv_load %d of %d: style %d, %s", n, DR_NSTYLES, n < 0 ? -n - 1 : n, gv_error());
    for (i = 0; i < gv_count(); i++) {
        const gv_style_t *st = gv_style(i);
        int p;
        for (p = 0; p < GV_PARTS; p++) CHECK(st->n[p] > 0, "%s part %d empty", st->name, p);
        CHECK(st->bpm >= 30 && st->bpm <= 240, "%s bpm", st->name);
    }
}

static int find_style(const char *name)
{
    int i;
    for (i = 0; i < gv_count(); i++)
        if (!strcmp(gv_style(i)->name, name)) return i;
    return 0;
}

static void test_plain(void)
{
    uint32_t b;
    fresh(0, FA_BAR, 4, 1);
    for (b = 1; b < 9; b++) CHECK(to_bar(b) == P_MAIN_A, "bar %u: main A", b);
    CHECK(hits[L_KICK] > 0 && hits[L_SNARE] > 0, "kick and snare play");
    CHECK(hits[L_CRASH] == 0, "no crash without a fill");
}

/* BAR: a fill asked on any step of bar 2 is bar 3, then MAIN A again with a crash */
static void test_bar(void)
{
    uint32_t s, steps = gv_style(0)->steps;
    for (s = 0; s < steps; s++) {
        fresh(0, FA_BAR, 4, 1);
        to_bar(2);
        to_step(s);
        groove_fill(&g);
        CHECK(g.part == P_MAIN_A, "BAR step %u: bar 2 still main", s);
        CHECK(to_bar(3) == P_FILL_A, "BAR step %u: bar 3 the fill", s);
        hits[L_CRASH] = 0;
        CHECK(to_bar(4) == P_MAIN_A, "BAR step %u: bar 4 main again", s);
        CHECK(hits[L_CRASH] == 1, "BAR step %u: a crash on bar 4's one", s);
    }
}

/* NOW: from the next step to the bar's end; on the last step, the next bar */
static void test_now(void)
{
    uint32_t s, steps = gv_style(0)->steps;
    for (s = 0; s < steps; s++) {
        fresh(0, FA_NOW, 4, 1);
        to_bar(1);
        to_step(s);
        groove_fill(&g);
        if (s + 1 < steps) {
            CHECK(g.part == P_FILL_A && g.fill_from == s + 1, "NOW step %u: fill from %u (part %d from %u)", s, s + 1,
                  g.part, g.fill_from);
            CHECK(to_bar(2) == P_MAIN_A, "NOW step %u: bar 2 main", s);
        } else {
            CHECK(g.part == P_MAIN_A && g.req_fill, "NOW last step: waits");
            CHECK(to_bar(2) == P_FILL_A, "NOW last step: bar 2 the fill");
        }
    }
}

/* PHRASE: the last bar of the phrase; asked in that bar, now */
static void test_phrase(void)
{
    static const int ph[] = { 4, 8, 16 };
    uint32_t i, b;
    for (i = 0; i < 3; i++) {
        int p = ph[i];
        for (b = 0; b + 1 < (uint32_t)p; b++) {
            fresh(0, FA_PHRASE, p, 1);
            to_bar(b);
            to_step(5);
            groove_fill(&g);
            CHECK(to_bar((uint32_t)p - 1) == P_FILL_A, "PHRASE %d asked in bar %u: bar %d the fill", p, b, p - 1);
            CHECK(to_bar((uint32_t)p) == P_MAIN_A, "PHRASE %d: then main", p);
        }
        fresh(0, FA_PHRASE, p, 1);
        to_bar((uint32_t)p - 1);
        to_step(3);
        groove_fill(&g);
        CHECK(g.part == P_FILL_A && g.fill_from == 4, "PHRASE %d asked in its last bar: now", p);
        /* the phrase goes on counting: bar 2p-1 is the next phrase's last */
        CHECK(to_bar((uint32_t)p) == P_MAIN_A, "PHRASE %d: main after", p);
    }
}

static void test_main(void)
{
    /* BAR + AUTO FILL: the fill, then B (its bar 0) with a crash */
    fresh(0, FA_BAR, 4, 1);
    to_bar(1);
    to_step(7);
    groove_main(&g, 1);
    CHECK(to_bar(2) == P_FILL_A, "BAR auto: bar 2 fill");
    hits[L_CRASH] = 0;
    CHECK(to_bar(3) == P_MAIN_B && g.sec == 1 && g.bar_in_sec == 0, "BAR auto: bar 3 B, phrase from 0");
    CHECK(hits[L_CRASH] >= 1, "BAR auto: crash into B");
    /* BAR without: B at once */
    fresh(0, FA_BAR, 4, 0);
    to_bar(1);
    to_step(7);
    groove_main(&g, 1);
    CHECK(to_bar(2) == P_MAIN_B, "BAR plain: bar 2 B");
    /* NOW + AUTO FILL: the fill now, B next bar */
    fresh(0, FA_NOW, 4, 1);
    to_bar(1);
    to_step(7);
    groove_main(&g, 1);
    CHECK(g.part == P_FILL_A && g.fill_from == 8, "NOW auto: fill from step 8");
    CHECK(to_bar(2) == P_MAIN_B, "NOW auto: bar 2 B");
    /* PHRASE + AUTO FILL: the phrase's last bar is the fill, then B */
    fresh(0, FA_PHRASE, 8, 1);
    to_bar(1);
    groove_main(&g, 1);
    CHECK(to_bar(6) == P_MAIN_A, "PHRASE auto: bar 6 still A");
    CHECK(to_bar(7) == P_FILL_A, "PHRASE auto: bar 7 fill");
    CHECK(to_bar(8) == P_MAIN_B && g.bar_in_sec == 0, "PHRASE auto: bar 8 B");
    /* PHRASE without: B after the phrase, no fill */
    fresh(0, FA_PHRASE, 4, 0);
    to_bar(1);
    groove_main(&g, 1);
    CHECK(to_bar(3) == P_MAIN_A, "PHRASE plain: bar 3 A");
    CHECK(to_bar(4) == P_MAIN_B, "PHRASE plain: bar 4 B");
    /* the MAIN it plays: a fill that returns to it */
    fresh(0, FA_BAR, 4, 1);
    to_bar(1);
    groove_main(&g, 0);
    CHECK(to_bar(2) == P_FILL_A && to_bar(3) == P_MAIN_A, "same MAIN: a fill back into A");
    /* asked back before the change: cancelled */
    fresh(0, FA_PHRASE, 4, 1);
    to_bar(1);
    groove_main(&g, 1);
    groove_main(&g, 0);
    CHECK(to_bar(3) == P_MAIN_A && to_bar(4) == P_MAIN_A, "change cancelled");
    /* MAIN and a fill asked together: one fill, then B */
    fresh(0, FA_BAR, 4, 1);
    to_bar(1);
    groove_fill(&g);
    groove_main(&g, 1);
    CHECK(to_bar(2) == P_FILL_A, "fill + main: bar 2 fill");
    CHECK(to_bar(3) == P_MAIN_B, "fill + main: bar 3 B");
}

static void test_hold(void)
{
    fresh(0, FA_PHRASE, 4, 1);
    to_bar(1);
    to_step(6);
    groove_hold(&g, 1);
    CHECK(g.part == P_FILL_A && g.fill_from == 7, "hold: fill from the next step");
    CHECK(to_bar(2) == P_FILL_A, "hold: bar 2 the fill again");
    CHECK(to_bar(3) == P_FILL_A, "hold: bar 3 too");
    to_step(2);
    groove_hold(&g, 0);
    CHECK(g.part == P_FILL_A, "released: the bar plays out");
    CHECK(to_bar(4) == P_MAIN_B && g.bar_in_sec == 0, "released: then B");
    groove_hold(&g, 1);
    groove_hold(&g, 0);
    CHECK(to_bar(5) == P_MAIN_A, "and back to A");
}

static void test_end(void)
{
    uint32_t before;
    fresh(0, FA_PHRASE, 4, 1);
    to_bar(2);
    to_step(3);
    groove_end(&g);
    CHECK(to_bar(3) == P_END && g.state == GS_END, "end: bar 3 the ending");
    to_bar(4);
    CHECK(g.state == GS_STOP, "end: stopped after it");
    before = hits[L_KICK] + hits[L_SNARE] + hits[L_CHH];
    {
        uint32_t i;
        for (i = 0; i < 1000; i++) tick1();
    }
    CHECK(hits[L_KICK] + hits[L_SNARE] + hits[L_CHH] == before, "end: silence after");
}

static void test_metres(void)
{
    int w = find_style("WALTZ 3/4"), b = find_style("BALLAD 6/8"), sh = find_style("SHUFFLE");
    fresh(w, FA_BAR, 4, 1);
    CHECK(g.bar_ticks == 3 * GV_TPB, "waltz bar");
    to_bar(1);
    groove_fill(&g);
    CHECK(to_bar(2) == P_FILL_A && g.bar_start == 2u * 3u * GV_TPB, "waltz: fill at bar 2's tick");
    fresh(b, FA_PHRASE, 4, 1);
    CHECK(g.bar_ticks == 2 * GV_TPB && g.st->steps == 6, "6/8 bar: two beats of three");
    to_bar(1);
    groove_fill(&g);
    CHECK(to_bar(3) == P_FILL_A, "6/8 phrase fill");
    fresh(sh, FA_NOW, 4, 1);
    to_bar(1);
    to_step(4);
    groove_fill(&g);
    CHECK(g.fill_from == 5, "shuffle: steps of a triplet");
}

static void test_swing_mute(void)
{
    uint32_t straight[GV_LANES], i, b;
    fresh(0, FA_BAR, 4, 1);
    to_bar(4);
    memcpy(straight, hits, sizeof hits);
    fresh(0, FA_BAR, 4, 1);
    g.swing = 8;
    to_bar(4);
    for (i = 0; i < GV_LANES; i++) CHECK(hits[i] == straight[i], "swing keeps lane %u's notes", i);
    /* where the off 16th lands: the hats of POP 16 are 16ths */
    fresh(find_style("POP 16"), FA_BAR, 4, 1);
    g.swing = 8;
    to_bar(1);
    for (b = 0; b < 24 + 8; b++) {
        groove_tick(&g, t++, &out);
        if (b == 24) CHECK(out.n == 0, "swing: nothing on the straight off 16th");
        if (b == 24 + 8 - 1) CHECK(out.n > 0, "swing: the off 16th 8 ticks late");
    }
    fresh(0, FA_BAR, 4, 1);
    g.mutes = 1u << L_KICK;
    memset(hits, 0, sizeof hits);        /* (fresh played tick 0) */
    to_bar(4);
    CHECK(hits[L_KICK] == 0 && hits[L_SNARE] > 0, "mute: no kick, the rest plays");
}

static void test_style_change(void)
{
    fresh(0, FA_BAR, 4, 1);
    to_bar(1);
    to_step(3);
    groove_set_style(&g, gv_style(find_style("WALTZ 3/4")));
    CHECK(g.bar_ticks == 4 * GV_TPB, "style: this bar stays");
    to_bar(2);
    CHECK(g.bar_ticks == 3 * GV_TPB && g.bar_start == 2u * 4u * GV_TPB, "style: at the next bar");
}

int main(void)
{
    test_styles();
    test_plain();
    test_bar();
    test_now();
    test_phrase();
    test_main();
    test_hold();
    test_end();
    test_metres();
    test_swing_mute();
    test_style_change();
    printf("groove: %d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
