/* SPDX-License-Identifier: GPL-3.0-or-later */
/* stats: solve times the WCA way. A single shows truncated to 0.01 s; an average drops the best and the worst
 * (5 % of each end from 50 solves up, at least one), counts a DNF as the worst, and is a DNF when more DNFs than it
 * drops. Times in ms; ST_DNF a did-not-finish, ST_NONE not enough solves. Platform-free (tests/cube_test.c). */
#include "stats.h"

int32_t st_with_penalty(int32_t ms, int pen)
{
    if (pen == ST_DNF_PEN) return ST_DNF;
    if (pen == ST_PLUS2) return ms + 2000;
    return ms;
}

int32_t st_mean(const int32_t *t, int n)
{
    int i;
    int32_t s = 0;
    if (n <= 0) return ST_NONE;
    for (i = 0; i < n; i++) {
        if (t[i] == ST_DNF) return ST_DNF;
        s += t[i];
    }
    return (s + n / 2) / n;
}

int32_t st_average(const int32_t *t, int n)
{
    int32_t v[ST_AVG_MAX], x;
    int i, j, trim, dnf = 0;
    uint32_t s = 0;                      /* (no 64-bit division on the device; 100 times under 5 h fit) */
    if (n < 5 || n > ST_AVG_MAX) return ST_NONE;
    trim = n < 50 ? 1 : (n * 5 + 99) / 100;
    for (i = 0; i < n; i++) {            /* insertion sort, DNF last */
        x = t[i];
        if (x == ST_DNF) dnf++;
        for (j = i; j > 0 && v[j - 1] > x; j--) v[j] = v[j - 1];
        v[j] = x;
    }
    if (dnf > trim) return ST_DNF;
    for (i = trim; i < n - trim; i++) s += (uint32_t)v[i];
    return (int32_t)((s + (uint32_t)(n - 2 * trim) / 2u) / (uint32_t)(n - 2 * trim));
}

int32_t st_best(const int32_t *t, int n)
{
    int i;
    int32_t b = ST_NONE;
    for (i = 0; i < n; i++)
        if (t[i] != ST_DNF && (b == ST_NONE || t[i] < b)) b = t[i];
    return b == ST_NONE && n ? ST_DNF : b;
}

void st_format(int32_t ms, char *o)
{
    int32_t cs, s, m;
    int k = 0;
    if (ms == ST_DNF) {
        o[0] = 'D', o[1] = 'N', o[2] = 'F', o[3] = 0;
        return;
    }
    if (ms < 0) {
        o[0] = '-', o[1] = 0;
        return;
    }
    cs = ms / 10;
    m = cs / 6000;
    s = cs / 100 % 60;
    cs %= 100;
    if (m) {
        if (m >= 10) o[k++] = (char)('0' + m / 10 % 10);
        o[k++] = (char)('0' + m % 10);
        o[k++] = ':';
        o[k++] = (char)('0' + s / 10);
    } else if (s >= 10)
        o[k++] = (char)('0' + s / 10);
    o[k++] = (char)('0' + s % 10);
    o[k++] = '.';
    o[k++] = (char)('0' + cs / 10);
    o[k++] = (char)('0' + cs % 10);
    o[k] = 0;
}

int32_t st_round_cs(int32_t ms) { return ms == ST_DNF || ms < 0 ? ms : (ms + 5) / 10 * 10; }
