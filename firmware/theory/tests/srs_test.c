/* SPDX-License-Identifier: GPL-3.0-or-later */
/* srs.c and course.h on the computer: rounds, boxes, grades, interleaving, focus, groups, backup.
 *   just theory-unit */
#include <stdio.h>
#include <string.h>
#include "../theory.c"
#include "../srs.c"
#include "../course.h"

static int fails, checks;
#define CHECK(c, ...) do { checks++; if (!(c)) { printf("FAIL %s:%d ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); fails++; } } while (0)

static sr_state_t st;
static sr_round_t rd;

static void play_round(int grade, uint32_t focus, uint32_t seed)
{
    const sr_card_t *c;
    sr_build(&st, &rd, &course, 15, focus, seed);
    while ((c = sr_card(&rd)) != 0) sr_answer(&st, &rd, grade);
    sr_finish(&st, &rd, &course);
}

static void test_course(void)
{
    CHECK(course.n == CO_N, "63 items (%d)", course.n);
    CHECK(co_group_end[0] == 6 && co_group_end[1] == 11 && co_group_end[CO_KEYS - 1] == CO_N, "groups end at 6, 11 .. 63");
    CHECK(co_skill[0] == SK_SCALE && co_key[0] == 0, "item 0: the C major scale");
    CHECK(co_skill[4] == SK_EARQ && co_param[4] == TH_MAJ && co_skill[5] == SK_EARQ && co_param[5] == TH_MIN,
          "items 4, 5: major and minor by ear");
    CHECK(co_skill[6] == SK_SCALE && CO_TONIC[co_key[6]] == 1 && co_skill[7] == SK_KEYSIG, "item 6: G's scale, then its signature");
    CHECK(co_skill[CO_N - 1] == SK_EARDEG && CO_TONIC[co_key[CO_N - 1]] == 6, "the last: degrees by ear in F#");
    {
        int i, dim = -1, aug = -1;
        for (i = 0; i < CO_N; i++) {
            if (co_skill[i] == SK_EARQ && co_param[i] == TH_DIM) dim = i;
            if (co_skill[i] == SK_EARQ && co_param[i] == TH_AUG) aug = i;
            CHECK(co_seg[i] == (co_skill[i] >= SK_EARDEG ? SEG_EAR : SEG_THEORY), "item %d's segment", i);
            CHECK(!(co_skill[i] == SK_KEYSIG && co_key[i] == 0), "no key signature card for C");
        }
        CHECK(dim == 21 && aug == 32, "diminished with D (item %d), augmented with A (item %d)", dim, aug);
    }
}

static void test_first_round(void)
{
    int i, shows = 0, asks = 0, again = 0;
    sr_init(&st, &course);
    CHECK(st.open == 6 && st.round == 0 && sr_valid(&st, &course), "a new course: 6 open");
    CHECK(sr_new_count(&st, &course, 3) == 6 && sr_due_count(&st, &course, 3) == 0, "6 new, none due");
    sr_build(&st, &rd, &course, 15, 3, 7);
    for (i = 0; i < rd.n; i++) {
        if (rd.card[i].kind == SR_SHOW) {
            CHECK(i == shows, "the new ones shown first, together");
            CHECK(rd.card[i].item == i, "in course order (%d)", rd.card[i].item);
            shows++;
        } else if (rd.card[i].again) again++;
        else asks++;
    }
    CHECK(shows == 3, "3 new at most (%d)", shows);
    CHECK(asks == 3 && again == 5, "each asked, the chords twice more as practice, then again at the end (%d, %d)",
          asks, again);
    while (sr_card(&rd)) sr_answer(&st, &rd, SR_RIGHT);
    CHECK(sr_finish(&st, &rd, &course) == 0 && st.round == 1 && st.streak == 1, "round 1, streak 1");
    for (i = 0; i < 3; i++) CHECK(st.box[i] == 1, "item %d: box 1 (the repeat does not move it up) (%d)", i, st.box[i]);
    CHECK(st.answered == 8 && st.right == 8, "8 answered (%u)", (unsigned)st.answered);
}

static void test_grades(void)
{
    const sr_card_t *c;
    int it, n0;
    sr_init(&st, &course);
    st.round = 10;
    st.last[0] = 5;                      /* item 0: seen, box 2, due (5 rounds >= 4) */
    st.box[0] = 2;
    st.last[1] = 9;                      /* item 1: box 3, not due */
    st.box[1] = 3;
    CHECK(sr_due(&st, 0) && !sr_due(&st, 1), "due after 2^box rounds");
    sr_build(&st, &rd, &course, 4, 3, 3);
    while ((c = sr_card(&rd)) && (c->kind == SR_SHOW || c->item != 0)) sr_answer(&st, &rd, SR_RIGHT);
    CHECK(c && c->item == 0, "item 0 is asked");
    n0 = rd.n;
    sr_answer(&st, &rd, SR_WRONG);
    CHECK(st.box[0] == 0 && rd.n == n0 + 1 && rd.card[rd.n - 1].item == 0 && rd.card[rd.n - 1].again,
          "wrong: box 0, asked again at the end");
    /* slow keeps the box and asks again; easy goes up two once past box 0 */
    sr_init(&st, &course);
    st.round = 20;
    for (it = 0; it < 6; it++) { st.last[it] = 0; st.box[it] = 1; }
    sr_build(&st, &rd, &course, 6, 3, 9);
    c = sr_card(&rd);
    it = c->item;
    n0 = rd.n;
    sr_answer(&st, &rd, SR_SLOW);
    CHECK(st.box[it] == 1 && rd.n == n0 + 1, "slow: box stays, asked again");
    c = sr_card(&rd);
    it = c->item;
    sr_answer(&st, &rd, SR_EASY);
    CHECK(st.box[it] == 3, "easy from box 1: box 3 (%d)", st.box[it]);
    st.box[it] = 0;
    st.last[it] = 0;
    {
        sr_round_t r2;
        sr_build(&st, &r2, &course, 6, 3, 11);
        while ((c = sr_card(&r2)) && c->item != it) sr_answer(&st, &r2, SR_RIGHT);
        sr_answer(&st, &r2, SR_EASY);
        CHECK(st.box[it] == 1, "easy from box 0: one box only (%d)", st.box[it]);
    }
}

static void test_spread_focus(void)
{
    int i, t, ear = 0, theory = 0, seen_ear = 0;
    sr_init(&st, &course);
    st.open = CO_N;
    st.round = 100;
    for (i = 0; i < CO_N; i++) { st.last[i] = 0; st.box[i] = (uint8_t)(i % 3); }
    for (t = 0; t < 50; t++) {
        sr_build(&st, &rd, &course, 24, 3, 1000u + (uint32_t)t);
        for (i = 0; i < rd.n; i++) {
            int item = rd.card[i].item;
            if (co_seg[item] == SEG_EAR) seen_ear = 1;
            else CHECK(!seen_ear || rd.card[i].again, "round %d: theory before ear (card %d)", t, i);
            if (i && !rd.card[i].again && !rd.card[i - 1].again && co_seg[item] == co_seg[rd.card[i - 1].item]
                && co_key[item] != SR_NOKEY)
                CHECK(co_key[item] != co_key[rd.card[i - 1].item], "round %d: one key twice in a row at %d", t, i);
        }
        seen_ear = 0;
    }
    sr_build(&st, &rd, &course, 24, 1u << SEG_EAR, 5);
    for (i = 0; i < rd.n; i++) {
        if (co_seg[rd.card[i].item] == SEG_EAR) ear++;
        else theory++;
    }
    CHECK(ear > 0 && theory == 0, "focus on ear: no theory cards (%d, %d)", ear, theory);
    CHECK(rd.n <= (int)SR_CARDS_MAX, "a round fits");
}

static void test_variety(void)
{
    int i, t;
    /* the first group only: rounds fill up with practice, never one item twice in a row, and practice does not move */
    for (t = 0; t < 40; t++) {
        int extras = 0, per[CO_N];
        sr_init(&st, &course);
        st.round = 3;
        for (i = 0; i < 6; i++) { st.last[i] = 2; st.box[i] = 1; }
        memset(per, 0, sizeof per);
        sr_build(&st, &rd, &course, 15, 3, 77u + (uint32_t)t * 13u);
        for (i = 0; i < rd.n; i++) {
            int it = rd.card[i].item;
            if (i) CHECK(it != rd.card[i - 1].item || rd.card[i].again, "round %d: item %d twice in a row at %d", t, it, i);
            if (rd.card[i].again && i < rd.n) per[it]++;
        }
        for (i = 0; i < 6; i++) {
            if (co_vary[i]) extras += per[i];
            else CHECK(per[i] <= 1, "round %d: item %d (fixed question) practised (%d)", t, i, per[i]);
        }
        CHECK(extras > 0, "round %d: practice cards fill it", t);
        while (sr_card(&rd)) sr_answer(&st, &rd, SR_RIGHT);
        for (i = 0; i < 6; i++) CHECK(st.box[i] == 2, "round %d: item %d up one box only (%d)", t, i, st.box[i]);
    }
    /* one key */
    sr_init(&st, &course);
    st.open = CO_N;
    st.round = 50;
    for (i = 0; i < CO_N; i++) { st.last[i] = 0; st.box[i] = 2; }
    for (t = 0; t < 20; t++) {
        int k = t % CO_KEYS;
        sr_build(&st, &rd, &course, 15, SR_FOCUS_KEY(k), 5u + (uint32_t)t);
        CHECK(rd.n > 0, "key %d: a round", k);
        for (i = 0; i < rd.n; i++) CHECK(co_key[rd.card[i].item] == k, "key %d: only its items (%d)", k, rd.card[i].item);
    }
    CHECK(sr_due_count(&st, &course, SR_FOCUS_KEY(3)) == 5, "D: 5 items due (%d)", sr_due_count(&st, &course, SR_FOCUS_KEY(3)));
}

static void test_groups(void)
{
    uint8_t w[SR_MAX];
    int i;
    sr_init(&st, &course);
    for (i = 0; i < 6; i++) { st.last[i] = 0; st.box[i] = 2; }
    st.box[3] = 1;
    CHECK(sr_waiting(&st, &course, w, SR_MAX) == 1 && w[0] == 3, "item 3 holds the next key back");
    st.box[3] = 2;
    sr_build(&st, &rd, &course, 4, 3, 1);
    while (sr_card(&rd)) sr_answer(&st, &rd, SR_RIGHT);
    CHECK(sr_finish(&st, &rd, &course) == 1 && st.open == 11 && rd.opened, "every open item at box 2: G opens (%d)", st.open);
    /* a long run of right answers opens everything, and the state stays valid */
    sr_init(&st, &course);
    for (i = 0; i < 400 && st.open < CO_N; i++) play_round(SR_RIGHT, 3, 50u + (uint32_t)i);
    CHECK(st.open == CO_N && sr_valid(&st, &course), "all 63 open after %d rounds", i);
    /* random answers: always valid */
    {
        uint32_t rng = 99;
        sr_init(&st, &course);
        for (i = 0; i < 300; i++) {
            const sr_card_t *c;
            sr_build(&st, &rd, &course, 8 + i % 17, (i % 5) ? 3 : 1, rng);
            while ((c = sr_card(&rd)) != 0) sr_answer(&st, &rd, (int)(sr_rand(&rng) % 4));
            sr_finish(&st, &rd, &course);
            CHECK(sr_valid(&st, &course), "random round %d", i);
        }
    }
}

static void test_blob(void)
{
    static uint8_t b[SR_BLOB_N(SR_MAX)];
    static sr_state_t back;
    int n;
    sr_init(&st, &course);
    play_round(SR_RIGHT, 3, 1);
    play_round(SR_WRONG, 3, 2);
    n = sr_blob(&st, b);
    CHECK(n == SR_BLOB_N(CO_N), "blob is 14 + 3n bytes (%d)", n);
    sr_init(&back, &course);
    CHECK(sr_unblob(&back, &course, b, n) && !memcmp(&back, &st, sizeof st), "round trip");
    b[14] = 9;                           /* box 9 */
    sr_init(&back, &course);
    CHECK(!sr_unblob(&back, &course, b, n) && back.round == 0, "a bad box is refused, nothing changed");
    b[14] = 0;
    CHECK(!sr_unblob(&back, &course, b, n - 1), "a short blob is refused");
    b[4] = 62;
    CHECK(!sr_unblob(&back, &course, b, n), "another course's blob is refused");
}

int main(void)
{
    course_init();
    test_course();
    test_first_round();
    test_grades();
    test_spread_focus();
    test_variety();
    test_groups();
    test_blob();
    printf("%s: %d checks, %d failed\n", fails ? "FAIL" : "ok", checks, fails);
    return fails != 0;
}
