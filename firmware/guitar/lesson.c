/* SPDX-License-Identifier: GPL-3.0-or-later */
/* lesson: see lesson.h. */
#include "lesson.h"

/* the course (docs/dev/guitar.md#v02-learn). Roots on the line of fifths: C 0, G 1, D 2, A 3, E 4, B 5, F# 6, C# 7,
 * F -1, Bb -2. The shapes are the ones guitar books teach first; a chord that needs a barre is taught in a smaller
 * shape (easy = 1) until the last stage */
const ls_item_t ls_items[] = {
    /* root q         frets     stage easy */
    { 0, TH_MAJ,  "x32010", 0, 0 },     /* C */
    { 1, TH_MAJ,  "320003", 0, 0 },     /* G */
    { 3, TH_MIN,  "x02210", 0, 0 },     /* Am */
    { -1, TH_MAJ, "xx3211", 0, 1 },     /* F, the small one */
    { 4, TH_MIN,  "022000", 1, 0 },     /* Em */
    { 2, TH_MIN,  "xx0231", 1, 0 },     /* Dm */
    { 1, TH_7,    "320001", 1, 0 },     /* G7 */
    { 2, TH_MAJ,  "xx0232", 2, 0 },     /* D */
    { 2, TH_7,    "xx0212", 2, 0 },     /* D7 */
    { 0, TH_MAJ7, "x32000", 2, 0 },     /* Cmaj7 */
    { 3, TH_MAJ,  "x02220", 3, 0 },     /* A */
    { 5, TH_MIN,  "x2443x", 3, 1 },     /* Bm, without the barre */
    { 3, TH_7,    "x02020", 3, 0 },     /* A7 */
    { 4, TH_MAJ,  "022100", 4, 0 },     /* E */
    { 6, TH_MIN,  "xx4222", 4, 1 },     /* F#m, on four strings */
    { 4, TH_7,    "020100", 4, 0 },     /* E7 */
    { 5, TH_7,    "x21202", 4, 0 },     /* B7 */
    { -1, TH_MAJ, "133211", 5, 0 },     /* F, the E-form barre */
    { 5, TH_MIN,  "x24432", 5, 0 },     /* Bm, the A-form barre */
    { -2, TH_MAJ, "x13331", 5, 0 },     /* Bb */
    { 1, TH_MIN,  "355333", 5, 0 },     /* Gm */
};
const int ls_nitems = (int)(sizeof ls_items / sizeof ls_items[0]);

const ls_stage_t ls_stages[] = {
    { "C", "C G AM F" },
    { "C", "EM DM G7" },
    { "G", "D D7 CMAJ7" },
    { "D", "A BM A7" },
    { "A E", "E F#M E7 B7" },
    { "F", "BARRE CHORDS" },
};
const int ls_nstages = (int)(sizeof ls_stages / sizeof ls_stages[0]);

void ls_reset(ls_prog_t *p)
{
    int i;
    for (i = 0; i < LS_ITEMS_MAX; i++) p->box[i] = 0;
    p->stage = 0;
    p->rsv = 0;
    p->rounds = 0;
}

int ls_valid(const ls_prog_t *p)
{
    int i;
    if (p->stage >= ls_nstages) return 0;
    for (i = 0; i < LS_ITEMS_MAX; i++)
        if (p->box[i] > LS_BOXES) return 0;
    return 1;
}

uint32_t ls_rand(uint32_t *s)
{
    uint32_t x = *s ? *s : 0x9E3779B9u;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *s = x;
    return x;
}

int ls_open(const ls_prog_t *p, int i) { return i >= 0 && i < ls_nitems && ls_items[i].stage <= p->stage; }

th_chord_t ls_chord(int i)
{
    th_chord_t c;
    c.root = ls_items[i].root;
    c.q = ls_items[i].q;
    c.bass = c.root;
    return c;
}

int ls_shape(int i, fr_shape_t *sh)
{
    int8_t f[FR_STR_MAX];
    int s;
    for (s = 0; s < FR_STR_MAX; s++) {
        char ch = ls_items[i].frets[s];
        f[s] = (int8_t)(ch == 'x' ? -1 : ch - '0');
    }
    return fr_shape_make(&fr_tunings[FR_STANDARD], f, sh);
}

int ls_pcs(int item)
{
    fr_shape_t sh;
    uint8_t m[FR_STR_MAX];
    int n, i, pcs = 0;
    if (!ls_shape(item, &sh)) return 0;
    n = fr_shape_midi(&fr_tunings[FR_STANDARD], &sh, m);
    for (i = 0; i < n; i++) pcs |= 1 << (m[i] % 12);
    return pcs;
}

int ls_round(const ls_prog_t *p, uint32_t *rng, uint8_t *cards, int n)
{
    uint8_t order[LS_ITEMS_MAX];
    uint16_t key[LS_ITEMS_MAX];
    int k = 0, i, j, c = 0;
    for (i = 0; i < ls_nitems; i++)
        if (ls_open(p, i)) {            /* weakest first; the newest stage first among equals; then at random */
            uint16_t v = (uint16_t)(p->box[i] << 12 | (ls_nstages - ls_items[i].stage) << 8 | (ls_rand(rng) & 0xFFu));
            for (j = k; j > 0 && key[j - 1] > v; j--) {
                key[j] = key[j - 1];
                order[j] = order[j - 1];
            }
            key[j] = v;
            order[j] = (uint8_t)i;
            k++;
        }
    if (!k) return 0;
    for (i = 0; c < n; i++) {
        uint8_t it = order[i % k];
        if (c && cards[c - 1] == it && k > 1) continue;
        cards[c++] = it;
    }
    /* shuffle, keeping no item twice in a row */
    for (i = c - 1; i > 0; i--) {
        uint8_t t;
        j = (int)(ls_rand(rng) % (uint32_t)(i + 1));
        t = cards[i];
        cards[i] = cards[j];
        cards[j] = t;
    }
    for (i = 1; i < c && k > 1; i++)
        if (cards[i] == cards[i - 1])
            for (j = 0; j < c; j++)
                if (cards[j] != cards[i] && (j == 0 || cards[j - 1] != cards[i]) && (j + 1 >= c || cards[j + 1] != cards[i])
                    && (i + 1 >= c || cards[i + 1] != cards[j]) && cards[i - 1] != cards[j]) {
                    uint8_t t = cards[i];
                    cards[i] = cards[j];
                    cards[j] = t;
                    break;
                }
    return c;
}

int ls_card_kind(const ls_prog_t *p, int item, uint32_t *rng)
{
    if (p->box[item] < 1) return LS_NAME_TO_SHAPE;    /* a new chord: recognise its shape first */
    return (ls_rand(rng) >> 7 & 1u) ? LS_SHAPE_TO_NOTES : LS_NAME_TO_SHAPE;
}

static int ls_same_frets(int a, int b)
{
    int s;
    for (s = 0; s < FR_STR_MAX; s++)
        if (ls_items[a].frets[s] != ls_items[b].frets[s]) return 0;
    return 1;
}

int ls_distractors(const ls_prog_t *p, int item, uint32_t *rng, uint8_t *out)
{
    int pass, i, k = 0, j;
    /* pass 0: same stage or same quality; pass 1: any open item; pass 2: any item (the first stage has few) */
    for (pass = 0; pass < 3 && k < 3; pass++) {
        uint8_t cand[LS_ITEMS_MAX];
        int nc = 0;
        for (i = 0; i < ls_nitems; i++) {
            int ok = i != item && !ls_same_frets(i, item);
            for (j = 0; j < k && ok; j++)
                if (out[j] == i || ls_same_frets(out[j], i)) ok = 0;
            if (!ok) continue;
            if (pass == 0 && !(ls_open(p, i) && (ls_items[i].stage == ls_items[item].stage || ls_items[i].q == ls_items[item].q)))
                continue;
            if (pass == 1 && !ls_open(p, i)) continue;
            cand[nc++] = (uint8_t)i;
        }
        while (nc && k < 3) {
            int r = (int)(ls_rand(rng) % (uint32_t)nc);
            out[k++] = cand[r];
            cand[r] = cand[--nc];
        }
    }
    return k;
}

int ls_stage_done(const ls_prog_t *p, int stage)
{
    int i;
    for (i = 0; i < ls_nitems; i++)
        if (ls_items[i].stage == stage && p->box[i] < LS_PASS) return 0;
    return 1;
}

int ls_answer(ls_prog_t *p, int item, int right)
{
    int s;
    if (right) {
        if (p->box[item] < LS_BOXES) p->box[item]++;
    } else
        p->box[item] = (uint8_t)(p->box[item] >= 3 ? 1 : 0);
    if (p->stage + 1 >= ls_nstages) return 0;
    for (s = 0; s <= p->stage; s++)
        if (!ls_stage_done(p, s)) return 0;
    p->stage++;
    return 1;
}
