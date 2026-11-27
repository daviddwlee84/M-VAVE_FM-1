/* SPDX-License-Identifier: GPL-3.0-or-later */
/* solve: see solve.h.
 *
 * Cubies and coordinates are Kociemba's (corners URF UFL ULB UBR DFR DLF DBL DRB, edges UR UF UL UB DR DF DL DB FR FL
 * BL BR; a cube is "where each piece is, how it is turned"; a * b is a then b). A symmetry s is the cube S_s =
 * F2^a U4^b LR2^c (s = 8a + 2b + c); conj(c, s) = S_s c S_s^-1. A symmetry coordinate stores a class and a symmetry:
 * raw = conj(rep, sym), rep the class's smallest raw value.
 *
 * Twist is reduced by the 8 rotations only (c = 0): their cubes turn no corner, so conj moves the twist's digits
 * around and nothing else. The permutations are reduced by all 16 (a mirror changes no permutation's meaning). */
#include "solve.h"

enum { URF, UFL, ULB, UBR, DFR, DLF, DBL, DRB };
enum { UR, UF, UL, UB, DR, DF, DL, DB, FR, FL, BL, BR };

#define N_TWIST 2187
#define N_FLIP 2048
#define N_SLICE 495
#define N_PERM 40320
#define N_MPERM 24
#define N_TWIST_SYM 291
#define N_PERM_SYM 2768
#define N_P2 10
#define N_SLICE_SYM 45            /* slice classes under the 16 symmetries (solve_init checks) */

/* the 6 faces' quarter turns, clockwise (Kociemba's cubie tables) */
static const cc_t FACE_CC[6] = {
    {{UBR, URF, UFL, ULB, DFR, DLF, DBL, DRB}, {0, 0, 0, 0, 0, 0, 0, 0},
     {UB, UR, UF, UL, DR, DF, DL, DB, FR, FL, BL, BR}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},   /* U */
    {{DFR, UFL, ULB, URF, DRB, DLF, DBL, UBR}, {2, 0, 0, 1, 1, 0, 0, 2},
     {FR, UF, UL, UB, BR, DF, DL, DB, DR, FL, BL, UR}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},   /* R */
    {{UFL, DLF, ULB, UBR, URF, DFR, DBL, DRB}, {1, 2, 0, 0, 2, 1, 0, 0},
     {UR, FL, UL, UB, DR, FR, DL, DB, UF, DF, BL, BR}, {0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0}},   /* F */
    {{URF, UFL, ULB, UBR, DLF, DBL, DRB, DFR}, {0, 0, 0, 0, 0, 0, 0, 0},
     {UR, UF, UL, UB, DF, DL, DB, DR, FR, FL, BL, BR}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},   /* D */
    {{URF, ULB, DBL, UBR, DFR, UFL, DLF, DRB}, {0, 1, 2, 0, 0, 2, 1, 0},
     {UR, UF, BL, UB, DR, DF, FL, DB, FR, UL, DL, BR}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},   /* L */
    {{URF, UFL, UBR, DRB, DFR, DLF, ULB, DBL}, {0, 0, 1, 2, 0, 0, 2, 1},
     {UR, UF, UL, BR, DR, DF, DL, BL, FR, FL, UB, DB}, {0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1}},   /* B */
};
/* the symmetry generators: 180 degrees about F-B, 90 degrees about U-D, the left-right mirror (its corner turns are
 * not used: mirrors only ever act on permutations here) */
static const cc_t S_F2 = {{DLF, DFR, DRB, DBL, UFL, URF, UBR, ULB}, {0, 0, 0, 0, 0, 0, 0, 0},
                          {DL, DF, DR, DB, UL, UF, UR, UB, FL, FR, BR, BL}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
static const cc_t S_U4 = {{UBR, URF, UFL, ULB, DRB, DFR, DLF, DBL}, {0, 0, 0, 0, 0, 0, 0, 0},
                          {UB, UR, UF, UL, DB, DR, DF, DL, BR, FR, FL, BL}, {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1}};
static const cc_t S_LR2 = {{UFL, URF, UBR, ULB, DLF, DFR, DRB, DBL}, {0, 0, 0, 0, 0, 0, 0, 0},
                           {UL, UF, UR, UB, DL, DF, DR, DB, FL, FR, BR, BL}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

/* 120 degrees about the URF-DBL diagonal: U -> R -> F (the search also runs the cube seen along R-L and F-B) */
static const cc_t S_URF3 = {{URF, DFR, DLF, UFL, UBR, DRB, DBL, ULB}, {1, 2, 1, 2, 2, 1, 2, 1},
                            {UF, FR, DF, FL, UB, BR, DB, BL, UR, DR, DL, UL}, {1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1}};

/* phase 2's moves: U U2 U' D D2 D' R2 L2 F2 B2, as face move ids */
static const uint8_t P2_MOVE[N_P2] = {0, 1, 2, 9, 10, 11, 4, 13, 7, 16};

static cc_t mv_cc[18], sym_cc[16];
static uint8_t sym_inv[16], sym_mult[16][16], sym_move[16][18];
static int8_t p2_index[18];
static uint16_t fc_chunk[16][3][16];     /* flip bits moved as S_s^-1 moves positions */
static uint16_t fc_xor[16];              /* S_s^-1's own flips */
static uint8_t fc_e[16];                 /* S_s flips the middle-layer pieces */
static uint8_t small_built;
static cc_t urf_cc[3], urf_inv_cc[3];    /* URF3^a and its inverse */
static uint8_t urf_move[3][18];          /* a move of the cube seen along axis a, as a move of the cube itself */

/* ------------------------------------------------------------------- cubies --- */
void cc_ident(cc_t *c)
{
    int i;
    for (i = 0; i < 8; i++) c->cp[i] = (uint8_t)i, c->co[i] = 0;
    for (i = 0; i < 12; i++) c->ep[i] = (uint8_t)i, c->eo[i] = 0;
}

static void cc_mul(const cc_t *a, const cc_t *b, cc_t *o)   /* o = a * b (o is not a or b) */
{
    int i;
    for (i = 0; i < 8; i++) {
        o->cp[i] = a->cp[b->cp[i]];
        o->co[i] = (uint8_t)((a->co[b->cp[i]] + b->co[i]) % 3);
    }
    for (i = 0; i < 12; i++) {
        o->ep[i] = a->ep[b->ep[i]];
        o->eo[i] = (uint8_t)(a->eo[b->ep[i]] ^ b->eo[i]);
    }
}

static void cc_inv(const cc_t *c, cc_t *o)
{
    int i;
    for (i = 0; i < 8; i++) {
        o->cp[c->cp[i]] = (uint8_t)i;
        o->co[c->cp[i]] = (uint8_t)((3 - c->co[i]) % 3);
    }
    for (i = 0; i < 12; i++) {
        o->ep[c->ep[i]] = (uint8_t)i;
        o->eo[c->ep[i]] = c->eo[i];
    }
}

static int cc_same_perm(const cc_t *a, const cc_t *b)
{
    int i;
    for (i = 0; i < 8; i++)
        if (a->cp[i] != b->cp[i]) return 0;
    for (i = 0; i < 12; i++)
        if (a->ep[i] != b->ep[i]) return 0;
    return 1;
}

static int cc_same(const cc_t *a, const cc_t *b)
{
    int i;
    if (!cc_same_perm(a, b)) return 0;
    for (i = 0; i < 8; i++)
        if (a->co[i] != b->co[i]) return 0;
    for (i = 0; i < 12; i++)
        if (a->eo[i] != b->eo[i]) return 0;
    return 1;
}

static void conj(const cc_t *c, int s, cc_t *o)   /* S_s c S_s^-1 (orientations right for the rotations only) */
{
    cc_t t;
    cc_mul(&sym_cc[s], c, &t);
    cc_mul(&t, &sym_cc[sym_inv[s]], o);
}

void cc_move(cc_t *c, int m)
{
    cc_t t;
    cc_mul(c, &mv_cc[m], &t);
    *c = t;
}

static int parity(const uint8_t *p, int n)
{
    int i, j, s = 0;
    for (i = 0; i < n; i++)
        for (j = i + 1; j < n; j++)
            if (p[j] < p[i]) s ^= 1;
    return s;
}

int cc_valid(const cc_t *c)
{
    int i, s = 0, seen = 0;
    for (i = 0; i < 8; i++) {
        if (c->cp[i] > 7 || c->co[i] > 2) return 0;
        seen |= 1 << c->cp[i];
        s += c->co[i];
    }
    if (seen != 0xFF || s % 3) return 0;
    for (i = 0, s = 0, seen = 0; i < 12; i++) {
        if (c->ep[i] > 11 || c->eo[i] > 1) return 0;
        seen |= 1 << c->ep[i];
        s += c->eo[i];
    }
    if (seen != 0xFFF || s % 2) return 0;
    return parity(c->cp, 8) == parity(c->ep, 12);
}

/* --------------------------------------------------------------- facelets --- */
static const uint8_t CORNER_FL[8][3] = {{8, 9, 20}, {6, 18, 38}, {0, 36, 47}, {2, 45, 11},
                                        {29, 26, 15}, {27, 44, 24}, {33, 53, 42}, {35, 17, 51}};
static const uint8_t EDGE_FL[12][2] = {{5, 10}, {7, 19}, {3, 37}, {1, 46}, {32, 16}, {28, 25},
                                       {30, 43}, {34, 52}, {23, 12}, {21, 41}, {50, 39}, {48, 14}};
/* faces U R F D L B = 0..5 */
static const uint8_t CORNER_COL[8][3] = {{0, 1, 2}, {0, 2, 4}, {0, 4, 5}, {0, 5, 1},
                                         {3, 2, 1}, {3, 4, 2}, {3, 5, 4}, {3, 1, 5}};
static const uint8_t EDGE_COL[12][2] = {{0, 1}, {0, 2}, {0, 4}, {0, 5}, {3, 1}, {3, 2},
                                        {3, 4}, {3, 5}, {2, 1}, {2, 4}, {5, 4}, {5, 1}};
static const char CC_FACE_CH[6] = {'U', 'R', 'F', 'D', 'L', 'B'};

static int cc_face_of(char ch)
{
    int f;
    for (f = 0; f < 6; f++)
        if (CC_FACE_CH[f] == ch) return f;
    return -1;
}

void cc_facelets(const cc_t *c, char *out)
{
    int i, n;
    for (i = 0; i < 6; i++) out[i * 9 + 4] = CC_FACE_CH[i];
    for (i = 0; i < 8; i++)
        for (n = 0; n < 3; n++) out[CORNER_FL[i][(n + c->co[i]) % 3]] = CC_FACE_CH[CORNER_COL[c->cp[i]][n]];
    for (i = 0; i < 12; i++)
        for (n = 0; n < 2; n++) out[EDGE_FL[i][(n + c->eo[i]) % 2]] = CC_FACE_CH[EDGE_COL[c->ep[i]][n]];
    out[54] = 0;
}

int cc_from_facelets(cc_t *c, const char *s)
{
    int i, j, o, f[54];
    for (i = 0; i < 54; i++)
        if ((f[i] = cc_face_of(s[i])) < 0) return 0;
    for (i = 0; i < 6; i++)
        if (f[i * 9 + 4] != i) return 0;
    for (i = 0; i < 8; i++) {
        for (o = 0; o < 3; o++)
            if (f[CORNER_FL[i][o]] == 0 || f[CORNER_FL[i][o]] == 3) break;
        if (o == 3) return 0;
        for (j = 0; j < 8; j++)
            if (f[CORNER_FL[i][(o + 1) % 3]] == CORNER_COL[j][1] && f[CORNER_FL[i][(o + 2) % 3]] == CORNER_COL[j][2])
                break;
        if (j == 8) return 0;
        c->cp[i] = (uint8_t)j;
        c->co[i] = (uint8_t)o;
    }
    for (i = 0; i < 12; i++) {
        for (j = 0; j < 12; j++) {
            if (f[EDGE_FL[i][0]] == EDGE_COL[j][0] && f[EDGE_FL[i][1]] == EDGE_COL[j][1]) {
                c->ep[i] = (uint8_t)j, c->eo[i] = 0;
                break;
            }
            if (f[EDGE_FL[i][0]] == EDGE_COL[j][1] && f[EDGE_FL[i][1]] == EDGE_COL[j][0]) {
                c->ep[i] = (uint8_t)j, c->eo[i] = 1;
                break;
            }
        }
        if (j == 12) return 0;
    }
    return cc_valid(c);
}

/* ------------------------------------------------------------- coordinates --- */
static int cnk(int n, int k)
{
    int i, r = 1;
    if (n < k) return 0;
    if (k > n / 2) k = n - k;
    for (i = 1; i <= k; i++) r = r * (n - i + 1) / i;
    return r;
}

static int get_twist(const cc_t *c)
{
    int i, t = 0;
    for (i = 0; i < 7; i++) t = t * 3 + c->co[i];
    return t;
}

static void set_twist(cc_t *c, int t)
{
    int i, s = 0;
    for (i = 6; i >= 0; i--) {
        c->co[i] = (uint8_t)(t % 3);
        s += c->co[i];
        t /= 3;
    }
    c->co[7] = (uint8_t)((3 - s % 3) % 3);
}

static int get_flip12(const cc_t *c)     /* bit i: edge i flipped */
{
    int i, f = 0;
    for (i = 0; i < 12; i++) f |= c->eo[i] << i;
    return f;
}

static int get_slice(const cc_t *c)      /* where FR FL BL BR are: 0 = in the middle layer */
{
    int j, a = 0, x = 0;
    for (j = 11; j >= 0; j--)
        if (c->ep[j] >= FR) a += cnk(11 - j, x + 1), x++;
    return a;
}

static void set_slice(cc_t *c, int a)
{
    int j, x = 3, o = 0;
    for (j = 0; j < 12; j++) c->ep[j] = 0xFF;
    for (j = 0; j < 12; j++)
        if (x >= 0 && a - cnk(11 - j, x + 1) >= 0) {
            c->ep[j] = (uint8_t)(FR + 3 - x);
            a -= cnk(11 - j, x + 1);
            x--;
        }
    for (j = 0; j < 12; j++)
        if (c->ep[j] == 0xFF) c->ep[j] = (uint8_t)o++;
}

static int rank_perm(const uint8_t *p, int n)
{
    int i, j, r = 0;
    for (i = 0; i < n; i++) {
        int c = 0;
        for (j = i + 1; j < n; j++)
            if (p[j] < p[i]) c++;
        r = r * (n - i) + c;
    }
    return r;
}

static void unrank_perm(uint8_t *p, int n, int r)
{
    int i, j, d[12];
    uint8_t avail[12];
    for (i = n - 1; i >= 0; i--) {
        d[i] = r % (n - i);
        r /= n - i;
    }
    for (i = 0; i < n; i++) avail[i] = (uint8_t)i;
    for (i = 0; i < n; i++) {
        p[i] = avail[d[i]];
        for (j = d[i]; j < n - 1 - i; j++) avail[j] = avail[j + 1];
    }
}

static int get_cperm(const cc_t *c) { return rank_perm(c->cp, 8); }
static int get_eperm(const cc_t *c) { return rank_perm(c->ep, 8); }
static int get_mperm(const cc_t *c)
{
    uint8_t m[4];
    int i;
    for (i = 0; i < 4; i++) m[i] = (uint8_t)(c->ep[8 + i] - FR);
    return rank_perm(m, 4);
}

static void set_cperm(cc_t *c, int r) { unrank_perm(c->cp, 8, r); }
static void set_eperm(cc_t *c, int r)
{
    int i;
    unrank_perm(c->ep, 8, r);
    for (i = 8; i < 12; i++) c->ep[i] = (uint8_t)i;
}
static void set_mperm(cc_t *c, int r)
{
    int i;
    unrank_perm(c->ep + 8, 4, r);
    for (i = 8; i < 12; i++) c->ep[i] = (uint8_t)(c->ep[i] + FR);
    for (i = 0; i < 8; i++) c->ep[i] = (uint8_t)i;
}

/* ---------------------------------------------------------------- symmetry --- */
/* the flip of conj(c, s), from c's flip f (12 bits) and where its middle-layer pieces are (e) */
static int flip_conj(int f, int e, int s)
{
    if (fc_e[s]) f ^= e;
    return (fc_chunk[s][0][f & 15] | fc_chunk[s][1][(f >> 4) & 15] | fc_chunk[s][2][(f >> 8) & 15]) ^ fc_xor[s];
}
static int flip_full(int f11)            /* the 12th bit from the parity of the other 11 */
{
    int p = f11;
    p ^= p >> 8;
    p ^= p >> 4;
    p ^= p >> 2;
    p ^= p >> 1;
    return f11 | (p & 1) << 11;
}
static void build_small(void)
{
    int s, t, m, f, i;
    cc_t c, d;
    for (f = 0; f < 6; f++) {
        cc_t a = FACE_CC[f];
        mv_cc[f * 3] = a;
        cc_mul(&a, &FACE_CC[f], &mv_cc[f * 3 + 1]);
        cc_mul(&mv_cc[f * 3 + 1], &FACE_CC[f], &mv_cc[f * 3 + 2]);
    }
    for (s = 0; s < 16; s++) {
        int a = s >> 3, b = (s >> 1) & 3, l = s & 1;
        cc_ident(&c);
        if (a) cc_mul(&c, &S_F2, &d), c = d;
        for (i = 0; i < b; i++) cc_mul(&c, &S_U4, &d), c = d;
        if (l) cc_mul(&c, &S_LR2, &d), c = d;
        sym_cc[s] = c;
    }
    for (s = 0; s < 16; s++)
        for (t = 0; t < 16; t++) {
            int u;
            cc_mul(&sym_cc[s], &sym_cc[t], &c);
            for (u = 0; u < 16 && !cc_same_perm(&c, &sym_cc[u]); u++) {}
            sym_mult[s][t] = (uint8_t)u;
            if (u == 0) sym_inv[s] = (uint8_t)t;
        }
    for (s = 0; s < 16; s++)
        for (m = 0; m < 18; m++) {
            int u;
            conj(&mv_cc[m], s, &c);
            for (u = 0; u < 18; u++)
                if ((s & 1) ? cc_same_perm(&c, &mv_cc[u]) : cc_same(&c, &mv_cc[u])) break;
            sym_move[s][m] = (uint8_t)u;   /* 18: not a move (a bug; solve_init reports it) */
        }
    for (s = 0; s < 16; s++) {
        const cc_t *si = &sym_cc[sym_inv[s]];
        int k, v;
        fc_e[s] = sym_cc[s].eo[FR];
        fc_xor[s] = (uint16_t)get_flip12(si);
        for (k = 0; k < 3; k++)
            for (v = 0; v < 16; v++) {
                uint16_t o = 0;
                for (i = 0; i < 12; i++) {
                    int src = si->ep[i];
                    if (src / 4 == k && (v >> (src % 4) & 1)) o |= (uint16_t)(1 << i);
                }
                fc_chunk[s][k][v] = o;
            }
    }
    for (m = 0; m < 18; m++) p2_index[m] = -1;
    for (i = 0; i < N_P2; i++) p2_index[P2_MOVE[i]] = (int8_t)i;
    cc_ident(&urf_cc[0]);
    cc_mul(&urf_cc[0], &S_URF3, &urf_cc[1]);
    cc_mul(&urf_cc[1], &S_URF3, &urf_cc[2]);
    for (i = 0; i < 3; i++) cc_inv(&urf_cc[i], &urf_inv_cc[i]);
    for (i = 0; i < 3; i++)
        for (m = 0; m < 18; m++) {
            int u;
            cc_mul(&urf_inv_cc[i], &mv_cc[m], &d);   /* S^-1 m S */
            cc_mul(&d, &urf_cc[i], &c);
            for (u = 0; u < 18 && !cc_same(&c, &mv_cc[u]); u++) {}
            urf_move[i][m] = (uint8_t)u;
        }
    small_built = 1;
}

/* ------------------------------------------------------------------ tables --- */
#ifdef SOLVE_GEN
static uint16_t TwistMove[N_TWIST_SYM][18];    /* class << 3 | rotation */
static uint16_t TwistR2S[N_TWIST];
static uint16_t SliceMove[N_SLICE][18];
static uint16_t SliceConj[N_SLICE][8];         /* by rotation */
static uint16_t FlipChunk[18][3][16];
static uint16_t FlipXor[18];
static uint8_t P1Prun[(N_TWIST_SYM * N_SLICE + 1) / 2];   /* nibbles: twist class x slice */
static uint16_t SliceR2S[N_SLICE];             /* class << 4 | symmetry */
static uint16_t SliceMask[N_SLICE];            /* the positions FR FL BL BR are in */
static uint8_t FSPrun[N_SLICE_SYM * N_FLIP / 2];   /* nibbles: slice class x flip (low 11 bits) */
static uint16_t CPermMove[N_PERM_SYM][N_P2];   /* class << 4 | symmetry */
static uint16_t EPermMove[N_PERM_SYM][N_P2];
static uint16_t CPermS2R[N_PERM_SYM];
static uint16_t EPermS2R[N_PERM_SYM];
static uint8_t MPermMove[N_MPERM][N_P2];
static uint8_t MPermConj[N_MPERM][16];
static uint8_t MCPrun[N_PERM_SYM * N_MPERM / 2];
static uint8_t MEPrun[N_PERM_SYM * N_MPERM / 2];
/* the generator's own */
static uint16_t SliceS2R[N_SLICE_SYM];
static uint16_t SliceSym[N_SLICE_SYM];
static int slice_classes;
static uint16_t TwistS2R[N_TWIST_SYM];
static uint8_t TwistSym[N_TWIST_SYM];          /* rotations that keep the class's representative */
static uint16_t CPermR2S[N_PERM], EPermR2S[N_PERM];
static uint16_t CPermSym[N_PERM_SYM], EPermSym[N_PERM_SYM];
static int twist_classes, perm_classes[2];
#else
#include "cube_tables.h"
#endif

static int nib(const uint8_t *t, int i) { return t[i >> 1] >> ((i & 1) << 2) & 15; }

#ifdef SOLVE_GEN
static void nib_set(uint8_t *t, int i, int v)
{
    t[i >> 1] = (uint8_t)((t[i >> 1] & ~(15 << ((i & 1) << 2))) | v << ((i & 1) << 2));
}

static void build_twist(void)
{
    int raw, s, m, n = 0;
    cc_t c, d;
    for (raw = 0; raw < N_TWIST; raw++) TwistR2S[raw] = 0xFFFF;
    for (raw = 0; raw < N_TWIST; raw++) {
        if (TwistR2S[raw] != 0xFFFF) continue;
        cc_ident(&c);
        set_twist(&c, raw);
        if (n < N_TWIST_SYM) TwistS2R[n] = (uint16_t)raw, TwistSym[n] = 0;
        for (s = 0; s < 16; s += 2) {
            int r;
            conj(&c, s, &d);
            r = get_twist(&d);
            if (n < N_TWIST_SYM) {
                if (TwistR2S[r] == 0xFFFF) TwistR2S[r] = (uint16_t)(n << 3 | s >> 1);
                if (r == raw) TwistSym[n] |= (uint8_t)(1 << (s >> 1));
            }
        }
        n++;
    }
    twist_classes = n;
    if (n != N_TWIST_SYM) return;
    for (n = 0; n < N_TWIST_SYM; n++)
        for (m = 0; m < 18; m++) {
            cc_ident(&c);
            set_twist(&c, TwistS2R[n]);
            cc_mul(&c, &mv_cc[m], &d);
            TwistMove[n][m] = TwistR2S[get_twist(&d)];
        }
}

static void build_slice_flip(void)
{
    int a, m, s, k, v, i;
    cc_t c, d;
    for (a = 0; a < N_SLICE; a++) {
        cc_ident(&c);
        set_slice(&c, a);
        for (m = 0; m < 18; m++) {
            cc_mul(&c, &mv_cc[m], &d);
            SliceMove[a][m] = (uint16_t)get_slice(&d);
        }
        for (s = 0; s < 16; s += 2) {
            conj(&c, s, &d);
            SliceConj[a][s >> 1] = (uint16_t)get_slice(&d);
        }
    }
    for (a = 0; a < N_SLICE; a++) SliceR2S[a] = 0xFFFF;
    for (a = 0, k = 0; a < N_SLICE; a++) {
        cc_ident(&c);
        set_slice(&c, a);
        for (i = 0, v = 0; i < 12; i++)
            if (c.ep[i] >= FR) v |= 1 << i;
        SliceMask[a] = (uint16_t)v;
        if (SliceR2S[a] != 0xFFFF) continue;
        if (k < N_SLICE_SYM) SliceS2R[k] = (uint16_t)a, SliceSym[k] = 0;
        for (s = 0; s < 16; s++) {
            int r;
            conj(&c, s, &d);
            r = get_slice(&d);
            if (k < N_SLICE_SYM) {
                if (SliceR2S[r] == 0xFFFF) SliceR2S[r] = (uint16_t)(k << 4 | s);
                if (r == a) SliceSym[k] |= (uint16_t)(1 << s);
            }
        }
        k++;
    }
    slice_classes = k;
    for (m = 0; m < 18; m++) {
        FlipXor[m] = (uint16_t)get_flip12(&mv_cc[m]);
        for (k = 0; k < 3; k++)
            for (v = 0; v < 16; v++) {
                uint16_t o = 0;
                for (i = 0; i < 12; i++) {
                    int src = mv_cc[m].ep[i];
                    if (src / 4 == k && (v >> (src % 4) & 1)) o |= (uint16_t)(1 << i);
                }
                FlipChunk[m][k][v] = o;
            }
    }
}
#endif

static int flip_move(int f, int m)
{
    return (FlipChunk[m][0][f & 15] | FlipChunk[m][1][(f >> 4) & 15] | FlipChunk[m][2][(f >> 8) & 15]) ^ FlipXor[m];
}

#ifdef SOLVE_GEN
static void build_p1_prune(void)
{
    int depth = 0, done = 1, i, m, r;
    for (i = 0; i < (int)sizeof P1Prun; i++) P1Prun[i] = 0xFF;
    nib_set(P1Prun, 0, 0);
    while (done < N_TWIST_SYM * N_SLICE) {
        for (i = 0; i < N_TWIST_SYM * N_SLICE; i++) {
            int cls = i / N_SLICE, sl = i % N_SLICE;
            if (nib(P1Prun, i) != depth) continue;
            for (m = 0; m < 18; m++) {
                int t = TwistMove[cls][m], cls2 = t >> 3, ri2 = t & 7;
                int sl2 = SliceConj[SliceMove[sl][m]][sym_inv[ri2 << 1] >> 1];
                int j = cls2 * N_SLICE + sl2;
                if (nib(P1Prun, j) != 15) continue;
                for (r = 0; r < 8; r++)
                    if (TwistSym[cls2] >> r & 1) {
                        int k = cls2 * N_SLICE + SliceConj[sl2][r];
                        if (nib(P1Prun, k) == 15) nib_set(P1Prun, k, depth + 1), done++;
                    }
            }
        }
        depth++;
    }
    for (i = 0; i < (int)sizeof FSPrun; i++) FSPrun[i] = 0xFF;
    nib_set(FSPrun, 0, 0);
    for (depth = 0, done = 1; done < N_SLICE_SYM * N_FLIP; depth++)
        for (i = 0; i < N_SLICE_SYM * N_FLIP; i++) {
            int cls = i / N_FLIP, f = flip_full(i % N_FLIP), rep = SliceS2R[cls];
            if (nib(FSPrun, i) != depth) continue;
            for (m = 0; m < 18; m++) {
                int sl2 = SliceMove[rep][m], f2 = flip_move(f, m), t = SliceR2S[sl2];
                int cls2 = t >> 4, f3 = flip_conj(f2, SliceMask[sl2], sym_inv[t & 15]);
                int rep2 = SliceS2R[cls2];
                if (nib(FSPrun, cls2 * N_FLIP + (f3 & 0x7FF)) != 15) continue;
                for (r = 0; r < 16; r++)
                    if (SliceSym[cls2] >> r & 1) {
                        int q = cls2 * N_FLIP + (flip_conj(f3, SliceMask[rep2], r) & 0x7FF);
                        if (nib(FSPrun, q) == 15) nib_set(FSPrun, q, depth + 1), done++;
                    }
            }
        }
}

static void build_perm(int corners)
{
    uint16_t *r2s = corners ? CPermR2S : EPermR2S, *s2r = corners ? CPermS2R : EPermS2R;
    uint16_t *symst = corners ? CPermSym : EPermSym;
    int raw, s, n = 0, j;
    cc_t c, d;
    for (raw = 0; raw < N_PERM; raw++) r2s[raw] = 0xFFFF;
    for (raw = 0; raw < N_PERM; raw++) {
        if (r2s[raw] != 0xFFFF) continue;
        cc_ident(&c);
        if (corners) set_cperm(&c, raw);
        else set_eperm(&c, raw);
        if (n < N_PERM_SYM) s2r[n] = (uint16_t)raw, symst[n] = 0;
        for (s = 0; s < 16; s++) {
            int r;
            conj(&c, s, &d);
            r = corners ? get_cperm(&d) : get_eperm(&d);
            if (n < N_PERM_SYM) {
                if (r2s[r] == 0xFFFF) r2s[r] = (uint16_t)(n << 4 | s);
                if (r == raw) symst[n] |= (uint16_t)(1 << s);
            }
        }
        n++;
    }
    perm_classes[!corners] = n;
    if (n != N_PERM_SYM) return;
    for (n = 0; n < N_PERM_SYM; n++)
        for (j = 0; j < N_P2; j++) {
            cc_ident(&c);
            if (corners) set_cperm(&c, s2r[n]);
            else set_eperm(&c, s2r[n]);
            cc_mul(&c, &mv_cc[P2_MOVE[j]], &d);
            if (corners) CPermMove[n][j] = r2s[get_cperm(&d)];
            else EPermMove[n][j] = r2s[get_eperm(&d)];
        }
}

static void build_mperm(void)
{
    int a, j, s;
    cc_t c, d;
    for (a = 0; a < N_MPERM; a++) {
        cc_ident(&c);
        set_mperm(&c, a);
        for (j = 0; j < N_P2; j++) {
            cc_mul(&c, &mv_cc[P2_MOVE[j]], &d);
            MPermMove[a][j] = (uint8_t)get_mperm(&d);
        }
        for (s = 0; s < 16; s++) {
            conj(&c, s, &d);
            MPermConj[a][s] = (uint8_t)get_mperm(&d);
        }
    }
}

static void build_p2_prune(int corners)
{
    uint8_t *pr = corners ? MCPrun : MEPrun;
    uint16_t (*mv)[N_P2] = corners ? CPermMove : EPermMove;
    uint16_t *symst = corners ? CPermSym : EPermSym;
    int depth = 0, done = 1, i, j, r, n = N_PERM_SYM * N_MPERM;
    for (i = 0; i < n / 2; i++) pr[i] = 0xFF;
    nib_set(pr, 0, 0);
    while (done < n) {
        for (i = 0; i < n; i++) {
            int cls = i / N_MPERM, mp = i % N_MPERM;
            if (nib(pr, i) != depth) continue;
            for (j = 0; j < N_P2; j++) {
                int t = mv[cls][j], cls2 = t >> 4, s2 = t & 15;
                int mp2 = MPermConj[MPermMove[mp][j]][sym_inv[s2]];
                int k = cls2 * N_MPERM + mp2;
                if (nib(pr, k) != 15) continue;
                for (r = 0; r < 16; r++)
                    if (symst[cls2] >> r & 1) {
                        int q = cls2 * N_MPERM + MPermConj[mp2][r];
                        if (nib(pr, q) == 15) nib_set(pr, q, depth + 1), done++;
                    }
            }
        }
        depth++;
    }
}

int solve_twist_classes(void) { return twist_classes; }
int solve_perm_classes(void) { return perm_classes[0] == perm_classes[1] ? perm_classes[0] : -1; }
#else
int solve_twist_classes(void) { return N_TWIST_SYM; }
int solve_perm_classes(void) { return N_PERM_SYM; }
#endif

int solve_init(void)
{
    int s, m;
    build_small();
    for (s = 0; s < 16; s++)
        for (m = 0; m < 18; m++)
            if (sym_move[s][m] >= 18) return -1;
    for (s = 0; s < 3; s++)
        for (m = 0; m < 18; m++)
            if (urf_move[s][m] >= 18) return -1;
#ifdef SOLVE_GEN
    build_twist();
    if (twist_classes != N_TWIST_SYM) return -2;
    build_slice_flip();
    if (slice_classes != N_SLICE_SYM) return -4;
    build_p1_prune();
    build_perm(1);
    build_perm(0);
    if (perm_classes[0] != N_PERM_SYM || perm_classes[1] != N_PERM_SYM) return -3;
    build_mperm();
    build_p2_prune(1);
    build_p2_prune(0);
#endif
    return 0;
}

/* ------------------------------------------------------------------ search --- */
static int p1_h(int tw, int fl, int sl)
{
    int cls = tw >> 3, ri = tw & 7;
    int a = nib(P1Prun, cls * N_SLICE + SliceConj[sl][sym_inv[ri << 1] >> 1]);
    int t = SliceR2S[sl];
    int b = nib(FSPrun, (t >> 4) * N_FLIP + (flip_conj(fl, SliceMask[sl], sym_inv[t & 15]) & 0x7FF));
    return a > b ? a : b;
}

static int p1_twist_move(int tw, int m)
{
    int ri = tw & 7, s = ri << 1;
    int t = TwistMove[tw >> 3][sym_move[sym_inv[s]][m]];
    return (t & ~7) | sym_mult[s][(t & 7) << 1] >> 1;
}

static int perm_bsearch(const uint16_t *s2r, int raw)
{
    int lo = 0, hi = N_PERM_SYM - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (s2r[mid] == raw) return mid;
        if (s2r[mid] < raw) lo = mid + 1;
        else hi = mid - 1;
    }
    return -1;
}

static int perm_sym(const cc_t *c, int corners)   /* class << 4 | sym, from a cube (once a phase 2) */
{
    int s, best = 0x7FFFFFFF, bs = 0, cls;
    cc_t d;
    for (s = 0; s < 16; s++) {
        int r;
        conj(c, s, &d);
        r = corners ? get_cperm(&d) : get_eperm(&d);
        if (r < best) best = r, bs = s;
    }
    cls = perm_bsearch(corners ? CPermS2R : EPermS2R, best);
    if (cls < 0) return -1;
    return cls << 4 | sym_inv[bs];
}

static int p2_perm_move(int ps, int m, const uint16_t (*mv)[N_P2])
{
    int s = ps & 15, t = mv[ps >> 4][p2_index[sym_move[sym_inv[s]][m]]];
    return (t & ~15) | sym_mult[s][t & 15];
}

static int p2_h(int cp, int ep, int mp)
{
    int a = nib(MCPrun, (cp >> 4) * N_MPERM + MPermConj[mp][sym_inv[cp & 15]]);
    int b = nib(MEPrun, (ep >> 4) * N_MPERM + MPermConj[mp][sym_inv[ep & 15]]);
    return a > b ? a : b;
}

#define MV_INV18(m) ((m) - (m) % 3 + 2 - (m) % 3)
static uint32_t p2_nodes;
#ifndef P2_NODE_CAP
#define P2_NODE_CAP 20000u
#endif

static int p2_dfs(int cp, int ep, int mp, int left, int pf, uint8_t *out)
{
    int j;
    if (left == 0) return cp == 0 && ep == 0 && mp == 0;
    for (j = 0; j < N_P2; j++) {
        int m = P2_MOVE[j], f = m / 3, cp2, ep2, mp2;
        if (f == pf || f == pf - 3) continue;
        if (++p2_nodes > P2_NODE_CAP) return 0;
        cp2 = p2_perm_move(cp, m, CPermMove);
        ep2 = p2_perm_move(ep, m, EPermMove);
        mp2 = MPermMove[mp][j];
        if (p2_h(cp2, ep2, mp2) >= left) continue;   /* (h > left - 1) */
        out[0] = (uint8_t)m;
        if (p2_dfs(cp2, ep2, mp2, left - 1, f, out + 1)) return 1;
    }
    return 0;
}

#ifndef P2_MAXD
#define P2_MAXD 12                       /* phase 2 longer than this: look for another phase 1 instead */
#endif
static int phase2(const cc_t *c, int maxd, int pf, uint8_t *out)   /* returns the length, or -1 */
{
    int cp = perm_sym(c, 1), ep = perm_sym(c, 0), mp = get_mperm(c), h, d;
    if (cp < 0 || ep < 0) return -1;
    if (maxd > P2_MAXD) maxd = P2_MAXD;
    h = p2_h(cp, ep, mp);
    p2_nodes = 0;
    for (d = h; d <= maxd; d++) {
        if (p2_dfs(cp, ep, mp, d, pf, out)) return d;
        if (p2_nodes > P2_NODE_CAP) return -1;
    }
    return -1;
}

static void dir_start(sv_dir_t *s, const cc_t *c, int maxlen)
{
    s->cc = *c;
    s->maxlen = (int8_t)(maxlen > SOLVE_MAX ? SOLVE_MAX : maxlen);
    s->tw[0] = TwistR2S[get_twist(c)];
    s->fl[0] = (uint16_t)get_flip12(c);
    s->sl[0] = (uint16_t)get_slice(c);
    s->d1 = (int8_t)p1_h(s->tw[0], s->fl[0], s->sl[0]);
    s->n = -1;
    s->done = s->failed = 0;
    s->nodes = s->nodes_level = 0;
    s->level_budget = 40000u;
    s->len = 0;
}

static int try_phase2(sv_dir_t *s, int d1)
{
    cc_t c = s->cc;
    uint8_t p2[SOLVE_MAX];
    int i, n, pf = d1 ? s->mv1[d1 - 1] / 3 : -1;
    for (i = 0; i < d1; i++) cc_move(&c, s->mv1[i]);
    n = phase2(&c, s->maxlen - d1, pf, p2);
    s->nodes += p2_nodes;
    s->nodes_level += p2_nodes;
    if (n < 0) return 0;
    for (i = 0; i < d1; i++) s->sol[i] = s->mv1[i];
    for (i = 0; i < n; i++) s->sol[d1 + i] = p2[i];
    s->len = (int8_t)(d1 + n);
    s->done = 1;
    return 1;
}

static int dir_step(sv_dir_t *s, uint32_t budget)
{
    if (s->done) return 1;
    if (s->failed) return -1;
    while (budget--) {
        int n, m, f, tw, fl, sl, h;
        if (s->nodes_level > s->level_budget) {   /* too long at this length: allow one more move */
            s->nodes_level = 0;
            if (s->maxlen < SOLVE_MAX) s->maxlen++;
            s->level_budget += s->level_budget / 2;
        }
        if (s->n < 0) {                  /* a new depth for phase 1 */
            if (s->d1 > s->maxlen) {
                if (s->maxlen >= SOLVE_MAX) {
                    s->failed = 1;
                    return -1;
                }
                s->maxlen++;
                s->d1 = (int8_t)p1_h(s->tw[0], s->fl[0], s->sl[0]);
            }
            if (s->d1 == 0) {            /* already in phase 2's group */
                if (try_phase2(s, 0)) return 1;
                s->d1 = 1;
                continue;
            }
            s->n = 0;
            s->mv[0] = -1;
        }
        n = s->n;
        m = ++s->mv[n];
        if (m >= 18) {
            if (--s->n < 0) s->d1++;
            continue;
        }
        f = m / 3;
        if (n > 0) {
            int pf = s->mv1[n - 1] / 3;
            if (f == pf || f == pf - 3) continue;
        }
        s->nodes++;
        s->nodes_level++;
        tw = p1_twist_move(s->tw[n], m);
        fl = flip_move(s->fl[n], m);
        sl = SliceMove[s->sl[n]][m];
        h = p1_h(tw, fl, sl);
        if (h > s->d1 - n - 1) continue;
        s->mv1[n] = (uint8_t)m;
        if (n + 1 == s->d1) {
            /* phase 1 ends on a quarter turn of R L F B (else a shorter phase 1 had it) */
            if ((f == 1 || f == 2 || f == 4 || f == 5) && m % 3 != 1 && try_phase2(s, n + 1)) return 1;
            continue;
        }
        s->tw[n + 1] = (uint16_t)tw;
        s->fl[n + 1] = (uint16_t)fl;
        s->sl[n + 1] = (uint16_t)sl;
        s->n = (int8_t)(n + 1);
        s->mv[n + 1] = -1;
    }
    return 0;
}

/* the cube in each of the six searches: seen along axis a (conj by URF3^a), the inverse for k >= 3 */
void solve_start(solve_t *s, const cc_t *c, int maxlen)
{
    int k;
    cc_t inv, t;
    cc_inv(c, &inv);
    for (k = 0; k < SOLVE_DIRS; k++) {
        const cc_t *x = k >= 3 ? &inv : c;
        cc_mul(&urf_cc[k % 3], x, &t);
        cc_mul(&t, &urf_inv_cc[k % 3], &s->d[k].cc);
        dir_start(&s->d[k], &s->d[k].cc, maxlen);
    }
    s->turn = 0;
    s->done = s->failed = 0;
    s->len = 0;
    s->nodes = 0;
}

int solve_step(solve_t *s, uint32_t budget)
{
    if (s->done) return 1;
    if (s->failed) return -1;
    while (budget) {
        sv_dir_t *d = &s->d[s->turn];
        uint32_t b = budget < 2000u ? budget : 2000u, before = d->nodes;
        int r = dir_step(d, b);
        budget -= b;
        s->nodes += d->nodes - before;
        if (r < 0) {
            s->failed = 1;
            return -1;
        }
        if (r > 0) {                     /* back to a solution of the cube itself */
            int k = s->turn, i;
            uint8_t t[SOLVE_MAX];
            for (i = 0; i < d->len; i++) t[i] = urf_move[k % 3][d->sol[i]];
            if (k >= 3)
                for (i = 0; i < d->len; i++) s->sol[i] = (uint8_t)MV_INV18(t[d->len - 1 - i]);
            else
                for (i = 0; i < d->len; i++) s->sol[i] = t[i];
            s->len = d->len;
            s->dir = (uint8_t)k;
            s->done = 1;
            return 1;
        }
        s->turn = (uint8_t)((s->turn + 1) % SOLVE_DIRS);
    }
    return 0;
}

/* ------------------------------------------------------------ random state --- */
static uint32_t rnd(uint32_t *x)
{
    uint32_t v = *x ? *x : 0x2545F491u;
    v ^= v << 13;
    v ^= v >> 17;
    v ^= v << 5;
    *x = v;
    return v;
}

static uint32_t rnd_below(uint32_t *x, uint32_t n)   /* uniform in 0..n-1 (rejection, no modulo bias) */
{
    uint32_t lim = 0xFFFFFFFFu - 0xFFFFFFFFu % n, v;
    do v = rnd(x);
    while (v >= lim);
    return v % n;
}

void solve_random_cc(cc_t *c, uint32_t *rng)
{
    int i, t, f, s;
    for (;;) {
        cc_ident(c);
        unrank_perm(c->cp, 8, (int)rnd_below(rng, N_PERM));
        for (i = 11; i > 0; i--) {       /* Fisher-Yates */
            int j = (int)rnd_below(rng, (uint32_t)i + 1u);
            uint8_t x = c->ep[i];
            c->ep[i] = c->ep[j];
            c->ep[j] = x;
        }
        if (parity(c->cp, 8) != parity(c->ep, 12)) {
            uint8_t x = c->ep[0];
            c->ep[0] = c->ep[1];
            c->ep[1] = x;
        }
        set_twist(c, (int)rnd_below(rng, N_TWIST));
        f = (int)rnd_below(rng, N_FLIP);
        for (i = 0, s = 0; i < 11; i++) c->eo[i] = (uint8_t)(f >> i & 1), s += c->eo[i];
        c->eo[11] = (uint8_t)(s & 1);
        /* the WCA does not give a scramble that 0 or 1 move solves */
        for (t = 0; t < 18; t++) {
            cc_t d = *c;
            cc_move(&d, t);
            if (get_cperm(&d) == 0 && get_eperm(&d) == 0 && get_twist(&d) == 0 && get_flip12(&d) == 0
                && d.ep[8] == FR && d.ep[9] == FL && d.ep[10] == BL && d.ep[11] == BR)
                break;
        }
        if (t == 18 && !(get_cperm(c) == 0 && get_twist(c) == 0 && get_flip12(c) == 0 && rank_perm(c->ep, 12) == 0))
            return;
    }
}

int solve_scramble(uint8_t *out, uint32_t *rng, int maxlen)
{
    static solve_t s;
    cc_t c;
    int i;
    if (!small_built) build_small();
    solve_random_cc(&c, rng);
    solve_start(&s, &c, maxlen);
    while (solve_step(&s, 100000u) == 0) {}
    if (!s.done) return -1;
    for (i = 0; i < s.len; i++) out[i] = (uint8_t)(s.sol[s.len - 1 - i] - s.sol[s.len - 1 - i] % 3 + 2
                                                  - s.sol[s.len - 1 - i] % 3);   /* the inverse */
    return s.len;
}

/* the bank of scrambles made on the computer (gen/cube_tables.h): for the moments before the FM-1 has found its own */
int solve_bank(uint32_t i, uint8_t *out)
{
#ifdef SOLVE_BANK_N
    const uint8_t *b = SOLVE_BANK[i % SOLVE_BANK_N];
    int n = b[0], j;
    for (j = 0; j < n; j++) out[j] = b[1 + j];
    return n;
#else
    (void)i;
    (void)out;
    return -1;
#endif
}
