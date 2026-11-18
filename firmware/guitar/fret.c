/* SPDX-License-Identifier: GPL-3.0-or-later */
/* fret: see fret.h. */
#include "fret.h"

const fr_tuning_t fr_tunings[FR_NTUNINGS] = {
    /* name          n  re  open strings, string 0 first */
    { "STANDARD",    6, 0, { 40, 45, 50, 55, 59, 64 } },     /* E2 A2 D3 G3 B3 E4 */
    { "DROP D",      6, 0, { 38, 45, 50, 55, 59, 64 } },     /* D2 A2 D3 G3 B3 E4 */
    { "DADGAD",      6, 0, { 38, 45, 50, 55, 57, 62 } },     /* D2 A2 D3 G3 A3 D4 */
    { "OPEN G",      6, 0, { 38, 43, 50, 55, 59, 62 } },     /* D2 G2 D3 G3 B3 D4 */
    { "BASS",        4, 0, { 28, 33, 38, 43 } },             /* E1 A1 D2 G2 */
    { "BASS 5",      5, 0, { 23, 28, 33, 38, 43 } },         /* B0 E1 A1 D2 G2 */
    { "UKULELE",     4, 1, { 67, 60, 64, 69 } },             /* G4 C4 E4 A4, re-entrant */
    { "MANDOLIN",    4, 0, { 55, 62, 69, 76 } },             /* G3 D4 A4 E5 */
};

const char fr_caged_name[5] = { 'C', 'A', 'G', 'E', 'D' };

/* where each CAGED window starts (the index finger's fret), counted from the parent major's tonic on string 0:
 * in C on a standard guitar the C shape is frets 0-3, A 2-5 (3 + ...), G 5-8, E 7-10, D 9-12 */
static const int8_t FR_CAGED_AT[5] = { 4, 6, 9, -1, 1 };

int fr_midi(const fr_tuning_t *t, int s, int fret) { return t->open[s] + fret; }

int fr_scale_pcs(th_key_t key)
{
    int8_t f[7];
    int n = th_scale(key.tonic, key.scale, f), m = 0, i;
    for (i = 0; i < n; i++) m |= 1 << th_pc(f[i]);
    return m;
}

uint32_t fr_pc_frets(const fr_tuning_t *t, int s, int pc, int lo, int hi)
{
    uint32_t m = 0;
    int f;
    if (lo < 0) lo = 0;
    if (hi > FR_FRETS) hi = FR_FRETS;
    for (f = lo; f <= hi; f++)
        if (th_mod(t->open[s] + f, 12) == th_mod(pc, 12)) m |= 1u << f;
    return m;
}

int fr_nps(th_key_t key)
{
    int n = th_scales[key.scale].n;
    return n == 7 ? 3 : n == 5 ? 2 : 0;
}

int fr_positions(th_key_t key, int system)
{
    switch (system) {
    case FR_ALL: return 1;
    case FR_CAGED: return 5;
    case FR_NPS: return fr_nps(key) ? th_scales[key.scale].n : 0;
    default: return 0;
    }
}

static int fr_in_pcs(int pcs, int midi) { return pcs >> th_mod(midi, 12) & 1; }

/* CAGED: a four-fret window a .. a+3 on every string. A scale note the window misses between its lowest and highest
 * note (it falls in the gap between two strings) is taken with a stretch: one fret back on the higher string. Where
 * two strings hold the same note inside the window (tunings with a narrow pair of strings), the lower string has it */
static int fr_caged(const fr_tuning_t *t, th_key_t key, int pos, uint32_t *mask)
{
    int pcs = fr_scale_pcs(key), parent = key.tonic + th_scales[key.scale].sig;   /* the major key with that signature */
    int r = th_mod(th_pc(parent) - t->open[0], 12), a = th_mod(r + FR_CAGED_AT[pos], 12), s, f, p, lo = 999, hi = -1;
    uint8_t have[128];
    for (p = 0; p < 128; p++) have[p] = 0;
    for (s = 0; s < FR_STR_MAX; s++) mask[s] = 0;
    for (s = 0; s < t->n; s++) {
        for (f = a; f <= a + 3; f++) {
            p = fr_midi(t, s, f);
            if (!fr_in_pcs(pcs, p) || have[p]) continue;
            have[p] = 1;
            mask[s] |= 1u << f;
            if (p < lo) lo = p;
            if (p > hi) hi = p;
        }
    }
    for (p = lo + 1; p < hi; p++) {      /* the gaps */
        if (!fr_in_pcs(pcs, p) || have[p]) continue;
        for (s = t->n - 1; s >= 0; s--) {
            f = p - t->open[s];
            if ((f == a - 1 || f == a + 4) && f >= 0) {    /* the highest string that has it */
                mask[s] |= 1u << f;
                have[p] = 1;
                break;
            }
        }
    }
    return 1;
}

/* NPS: from scale step pos on string 0 (its lowest fret 0..11), the scale upward, the same number of notes on every
 * string. Where that would need a fret below the nut on a higher string, an octave up */
static int fr_nps_from(const fr_tuning_t *t, th_key_t key, int p, uint32_t *mask)
{
    int k = fr_nps(key), pcs = fr_scale_pcs(key), s, i, f;
    for (s = 0; s < FR_STR_MAX; s++) mask[s] = 0;
    for (s = 0; s < t->n; s++)
        for (i = 0; i < k; i++) {
            if (s || i) {
                p++;
                while (!fr_in_pcs(pcs, p)) p++;
            }
            f = p - t->open[s];
            if (f < 0 || f > FR_FRETS) return 0;
            mask[s] |= 1u << f;
        }
    return 1;
}

static int fr_nps_pos(const fr_tuning_t *t, th_key_t key, int pos, uint32_t *mask)
{
    int8_t sc[7];
    int p;
    th_scale(key.tonic, key.scale, sc);
    p = t->open[0] + th_mod(th_pc(sc[pos]) - t->open[0], 12);
    return fr_nps_from(t, key, p, mask) || fr_nps_from(t, key, p + 12, mask);
}

int fr_position(const fr_tuning_t *t, th_key_t key, int system, int pos, uint32_t *mask)
{
    int s, pcs = fr_scale_pcs(key);
    if (pos < 0 || pos >= fr_positions(key, system)) return 0;
    if (system == FR_CAGED) return fr_caged(t, key, pos, mask);
    if (system == FR_NPS) return fr_nps_pos(t, key, pos, mask);
    for (s = 0; s < FR_STR_MAX; s++) {
        int pc;
        mask[s] = 0;
        if (s < t->n)
            for (pc = 0; pc < 12; pc++)
                if (pcs >> pc & 1) mask[s] |= fr_pc_frets(t, s, pc, 0, FR_FRETS);
    }
    return 1;
}

/* ---------------------------------------------------------- chord shapes --- */
int fr_shape_lo(const fr_shape_t *sh, int n)
{
    int s, lo = 0;
    for (s = 0; s < n; s++)
        if (sh->fret[s] > 0 && (!lo || sh->fret[s] < lo)) lo = sh->fret[s];
    return lo;
}

int fr_shape_hi(const fr_shape_t *sh, int n)
{
    int s, hi = 0;
    for (s = 0; s < n; s++)
        if (sh->fret[s] > hi) hi = sh->fret[s];
    return hi;
}

int fr_shape_midi(const fr_tuning_t *t, const fr_shape_t *sh, uint8_t *out)
{
    int s, n = 0;
    for (s = 0; s < t->n; s++)
        if (sh->fret[s] >= 0) out[n++] = (uint8_t)fr_midi(t, s, sh->fret[s]);
    return n;
}

typedef struct {
    const fr_tuning_t *t;
    int tones;                           /* pitch classes of the chord */
    int need;                            /* those that must sound */
    int five;                            /* the perfect fifth's pitch class (-1: none, or it must sound) */
    int root;                            /* the root's pitch class */
    int b;                               /* the window: frets b .. b+FR_SPAN-1, and open strings */
    int8_t fret[FR_STR_MAX];
    fr_shape_t *out;
    int max, n;
} fr_search_t;

/* fingers: with no barre, one per fretted note, from the lowest fret up (the lower string first at the same fret).
 * When that takes more than four, a barre at the lowest fret: from the lowest string with that fret to the highest,
 * every string under it fretted. Returns 0 if the hand cannot do it */
static int fr_fingers(const fr_tuning_t *t, fr_shape_t *sh)
{
    int s, f, k = 0, n = t->n, lo = fr_shape_lo(sh, n), hi = fr_shape_hi(sh, n), fretted = 0, from = -1, to = -1;
    for (s = 0; s < n; s++) {
        sh->finger[s] = 0;
        if (sh->fret[s] > 0) fretted++;
        if (sh->fret[s] == lo && lo) {
            if (from < 0) from = s;
            to = s;
        }
    }
    for (; s < FR_STR_MAX; s++) sh->finger[s] = 0;
    sh->barre = 0;
    sh->bfrom = sh->bto = 0;
    if (fretted > 4) {                   /* a barre at lo */
        if (from == to) return 0;
        for (s = from; s <= to; s++)
            if (sh->fret[s] < lo) return 0;          /* an open or muted string under it */
        sh->barre = (uint8_t)lo;
        sh->bfrom = (uint8_t)from;
        sh->bto = (uint8_t)to;
        for (s = from; s <= to; s++)
            if (sh->fret[s] == lo) sh->finger[s] = 1;
        k = 1;
    }
    for (f = lo; f && f <= hi; f++)
        for (s = 0; s < n; s++)
            if (sh->fret[s] == f && !sh->finger[s]) {
                if (++k > 4) return 0;
                sh->finger[s] = (uint8_t)k;
            }
    return 1;
}

static int fr_score(const fr_tuning_t *t, const fr_shape_t *sh)
{
    int s, n = t->n, first = -1, last = -1, v = 0, lo = fr_shape_lo(sh, n);
    for (s = 0; s < n; s++)
        if (sh->fret[s] >= 0) {
            if (first < 0) first = s;
            last = s;
        }
    for (s = 0; s < n; s++) {
        if (sh->fret[s] > 0 && !(sh->barre && sh->finger[s] == 1)) v += 2;     /* a finger */
        if (sh->fret[s] == 0) v -= 1;                                          /* an open string */
        if (sh->fret[s] < 0) v += s < first ? 2 : s > last ? 6 : 10;           /* bass, treble, inside mutes */
    }
    {                                    /* how it sounds: a unison, a second or fourth right above the bass */
        uint8_t m[FR_STR_MAX];
        int k = fr_shape_midi(t, sh, m), i, j, b0 = 999, b1 = 999;
        for (i = 0; i < k; i++)
            for (j = i + 1; j < k; j++)
                if (m[i] == m[j]) v += 4;
        for (i = 0; i < k; i++)
            if (m[i] < b0) {
                b1 = b0;
                b0 = m[i];
            } else if (m[i] < b1)
                b1 = m[i];
        if (b1 < 999 && (b1 - b0 < 3 || b1 - b0 == 5)) v += 4;
    }
    if (sh->barre) v += 6;
    v += lo;                             /* up the neck */
    if (sh->no5) v += 3;
    return v;
}

static int fr_same(const fr_shape_t *a, const int8_t *fret, int n)
{
    int s;
    for (s = 0; s < n; s++)
        if (a->fret[s] != fret[s]) return 0;
    return 1;
}

static void fr_leaf(fr_search_t *q)
{
    const fr_tuning_t *t = q->t;
    fr_shape_t sh;
    int s, have = 0, sounding = 0, lowest = 999, low_pc = -1, i, j, minimum = t->n >= 6 ? t->n - 2 : t->n - 1;
    for (s = 0; s < t->n; s++) {
        int p;
        if (q->fret[s] < 0) continue;
        p = fr_midi(t, s, q->fret[s]);
        have |= 1 << th_mod(p, 12);
        sounding++;
        if (p < lowest) {
            lowest = p;
            low_pc = th_mod(p, 12);
        }
    }
    if (sounding < minimum || sounding < 3) return;
    if ((have & q->need) != q->need) return;
    if (!t->reentrant && low_pc != q->root) return;
    for (s = 0; s < FR_STR_MAX; s++) sh.fret[s] = (int8_t)(s < t->n ? q->fret[s] : -1);
    if (fr_shape_hi(&sh, t->n) - fr_shape_lo(&sh, t->n) >= FR_SPAN) return;
    for (i = 0; i < q->n; i++)
        if (fr_same(&q->out[i], sh.fret, t->n)) return;
    if (!fr_fingers(t, &sh)) return;
    sh.no5 = (uint8_t)(q->five >= 0 && !(have >> q->five & 1));
    sh.score = (int16_t)fr_score(t, &sh);
    /* keep the best max, in order (ties: lower on the neck, then the fret numbers) */
    for (i = 0; i < q->n; i++) {
        const fr_shape_t *o = &q->out[i];
        int lo_a = fr_shape_lo(&sh, t->n), lo_b = fr_shape_lo(o, t->n), d = 0;
        if (sh.score != o->score) d = sh.score - o->score;
        else if (lo_a != lo_b) d = lo_a - lo_b;
        else
            for (s = 0; s < t->n && !d; s++) d = sh.fret[s] - o->fret[s];
        if (d < 0) break;
    }
    if (i >= q->max) return;
    j = q->n < q->max ? q->n : q->max - 1;
    for (; j > i; j--) q->out[j] = q->out[j - 1];
    q->out[i] = sh;
    if (q->n < q->max) q->n++;
}

static void fr_search(fr_search_t *q, int s)
{
    const fr_tuning_t *t = q->t;
    int f;
    if (s == t->n) {
        fr_leaf(q);
        return;
    }
    q->fret[s] = -1;
    fr_search(q, s + 1);
    if (q->tones >> th_mod(t->open[s], 12) & 1) {
        q->fret[s] = 0;
        fr_search(q, s + 1);
    }
    for (f = q->b; f < q->b + FR_SPAN && f <= FR_FRETS; f++)
        if (q->tones >> th_mod(t->open[s] + f, 12) & 1) {
            q->fret[s] = (int8_t)f;
            fr_search(q, s + 1);
        }
    q->fret[s] = -1;
}

int fr_shapes(const fr_tuning_t *t, th_chord_t c, fr_shape_t *out, int max)
{
    fr_search_t q;
    int8_t f[TH_TONES_MAX];
    const th_qual_t *qu = &th_quals[c.q];
    int n = th_chord(c.root, c.q, f), i;
    q.t = t;
    q.tones = q.need = 0;
    q.five = -1;
    q.root = th_pc(c.root);
    for (i = 0; i < n; i++) {
        int pc = th_pc(f[i]);
        q.tones |= 1 << pc;
        if (qu->f[i] == 1 && n >= 3) q.five = pc;   /* a perfect fifth, in a chord with more than root and fifth */
        else q.need |= 1 << pc;
    }
    q.out = out;
    q.max = max > FR_SHAPES_MAX ? FR_SHAPES_MAX : max;
    q.n = 0;
    for (q.b = 1; q.b <= 12; q.b++) fr_search(&q, 0);
    return q.n;
}
