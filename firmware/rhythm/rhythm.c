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

/* ------------------------------------------------------------------ v0.3 --- */
static int iroundf(float v) { return (int)(v < 0.0f ? v - 0.5f : v + 0.5f); }

static float sqrtf_(float v)             /* (no libm here) */
{
    float x = v > 1.0f ? v : 1.0f;
    int i;
    if (v <= 0.0f) return 0.0f;
    for (i = 0; i < 24; i++) x = 0.5f * (x + v / x);
    return x;
}

/* a least-squares line through (x, y): slope and intercept; 0 with fewer than two distinct x */
static int fit(const float *x, const float *y, int n, float *b, float *a)
{
    float mx = 0.0f, my = 0.0f, sxx = 0.0f, sxy = 0.0f;
    int i;
    if (n < 2) return 0;
    for (i = 0; i < n; i++) mx += x[i], my += y[i];
    mx /= (float)n;
    my /= (float)n;
    for (i = 0; i < n; i++) sxx += (x[i] - mx) * (x[i] - mx), sxy += (x[i] - mx) * (y[i] - my);
    if (sxx <= 0.0f) return 0;
    *b = sxy / sxx;
    *a = my - *b * mx;
    return 1;
}

static int16_t clamp16(float v) { return (int16_t)(v > 32000.0f ? 32000 : v < -32000.0f ? -32000 : iroundf(v)); }

void rh_pulse(int bpm, int beats, const int32_t *taps, int m, rh_pulse_t *r)
{
    float beat = 60000.0f / (float)bpm, p = beat, x[RH_PULSE_MAX], y[RH_PULSE_MAX], b, a;
    int32_t gaps[RH_PULSE_TAPS];
    int i, j, k, n = 0, last = -1, idx = 0, ng = 0;
    if (beats > RH_PULSE_MAX) beats = RH_PULSE_MAX;
    if (m > RH_PULSE_TAPS) m = RH_PULSE_TAPS;
    r->n = (uint8_t)beats;
    r->answered = r->missed = r->extra = r->good = r->ok = r->off = 0;
    r->mean = r->bpm_x10 = r->tempo_x10 = r->trend_x10 = r->end_ms = r->jitter = 0;
    r->tol_good = (uint16_t)rh_tol_good(bpm);
    r->tol_ok = (uint16_t)(2 * r->tol_good);
    for (k = 0; k < beats; k++) r->tap_of[k] = -1, r->err[k] = 0, r->grade[k] = RH_G_MISS;
    for (k = 0; k < RH_HIST; k++) r->hist[k] = 0;
    /* your pulse: the median gap between taps (a skipped beat or a double tap does not move it) */
    for (i = 1; i < m; i++) {
        int32_t g = taps[i] - taps[i - 1];
        if ((float)g > 0.3f * beat) gaps[ng++] = g;
    }
    for (i = 1; i < ng; i++)             /* (insertion sort: a few dozen) */
        for (j = i; j > 0 && gaps[j - 1] > gaps[j]; j--) {
            int32_t t = gaps[j];
            gaps[j] = gaps[j - 1];
            gaps[j - 1] = t;
        }
    if (ng >= 2) p = (float)gaps[ng / 2];
    if (p < 0.6f * beat) p = 0.6f * beat;
    if (p > 1.6f * beat) p = 1.6f * beat;
    /* each tap's beat: the first by the grid, the next ones by how many of your own beats since the last */
    for (i = 0; i < m; i++) {
        if (last < 0) {
            idx = iroundf((float)taps[i] / beat);
            if (idx < 0) idx = 0;
        } else {
            int step = iroundf((float)(taps[i] - taps[last]) / p);
            if (step < 1) {              /* a second tap on one beat */
                r->extra++;
                continue;
            }
            idx += step;
        }
        if (idx >= beats) {
            r->extra++;
            continue;
        }
        last = i;
        r->tap_of[idx] = (int8_t)i;
    }
    {
        int32_t sum = 0;
        for (k = 0; k < beats; k++) {
            int32_t e, ae;
            int bin;
            if (r->tap_of[k] < 0) {
                r->missed++;
                continue;
            }
            e = taps[r->tap_of[k]] - (int32_t)iroundf((float)k * beat);
            ae = iabs(e);
            r->err[k] = (int16_t)(e > 9999 ? 9999 : e < -9999 ? -9999 : e);
            r->answered++;
            sum += e;
            if (ae <= r->tol_good) r->grade[k] = RH_G_GOOD, r->good++;
            else if (ae <= r->tol_ok) r->grade[k] = RH_G_OK, r->ok++;
            else r->grade[k] = RH_G_OFF, r->off++;
            bin = iroundf((float)e / (float)r->tol_good);
            bin = bin < -RH_HIST / 2 ? -RH_HIST / 2 : bin > RH_HIST / 2 ? RH_HIST / 2 : bin;
            if (r->hist[bin + RH_HIST / 2] < 255) r->hist[bin + RH_HIST / 2]++;
            x[n] = (float)k;
            y[n] = (float)taps[r->tap_of[k]];
            n++;
        }
        if (r->answered) r->mean = clamp16((float)sum / (float)r->answered);
    }
    if (fit(x, y, n, &b, &a) && b > 0.0f) {
        float ss = 0.0f, b1, a1, b2, a2;
        int h;
        r->bpm_x10 = clamp16(600000.0f / b);
        r->tempo_x10 = clamp16((beat / b - 1.0f) * 1000.0f);
        r->end_ms = clamp16(a + b * (float)(beats - 1) - beat * (float)(beats - 1));
        for (i = 0; i < n; i++) ss += (y[i] - a - b * x[i]) * (y[i] - a - b * x[i]);
        r->jitter = clamp16(sqrtf_(ss / (float)n));
        for (h = 0; h < n && x[h] < (float)beats / 2.0f; h++) {}
        if (h >= 3 && n - h >= 3 && fit(x, y, h, &b1, &a1) && fit(x + h, y + h, n - h, &b2, &a2) && b2 > 0.0f)
            r->trend_x10 = clamp16((b1 / b2 - 1.0f) * 1000.0f);
    }
    r->score = (uint8_t)(beats + r->extra ? (100 * (2 * r->good + r->ok)) / (2 * (beats + r->extra)) : 0);
    {
        int many = beats / 8 > 2 ? beats / 8 : 2;
        if (!m) r->hint = RH_P_NONE;
        else if (r->missed >= many) r->hint = RH_P_MISSED;
        else if (r->extra >= 2) r->hint = RH_P_EXTRA;
        else if (r->trend_x10 > 30) r->hint = RH_P_SPEEDING;
        else if (r->trend_x10 < -30) r->hint = RH_P_SLOWING;
        else if (r->tempo_x10 > 20) r->hint = RH_P_RUSHING;
        else if (r->tempo_x10 < -20) r->hint = RH_P_DRAGGING;
        else if (r->jitter > r->tol_good * 3 / 4) r->hint = RH_P_UNSTEADY;
        else if (r->mean < -(int)r->tol_good) r->hint = RH_P_RUSHING;
        else if (r->mean > (int)r->tol_good) r->hint = RH_P_DRAGGING;
        else if (r->missed) r->hint = RH_P_MISSED;
        else if (r->extra) r->hint = RH_P_EXTRA;
        else if (r->good == beats) r->hint = RH_P_GOOD;
        else r->hint = RH_P_CLOSE;
    }
}

const char *rh_pulse_hint(int h)
{
    static const char *const T[RH_P_N] = { "", "STEADY", "CLOSE", "MISSED BEATS", "EXTRA TAPS", "SPEEDING UP",
                                           "SLOWING DOWN", "RUSHING", "DRAGGING", "UNSTEADY" };
    return h >= 0 && h < RH_P_N ? T[h] : "";
}

const char *rh_pulse_more(int h)
{
    static const char *const T[RH_P_N] = { "TAP ANY KEY", "A CLOCK", "AGAIN, SAME TEMPO", "COUNT EVERY BEAT",
                                           "ONE TAP A BEAT", "SETTLE INTO IT", "KEEP IT MOVING", "HOLD BACK",
                                           "PUSH A LITTLE", "SUBDIVIDE IN YOUR HEAD" };
    return h >= 0 && h < RH_P_N ? T[h] : "";
}

/* ---- JUDGE */
static const float ST_START[RH_J_N] = { 8.0f, 60.0f, 12.0f }, ST_MIN[RH_J_N] = { 0.5f, 3.0f, 1.0f },
                   ST_MAX[RH_J_N] = { 30.0f, 150.0f, 40.0f };

const char *rh_judge_name(int t)
{
    static const char *const T[RH_J_N] = { "TEMPO", "FEEL", "DRIFT" };
    return t >= 0 && t < RH_J_N ? T[t] : "";
}
const char *rh_judge_unit(int t) { return t == RH_J_FEEL ? "MS" : "%"; }
float rh_judge_min(int t) { return t >= 0 && t < RH_J_N ? ST_MIN[t] : 1.0f; }

void rh_stair_init(rh_stair_t *s, int type)
{
    int i;
    if (type < 0 || type >= RH_J_N) type = 0;
    s->type = (uint8_t)type;
    s->trials = s->run = s->revs = s->dir = s->done = 0;
    s->d = ST_START[type];
    for (i = 0; i < RH_ST_REVS; i++) s->rev[i] = 0.0f;
    for (i = 0; i < RH_ST_TRIALS; i++) s->hist_x10[i] = 0, s->right[i] = 0;
}

int rh_stair_answer(rh_stair_t *s, int correct)
{
    float f = s->revs < 2 ? 0.7f : 0.84f;   /* big steps first, then finer */
    int move = 0;
    if (s->done) return 1;
    s->hist_x10[s->trials] = (uint16_t)iroundf(s->d * 10.0f);
    s->right[s->trials] = (uint8_t)(correct != 0);
    s->trials++;
    if (correct) {
        if (++s->run >= 2) s->run = 0, move = 1;
    } else
        s->run = 0, move = 2;
    if (move) {
        if (s->dir && s->dir != move && s->revs < RH_ST_REVS) s->rev[s->revs++] = s->d;
        s->dir = (uint8_t)move;
        s->d = move == 1 ? s->d * f : s->d / f;
        if (s->d < ST_MIN[s->type]) s->d = ST_MIN[s->type];
        if (s->d > ST_MAX[s->type]) s->d = ST_MAX[s->type];
    }
    if (s->revs >= RH_ST_REVS || s->trials >= RH_ST_TRIALS) s->done = 1;
    return s->done;
}

float rh_stair_threshold(const rh_stair_t *s)
{
    int i, from = s->revs > 6 ? s->revs - 6 : 0, n = s->revs - from;
    float prod = 1.0f, x = 0.0f;
    if (s->revs < 2) return s->d;
    for (i = from; i < s->revs; i++) prod *= s->rev[i], x += s->rev[i];
    x /= (float)n;                       /* the n-th root of the product, from the mean (Newton, from above) */
    for (i = 0; i < 40; i++) {
        float xn = 1.0f;
        int k;
        for (k = 0; k < n - 1; k++) xn *= x;
        x = x - (xn * x - prod) / ((float)n * xn);
    }
    return x;
}

static void seq_add(rh_seq_t *q, float us, uint8_t ev)
{
    int i;
    uint32_t u = (uint32_t)(us < 0.0f ? 0.0f : us + 0.5f);
    if (q->n >= RH_SEQ_MAX) return;
    for (i = q->n; i > 0 && q->us[i - 1] > u; i--) q->us[i] = q->us[i - 1], q->ev[i] = q->ev[i - 1];   /* in order */
    q->us[i] = u;
    q->ev[i] = ev;
    q->n++;
}

#define LEAD_US 50000.0f                 /* before the first sound: a FEEL note may come early */

void rh_judge_seq(int type, int bpm, int bpb, float d, int sign, rh_seq_t *q)
{
    float beat = 60000000.0f / (float)bpm, t = LEAD_US;
    int k;
    q->n = 0;
    if (type == RH_J_TEMPO) {            /* two bars, a beat's rest, two bars at the other tempo */
        float b2 = beat / (1.0f + (float)sign * d / 100.0f);
        for (k = 0; k < 2 * bpb; k++) seq_add(q, t + (float)k * beat, k % bpb ? RH_S_CLICK : RH_S_ACCENT);
        t += (float)(2 * bpb + 1) * beat;
        for (k = 0; k < 2 * bpb; k++) seq_add(q, t + (float)k * b2, k % bpb ? RH_S_CLICK : RH_S_ACCENT);
    } else if (type == RH_J_FEEL) {      /* a bar of clicks; then two bars of eighths ahead of or behind them */
        float off = (float)sign * d * -1000.0f;   /* rushing: early */
        for (k = 0; k < 3 * bpb; k++) seq_add(q, t + (float)k * beat, k % bpb ? RH_S_CLICK : RH_S_ACCENT);
        for (k = 0; k < 4 * bpb; k++) seq_add(q, t + (float)bpb * beat + (float)k * beat / 2.0f + off, RH_S_NOTE);
    } else {                             /* a bar at the tempo, then three that turn faster or slower beat by beat */
        float end = beat / (1.0f + (float)sign * d / 100.0f);
        int n = 3 * bpb;
        for (k = 0; k < bpb; k++) seq_add(q, t + (float)k * beat, k % bpb ? RH_S_CLICK : RH_S_ACCENT);
        t += (float)bpb * beat;
        for (k = 0; k < n; k++) {
            seq_add(q, t, k % bpb ? RH_S_CLICK : RH_S_ACCENT);
            t += beat + (end - beat) * (float)(k + 1) / (float)n;
        }
    }
}

void rh_clicks_seq(int bpm, int bpb, int bars, rh_seq_t *q)
{
    float beat = 60000000.0f / (float)bpm;
    int k;
    q->n = 0;
    for (k = 0; k < bars * bpb; k++) seq_add(q, LEAD_US + (float)k * beat, k % bpb ? RH_S_CLICK : RH_S_ACCENT);
}

/* ---- QUIZ */
const rh_term_t RH_TERMS[RH_NTERMS] = {
    { "LARGHISSIMO", 0, 24, 0 },  { "GRAVE", 24, 40, 0 },     { "LARGO", 40, 66, 1 },
    { "LENTO", 40, 60, 0 },       { "LARGHETTO", 44, 66, 0 }, { "ADAGIO", 44, 66, 1 },
    { "ADAGIETTO", 46, 80, 0 },   { "ANDANTE", 56, 108, 1 },  { "ANDANTINO", 80, 108, 0 },
    { "MODERATO", 108, 120, 1 },  { "ALLEGRETTO", 112, 120, 0 }, { "ALLEGRO MODERATO", 116, 120, 0 },
    { "ALLEGRO", 120, 156, 1 },   { "VIVACE", 156, 176, 1 },  { "PRESTO", 168, 200, 1 },
    { "PRESTISSIMO", 200, 0, 0 },
};

/* kinds: 0 gradually faster, 1 at once faster, 2 gradually slower, 3 at once slower, 4 back, 5 free */
const rh_change_t RH_CHANGES[RH_NCHANGES] = {
    { "ACCELERANDO", "GRADUALLY FASTER", 0 },  { "RITARDANDO", "GRADUALLY SLOWER", 2 },
    { "A TEMPO", "BACK TO THE TEMPO", 4 },     { "PIU MOSSO", "MORE MOTION: FASTER", 1 },
    { "MENO MOSSO", "LESS MOTION: SLOWER", 3 }, { "RUBATO", "FREE, STOLEN TIME", 5 },
    { "RALLENTANDO", "SLOWING DOWN", 2 },      { "RITENUTO", "HELD BACK AT ONCE", 3 },
    { "STRETTO", "PRESSING FASTER", 1 },       { "ALLARGANDO", "BROADER, SLOWER", 2 },
    { "TEMPO PRIMO", "THE FIRST TEMPO", 4 },
};
#define CHANGES_L1 6

const rh_meter_t RH_METERS[RH_NMETERS] = {
    { "3/4", 6, 3, { 2, 2, 2 }, 1 },    { "4/4", 8, 4, { 2, 2, 2, 2 }, 1 },
    { "6/8", 6, 2, { 3, 3 }, 1 },       { "5/4", 10, 5, { 2, 2, 2, 2, 2 }, 1 },
    { "7/8", 7, 3, { 2, 2, 3 }, 2 },    { "9/8", 9, 3, { 3, 3, 3 }, 2 },
};

int rh_term_has(int t, int bpm)
{
    const rh_term_t *e = &RH_TERMS[t];
    return bpm >= e->lo && (!e->hi || bpm <= e->hi);
}

int rh_term_draw_bpm(uint32_t *seed, int t)
{
    const rh_term_t *e = &RH_TERMS[t];
    int lo = e->lo ? e->lo : 18, hi = e->hi ? e->hi : 230;
    return lo + (int)(rnd(seed) % (uint32_t)(hi - lo + 1));
}

static void put_int(char *d, int v)
{
    char t[8];
    int i = 0;
    while (*d) d++;
    do t[i++] = (char)('0' + v % 10), v /= 10;
    while (v && i < 7);
    while (i) *d++ = t[--i];
    *d = 0;
}

void rh_term_range(int t, char *out)
{
    const rh_term_t *e = &RH_TERMS[t];
    out[0] = 0;
    if (!e->lo) {
        put_int(out, e->hi);
        while (*out) out++;
        out[0] = '-', out[1] = 0;
        return;
    }
    put_int(out, e->lo);
    while (*out) out++;
    if (!e->hi) {
        out[0] = '+', out[1] = 0;
        return;
    }
    *out++ = '-';
    *out = 0;
    put_int(out, e->hi);
}

int rh_quiz_items(int type, int level)
{
    int i, n = 0;
    switch (type) {
    case RH_Q_CHANGE:
        return level <= 1 ? CHANGES_L1 : RH_NCHANGES;
    case RH_Q_METER:
        for (i = 0; i < RH_NMETERS; i++) n += RH_METERS[i].level <= (level <= 1 ? 1 : 2);
        return n;
    default:
        if (level > 1) return RH_NTERMS;
        for (i = 0; i < RH_NTERMS; i++) n += RH_TERMS[i].core;
        return n;
    }
}

/* the i-th item a type has at a level: its index in its table */
static int quiz_item(int type, int level, int i)
{
    int k, n = 0;
    if (type == RH_Q_CHANGE) return i;
    for (k = 0; k < (type == RH_Q_METER ? RH_NMETERS : RH_NTERMS); k++) {
        int in = type == RH_Q_METER ? RH_METERS[k].level <= (level <= 1 ? 1 : 2) : level > 1 || RH_TERMS[k].core;
        if (in && n++ == i) return k;
    }
    return 0;
}

const char *rh_quiz_name(int t)
{
    static const char *const T[RH_Q_N] = { "TERM>BPM", "BPM>TERM", "HEAR", "CHANGE", "METER" };
    return t >= 0 && t < RH_Q_N ? T[t] : "";
}

void rh_quiz_make(uint32_t *seed, int type, int level, const uint8_t *weight, int prev, rh_quiz_t *q)
{
    int n = rh_quiz_items(type, level), i, k, tot = 0, pick, got = 1, tries;
    int item[RH_NTERMS];
    uint8_t opt[4];
    uint16_t val[4];
    for (i = 0; i < n; i++) item[i] = quiz_item(type, level, i);
    for (i = 0; i < n; i++)
        if (item[i] != prev || n < 2) tot += 1 + 2 * (weight && weight[item[i]] > 4 ? 4 : weight ? weight[item[i]] : 0);
    pick = (int)(rnd(seed) % (uint32_t)tot);
    for (i = 0; i < n; i++) {
        if (item[i] == prev && n >= 2) continue;
        pick -= 1 + 2 * (weight && weight[item[i]] > 4 ? 4 : weight ? weight[item[i]] : 0);
        if (pick < 0) break;
    }
    if (i >= n) i = n - 1;
    q->type = (uint8_t)type;
    q->level = (uint8_t)level;
    q->ask = (uint8_t)item[i];
    q->bpm = 0;
    q->dir = 0;
    opt[0] = q->ask;
    val[0] = 0;
    if (type == RH_Q_TERM_BPM) {         /* BPMs from the other terms, outside the asked range, apart */
        val[0] = (uint16_t)rh_term_draw_bpm(seed, q->ask);
        for (tries = 0; got < 4 && tries < 400; tries++) {
            int j = item[rnd(seed) % (uint32_t)n], v = rh_term_draw_bpm(seed, j), ok = !rh_term_has(q->ask, v);
            for (k = 0; k < got && ok; k++) ok = v - val[k] >= 8 || val[k] - v >= 8;
            if (ok) opt[got] = (uint8_t)j, val[got++] = (uint16_t)v;
        }
        for (k = 16; got < 4 && k <= 240; k += 8) {   /* (never seen) */
            int ok = !rh_term_has(q->ask, k), x;
            for (x = 0; x < got && ok; x++) ok = k - val[x] >= 8 || val[x] - k >= 8;
            if (ok) opt[got] = 0, val[got++] = (uint16_t)k;
        }
    } else if (type == RH_Q_BPM_TERM || type == RH_Q_HEAR) {   /* terms whose range does not hold the BPM */
        q->bpm = (uint16_t)rh_term_draw_bpm(seed, q->ask);
        for (tries = 0; got < 4 && tries < 400; tries++) {
            int j = item[rnd(seed) % (uint32_t)n], ok = !rh_term_has(j, q->bpm);
            for (k = 0; k < got && ok; k++) ok = opt[k] != j;
            if (ok) opt[got++] = (uint8_t)j;
        }
    } else if (type == RH_Q_CHANGE) {    /* another kind of change */
        q->dir = (uint8_t)(rnd(seed) & 1u);
        for (tries = 0; got < 4 && tries < 400; tries++) {
            int j = item[rnd(seed) % (uint32_t)n], ok = RH_CHANGES[j].kind != RH_CHANGES[q->ask].kind;
            for (k = 0; k < got && ok; k++) ok = opt[k] != j;
            if (ok) opt[got++] = (uint8_t)j;
        }
    } else {                             /* METER: the others */
        q->bpm = (uint16_t)(80 + rnd(seed) % 41u);
        for (tries = 0; got < 4 && tries < 400; tries++) {
            int j = item[rnd(seed) % (uint32_t)n], ok = 1;
            for (k = 0; k < got && ok; k++) ok = opt[k] != j;
            if (ok) opt[got++] = (uint8_t)j;
        }
    }
    while (got < 4) opt[got] = opt[got - 1], val[got] = val[got - 1], got++;   /* (never seen: tables too small) */
    q->answer = (uint8_t)(rnd(seed) % 4u);
    for (i = 0, k = 1; i < 4; i++) {
        int from = i == q->answer ? 0 : k++;
        q->opt[i] = opt[from];
        q->val[i] = val[from];
    }
}

void rh_meter_seq(int meter, int bpm, int level, int bars, rh_seq_t *q)
{
    const rh_meter_t *m = &RH_METERS[meter];
    float e = 30000000.0f / (float)bpm, t = LEAD_US;
    int b, g, k;
    q->n = 0;
    for (b = 0; b < bars; b++)
        for (g = 0; g < m->ngroups; g++)
            for (k = 0; k < m->group[g]; k++, t += e) {
                uint8_t ev = RH_S_SUB;
                if (!g && !k) ev = level >= 3 ? (RH_S_CLICK | RH_S_LOW) : (RH_S_ACCENT | RH_S_LOW);
                else if (!k) ev = RH_S_CLICK;
                seq_add(q, t, ev);
            }
}
