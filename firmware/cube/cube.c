/* SPDX-License-Identifier: GPL-3.0-or-later */
/* cube: see cube.h. Every move is a sticker permutation built once from the cube's geometry: a layer of stickers
 * turns a quarter about an axis, and each sticker lands where its rotated position and normal say. */
#include "cube.h"

/* each base: axis (0 x, 1 y, 2 z), sign of its quarter turn about +axis (+1 counter-clockwise from the tip), and
 * the layers it takes (bit 0: coordinate -1, bit 1: 0, bit 2: +1). Clockwise looking at a face is -1 about its
 * outward axis */
static const int8_t MB_AXIS[MB_N] = {1, 0, 2, 1, 0, 2, 0, 1, 2, 1, 0, 2, 1, 0, 2, 0, 1, 2};
static const int8_t MB_SIGN[MB_N] = {-1, -1, -1, 1, 1, 1, 1, 1, -1, -1, -1, -1, 1, 1, 1, -1, -1, -1};
static const uint8_t MB_LAYERS[MB_N] = {4, 4, 4, 1, 1, 1, 2, 2, 2, 6, 6, 6, 3, 3, 3, 7, 7, 7};
static const char MB_CHAR[MB_N] = {'U', 'R', 'F', 'D', 'L', 'B', 'M', 'E', 'S', 'u', 'r', 'f', 'd', 'l', 'b',
                                   'x', 'y', 'z'};

static uint8_t qperm[MB_N][54];          /* a quarter turn of each base: new[i] = old[qperm[b][i]] */
static uint8_t built;

void cube_sticker_pos(int i, int8_t *p, int8_t *n)
{
    int f = i / 9, r = i % 9 / 3, c = i % 3;
    n[0] = n[1] = n[2] = 0;
    switch (f) {
    case CF_U: p[0] = (int8_t)(c - 1), p[1] = 1, p[2] = (int8_t)(r - 1), n[1] = 1; break;
    case CF_R: p[0] = 1, p[1] = (int8_t)(1 - r), p[2] = (int8_t)(1 - c), n[0] = 1; break;
    case CF_F: p[0] = (int8_t)(c - 1), p[1] = (int8_t)(1 - r), p[2] = 1, n[2] = 1; break;
    case CF_D: p[0] = (int8_t)(c - 1), p[1] = -1, p[2] = (int8_t)(1 - r), n[1] = -1; break;
    case CF_L: p[0] = -1, p[1] = (int8_t)(1 - r), p[2] = (int8_t)(c - 1), n[0] = -1; break;
    default: p[0] = (int8_t)(1 - c), p[1] = (int8_t)(1 - r), p[2] = -1, n[2] = -1; break;
    }
}

int cube_sticker_at(const int8_t *p, const int8_t *n)
{
    int f, r, c;
    if (n[1] == 1) f = CF_U, c = p[0] + 1, r = p[2] + 1;
    else if (n[0] == 1) f = CF_R, r = 1 - p[1], c = 1 - p[2];
    else if (n[2] == 1) f = CF_F, c = p[0] + 1, r = 1 - p[1];
    else if (n[1] == -1) f = CF_D, c = p[0] + 1, r = 1 - p[2];
    else if (n[0] == -1) f = CF_L, r = 1 - p[1], c = p[2] + 1;
    else if (n[2] == -1) f = CF_B, c = 1 - p[0], r = 1 - p[1];
    else return -1;
    if (r < 0 || r > 2 || c < 0 || c > 2) return -1;
    return f * 9 + r * 3 + c;
}

static void rot(int8_t *v, int axis, int sign)   /* a quarter turn of v about axis */
{
    int8_t a, b;
    if (axis == 0) {
        a = v[1], b = v[2];
        v[1] = (int8_t)(sign > 0 ? -b : b), v[2] = (int8_t)(sign > 0 ? a : -a);
    } else if (axis == 1) {
        a = v[0], b = v[2];
        v[0] = (int8_t)(sign > 0 ? b : -b), v[2] = (int8_t)(sign > 0 ? -a : a);
    } else {
        a = v[0], b = v[1];
        v[0] = (int8_t)(sign > 0 ? -b : b), v[1] = (int8_t)(sign > 0 ? a : -a);
    }
}

uint64_t cube_move_layer(int b, int *axis, int *sign)
{
    uint64_t m = 0;
    int i;
    int8_t p[3], n[3];
    for (i = 0; i < 54; i++) {
        cube_sticker_pos(i, p, n);
        if (MB_LAYERS[b] >> (p[MB_AXIS[b]] + 1) & 1) m |= (uint64_t)1 << i;
    }
    if (axis) *axis = MB_AXIS[b];
    if (sign) *sign = MB_SIGN[b];
    return m;
}

static void build(void)
{
    int b, i;
    int8_t p[3], n[3];
    for (b = 0; b < MB_N; b++) {
        for (i = 0; i < 54; i++) qperm[b][i] = (uint8_t)i;
        for (i = 0; i < 54; i++) {
            int j;
            cube_sticker_pos(i, p, n);
            if (!(MB_LAYERS[b] >> (p[MB_AXIS[b]] + 1) & 1)) continue;
            rot(p, MB_AXIS[b], MB_SIGN[b]);
            rot(n, MB_AXIS[b], MB_SIGN[b]);
            j = cube_sticker_at(p, n);
            qperm[b][j] = (uint8_t)i;    /* the sticker at i goes to j */
        }
    }
    built = 1;
}

void cube_reset(cube_t *c)
{
    int i;
    for (i = 0; i < 54; i++) c->f[i] = (uint8_t)(i / 9);
}

void cube_move(cube_t *c, int mv)
{
    int t, i, b;
    uint8_t o[54];
    if (mv < 0 || mv >= MV_N) return;
    if (!built) build();
    b = MV_BASE(mv);
    for (t = MV_TURNS(mv); t > 0; t--) {
        for (i = 0; i < 54; i++) o[i] = c->f[i];
        for (i = 0; i < 54; i++) c->f[i] = o[qperm[b][i]];
    }
}

void cube_apply(cube_t *c, const uint8_t *mv, int n)
{
    int i;
    for (i = 0; i < n; i++) cube_move(c, mv[i]);
}

int cube_equal(const cube_t *a, const cube_t *b)
{
    int i;
    for (i = 0; i < 54; i++)
        if (a->f[i] != b->f[i]) return 0;
    return 1;
}

/* ------------------------------------------------------------------ notation --- */
static int base_of(char ch, int wide)
{
    int b;
    for (b = 0; b < MB_N; b++)
        if (MB_CHAR[b] == ch) break;
    if (b == MB_N) return -1;
    if (wide) {
        if (b > MB_B) return -1;         /* Mw, xw: no */
        b = (int[]){MB_u, MB_r, MB_f, MB_d, MB_l, MB_b}[b];
    }
    return b;
}

int cube_parse(const char *s, uint8_t *out, int max)
{
    int n = 0;
    while (*s) {
        int b, t = 1, wide = 0;
        char ch = *s;
        if (ch == ' ' || ch == ',' || ch == '(' || ch == ')' || ch == '[' || ch == ']' || ch == '\t' || ch == '\n') {
            s++;
            continue;
        }
        s++;
        if (*s == 'w') wide = 1, s++;
        b = base_of(ch, wide);
        if (b < 0) return -1;
        if (*s >= '1' && *s <= '9') t = (*s++ - '0') % 4;
        if (*s == '\'') t = (4 - t) % 4, s++;
        if (*s == '\'' ) s++;            /* R2'' and the like: as written once */
        if (t == 0) continue;            /* R4 */
        if (n >= max) return -1;
        out[n++] = MV(b, t);
    }
    return n;
}

int cube_mv_name(int mv, char *out)
{
    int t = MV_TURNS(mv), n = 0;
    out[n++] = MB_CHAR[MV_BASE(mv)];
    if (t == 2) out[n++] = '2';
    else if (t == 3) out[n++] = '\'';
    out[n] = 0;
    return n;
}

int cube_print(const uint8_t *mv, int n, char *out, int max)
{
    int i, k = 0;
    char t[4];
    out[0] = 0;
    for (i = 0; i < n; i++) {
        int l = cube_mv_name(mv[i], t), j;
        if (k + l + (i > 0) >= max) break;
        if (i > 0) out[k++] = ' ';
        for (j = 0; j < l; j++) out[k++] = t[j];
    }
    out[k] = 0;
    return k;
}

void cube_invert(const uint8_t *in, int n, uint8_t *out)
{
    int i;
    for (i = 0; i < (n + 1) / 2; i++) {
        uint8_t a = in[i], b = in[n - 1 - i];
        out[i] = MV_INV(b);
        out[n - 1 - i] = MV_INV(a);
    }
}

int cube_merge(uint8_t *mv, int n)
{
    int i, k = 0;
    for (i = 0; i < n; i++) {
        if (k && MV_BASE(mv[k - 1]) == MV_BASE(mv[i])) {
            int t = (MV_TURNS(mv[k - 1]) + MV_TURNS(mv[i])) % 4;
            if (t) mv[k - 1] = MV(MV_BASE(mv[i]), t);
            else k--;
        } else
            mv[k++] = mv[i];
    }
    return k;
}

/* -------------------------------------------------------------------- checks --- */
int cube_solved(const cube_t *c)
{
    int f, i;
    for (f = 0; f < 6; f++)
        for (i = 0; i < 9; i++)
            if (c->f[f * 9 + i] != c->f[f * 9 + 4]) return 0;
    return 1;
}

int cube_solved_turn(const cube_t *c)
{
    int m;
    if (cube_solved(c)) return 1;
    for (m = 0; m < 18; m++) {
        cube_t t = *c;
        cube_move(&t, m);
        if (cube_solved(&t)) return 1;
    }
    return 0;
}

int cube_f2l_solved(const cube_t *c)
{
    int i, f;
    for (i = 0; i < 54; i++)
        if (i / 9 != CF_U && i % 9 / 3 == 1 && c->f[i] != i / 9) return 0;   /* middle rows and centres (U's too) */
    if (c->f[4] != CF_U) return 0;
    for (i = 0; i < 9; i++)
        if (c->f[CF_D * 9 + i] != CF_D) return 0;
    for (f = 0; f < 6; f++) {
        if (f == CF_U || f == CF_D) continue;
        for (i = 6; i < 9; i++)
            if (c->f[f * 9 + i] != f) return 0;
    }
    return 1;
}

static const char FACE_CH[6] = {'U', 'R', 'F', 'D', 'L', 'B'};

void cube_facelets(const cube_t *c, char *out)
{
    int i;
    for (i = 0; i < 54; i++) out[i] = FACE_CH[c->f[i] % 6];
    out[54] = 0;
}

int cube_from_facelets(cube_t *c, const char *s)
{
    int i, f;
    for (i = 0; i < 54; i++) {
        for (f = 0; f < 6 && FACE_CH[f] != s[i]; f++) {}
        if (f == 6) return 0;
        c->f[i] = (uint8_t)f;
    }
    return 1;
}

/* ------------------------------------------------------------------ scramble --- */
uint32_t cube_rand(uint32_t *x)
{
    uint32_t v = *x ? *x : 0x9E3779B9u;
    v ^= v << 13;
    v ^= v >> 17;
    v ^= v << 5;
    *x = v;
    return v;
}

int cube_scramble_moves(uint8_t *out, int n, uint32_t *rng)
{
    int i = 0, last = -1, prev = -1;
    while (i < n) {
        int f = (int)(cube_rand(rng) % 6u);
        if (f == last) continue;
        if (last >= 0 && prev >= 0 && f % 3 == last % 3 && f % 3 == prev % 3) continue;   /* R L R */
        out[i++] = MV(f, (int)(cube_rand(rng) % 3u) + 1);
        prev = last;
        last = f;
    }
    return n;
}
