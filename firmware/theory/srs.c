/* SPDX-License-Identifier: GPL-3.0-or-later */
/* srs: see srs.h. */
#include "srs.h"

uint32_t sr_rand(uint32_t *x)
{
    uint32_t v = *x ? *x : 0x9E3779B9u;
    v ^= v << 13;
    v ^= v >> 17;
    v ^= v << 5;
    *x = v;
    return v;
}

void sr_init(sr_state_t *s, const sr_course_t *c)
{
    uint32_t i;
    uint8_t *p = (uint8_t *)s;
    for (i = 0; i < sizeof *s; i++) p[i] = 0;
    s->n = c->n;
    s->open = c->ngroups ? c->group_end[0] : c->n;
    for (i = 0; i < SR_MAX; i++) s->last[i] = SR_NEVER;
}

int sr_valid(const sr_state_t *s, const sr_course_t *c)
{
    uint32_t i;
    if (s->n != c->n || s->n > SR_MAX || s->open > s->n || s->right > s->answered) return 0;
    if (c->ngroups && s->open < c->group_end[0]) return 0;
    for (i = 0; i < s->n; i++)
        if (s->box[i] > SR_TOP || (s->last[i] != SR_NEVER && s->last[i] > s->round)) return 0;
    return 1;
}

int sr_due(const sr_state_t *s, int item)
{
    if (s->last[item] == SR_NEVER) return 0;
    return (uint16_t)(s->round - s->last[item]) >= (1u << s->box[item]);
}

static int sr_in_focus(const sr_course_t *c, int item, uint32_t focus)
{
    uint32_t key = focus >> 8 & 0xFFu;
    if (key) return c->key[item] == key - 1u;
    return (int)(focus >> c->seg[item] & 1u);
}

int sr_due_count(const sr_state_t *s, const sr_course_t *c, uint32_t focus)
{
    int i, n = 0;
    for (i = 0; i < s->open; i++) n += sr_in_focus(c, i, focus) && sr_due(s, i);
    return n;
}

int sr_new_count(const sr_state_t *s, const sr_course_t *c, uint32_t focus)
{
    int i, n = 0;
    for (i = 0; i < s->open; i++) n += sr_in_focus(c, i, focus) && s->last[i] == SR_NEVER;
    return n;
}

/* shuffle, then lay the cards out again so that no item comes twice in a row and no two of one key follow each other:
 * each place takes, of the cards left that differ from the one before, the one whose item, then key, has the most
 * cards left (so nothing is left over at the end); keyless cards fit anywhere */
static void sr_spread(uint8_t *it, uint8_t *ex, int n, const sr_course_t *c, uint32_t *rng)
{
    static uint8_t left[256], ileft[SR_MAX];
    int i, j;
    for (i = n - 1; i > 0; i--) {
        uint8_t t;
        j = (int)(sr_rand(rng) % (uint32_t)(i + 1));
        t = it[i]; it[i] = it[j]; it[j] = t;
        t = ex[i]; ex[i] = ex[j]; ex[j] = t;
    }
    for (i = 0; i < 256; i++) left[i] = 0;
    for (i = 0; i < (int)SR_MAX; i++) ileft[i] = 0;
    for (i = 0; i < n; i++) { left[c->key[it[i]]]++; ileft[it[i]]++; }
    for (i = 0; i < n; i++) {
        int prev = i ? c->key[it[i - 1]] : -1, pit = i ? it[i - 1] : -1, best = -1, bs = -1, other = -1, os = -1;
        uint8_t t;
        for (j = i; j < n; j++) {
            int k = c->key[it[j]], sc = ileft[it[j]] * 256 + (k == SR_NOKEY ? 0 : left[k]);
            if (it[j] == pit) continue;  /* one item twice in a row: never, if anything else is left */
            if (sc > os) { os = sc; other = j; }
            if (k != SR_NOKEY && k == prev) continue;
            if (sc > bs) { bs = sc; best = j; }
        }
        if (best < 0) best = other;      /* only its own key left: it has to follow */
        if (best < 0) best = i;
        t = it[i]; it[i] = it[best]; it[best] = t;
        t = ex[i]; ex[i] = ex[best]; ex[best] = t;
        left[c->key[it[i]]]--;
        ileft[it[i]]--;
    }
}

void sr_build(const sr_state_t *s, sr_round_t *r, const sr_course_t *c, int n, uint32_t focus, uint32_t seed)
{
    static uint8_t pick[SR_ROUND_MAX], extra[SR_ROUND_MAX], used[SR_MAX], seg_items[SR_ROUND_MAX], seg_ex[SR_ROUND_MAX];
    static uint32_t key[SR_MAX];
    int npick = 0, nnew = 0, base, i, j, seg, maxseg = 0, weak = 0, k;
    uint8_t *p = (uint8_t *)r;
    for (i = 0; i < (int)sizeof *r; i++) p[i] = 0;
    r->rng = seed ? seed : 1u;
    if (n < 4) n = 4;
    if (n > (int)SR_ROUND_MAX) n = SR_ROUND_MAX;
    for (i = 0; i < s->n; i++) {
        used[i] = 0;
        if (c->seg[i] > maxseg) maxseg = c->seg[i];
    }
    /* new items, in course order: shown first, together, then asked in their segment */
    for (i = 0; i < s->open && nnew < (int)SR_NEW_MAX && npick < n; i++)
        if (sr_in_focus(c, i, focus) && s->last[i] == SR_NEVER) {
            r->card[r->n].item = (uint8_t)i;
            r->card[r->n].kind = SR_SHOW;
            r->n++;
            used[i] = 1;
            extra[npick] = 0;
            pick[npick++] = (uint8_t)i;
            nnew++;
        }
    /* due items, lowest box and longest wait first; then not due, to fill the round */
    base = npick;
    for (j = 0; j < 2 && npick < n; j++) {
        int cand = 0;
        static uint8_t ci[SR_MAX];
        for (i = 0; i < s->open; i++) {
            uint16_t wait;
            if (used[i] || !sr_in_focus(c, i, focus) || s->last[i] == SR_NEVER || sr_due(s, i) != (j == 0)) continue;
            wait = (uint16_t)(s->round - s->last[i]);
            key[i] = (uint32_t)s->box[i] * 0x100000u + (uint32_t)(0xFFFFu - wait) * 16u + (sr_rand(&r->rng) & 15u);
            for (k = cand; k > 0 && key[ci[k - 1]] > key[i]; k--) ci[k] = ci[k - 1];
            ci[k] = (uint8_t)i;
            cand++;
        }
        for (k = 0; k < cand && npick < n; k++) {
            used[ci[k]] = 1;
            extra[npick] = 0;
            pick[npick++] = ci[k];
        }
        if (j == 0) base = npick;        /* new + due: the weak ones among these come back at the end */
    }
    /* still short: the items whose question changes, once more each in turn (twice at most), as practice */
    if (c->vary && npick < n) {
        static uint8_t vi[SR_MAX];
        int nv = 0, t;
        for (i = 0; i < s->open; i++)
            if (c->vary[i] && sr_in_focus(c, i, focus) && (s->last[i] != SR_NEVER || used[i])) vi[nv++] = (uint8_t)i;
        for (i = nv - 1; i > 0; i--) {
            uint8_t x;
            j = (int)(sr_rand(&r->rng) % (uint32_t)(i + 1));
            x = vi[i]; vi[i] = vi[j]; vi[j] = x;
        }
        for (t = 0; t < nv * (int)SR_EXTRA_MAX && npick < n; t++) {
            extra[npick] = 1;
            pick[npick++] = vi[t % nv];
        }
    }
    /* each segment in turn, spread by key */
    for (seg = 0; seg <= maxseg; seg++) {
        int m = 0;
        for (i = 0; i < npick; i++)
            if (c->seg[pick[i]] == seg) {
                seg_ex[m] = extra[i];
                seg_items[m++] = pick[i];
            }
        sr_spread(seg_items, seg_ex, m, c, &r->rng);
        for (i = 0; i < m && r->n < SR_CARDS_MAX; i++) {
            r->card[r->n].item = seg_items[i];
            r->card[r->n].kind = SR_ASK;
            r->card[r->n].again = seg_ex[i];
            r->n++;
        }
    }
    /* the weak ones (new, or box 0..1) once more at the end */
    for (i = 0; i < base; i++)
        if (s->last[pick[i]] == SR_NEVER || s->box[pick[i]] <= 1u) {
            seg_ex[weak] = 1;
            seg_items[weak++] = pick[i];
        }
    sr_spread(seg_items, seg_ex, weak, c, &r->rng);
    for (i = 0; i < weak && r->n < SR_CARDS_MAX; i++) {
        r->card[r->n].item = seg_items[i];
        r->card[r->n].kind = SR_ASK;
        r->card[r->n].again = 1;
        r->n++;
    }
}

const sr_card_t *sr_card(const sr_round_t *r) { return r->at < r->n ? &r->card[r->at] : 0; }

static void sr_again(sr_round_t *r, uint8_t item)
{
    if (r->n >= SR_CARDS_MAX) return;
    r->card[r->n].item = item;
    r->card[r->n].kind = SR_ASK;
    r->card[r->n].again = 1;
    r->n++;
}

void sr_answer(sr_state_t *s, sr_round_t *r, int grade)
{
    const sr_card_t *c = sr_card(r);
    uint8_t it;
    if (!c) return;
    it = c->item;
    if (c->kind == SR_SHOW) {
        r->shown++;
        r->at++;
        return;
    }
    s->answered++;
    switch (grade) {
    case SR_WRONG:
        s->box[it] = 0;
        r->miss++;
        sr_again(r, it);
        break;
    case SR_SLOW:                        /* right, but it took long: the box stays, asked once more */
        s->right++;
        r->ok++;
        r->slow++;
        if (!c->again) sr_again(r, it);
        break;
    default:                             /* right, or easy: up one box, or two once it is past the first */
        s->right++;
        r->ok++;
        if (!c->again) {
            int up = grade == SR_EASY && s->box[it] >= 1u ? 2 : 1;
            s->box[it] = (uint8_t)(s->box[it] + up > (int)SR_TOP ? SR_TOP : s->box[it] + up);
            r->up++;
        }
        break;
    }
    s->last[it] = s->round;
    r->at++;
}

int sr_finish(sr_state_t *s, sr_round_t *r, const sr_course_t *c)
{
    int asked = r->ok + r->miss, i, g;
    r->opened = 0;
    if (asked + r->shown == 0) return 0;
    s->round++;
    if (asked) {
        if (r->ok * 100 >= asked * 80) s->streak++;
        else s->streak = 0;
    }
    if (s->open >= s->n) return 0;
    for (i = 0; i < s->open; i++)
        if (s->box[i] < c->unlock_box) return 0;
    for (g = 0; g < c->ngroups; g++)
        if (c->group_end[g] > s->open) {
            s->open = c->group_end[g];
            r->opened = 1;
            return 1;
        }
    return 0;
}

int sr_waiting(const sr_state_t *s, const sr_course_t *c, uint8_t *items, int max)
{
    int i, n = 0;
    for (i = 0; i < s->open && n < max; i++)
        if (s->box[i] < c->unlock_box) items[n++] = (uint8_t)i;
    return n;
}

/* ------------------------------------------------------------------ backup --- */
static int sr_put(uint8_t *o, int i, uint32_t v, int bytes)
{
    int b;
    for (b = 0; b < bytes; b++) o[i++] = (uint8_t)(v >> (8 * b));
    return i;
}

static uint32_t sr_get(const uint8_t *p, int i, int bytes)
{
    uint32_t v = 0;
    int b;
    for (b = 0; b < bytes; b++) v |= (uint32_t)p[i + b] << (8 * b);
    return v;
}

int sr_blob(const sr_state_t *s, uint8_t *out)
{
    int i = 0, k;
    i = sr_put(out, i, s->round, 2);
    i = sr_put(out, i, s->streak, 2);
    i = sr_put(out, i, s->n, 1);
    i = sr_put(out, i, s->open, 1);
    i = sr_put(out, i, s->answered, 4);
    i = sr_put(out, i, s->right, 4);
    for (k = 0; k < s->n; k++) i = sr_put(out, i, s->box[k], 1);
    for (k = 0; k < s->n; k++) i = sr_put(out, i, s->last[k], 2);
    return i;
}

int sr_unblob(sr_state_t *s, const sr_course_t *c, const uint8_t *in, int len)
{
    static sr_state_t t;
    int k, n;
    if (len < 14) return 0;
    n = in[4];
    if (n != c->n || len != SR_BLOB_N(n)) return 0;
    sr_init(&t, c);
    t.round = (uint16_t)sr_get(in, 0, 2);
    t.streak = (uint16_t)sr_get(in, 2, 2);
    t.n = (uint8_t)n;
    t.open = in[5];
    t.answered = sr_get(in, 6, 4);
    t.right = sr_get(in, 10, 4);
    for (k = 0; k < n; k++) t.box[k] = in[14 + k];
    for (k = 0; k < n; k++) t.last[k] = (uint16_t)sr_get(in, 14 + n + 2 * k, 2);
    if (!sr_valid(&t, c)) return 0;
    {
        const uint8_t *a = (const uint8_t *)&t;
        uint8_t *b = (uint8_t *)s;
        uint32_t i;
        for (i = 0; i < sizeof t; i++) b[i] = a[i];
    }
    return 1;
}
