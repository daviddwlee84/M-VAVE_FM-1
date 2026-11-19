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

/* ---- v0.3 */
/* N beats at bpm, every tap moved by off ms, drift ms more each beat, tempo scaled by (1 + pct/1000) */
static int pulse_taps(int bpm, int n, int off, float drift, int pct_x10, uint64_t skip, int32_t *t)
{
    float beat = 60000.0f / (float)bpm / (1.0f + (float)pct_x10 / 1000.0f);
    int k, m = 0;
    for (k = 0; k < n; k++)
        if (!(skip >> k & 1u)) t[m++] = (int32_t)((float)k * beat + (float)off + drift * (float)k + 0.5f);
    return m;
}

static void test_pulse(void)
{
    int32_t t[RH_PULSE_TAPS];
    rh_pulse_t r;
    int m, k, i;
    uint32_t seed = 99;
    /* exact */
    m = pulse_taps(100, 16, 0, 0.0f, 0, 0, t);
    rh_pulse(100, 16, t, m, &r);
    CHECK(r.answered == 16 && r.missed == 0 && r.extra == 0 && r.good == 16 && r.score == 100);
    CHECK(r.hint == RH_P_GOOD && r.bpm_x10 == 1000 && r.tempo_x10 == 0 && r.jitter == 0 && r.hist[RH_HIST / 2] == 16);
    /* 3 % fast over 8 bars: rushing, counted beat by beat though the taps end a beat early */
    m = pulse_taps(100, 32, 0, 0.0f, 30, 0, t);
    rh_pulse(100, 32, t, m, &r);
    CHECK(r.answered == 32 && r.extra == 0 && r.missed == 0);
    CHECK(r.tempo_x10 >= 29 && r.tempo_x10 <= 31 && r.bpm_x10 >= 1029 && r.bpm_x10 <= 1031);
    CHECK(r.hint == RH_P_RUSHING && r.end_ms < -500 && r.trend_x10 > -5 && r.trend_x10 < 5);
    /* 4 % slow: dragging */
    m = pulse_taps(120, 16, 0, 0.0f, -40, 0, t);
    rh_pulse(120, 16, t, m, &r);
    CHECK(r.hint == RH_P_DRAGGING && r.tempo_x10 <= -38 && r.tempo_x10 >= -42);
    /* speeding up: first half on tempo, second 8 % fast */
    m = pulse_taps(90, 16, 0, 0.0f, 0, 0, t);
    for (k = 9; k < 16; k++) t[k] = t[8] + (int32_t)((float)(k - 8) * 60000.0f / 90.0f / 1.08f);
    rh_pulse(90, 16, t, m, &r);
    CHECK(r.trend_x10 > 30 && r.hint == RH_P_SPEEDING);
    for (k = 9; k < 16; k++) t[k] = t[8] + (int32_t)((float)(k - 8) * 60000.0f / 90.0f / 0.92f);
    rh_pulse(90, 16, t, m, &r);
    CHECK(r.trend_x10 < -30 && r.hint == RH_P_SLOWING);
    /* steady offset 60 ms early, right tempo: rushing by the mean */
    m = pulse_taps(100, 16, -60, 0.0f, 0, 0, t);
    rh_pulse(100, 16, t, m, &r);
    CHECK(r.mean == -60 && r.tempo_x10 == 0 && r.hint == RH_P_RUSHING);
    /* jitter: alternate +-30 ms */
    m = pulse_taps(100, 16, 0, 0.0f, 0, 0, t);
    for (k = 0; k < m; k++) t[k] += k % 2 ? 30 : -30;
    rh_pulse(100, 16, t, m, &r);
    CHECK(r.jitter >= 28 && r.jitter <= 31 && r.hint == RH_P_UNSTEADY);
    /* a missed beat, then a double tap: the rest still on their beats */
    m = pulse_taps(100, 16, 0, 0.0f, 0, 1u << 5, t);
    rh_pulse(100, 16, t, m, &r);
    CHECK(r.missed == 1 && r.tap_of[5] < 0 && r.good == 15 && r.hint == RH_P_MISSED);
    m = pulse_taps(100, 16, 0, 0.0f, 0, 0, t);
    for (k = m; k > 7; k--) t[k] = t[k - 1];
    t[7] = t[6] + 40;
    m++;
    rh_pulse(100, 16, t, m, &r);
    CHECK(r.extra == 1 && r.missed == 0 && r.good == 16 && r.hint == RH_P_EXTRA);
    /* taps after the last beat are extra; none at all */
    m = pulse_taps(100, 20, 0, 0.0f, 0, 0, t);
    rh_pulse(100, 16, t, m, &r);
    CHECK(r.extra == 4 && r.good == 16);
    rh_pulse(100, 16, t, 0, &r);
    CHECK(r.hint == RH_P_NONE && r.missed == 16 && r.score == 0);
    /* random: never out of bounds, the histogram adds up */
    for (i = 0; i < 3000; i++) {
        int bpm = 40 + (int)(rnd(&seed) % 200u), n = 4 + (int)(rnd(&seed) % 61u), s = 0;
        m = (int)(rnd(&seed) % (uint32_t)RH_PULSE_TAPS);
        t[0] = (int32_t)(rnd(&seed) % 500u);
        for (k = 1; k < m; k++) t[k] = t[k - 1] + (int32_t)(rnd(&seed) % 1500u);
        rh_pulse(bpm, n, t, m, &r);
        for (k = 0; k < RH_HIST; k++) s += r.hist[k];
        CHECK(s == r.answered && r.answered + r.missed == n && r.score <= 100 && r.hint < RH_P_N);
    }
    for (k = 0; k < RH_P_N; k++) CHECK(rh_pulse_more(k)[0] != 0);
}

/* a listener right 98 % above thr, and below it from a coin (d 0) rising to 98 % (d thr): the 2-down 1-up
 * staircase should settle where it is right 71 % of the time, about 0.44 thr */
static void test_stair(void)
{
    int type, run;
    uint32_t seed = 7;
    for (type = 0; type < RH_J_N; type++) {
        float thr = type == RH_J_FEEL ? 40.0f : 6.0f, sum = 0.0f;
        for (run = 0; run < 200; run++) {
            rh_stair_t s;
            float got;
            rh_stair_init(&s, type);
            while (!rh_stair_answer(&s, (float)(rnd(&seed) % 1000u) < 1000.0f * (s.d >= thr ? 0.98f : 0.5f + 0.48f * s.d / thr))) {}
            got = rh_stair_threshold(&s);
            sum += got;
            CHECK(s.trials <= RH_ST_TRIALS && s.d >= rh_judge_min(type));
            CHECK(got > thr * 0.08f && got < thr * 1.5f);
        }
        sum /= 200.0f;
        printf("  stair %s: mean threshold %.2f (listener's 71 %% point %.2f)\n", rh_judge_name(type), (double)sum, (double)(thr * 0.44f));
        CHECK(sum > thr * 0.3f && sum < thr * 0.65f);
    }
    {                                    /* always right: down to the floor */
        rh_stair_t s;
        int i;
        rh_stair_init(&s, RH_J_TEMPO);
        for (i = 0; i < 30; i++) rh_stair_answer(&s, 1);
        CHECK(s.d <= rh_judge_min(RH_J_TEMPO) + 0.001f && s.hist_x10[0] == 80 && s.right[0] == 1);
    }
}

static void test_seq(void)
{
    rh_seq_t q;
    int k, sign;
    float beat = 60000000.0f / 100.0f;
    for (sign = -1; sign <= 1; sign += 2) {
        /* TEMPO: the second phrase's beat is beat / (1 + d) */
        rh_judge_seq(RH_J_TEMPO, 100, 4, 10.0f, sign, &q);
        CHECK(q.n == 16 && q.ev[0] == RH_S_ACCENT && q.ev[8] == RH_S_ACCENT);
        {
            float b2 = (float)(q.us[15] - q.us[8]) / 7.0f, want = beat / (1.0f + (float)sign * 0.1f);
            CHECK(b2 > want - 2.0f && b2 < want + 2.0f);
            CHECK((float)(q.us[8] - q.us[7]) > 1.9f * beat && (float)(q.us[8] - q.us[7]) < 2.1f * beat);
        }
        /* FEEL: every note d ms off its eighth, early when rushing */
        rh_judge_seq(RH_J_FEEL, 100, 4, 40.0f, sign, &q);
        CHECK(q.n == 12 + 16);
        for (k = 1; k < q.n; k++) CHECK(q.us[k] >= q.us[k - 1]);
        {
            int notes = 0, i;
            for (i = 0; i < q.n; i++)
                if (q.ev[i] == RH_S_NOTE) {
                    float grid = 50000.0f + 4.0f * beat + (float)notes * beat / 2.0f;
                    float off = (float)q.us[i] - grid;
                    CHECK(off > -40000.0f * (float)sign - 2.0f && off < -40000.0f * (float)sign + 2.0f);
                    notes++;
                }
            CHECK(notes == 16);
        }
        /* DRIFT: the last gap is about the end tempo */
        rh_judge_seq(RH_J_DRIFT, 100, 4, 20.0f, sign, &q);
        CHECK(q.n == 16);
        {
            float last = (float)(q.us[15] - q.us[14]), want = beat / (1.0f + (float)sign * 0.2f);
            CHECK(last > want - 0.1f * beat && last < want + 0.1f * beat);
            CHECK(sign > 0 ? last < beat : last > beat);
        }
    }
    rh_clicks_seq(60, 3, 2, &q);
    CHECK(q.n == 6 && q.ev[3] == RH_S_ACCENT && q.us[1] - q.us[0] == 1000000u);
    for (k = 0; k < RH_NMETERS; k++) {
        int i, acc = 0, groups = 0;
        rh_meter_seq(k, 100, 1, 3, &q);
        CHECK(q.n == 3 * RH_METERS[k].eighths);
        for (i = 0; i < q.n; i++) acc += (q.ev[i] & RH_S_ACCENT) != 0, groups += (q.ev[i] & (RH_S_ACCENT | RH_S_CLICK)) != 0;
        CHECK(acc == 3 && groups == 3 * RH_METERS[k].ngroups);
    }
}

static void test_quiz(void)
{
    uint32_t seed = 12345;
    int type, level, i, k, b;
    uint8_t w[RH_NTERMS] = { 0 };
    char s[16];
    /* every BPM leaves at least three terms out, at every level */
    for (level = 1; level <= 2; level++)
        for (b = 16; b <= 240; b++) {
            int n = 0, items = rh_quiz_items(RH_Q_BPM_TERM, level);
            for (i = 0; i < items; i++) n += !rh_term_has(quiz_item(RH_Q_BPM_TERM, level, i), b);
            CHECK(n >= 3);
        }
    for (i = 0; i < RH_NTERMS; i++)
        for (k = 0; k < 200; k++) CHECK(rh_term_has(i, rh_term_draw_bpm(&seed, i)));
    rh_term_range(2, s);
    CHECK(!strcmp(s, "40-66"));
    rh_term_range(0, s);
    CHECK(!strcmp(s, "24-"));
    rh_term_range(15, s);
    CHECK(!strcmp(s, "200+"));
    CHECK(rh_quiz_items(RH_Q_TERM_BPM, 1) == 7 && rh_quiz_items(RH_Q_TERM_BPM, 2) == RH_NTERMS);
    CHECK(rh_quiz_items(RH_Q_METER, 1) == 4 && rh_quiz_items(RH_Q_METER, 3) == RH_NMETERS);
    CHECK(rh_quiz_items(RH_Q_CHANGE, 1) == 6 && rh_quiz_items(RH_Q_CHANGE, 2) == RH_NCHANGES);
    /* the only right answer is the one marked */
    for (type = 0; type < RH_Q_N; type++)
        for (level = 1; level <= 3; level++)
            for (i = 0; i < 3000; i++) {
                rh_quiz_t q;
                int right = 0, prev = i % 3 ? -1 : 2;
                rh_quiz_make(&seed, type, level, w, prev, &q);
                CHECK(q.answer < 4 && (prev < 0 || q.ask != prev || rh_quiz_items(type, level) < 2));
                for (k = 0; k < 4; k++) {
                    int ok;
                    if (type == RH_Q_TERM_BPM) ok = rh_term_has(q.ask, q.val[k]);
                    else if (type == RH_Q_BPM_TERM || type == RH_Q_HEAR) ok = rh_term_has(q.opt[k], q.bpm);
                    else if (type == RH_Q_CHANGE) ok = RH_CHANGES[q.opt[k]].kind == RH_CHANGES[q.ask].kind;
                    else ok = q.opt[k] == q.ask;
                    right += ok;
                    if (k == q.answer) CHECK(ok);
                    if (type != RH_Q_TERM_BPM && type != RH_Q_METER) {
                        int j;
                        for (j = 0; j < k; j++) CHECK(q.opt[j] != q.opt[k]);
                    }
                    if (type == RH_Q_METER) CHECK(RH_METERS[q.opt[k]].level <= (level <= 1 ? 1 : 2));
                    if (type <= RH_Q_HEAR && level == 1) CHECK(RH_TERMS[q.opt[k]].core || type == RH_Q_TERM_BPM);
                }
                CHECK(right == 1);
                if (type == RH_Q_TERM_BPM)
                    for (k = 0; k < 4; k++) {
                        int j;
                        for (j = 0; j < k; j++) CHECK(q.val[j] - q.val[k] >= 8 || q.val[k] - q.val[j] >= 8);
                    }
            }
    {                                    /* a missed term comes back more often */
        int hits = 0;
        uint8_t w2[RH_NTERMS] = { 0 };
        rh_quiz_t q;
        w2[7] = 4;
        for (i = 0; i < 7000; i++) {
            rh_quiz_make(&seed, RH_Q_TERM_BPM, 1, w2, -1, &q);
            hits += q.ask == 7;
        }
        CHECK(hits > 7000 * 9 / 15 - 400 && hits < 7000 * 9 / 15 + 400);   /* 9 of (6 + 9) */
    }
    for (i = 0; i < RH_Q_N; i++) CHECK(rh_quiz_name(i)[0] != 0);
}

int main(void)
{
    test_parse();
    test_generate();
    test_timing();
    test_score();
    test_pulse();
    test_stair();
    test_seq();
    test_quiz();
    printf("rhythm: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
