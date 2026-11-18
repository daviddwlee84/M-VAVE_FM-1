/* SPDX-License-Identifier: GPL-3.0-or-later */
/* fret.c on the computer: tunings, scale positions, chord shapes.
 *   just guitar-unit */
#include <stdio.h>
#include <string.h>
#include "../../theory/theory.c"
#include "../fret.c"
#include "../lesson.c"

static int fails, checks;
#define CHECK(c, ...) do { checks++; if (!(c)) { printf("FAIL %s:%d ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); fails++; } } while (0)

static const fr_tuning_t *STD = &fr_tunings[FR_STANDARD];

static int F(const char *name)
{
    int f = 0;
    if (th_parse_name(name, &f) != (int)strlen(name)) { printf("bad name in a test: %s\n", name); fails++; }
    return f;
}

static th_key_t key(const char *tonic, int scale)
{
    th_key_t k;
    k.tonic = (int8_t)F(tonic);
    k.scale = (uint8_t)scale;
    return k;
}

/* "x32010" (frets 10+ as a..f) */
static void parse_frets(const char *s, int8_t *fret, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        char c = s[i];
        fret[i] = (int8_t)(c == 'x' ? -1 : c >= 'a' ? c - 'a' + 10 : c - '0');
    }
}

static const char *frets_text(const fr_shape_t *sh, int n)
{
    static char b[FR_STR_MAX + 1];
    int i;
    for (i = 0; i < n; i++) {
        int f = sh->fret[i];
        b[i] = (char)(f < 0 ? 'x' : f >= 10 ? 'a' + f - 10 : '0' + f);
    }
    b[n] = 0;
    return b;
}

/* a position as text, one string a line: "8 10 12" */
static const char *mask_text(const uint32_t *m, int n)
{
    static char b[256];
    int s, f;
    b[0] = 0;
    for (s = 0; s < n; s++) {
        int first = 1;
        for (f = 0; f <= FR_FRETS; f++)
            if (m[s] >> f & 1u) {
                char t[8];
                sprintf(t, first ? "%d" : " %d", f);
                strcat(b, t);
                first = 0;
            }
        if (s < n - 1) strcat(b, "|");
    }
    return b;
}

/* ------------------------------------------------------------- tunings --- */
static void test_tunings(void)
{
    int i, s;
    CHECK(fr_midi(STD, 0, 0) == 40, "low E");
    CHECK(fr_midi(STD, 5, 12) == 76, "high E 12th fret");
    CHECK(fr_midi(STD, 1, 3) == 48, "C on the A string");
    for (i = 0; i < FR_NTUNINGS; i++) {
        const fr_tuning_t *t = &fr_tunings[i];
        CHECK(t->n >= 4 && t->n <= FR_STR_MAX, "%s strings", t->name);
        for (s = 1; s < t->n; s++)
            CHECK(t->reentrant || t->open[s] > t->open[s - 1], "%s rises", t->name);
    }
    /* every E on the low string, 0..24 */
    CHECK(fr_pc_frets(STD, 0, 4, 0, 24) == (1u | 1u << 12 | 1u << 24), "E frets");
    CHECK(fr_pc_frets(STD, 4, 0, 0, 12) == (1u << 1), "C on the B string");
}

/* ---------------------------------------------------------- positions --- */
static void each_once(const char *what, th_key_t k, int system, int pos)
{
    /* every scale note between the position's lowest and highest is in it exactly once; nothing outside the key */
    uint32_t m[FR_STR_MAX];
    int pcs = fr_scale_pcs(k), count[128], s, f, p, lo = 999, hi = -1;
    if (!fr_position(STD, k, system, pos, m)) { CHECK(0, "%s: no position %d", what, pos); return; }
    for (p = 0; p < 128; p++) count[p] = 0;
    for (s = 0; s < STD->n; s++)
        for (f = 0; f <= FR_FRETS; f++)
            if (m[s] >> f & 1u) {
                p = fr_midi(STD, s, f);
                CHECK(pcs >> (p % 12) & 1, "%s pos %d: %d not in the key", what, pos, p);
                count[p]++;
                if (p < lo) lo = p;
                if (p > hi) hi = p;
            }
    for (p = lo; p <= hi; p++)
        if (pcs >> (p % 12) & 1) CHECK(count[p] == 1, "%s pos %d: note %d %d times", what, pos, p, count[p]);
}

static void test_positions(void)
{
    uint32_t m[FR_STR_MAX];
    th_key_t c = key("C", TH_MAJOR), g = key("G", TH_MAJOR), a_min = key("A", TH_MINOR), am_p = key("A", TH_MPENTA);
    int t, sc, pos, s, f;
    /* the E shape of G major: frets 2-5, a textbook box */
    fr_position(STD, g, FR_CAGED, 3, m);
    CHECK(!strcmp(mask_text(m, 6), "2 3 5|2 3 5|2 4 5|2 4 5|3 5|2 3 5"), "G E shape: %s", mask_text(m, 6));
    /* C major: the open C shape, the E shape at 7-10 */
    fr_position(STD, c, FR_CAGED, 0, m);
    CHECK(!strcmp(mask_text(m, 6), "0 1 3|0 2 3|0 2 3|0 2|0 1 3|0 1 3"), "C shape: %s", mask_text(m, 6));
    fr_position(STD, c, FR_CAGED, 3, m);
    CHECK(!strcmp(mask_text(m, 6), "7 8 10|7 8 10|7 9 10|7 9 10|8 10|7 8 10"), "C E shape: %s", mask_text(m, 6));
    /* the G shape needs the index back a fret on the G string (B at 4) */
    fr_position(STD, c, FR_CAGED, 2, m);
    CHECK(!strcmp(mask_text(m, 6), "5 7 8|5 7 8|5 7|4 5 7|5 6 8|5 7 8"), "C G shape: %s", mask_text(m, 6));
    /* A minor shares C major's boxes */
    {
        uint32_t m2[FR_STR_MAX];
        for (pos = 0; pos < 5; pos++) {
            fr_position(STD, c, FR_CAGED, pos, m);
            fr_position(STD, a_min, FR_CAGED, pos, m2);
            CHECK(!memcmp(m, m2, sizeof m), "A minor = C major, CAGED %d", pos);
        }
    }
    /* 3NPS: G major from G on the 3rd fret */
    CHECK(fr_positions(g, FR_NPS) == 7, "7 NPS positions");
    fr_position(STD, g, FR_NPS, 0, m);
    CHECK(!strcmp(mask_text(m, 6), "3 5 7|3 5 7|4 5 7|4 5 7|5 7 8|5 7 8"), "G 3NPS 1: %s", mask_text(m, 6));
    /* the A minor pentatonic box 1, two a string */
    CHECK(fr_positions(am_p, FR_NPS) == 5 && fr_nps(am_p) == 2, "pentatonic: 2 NPS, 5 positions");
    fr_position(STD, am_p, FR_NPS, 0, m);
    CHECK(!strcmp(mask_text(m, 6), "5 8|5 7|5 7|5 7|5 8|5 8"), "Am pentatonic box 1: %s", mask_text(m, 6));
    CHECK(fr_positions(key("A", TH_BLUES), FR_NPS) == 0, "blues: no NPS");
    /* ALL: every scale note */
    fr_position(STD, c, FR_ALL, 0, m);
    for (s = 0; s < 6; s++)
        for (f = 0; f <= FR_FRETS; f++)
            CHECK(!!(m[s] >> f & 1u) == !!(fr_scale_pcs(c) >> ((STD->open[s] + f) % 12) & 1), "ALL %d %d", s, f);
    /* every key, scale, system: notes once each */
    for (t = -7; t <= 7; t++)
        for (sc = 0; sc < TH_NSCALES; sc++) {
            th_key_t k;
            char what[48], nm[TH_NAME_MAX];
            k.tonic = (int8_t)t;
            k.scale = (uint8_t)sc;
            th_name(t, nm);
            sprintf(what, "%s %s CAGED", nm, th_scales[sc].name);
            for (pos = 0; pos < 5; pos++) each_once(what, k, FR_CAGED, pos);
            sprintf(what, "%s %s NPS", nm, th_scales[sc].name);
            for (pos = 0; pos < fr_positions(k, FR_NPS); pos++) {
                each_once(what, k, FR_NPS, pos);
                fr_position(STD, k, FR_NPS, pos, m);
                for (s = 0; s < 6; s++) {
                    int c2 = 0;
                    for (f = 0; f <= FR_FRETS; f++) c2 += (int)(m[s] >> f & 1u);
                    CHECK(c2 == fr_nps(k), "%s %d: string %d has %d", what, pos, s, c2);
                }
            }
            /* CAGED windows reach at most 5 frets (with a stretch) */
            for (pos = 0; pos < 5; pos++) {
                int lo = 99, hi = -1;
                fr_position(STD, k, FR_CAGED, pos, m);
                for (s = 0; s < 6; s++)
                    for (f = 0; f <= FR_FRETS; f++)
                        if (m[s] >> f & 1u) {
                            if (f < lo) lo = f;
                            if (f > hi) hi = f;
                        }
                CHECK(hi - lo <= 5, "%s %d: frets %d-%d", what, pos, lo, hi);
            }
        }
}

/* ------------------------------------------------------------- shapes --- */
/* open chords as a guitarist learns them: every one must be found, among the first three; the barre chords (where
 * easier shapes with open strings come first, as on ChordRoot) among the first FR_SHAPES_MAX */
#define NOPEN 26                         /* the open ones; then the barre chords */
static const struct { const char *root, *q, *frets; } OPEN[] = {
    { "C", "", "x32010" }, { "A", "", "x02220" }, { "G", "", "320003" }, { "E", "", "022100" }, { "D", "", "xx0232" },
    { "A", "m", "x02210" }, { "E", "m", "022000" }, { "D", "m", "xx0231" },
    { "C", "7", "x32310" }, { "A", "7", "x02020" }, { "G", "7", "320001" }, { "E", "7", "020100" },
    { "D", "7", "xx0212" }, { "B", "7", "x21202" },
    { "A", "m7", "x02010" }, { "E", "m7", "020000" }, { "D", "m7", "xx0211" },
    { "C", "maj7", "x32000" }, { "A", "maj7", "x02120" }, { "D", "maj7", "xx0222" }, { "F", "maj7", "xx3210" },
    { "A", "sus4", "x02230" }, { "D", "sus4", "xx0233" }, { "E", "sus4", "022200" },
    { "A", "sus2", "x02200" }, { "D", "sus2", "xx0230" },
    /* barre chords: no open shape exists */
    { "F", "", "133211" }, { "B", "m", "x24432" }, { "F", "m", "133111" },
};

static th_chord_t chord(const char *root, const char *q)
{
    th_chord_t c;
    c.root = (int8_t)F(root);
    c.q = (uint8_t)th_qual_find(q);
    c.bass = c.root;
    return c;
}

static void check_shape(const char *what, const fr_tuning_t *t, th_chord_t c, const fr_shape_t *sh)
{
    int8_t f[TH_TONES_MAX];
    int n = th_chord(c.root, c.q, f), tones = 0, have = 0, s, i, lowest = 999, fingers = 0, used[5] = { 0 };
    for (i = 0; i < n; i++) tones |= 1 << th_pc(f[i]);
    for (s = 0; s < t->n; s++) {
        int p;
        if (sh->fret[s] < 0) continue;
        p = fr_midi(t, s, sh->fret[s]);
        CHECK(tones >> (p % 12) & 1, "%s %s: string %d not a chord tone", what, frets_text(sh, t->n), s);
        have |= 1 << (p % 12);
        if (p < lowest) lowest = p;
        CHECK(!sh->fret[s] == !sh->finger[s], "%s %s: a finger on string %d", what, frets_text(sh, t->n), s);
        if (sh->finger[s] && !used[sh->finger[s]]) {
            used[sh->finger[s]] = 1;
            fingers++;
        }
    }
    CHECK(fingers <= 4, "%s: fingers", what);
    if (!t->reentrant) CHECK(lowest % 12 == th_pc(c.root), "%s %s: root in the bass", what, frets_text(sh, t->n));
    for (i = 0; i < n; i++)
        if (!(have >> th_pc(f[i]) & 1))
            CHECK(th_quals[c.q].f[i] == 1 && sh->no5, "%s %s: tone %d missing", what, frets_text(sh, t->n), i);
    CHECK(fr_shape_lo(sh, t->n) == 0 || fr_shape_hi(sh, t->n) - fr_shape_lo(sh, t->n) < FR_SPAN, "%s: span", what);
}

static void test_shapes(void)
{
    fr_shape_t out[FR_SHAPES_MAX];
    int i, j, n, r, q, ti;
    for (i = 0; i < (int)(sizeof OPEN / sizeof OPEN[0]); i++) {
        th_chord_t c = chord(OPEN[i].root, OPEN[i].q);
        int8_t want[FR_STR_MAX];
        int at = -1;
        char nm[TH_CHORD_MAX];
        th_chord_name(c, nm);
        parse_frets(OPEN[i].frets, want, 6);
        n = fr_shapes(STD, c, out, FR_SHAPES_MAX);
        for (j = 0; j < n; j++)
            if (!memcmp(out[j].fret, want, 6)) at = j;
        CHECK(at >= 0 && (at < 3 || i >= NOPEN), "%s %s: found at %d (first: %s)", nm, OPEN[i].frets, at, n ? frets_text(&out[0], 6) : "-");
        for (j = 0; j < n; j++) check_shape(nm, STD, c, &out[j]);
    }
    /* fingers as taught: C 3 2 . 1 . ; F barre 1 3 4 2 1 1 */
    n = fr_shapes(STD, chord("C", ""), out, FR_SHAPES_MAX);
    CHECK(n && !memcmp(out[0].fret, "\xff\x03\x02\x00\x01\x00", 6), "C first: %s", frets_text(&out[0], 6));
    CHECK(n && out[0].finger[1] == 3 && out[0].finger[2] == 2 && out[0].finger[4] == 1, "C fingers");
    n = fr_shapes(STD, chord("F", ""), out, FR_SHAPES_MAX);
    for (j = 0; j < n && memcmp(out[j].fret, "\x01\x03\x03\x02\x01\x01", 6); j++) {}
    CHECK(j < n && out[j].barre == 1 && out[j].bfrom == 0 && out[j].bto == 5, "F barre");
    CHECK(j < n && out[j].finger[0] == 1 && out[j].finger[1] == 3 && out[j].finger[2] == 4 && out[j].finger[3] == 2,
          "F fingers %d %d %d %d", out[j].finger[0], out[j].finger[1], out[j].finger[2], out[j].finger[3]);
    /* G7 without its fifth is still G7: 3x340x */
    /* every root and quality on every tuning: what is found is playable and right */
    for (ti = 0; ti < FR_NTUNINGS; ti++)
        for (r = -6; r <= 6; r++)
            for (q = 0; q < TH_NQUAL; q++) {
                th_chord_t c;
                char nm[TH_CHORD_MAX + 16];
                c.root = (int8_t)r;
                c.q = (uint8_t)q;
                c.bass = c.root;
                th_chord_name(c, nm);
                strcat(nm, " ");
                strcat(nm, fr_tunings[ti].name);
                n = fr_shapes(&fr_tunings[ti], c, out, FR_SHAPES_MAX);
                if (ti == FR_STANDARD && th_quals[q].n <= 4) CHECK(n > 0, "%s: no shape", nm);
                for (j = 0; j < n; j++) check_shape(nm, &fr_tunings[ti], c, &out[j]);
                for (j = 1; j < n; j++) CHECK(out[j].score >= out[j - 1].score, "%s: order", nm);
            }
    /* the ukulele's C: 0003 */
    n = fr_shapes(&fr_tunings[FR_UKULELE], chord("C", ""), out, FR_SHAPES_MAX);
    CHECK(n && !memcmp(out[0].fret, "\x00\x00\x00\x03", 4), "uke C first: %s", n ? frets_text(&out[0], 4) : "-");
}

/* v0.2: power chords on two or three strings; barre chords of the E and A forms in every key */
static int has(const fr_shape_t *o, int n, const int8_t *want)
{
    int j;
    for (j = 0; j < n; j++)
        if (!memcmp(o[j].fret, want, 6)) return 1;
    return 0;
}

static void test_forms(void)
{
    fr_shape_t out[FR_SHAPES_MAX];
    static const char *const E_FORM[] = { "", "m", "7", "m7" };
    static const int8_t E_REL[4][6] = { { 0, 2, 2, 1, 0, 0 }, { 0, 2, 2, 0, 0, 0 }, { 0, 2, 0, 1, 0, 0 }, { 0, 2, 0, 0, 0, 0 } };
    static const int8_t A_REL[4][6] = { { -1, 0, 2, 2, 2, 0 }, { -1, 0, 2, 2, 1, 0 }, { -1, 0, 2, 0, 2, 0 }, { -1, 0, 2, 0, 1, 0 } };
    int n, r, i, s;
    n = fr_shapes(STD, chord("G", "5"), out, FR_SHAPES_MAX);
    CHECK(n && !memcmp(out[0].fret, "\x03\x05\x05\xff\xff\xff", 6), "G5 first: %s", n ? frets_text(&out[0], 6) : "-");
    CHECK(n && out[0].finger[0] == 1 && out[0].finger[1] == 2 && out[0].finger[2] == 3, "G5 fingers");
    n = fr_shapes(STD, chord("C", "5"), out, FR_SHAPES_MAX);
    CHECK(n && !memcmp(out[0].fret, "\xff\x03\x05\x05\xff\xff", 6), "C5 first: %s", n ? frets_text(&out[0], 6) : "-");
    for (r = 0; r < 12; r++) {           /* every root: a 6th-string and a 5th-string power chord */
        th_chord_t c;
        int8_t six[6] = { -1, -1, -1, -1, -1, -1 }, five[6] = { -1, -1, -1, -1, -1, -1 };
        int f6 = (r - 4 + 12) % 12, f5 = (r - 9 + 12) % 12;
        c.root = (int8_t)th_spell_pc(r, 0);
        c.q = TH_5;
        c.bass = c.root;
        six[0] = (int8_t)f6; six[1] = (int8_t)(f6 + 2); six[2] = (int8_t)(f6 + 2);
        five[1] = (int8_t)f5; five[2] = (int8_t)(f5 + 2); five[3] = (int8_t)(f5 + 2);
        n = fr_shapes(STD, c, out, FR_SHAPES_MAX);
        CHECK(has(out, n, six) && has(out, n, five), "power chord on pc %d", r);
    }
    for (i = 0; i < 4; i++)              /* barre forms: roots on the 6th string at frets 1-9, the 5th at 1-7 */
        for (r = 1; r <= 9; r++) {
            th_chord_t c;
            int8_t e[6], a[6];
            int fe, fa;
            for (s = 0; s < 6; s++) {
                e[s] = (int8_t)(E_REL[i][s] + r);
                a[s] = (int8_t)(A_REL[i][s] < 0 ? -1 : A_REL[i][s] + r);
            }
            c.root = (int8_t)th_spell_pc((4 + r) % 12, 0);
            c.q = (uint8_t)th_qual_find(E_FORM[i]);
            c.bass = c.root;
            n = fr_shapes_filter(STD, c, FR_F_BARRE, out, FR_SHAPES_MAX);
            fe = has(out, n, e);
            CHECK(fe, "E form %s at %d", E_FORM[i], r);
            for (s = 0; s < n; s++) CHECK(out[s].barre, "BARRE filter: %s", frets_text(&out[s], 6));
            c.root = (int8_t)th_spell_pc((9 + r) % 12, 0);
            n = fr_shapes_filter(STD, c, FR_F_BARRE, out, FR_SHAPES_MAX);
            fa = has(out, n, a);
            CHECK(fa || r > 7, "A form %s at %d", E_FORM[i], r);   /* above 7 the 6th-string form is lower */
            n = fr_shapes_filter(STD, c, FR_F_MOVABLE, out, FR_SHAPES_MAX);
            for (s = 0; s < n; s++) CHECK(!fr_shape_open(&out[s], 6), "MOVABLE: %s", frets_text(&out[s], 6));
            n = fr_shapes_filter(STD, c, FR_F_OPEN, out, FR_SHAPES_MAX);
            for (s = 0; s < n; s++) CHECK(fr_shape_open(&out[s], 6), "OPEN: %s", frets_text(&out[s], 6));
        }
}

static void test_lesson(void)
{
    ls_prog_t p;
    uint32_t rng = 1;
    uint8_t cards[LS_ROUND], d[3];
    int i, j, n, last = -1;
    CHECK(ls_nitems <= LS_ITEMS_MAX, "items fit");
    for (i = 0; i < ls_nitems; i++) {
        fr_shape_t sh;
        th_chord_t c = ls_chord(i);
        int8_t notes[8];
        int k = th_chord(c.root, c.q, notes), want = 0, got, pcs;
        for (j = 0; j < k; j++) want |= 1 << th_pc(notes[j]);
        CHECK(ls_items[i].stage >= last, "item %d in stage order", i);
        last = ls_items[i].stage;
        CHECK(ls_shape(i, &sh), "item %d (%s) playable", i, ls_items[i].frets);
        pcs = ls_pcs(i);
        got = pcs;
        CHECK((got & ~want) == 0, "item %d (%s): a note outside the chord", i, ls_items[i].frets);
        CHECK((want & ~got) == 0 || (want & ~got) == 1 << th_pc(notes[2]), "item %d (%s): a tone missing", i,
              ls_items[i].frets);
        CHECK(fr_shape_midi(STD, &sh, (uint8_t *)notes) > 0 && (uint8_t)notes[0] % 12 == th_pc(c.root),
              "item %d (%s): root in the bass", i, ls_items[i].frets);
        CHECK(ls_items[i].stage < ls_nstages, "item %d stage", i);
        if (ls_items[i].easy) {         /* a stand-in: the full barre version is taught later */
            for (j = i + 1; j < ls_nitems; j++)
                if (ls_items[j].root == ls_items[i].root && ls_items[j].q == ls_items[i].q && !ls_items[j].easy) break;
            CHECK(j < ls_nitems || ls_items[i].root == 6, "item %d: easy without a later barre", i);
        }
    }
    ls_reset(&p);
    CHECK(ls_valid(&p), "reset is valid");
    for (i = 0; i < 50; i++) {
        n = ls_round(&p, &rng, cards, LS_ROUND);
        CHECK(n == LS_ROUND, "round size %d", n);
        for (j = 0; j < n; j++) {
            CHECK(ls_open(&p, cards[j]), "round: only open items");
            CHECK(!j || cards[j] != cards[j - 1], "round: %d twice in a row", cards[j]);
        }
        CHECK(ls_distractors(&p, cards[0], &rng, d) == 3, "three distractors");
        for (j = 0; j < 3; j++) CHECK(d[j] != cards[0] && strcmp(ls_items[d[j]].frets, ls_items[cards[0]].frets),
                                      "distractor differs");
        CHECK(d[0] != d[1] && d[1] != d[2] && d[0] != d[2], "distractors distinct");
        CHECK(ls_card_kind(&p, cards[0], &rng) == LS_NAME_TO_SHAPE || p.box[cards[0]] >= 1, "new: name to shape");
    }
    /* stage 0 opens stage 1 when each of its chords reaches box 2, not before */
    for (i = 0; i < ls_nitems; i++)
        if (ls_items[i].stage == 0) CHECK(!ls_answer(&p, i, 1), "one right is not enough");
    n = 0;
    for (i = 0; i < ls_nitems; i++)
        if (ls_items[i].stage == 0) n += ls_answer(&p, i, 1);
    CHECK(n == 1 && p.stage == 1, "stage 1 opened (%d, %d)", n, p.stage);
    ls_answer(&p, 0, 0);
    CHECK(p.box[0] == 0 && p.stage == 1, "wrong: back to box 0, the stage stays open");
    for (i = 0; i < 5; i++) ls_answer(&p, 1, 1);
    CHECK(p.box[1] == LS_BOXES, "box capped");
    ls_answer(&p, 1, 0);
    CHECK(p.box[1] == 1, "wrong from a high box: box 1");
    /* everything right, a stage at a time, reaches the last stage */
    for (j = 0; j < 20; j++)
        for (i = 0; i < ls_nitems; i++)
            if (ls_open(&p, i)) ls_answer(&p, i, 1);
    CHECK(p.stage == ls_nstages - 1 && ls_valid(&p), "all stages open");
    p.stage = (uint8_t)ls_nstages;
    CHECK(!ls_valid(&p), "a stage past the end is not valid");
}

/* the solfege names (Mi型 = C shape ...): the named degree of the parent major lies on the low string in that position
 * in every key, as in the book diagrams of C major */
static void test_solfege(void)
{
    static const int SEMI[5] = { 4, 7, 9, 0, 2 };     /* MI SOL LA DO RE */
    uint32_t m[FR_STR_MAX];
    int t, p, f, ok;
    for (t = -6; t <= 6; t++) {
        th_key_t k;
        k.tonic = (int8_t)t;
        k.scale = TH_MAJOR;
        for (p = 0; p < 5; p++) {
            fr_position(STD, k, FR_CAGED, p, m);
            for (ok = 0, f = 0; f <= FR_FRETS; f++)
                if ((m[0] >> f & 1u) && (STD->open[0] + f) % 12 == (th_pc(t) + SEMI[p]) % 12) ok = 1;
            CHECK(ok, "key %d: %s shape (CAGED %c) has its degree on the low string", t, fr_solfege_name[p],
                  fr_caged_name[p]);
        }
    }
}

int main(void)
{
    test_tunings();
    test_positions();
    test_shapes();
    test_forms();
    test_lesson();
    test_solfege();
    printf("%d checks, %d failed\n", checks, fails);
    return fails != 0;
}
