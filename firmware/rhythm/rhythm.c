/* SPDX-License-Identifier: GPL-3.0-or-later */
/* rhythm: RHYTHM's platform-free core (rhythm.h). No fm1-base, no libc beyond nothing: the app includes it in its
 * unity build, tests/rhythm_test.c on this computer. */
#include "rhythm.h"

typedef struct {
    const char *tok, *name;
    uint8_t beats, level, n;
    uint8_t on[4];                       /* ticks inside the cell */
} rh_cell_t;

static const rh_cell_t CELLS[RH_NCELLS] = {
    { "q", "TA", 1, 1, 1, { 0 } },
    { "ee", "TI-TI", 1, 1, 2, { 0, 6 } },
    { "r", "REST", 1, 2, 0, { 0 } },
    { "h", "TA-A", 2, 3, 1, { 0 } },
    { "ssss", "TIKA-TIKA", 1, 4, 4, { 0, 3, 6, 9 } },
    { "ess", "TI-TIKA", 1, 5, 3, { 0, 6, 9 } },
    { "sse", "TIKA-TI", 1, 5, 3, { 0, 3, 6 } },
    { "ds", "TIM-KA", 1, 6, 2, { 0, 9 } },
    { "syn", "SYN-CO-PA", 2, 7, 3, { 0, 6, 18 } },
    { "trip", "TRI-O-LA", 1, 8, 3, { 0, 4, 8 } },
};

const char *rh_cell_token(int c) { return c >= 0 && c < RH_NCELLS ? CELLS[c].tok : ""; }
const char *rh_cell_name(int c) { return c >= 0 && c < RH_NCELLS ? CELLS[c].name : ""; }
int rh_cell_beats(int c) { return c >= 0 && c < RH_NCELLS ? CELLS[c].beats : 0; }
int rh_cell_level(int c) { return c >= 0 && c < RH_NCELLS ? CELLS[c].level : 0; }

static int streq(const char *a, const char *b, int n)   /* a's first n chars are all of b */
{
    int i;
    for (i = 0; i < n; i++)
        if (!b[i] || a[i] != b[i]) return 0;
    return b[n] == 0;
}

/* the onsets from the cells, with their flags */
static void build(rh_pattern_t *p)
{
    int i, j, beat = 0, prev_rest = 0;
    p->n = 0;
    for (i = 0; i < p->ncells; i++) {
        const rh_cell_t *c = &CELLS[p->cells[i]];
        for (j = 0; j < c->n && p->n < RH_MAX_ON; j++) {
            uint16_t t = (uint16_t)(beat * RH_TPB + c->on[j]);
            uint8_t f = 0;
            if (j == 0 && prev_rest) f |= RH_F_AFTER_REST;
            if (p->cells[i] == RH_TIKA) f |= RH_F_SIXTEENTH;
            if (p->cells[i] == RH_SYNCO && j == 1) f |= RH_F_SYNCO;
            if (t % RH_TPB == 0) f |= RH_F_DOWNBEAT;
            p->on[p->n] = t;
            p->flag[p->n] = f;
            p->group[p->n] = (uint8_t)i;
            p->n++;
        }
        prev_rest = p->cells[i] == RH_REST;
        beat += c->beats;
    }
    p->nbeats = (uint8_t)beat;
}

int rh_parse(const char *s, int bpb, rh_pattern_t *p)
{
    int beats = 0;
    p->bpb = (uint8_t)bpb;
    p->ncells = 0;
    while (*s) {
        int n = 0, c;
        while (*s == ' ' || *s == '|' || *s == '_') s++;
        if (!*s) break;
        while (s[n] && s[n] != ' ' && s[n] != '|' && s[n] != '_') n++;
        for (c = 0; c < RH_NCELLS; c++)
            if (streq(s, CELLS[c].tok, n)) break;
        if (c == RH_NCELLS || p->ncells >= RH_MAX_BEATS || beats + CELLS[c].beats > RH_MAX_BEATS) return -1;
        p->cells[p->ncells++] = (uint8_t)c;
        beats += CELLS[c].beats;
        s += n;
    }
    if (!p->ncells) return -1;
    build(p);
    return 0;
}

void rh_text(const rh_pattern_t *p, char *out)
{
    int i, beat = 0, k = 0;
    const char *t;
    for (i = 0; i < p->ncells; i++) {
        if (i) {
            out[k++] = ' ';
            if (p->bpb && beat % p->bpb == 0) {
                out[k++] = '|';
                out[k++] = ' ';
            }
        }
        for (t = CELLS[p->cells[i]].tok; *t; t++) out[k++] = *t;
        beat += CELLS[p->cells[i]].beats;
    }
    out[k] = 0;
}

static uint32_t rnd(uint32_t *s)         /* xorshift32 */
{
    uint32_t x = *s ? *s : 0x9E3779B9u;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *s = x;
    return x;
}

void rh_generate(uint32_t *seed, int level, int bpb, int bars, rh_pattern_t *p)
{
    int tries, bar, c;
    if (level < 1) level = 1;
    if (level > RH_LEVELS) level = RH_LEVELS;
    if (bpb < 2) bpb = 2;
    if (bpb > 4) bpb = 4;
    if (bars < 1) bars = 1;
    if (bars * bpb > RH_MAX_BEATS) bars = RH_MAX_BEATS / bpb;
    for (tries = 0; tries < 16; tries++) {
        p->bpb = (uint8_t)bpb;
        p->ncells = 0;
        for (bar = 0; bar < bars; bar++) {
            int left = bpb;
            while (left > 0) {
                uint32_t total = 0, pick;
                int w[RH_NCELLS];
                for (c = 0; c < RH_NCELLS; c++) {
                    w[c] = 0;
                    if (CELLS[c].level > level || CELLS[c].beats > left) continue;
                    if (c == RH_REST && (p->ncells == 0 || p->cells[p->ncells - 1] == RH_REST)) continue;
                    w[c] = CELLS[c].level == level && level > 1 ? 3 : 1;   /* the newest cell, more often */
                    if (CELLS[c].beats == 2) w[c] *= 2;                    /* (fewer places it fits) */
                    total += (uint32_t)w[c];
                }
                pick = rnd(seed) % total;
                for (c = 0; c < RH_NCELLS; c++) {
                    if (pick < (uint32_t)w[c]) break;
                    pick -= (uint32_t)w[c];
                }
                p->cells[p->ncells++] = (uint8_t)c;
                left -= CELLS[c].beats;
            }
        }
        build(p);
        if (p->n >= 2) return;
    }
    rh_parse("q q", bpb, p);             /* (never seen: sixteen tries without two sounds) */
}

uint32_t rh_tick_ms_x1000(int bpm) { return (uint32_t)(60000000u / (uint32_t)(bpm * RH_TPB)); }

int32_t rh_onset_ms(const rh_pattern_t *p, int i, int bpm)
{
    return (int32_t)(((uint32_t)p->on[i] * 60000u * 2u / (uint32_t)(bpm * RH_TPB) + 1u) / 2u);
}

int rh_tol_good(int bpm)
{
    int beat = 60000 / bpm, t = beat * 7 / 100;
    return t < 35 ? t : 35;
}

static int32_t iabs(int32_t v) { return v < 0 ? -v : v; }

/* the alignment: cost[i][j] = least cost of onsets 0..i-1 against taps 0..j-1 */
static uint32_t dp_cost[RH_MAX_ON + 1][RH_MAX_TAPS + 1];
static uint8_t dp_how[RH_MAX_ON + 1][RH_MAX_TAPS + 1];   /* 0 matched, 1 onset missed, 2 tap extra */

void rh_score(const rh_pattern_t *p, int bpm, const int32_t *taps, int m, rh_result_t *r)
{
    int32_t e[RH_MAX_ON], w;
    int n = p->n, i, j, matched = 0, beat = 60000 / bpm;
    int32_t sum = 0;
    if (m > RH_MAX_TAPS) m = RH_MAX_TAPS;
    r->n = (uint8_t)n;
    r->good = r->ok = r->off = r->missed = r->extra = 0;
    r->tol_good = (uint16_t)rh_tol_good(bpm);
    r->tol_ok = (uint16_t)(2 * r->tol_good);
    w = beat / 2;                        /* further than half a beat: not this note */
    for (i = 0; i < n; i++) e[i] = rh_onset_ms(p, i, bpm);
    for (i = 0; i <= n; i++)
        for (j = 0; j <= m; j++) {
            uint32_t best = 0xFFFFFFFFu, c;
            uint8_t how = 0;
            if (!i && !j) {
                dp_cost[0][0] = 0;
                continue;
            }
            if (i && j && iabs(taps[j - 1] - e[i - 1]) <= w) {
                c = dp_cost[i - 1][j - 1] + (uint32_t)iabs(taps[j - 1] - e[i - 1]);
                if (c < best) best = c, how = 0;
            }
            if (i && (c = dp_cost[i - 1][j] + (uint32_t)w) < best) best = c, how = 1;
            if (j && (c = dp_cost[i][j - 1] + (uint32_t)w) < best) best = c, how = 2;
            dp_cost[i][j] = best;
            dp_how[i][j] = how;
        }
    for (i = n, j = m; i || j;) {
        uint8_t how = dp_how[i][j];
        if (!i) how = 2;
        else if (!j) how = 1;
        if (how == 0) {
            r->tap_of[i - 1] = (int8_t)(j - 1);
            r->err[i - 1] = (int16_t)(taps[j - 1] - e[i - 1]);
            i--, j--;
        } else if (how == 1) {
            r->tap_of[i - 1] = -1;
            r->err[i - 1] = 0;
            i--;
        } else {
            r->extra++;
            j--;
        }
    }
    for (i = 0; i < n; i++) {
        int32_t a = iabs(r->err[i]);
        if (r->tap_of[i] < 0) {
            r->grade[i] = RH_G_MISS;
            r->missed++;
            continue;
        }
        matched++;
        sum += r->err[i];
        if (a <= r->tol_good) r->grade[i] = RH_G_GOOD, r->good++;
        else if (a <= r->tol_ok) r->grade[i] = RH_G_OK, r->ok++;
        else r->grade[i] = RH_G_OFF, r->off++;
    }
    r->mean = (int16_t)(matched ? (sum >= 0 ? (sum + matched / 2) / matched : -((-sum + matched / 2) / matched)) : 0);
    r->score = (uint8_t)(n + r->extra ? (100 * (2 * r->good + r->ok)) / (2 * (n + r->extra)) : 0);

    /* the hint: the one thing most worth fixing */
    {
        int32_t thr = r->tol_good * 6 / 10, sync_d = 0, ar = 0, other = 0, dev = 0;
        int nsync = 0, nar = 0, nother = 0, ngroups = 0;
        if (thr < 6) thr = 6;
        for (i = 0; i < n; i++) {
            if (r->tap_of[i] < 0) continue;
            if (p->flag[i] & RH_F_AFTER_REST) ar += r->err[i], nar++;
            else other += r->err[i], nother++;
        }
        for (i = 0; i < n; i++)          /* an off-beat compared with the notes around it */
            if ((p->flag[i] & RH_F_SYNCO) && r->tap_of[i] >= 0) {
                int32_t ref = nother > 1 ? (other - r->err[i]) / (nother - 1) : 0;
                sync_d += iabs(r->err[i] - ref);
                nsync++;
            }
        for (i = 0; i + 3 < n; i++)      /* each TIKA-TIKA tapped whole: how far its gaps are from their mean */
            if ((p->flag[i] & RH_F_SIXTEENTH) && p->on[i] % RH_TPB == 0 && p->group[i + 3] == p->group[i]
                && r->tap_of[i] >= 0 && r->tap_of[i + 1] >= 0 && r->tap_of[i + 2] >= 0 && r->tap_of[i + 3] >= 0) {
                int32_t d[3], mean, k;
                for (k = 0; k < 3; k++) d[k] = taps[r->tap_of[i + k + 1]] - taps[r->tap_of[i + k]];
                mean = (d[0] + d[1] + d[2]) / 3;
                dev += (iabs(d[0] - mean) + iabs(d[1] - mean) + iabs(d[2] - mean)) / 3;
                ngroups++;
            }
        if (m == 0) r->hint = RH_H_NONE;
        else if (r->missed >= 2) r->hint = RH_H_MISSED;
        else if (r->extra >= 2) r->hint = RH_H_EXTRA;
        else if (nsync && sync_d / nsync > r->tol_ok) r->hint = RH_H_SYNCO;
        else if (nar && nother && ar / nar - other / nother > r->tol_good && ar / nar > thr) r->hint = RH_H_LATE_REST;
        else if (ngroups && dev / ngroups > (beat / 4 * 12 / 100 > 8 ? beat / 4 * 12 / 100 : 8)) r->hint = RH_H_UNEVEN16;
        else if (matched >= 2 && r->mean < -thr) r->hint = RH_H_RUSHING;
        else if (matched >= 2 && r->mean > thr) r->hint = RH_H_DRAGGING;
        else if (r->missed) r->hint = RH_H_MISSED;
        else if (r->extra) r->hint = RH_H_EXTRA;
        else if (r->good == n) r->hint = RH_H_GOOD;
        else r->hint = RH_H_ALMOST;
    }
}

const char *rh_hint_text(int h)
{
    static const char *const T[RH_H_N] = { "", "GOOD", "CLOSE", "MISSED NOTES", "EXTRA TAPS", "SYNCOPATION SQUARED",
                                           "LATE AFTER RESTS", "UNEVEN 16THS", "RUSHING", "DRAGGING" };
    return h >= 0 && h < RH_H_N ? T[h] : "";
}

const char *rh_hint_more(int h)
{
    static const char *const T[RH_H_N] = { "TAP ANY KEY", "ON THE GRID", "AGAIN, SAME TEMPO", "PLAY: HEAR IT AGAIN",
                                           "ONE TAP A NOTE", "HOLD THE OFF-BEAT", "COUNT THROUGH THE REST",
                                           "SAY TI-KA-TI-KA", "WAIT FOR THE CLICK", "LEAN INTO THE BEAT" };
    return h >= 0 && h < RH_H_N ? T[h] : "";
}
