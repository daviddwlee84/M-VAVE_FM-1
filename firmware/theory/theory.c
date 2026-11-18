/* SPDX-License-Identifier: GPL-3.0-or-later */
/* theory: see theory.h. */
#include "theory.h"

/* ------------------------------------------------------------------ tables --- */
const th_scale_t th_scales[TH_NSCALES] = {
    /* name               n  sig  fifths from the tonic, by step */
    { "major",            7,  0, { 0,  2,  4, -1,  1,  3,  5 } },
    { "dorian",           7, -2, { 0,  2, -3, -1,  1,  3, -2 } },
    { "phrygian",         7, -4, { 0, -5, -3, -1,  1, -4, -2 } },
    { "lydian",           7,  1, { 0,  2,  4,  6,  1,  3,  5 } },
    { "mixolydian",       7, -1, { 0,  2,  4, -1,  1,  3, -2 } },
    { "minor",            7, -3, { 0,  2, -3, -1,  1, -4, -2 } },
    { "locrian",          7, -5, { 0, -5, -3, -1, -6, -4, -2 } },
    { "harmonic minor",   7, -3, { 0,  2, -3, -1,  1, -4,  5 } },
    { "melodic minor",    7, -3, { 0,  2, -3, -1,  1,  3,  5 } },
    { "major pentatonic", 5,  0, { 0,  2,  4,  1,  3 } },
    { "minor pentatonic", 5, -3, { 0, -3, -1,  1, -2 } },
    { "blues",            6, -3, { 0, -3, -1, -6,  1, -2 } },
};

const th_qual_t th_quals[TH_NQUAL] = {
    /* symbol     after a numeral        lower n  fifths from the root */
    { "",        "",                    0, 3, { 0,  4,  1 } },
    { "m",       "",                    1, 3, { 0, -3,  1 } },
    { "dim",     TH_GLYPH_DIM,          1, 3, { 0, -3, -6 } },
    { "aug",     "+",                   0, 3, { 0,  4,  8 } },
    { "sus4",    "sus4",                0, 3, { 0, -1,  1 } },
    { "sus2",    "sus2",                0, 3, { 0,  2,  1 } },
    { "7",       "7",                   0, 4, { 0,  4,  1, -2 } },
    { "maj7",    "M7",                  0, 4, { 0,  4,  1,  5 } },
    { "m7",      "7",                   1, 4, { 0, -3,  1, -2 } },
    { "m7b5",    TH_GLYPH_HDIM "7",     1, 4, { 0, -3, -6, -2 } },
    { "dim7",    TH_GLYPH_DIM "7",      1, 4, { 0, -3, -6, -9 } },
    { "m(maj7)", "M7",                  1, 4, { 0, -3,  1,  5 } },
    { "6",       "add6",                0, 4, { 0,  4,  1,  3 } },
    { "m6",      "add6",                1, 4, { 0, -3,  1,  3 } },
    { "7sus4",   "7sus4",               0, 4, { 0, -1,  1, -2 } },
    { "add9",    "add9",                0, 4, { 0,  4,  1,  2 } },
    { "m(add9)", "add9",                1, 4, { 0, -3,  1,  2 } },
    { "9",       "9",                   0, 5, { 0,  4,  1, -2,  2 } },
    { "maj9",    "M9",                  0, 5, { 0,  4,  1,  5,  2 } },
    { "m9",      "9",                   1, 5, { 0, -3,  1, -2,  2 } },
    { "7#5",     "+7",                  0, 4, { 0,  4,  8, -2 } },
    { "maj7#5",  "+M7",                 0, 4, { 0,  4,  8,  5 } },
    { "5",       "5",                   0, 2, { 0,  1 } },
};

static const int8_t TH_NAT[7] = { 0, 2, 4, -1, 1, 3, 5 };     /* C D E F G A B: also the major scale by step */
static const uint8_t TH_SEMI[7] = { 0, 2, 4, 5, 7, 9, 11 };
static const char TH_LETTERS[] = "CDEFGAB";

/* ------------------------------------------------------------- small tools --- */
int th_mod(int a, int m) { int r = a % m; return r < 0 ? r + m : r; }
int th_fdiv(int a, int m) { return (a - th_mod(a, m)) / m; }
static int th_iabs(int a) { return a < 0 ? -a : a; }

static int th_put(char *b, int i, const char *s)
{
    while (*s) b[i++] = *s++;
    b[i] = 0;
    return i;
}

static int th_put_int(char *b, int i, int v)
{
    char t[12];
    int n = 0;
    if (v < 0) { b[i++] = '-'; v = -v; }
    do { t[n++] = (char)('0' + v % 10); v /= 10; } while (v);
    while (n) b[i++] = t[--n];
    b[i] = 0;
    return i;
}

static int th_str_eq(const char *a, const char *b)
{
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

static int th_prefix(const char *s, const char *p)     /* length of p when s starts with it, else 0 */
{
    int n = 0;
    while (p[n]) { if (s[n] != p[n]) return 0; n++; }
    return n;
}

static int th_is_sep(char c)                         /* between chords: space, comma, bar line, dash */
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == ',' || c == '|' || c == '-';
}

/* ------------------------------------------------------------------- notes --- */
int th_pc(int f) { return th_mod(7 * f, 12); }

int th_letter(int f)
{
    static const uint8_t L[7] = { 3, 0, 4, 1, 5, 2, 6 };   /* F C G D A E B */
    return L[th_mod(f + 1, 7)];
}

int th_acc(int f) { return th_fdiv(f + 1, 7); }
int th_from(int letter, int acc) { return TH_NAT[th_mod(letter, 7)] + 7 * acc; }

static int th_put_acc(char *b, int i, int a)
{
    if (a > 3) a = 3;                    /* never in practice; keeps the buffer bounded */
    if (a < -3) a = -3;
    for (; a > 0; a--) i = th_put(b, i, TH_GLYPH_SHARP);
    for (; a < 0; a++) i = th_put(b, i, TH_GLYPH_FLAT);
    return i;
}

static int th_put_name(char *b, int i, int f)
{
    b[i++] = TH_LETTERS[th_letter(f)];
    b[i] = 0;
    return th_put_acc(b, i, th_acc(f));
}

int th_name(int f, char *buf) { return th_put_name(buf, 0, f); }

static int th_read_acc(const char *s, int *acc)        /* #, x, b, and the UTF-8 sharp and flat signs */
{
    int n = 0;
    *acc = 0;
    for (;;) {
        if (s[n] == '#') { *acc += 1; n++; }
        else if (s[n] == 'x') { *acc += 2; n++; }
        else if (s[n] == 'b') { *acc -= 1; n++; }
        else if (th_prefix(s + n, "\xE2\x99\xAF")) { *acc += 1; n += 3; }
        else if (th_prefix(s + n, "\xE2\x99\xAD")) { *acc -= 1; n += 3; }
        else return n;
    }
}

int th_parse_name(const char *s, int *f)
{
    int letter, acc, n;
    char c = s[0];
    if (c >= 'a' && c <= 'g') c = (char)(c - 32);
    if (c < 'A' || c > 'G') return 0;
    letter = (c - 'C' + 7) % 7;          /* C D E F G A B = 0..6 */
    n = th_read_acc(s + 1, &acc);
    *f = th_from(letter, acc);
    return 1 + n;
}

int th_spell_pc(int pc, int tonic)
{
    int lo = tonic - 5;                  /* the window b2 .. #4 around the tonic: Db Eb F# Ab Bb in C */
    return th_mod(7 * pc - lo, 12) + lo;
}

int th_midi(int f, int octave)
{
    return TH_SEMI[th_letter(f)] + th_acc(f) + 12 * (octave + 1);
}

int th_octave(int f, int midi)
{
    return th_fdiv(midi - TH_SEMI[th_letter(f)] - th_acc(f), 12) - 1;
}

int th_note_name(int midi, int tonic, char *buf)
{
    int f = th_spell_pc(th_mod(midi, 12), tonic);
    int i = th_put_name(buf, 0, f);
    return th_put_int(buf, i, th_octave(f, midi));
}

int th_interval_name(int d, char *buf)
{
    int l = th_letter(d), k = (d - TH_NAT[l]) / 7, i = 0;
    int perfect = l == 0 || l == 3 || l == 4;
    if (k == 0) buf[i++] = perfect ? 'P' : 'M';
    else if (k > 0) for (; k > 0; k--) buf[i++] = 'A';
    else if (!perfect && k == -1) buf[i++] = 'm';
    else for (k = perfect ? -k : -k - 1; k > 0; k--) buf[i++] = 'd';
    buf[i++] = (char)('1' + l);
    buf[i] = 0;
    return i;
}

/* ----------------------------------------------------------- scales, keys --- */
int th_scale(int tonic, int scale, int8_t *out)
{
    const th_scale_t *s = &th_scales[scale];
    int i;
    for (i = 0; i < s->n; i++) out[i] = (int8_t)(tonic + s->f[i]);
    return s->n;
}

int th_keysig(int tonic, int scale, int8_t *out)
{
    int c = tonic + th_scales[scale].sig, n = th_iabs(c), i;
    if (n > 7) n = 7;
    for (i = 0; i < n; i++) out[i] = (int8_t)(c > 0 ? 6 + i : -2 - i);   /* F C G D A E B, or B E A D G C F */
    return c;
}

int th_key_acc(th_key_t key, int letter)
{
    int lo = key.tonic + th_scales[key.scale].sig - 1;     /* the signature's seven notes are lo .. lo+6 */
    return th_acc(th_mod(TH_NAT[th_mod(letter, 7)] - lo, 7) + lo);
}

int th_minorish(th_key_t key)
{
    const th_scale_t *s = &th_scales[key.scale];
    int i;
    for (i = 1; i < s->n; i++) if (s->f[i] == -3) return 1;    /* a minor third above the tonic */
    return 0;
}

int th_key_mask(th_key_t key)
{
    const th_scale_t *s = &th_scales[key.scale];
    int m = 0, i;
    for (i = 0; i < s->n; i++) m |= 1 << th_pc(key.tonic + s->f[i]);
    if (s->n == 7 && th_minorish(key)) m |= 1 << th_pc(key.tonic + 5);     /* the leading tone */
    return m;
}

int th_circle_pos(int f) { return th_mod(f, 12); }

int th_circle_tonic(int pos, int minor)
{
    int f = th_mod(pos + 5, 12) - 5;     /* Db .. F#: signatures from 5 flats to 6 sharps */
    return minor ? f + 3 : f;
}

int th_key_for_pc(int pc, int scale)
{
    int sig = th_scales[scale].sig;
    int lo = sig == 0 ? -5 : -6;         /* major: Db, F#; others: Eb minor, G# minor */
    return th_mod(7 * pc + sig - lo, 12) + lo - sig;
}

/* ------------------------------------------------------------------ chords --- */
int th_chord(int root, int q, int8_t *out)
{
    const th_qual_t *t = &th_quals[q];
    int i;
    for (i = 0; i < t->n; i++) out[i] = (int8_t)(root + t->f[i]);
    return t->n;
}

int th_chord_name(th_chord_t c, char *buf)
{
    int i = th_put_name(buf, 0, c.root);
    i = th_put(buf, i, th_quals[c.q].sym);
    if (c.bass != c.root) {
        buf[i++] = '/';
        i = th_put_name(buf, i, c.bass);
    }
    return i;
}

int th_qual_find(const char *sym)
{
    static const struct { const char *s; uint8_t q; } ALIAS[] = {
        { "M", TH_MAJ }, { "maj", TH_MAJ }, { "min", TH_MIN }, { "-", TH_MIN }, { "o", TH_DIM }, { "+", TH_AUG },
        { "M7", TH_MAJ7 }, { "-7", TH_M7 }, { "min7", TH_M7 }, { "/o7", TH_M7B5 }, { "o7", TH_DIM7 },
        { "mM7", TH_MMAJ7 }, { "+7", TH_AUG7 }, { "aug7", TH_AUG7 }, { "+M7", TH_AUGMAJ7 }, { "madd9", TH_MADD9 },
    };
    unsigned i;
    for (i = 0; i < TH_NQUAL; i++) if (th_str_eq(sym, th_quals[i].sym)) return (int)i;
    for (i = 0; i < sizeof ALIAS / sizeof ALIAS[0]; i++) if (th_str_eq(sym, ALIAS[i].s)) return ALIAS[i].q;
    return -1;
}

static int th_qual_of(const int8_t *iv, int n)          /* the quality with exactly these fifths, -1 if none */
{
    int q, i;
    for (q = 0; q < TH_NQUAL; q++) {
        if (th_quals[q].n != n) continue;
        for (i = 0; i < n && th_quals[q].f[i] == iv[i]; i++) {}
        if (i == n) return q;
    }
    return -1;
}

int th_diatonic(th_key_t key, int degree, int sevenths, th_chord_t *out)
{
    const th_scale_t *s = &th_scales[key.scale];
    int8_t iv[4];
    int r, n = sevenths ? 4 : 3, i, q;
    if (s->n != 7 || degree < 0 || degree > 6) return 0;
    r = s->f[degree];
    for (i = 0; i < n; i++) iv[i] = (int8_t)(s->f[(degree + 2 * i) % 7] - r);
    q = th_qual_of(iv, n);
    if (q < 0) return 0;
    out->root = out->bass = (int8_t)(key.tonic + r);
    out->q = (uint8_t)q;
    return 1;
}

static int th_put_numeral(char *b, int i, int tonic, int f, int lower)
{
    static const char *const UP[7] = { "I", "II", "III", "IV", "V", "VI", "VII" };
    static const char *const LO[7] = { "i", "ii", "iii", "iv", "v", "vi", "vii" };
    int d = f - tonic, deg = th_letter(d);
    i = th_put_acc(b, i, (d - TH_NAT[deg]) / 7);
    return th_put(b, i, lower ? LO[deg] : UP[deg]);
}

int th_degree_name(th_key_t key, int f, char *buf)
{
    int d = f - key.tonic, deg = th_letter(d), i = th_put_acc(buf, 0, (d - TH_NAT[deg]) / 7);
    buf[i++] = (char)('1' + deg);
    buf[i] = 0;
    return i;
}

/* A major triad or dominant seventh that is not the key's own chord on that degree, a fifth above one of the
 * key's major or minor triads (not the tonic: V/I is V; not bVII in minor: D in A minor reads better as IV). */
static int th_secondary_target(th_key_t key, th_chord_t c)
{
    const th_scale_t *s = &th_scales[key.scale];
    th_chord_t dc;
    int d = c.root - key.tonic, deg, t, tdeg;
    if ((c.q != TH_MAJ && c.q != TH_7) || s->n != 7) return -1;
    deg = th_letter(d);
    if (s->f[deg] == d && th_diatonic(key, deg, c.q == TH_7, &dc) && dc.q == c.q) return -1;
    t = d - 1;
    tdeg = th_letter(t);
    if (s->f[tdeg] != t || tdeg == 0 || (tdeg == 6 && th_minorish(key))) return -1;
    if (!th_diatonic(key, tdeg, 0, &dc) || (dc.q != TH_MAJ && dc.q != TH_MIN)) return -1;
    return tdeg;
}

int th_roman_name(th_key_t key, th_chord_t c, char *buf)
{
    const th_qual_t *q = &th_quals[c.q];
    int tdeg = th_secondary_target(key, c), i;
    th_chord_t tc;
    if (tdeg >= 0) {
        th_diatonic(key, tdeg, 0, &tc);
        i = th_put(buf, 0, "V");
        i = th_put(buf, i, q->rn);
        buf[i++] = '/';
        return th_put_numeral(buf, i, key.tonic, tc.root, th_quals[tc.q].lower);
    }
    i = th_put_numeral(buf, 0, key.tonic, c.root, q->lower);
    return th_put(buf, i, q->rn);
}

int th_nashville_name(th_key_t key, th_chord_t c, char *buf)
{
    int i = th_degree_name(key, c.root, buf);
    return th_put(buf, i, c.q == TH_DIM ? TH_GLYPH_DIM : th_quals[c.q].sym);
}

/* ------------------------------------------------------- reading numerals --- */
static int th_read_numeral(const char *s, int *deg, int *lower)
{
    static const char *const UP[7] = { "I", "II", "III", "IV", "V", "VI", "VII" };
    char u[4];
    int n = 0, up = 0, lo = 0, i;
    while (n < 4 && (s[n] == 'I' || s[n] == 'V' || s[n] == 'i' || s[n] == 'v')) {
        if (s[n] == 'I' || s[n] == 'V') up++; else lo++;
        u[n] = (char)(s[n] == 'i' || s[n] == 'v' ? s[n] - 32 : s[n]);
        n++;
    }
    if (n == 0 || n == 4 || (up && lo)) return 0;
    u[n] = 0;
    for (i = 0; i < 7; i++) if (th_str_eq(u, UP[i])) { *deg = i; *lower = lo > 0; return n; }
    return 0;
}

static const struct { const char *s; uint8_t up, low; } TH_SUFFIX[] = {
    { "/o7", TH_M7B5, TH_M7B5 }, { "\xC3\xB8" "7", TH_M7B5, TH_M7B5 }, { "%7", TH_M7B5, TH_M7B5 },
    { "m7b5", TH_M7B5, TH_M7B5 },
    { "maj7#5", TH_AUGMAJ7, TH_AUGMAJ7 }, { "+M7", TH_AUGMAJ7, TH_AUGMAJ7 },
    { "maj7", TH_MAJ7, TH_MMAJ7 }, { "M7", TH_MAJ7, TH_MMAJ7 }, { "maj9", TH_MAJ9, TH_MAJ9 }, { "M9", TH_MAJ9, TH_MAJ9 },
    { "7sus4", TH_7SUS4, TH_7SUS4 }, { "sus4", TH_SUS4, TH_SUS4 }, { "sus2", TH_SUS2, TH_SUS2 },
    { "madd9", TH_MADD9, TH_MADD9 }, { "add9", TH_ADD9, TH_MADD9 }, { "add6", TH_6, TH_M6 },
    { "dim7", TH_DIM7, TH_DIM7 }, { "o7", TH_DIM7, TH_DIM7 }, { "\xC2\xB0" "7", TH_DIM7, TH_DIM7 },
    { "dim", TH_DIM, TH_DIM }, { "o", TH_DIM, TH_DIM }, { "\xC2\xB0", TH_DIM, TH_DIM },
    { "+7", TH_AUG7, TH_AUG7 }, { "7#5", TH_AUG7, TH_AUG7 }, { "aug", TH_AUG, TH_AUG }, { "+", TH_AUG, TH_AUG },
    { "m(maj7)", TH_MMAJ7, TH_MMAJ7 }, { "mM7", TH_MMAJ7, TH_MMAJ7 }, { "m9", TH_M9, TH_M9 }, { "m7", TH_M7, TH_M7 },
    { "m6", TH_M6, TH_M6 }, { "m", TH_MIN, TH_MIN }, { "maj", TH_MAJ, TH_MAJ },
    { "9", TH_9, TH_M9 }, { "7", TH_7, TH_M7 }, { "6", TH_6, TH_M6 }, { "5", TH_5, TH_5 },
};

static int th_read_suffix(const char *s, int *k)        /* longest match; its TH_SUFFIX index in *k */
{
    unsigned i;
    int best = 0, n;
    for (i = 0; i < sizeof TH_SUFFIX / sizeof TH_SUFFIX[0]; i++) {
        n = th_prefix(s, TH_SUFFIX[i].s);
        if (n > best) { best = n; *k = (int)i; }
    }
    return best;
}

int th_roman_parse(const char *s, th_key_t key, th_chord_t *out)
{
    const char *p = s;
    int acc, deg, lower, n, k = -1, digit = 0, q, root;
    th_chord_t dc;
    p += th_read_acc(p, &acc);
    if (*p >= '1' && *p <= '7') { deg = *p - '1'; lower = 0; digit = 1; p++; }
    else if ((n = th_read_numeral(p, &deg, &lower)) > 0) p += n;
    else return 0;
    n = (digit && *p >= '0' && *p <= '9') ? 0 : th_read_suffix(p, &k);
    p += n;
    if (digit) {                         /* Nashville: a bare number is the key's own chord on that degree */
        int have = acc == 0 && th_diatonic(key, deg, 0, &dc);
        if (have) lower = th_quals[dc.q].lower;
        if (n) q = lower ? TH_SUFFIX[k].low : TH_SUFFIX[k].up;
        else q = have ? dc.q : TH_MAJ;   /* b7 is a major chord on the flat seventh */
    } else {
        q = n ? (lower ? TH_SUFFIX[k].low : TH_SUFFIX[k].up) : (lower ? TH_MIN : TH_MAJ);
    }
    root = key.tonic + TH_NAT[deg] + 7 * acc;
    if (*p == '/') {                     /* V7/V: the numeral in the key of the target */
        int tacc, tdeg, tlower;
        const char *t = p + 1;
        t += th_read_acc(t, &tacc);
        if ((n = th_read_numeral(t, &tdeg, &tlower)) == 0) return 0;
        root += TH_NAT[tdeg] + 7 * tacc;
        p = t + n;
    }
    out->root = out->bass = (int8_t)root;
    out->q = (uint8_t)q;
    return (int)(p - s);
}

int th_prog_parse(const char *s, th_key_t key, th_chord_t *out, int max)
{
    int n = 0, len, digits, i;
    for (;;) {
        while (*s && th_is_sep(*s)) s++;
        if (!*s) return n;
        for (len = 0, digits = 1; s[len] && !th_is_sep(s[len]); len++)
            if (s[len] < '1' || s[len] > '7') digits = 0;
        if (digits && len > 1) {         /* 4536251: one chord per digit */
            for (i = 0; i < len; i++) {
                char one[2];
                one[0] = s[i];
                one[1] = 0;
                if (n >= max || th_roman_parse(one, key, &out[n]) != 1) return -1;
                n++;
            }
        } else {
            if (n >= max || th_roman_parse(s, key, &out[n]) != len) return -1;
            n++;
        }
        s += len;
    }
}

/* ---------------------------------------------------------------- identify --- */
static int th_qual_mask(int q)
{
    int m = 0, i;
    for (i = 0; i < th_quals[q].n; i++) m |= 1 << th_pc(th_quals[q].f[i]);
    return m;
}

/* Accidentals against the key signature, the root's counted twice: C#dim7 (C# E G Bb) over Dbdim7 in C, C# major
 * over Db in B major, and Cdim7 stays Cdim7 rather than B#dim7. */
static int th_spell_cost(th_key_t key, int root, int q)
{
    int8_t t[TH_TONES_MAX];
    int n = th_chord(root, q, t), cost = 0, i;
    for (i = 0; i < n; i++) cost += (i ? 1 : 2) * th_iabs(th_acc(t[i]) - th_key_acc(key, th_letter(t[i])));
    return cost;
}

int th_identify(const uint8_t *midi, int n, th_key_t key, th_ident_t *out)
{
    int mask = 0, low = 1000, bass, kmask = th_key_mask(key), best = -1, bscore = 1 << 30, broot = 0, bno5 = 0;
    int r, q, i;
    if (n <= 0) return 0;
    for (i = 0; i < n; i++) {
        mask |= 1 << (midi[i] % 12);
        if (midi[i] < low) low = midi[i];
    }
    bass = low % 12;
    for (r = 0; r < 12; r++) {
        int rel;
        if (!(mask >> r & 1)) continue;
        rel = ((mask >> r) | (mask << (12 - r))) & 0xFFF;  /* bit k: the note k semitones above r */
        for (q = 0; q < TH_NQUAL; q++) {
            int qm = th_qual_mask(q), no5 = 0, score;
            if (rel != qm) {
                if (th_quals[q].n >= 4 && (qm >> 7 & 1) && rel == (qm & ~(1 << 7))) no5 = 1;
                else continue;
            }
            score = no5 * 1000 + (r != bass) * 100 + !(kmask >> r & 1) * 30 + q;
            if (score < bscore) { bscore = score; best = q; broot = r; bno5 = no5; }
        }
    }
    if (best < 0) return 0;
    {
        int f0 = th_spell_pc(broot, key.tonic), f = f0, c = th_spell_cost(key, f0, best), k;
        int8_t t[TH_TONES_MAX];
        int nt;
        if (th_spell_cost(key, f0 - 12, best) < c) { f = f0 - 12; c = th_spell_cost(key, f, best); }
        if (th_spell_cost(key, f0 + 12, best) < c) f = f0 + 12;
        nt = th_chord(f, best, t);
        out->c.root = out->c.bass = (int8_t)f;
        out->c.q = (uint8_t)best;
        out->inv = 0;
        for (k = 0; k < nt; k++)
            if (th_pc(t[k]) == bass) { out->c.bass = t[k]; out->inv = (uint8_t)k; }
        out->no5 = (uint8_t)bno5;
        out->inkey = (mask & ~kmask) == 0;
    }
    return 1;
}

/* ------------------------------------------------------------------- voice --- */
static int th_movement(const uint8_t *v, int n, const uint8_t *prev, int np)
{
    int cost = 0, i, j;
    if (n == np) {                       /* same count: pair them lowest to lowest */
        uint8_t p[16];
        for (i = 0; i < np; i++) p[i] = prev[i];
        for (i = 1; i < np; i++)
            for (j = i; j > 0 && p[j - 1] > p[j]; j--) { uint8_t t = p[j]; p[j] = p[j - 1]; p[j - 1] = t; }
        for (i = 0; i < n; i++) cost += th_iabs(v[i] - p[i]);
        return cost;
    }
    for (i = 0; i < n; i++) {            /* otherwise: each note to the nearest previous one */
        int m = 1000;
        for (j = 0; j < np; j++) if (th_iabs(v[i] - prev[j]) < m) m = th_iabs(v[i] - prev[j]);
        cost += m;
    }
    return cost;
}

int th_voice_near(const uint8_t *prev, int np, th_chord_t c, int lo, int hi, uint8_t *out)
{
    int8_t t[TH_TONES_MAX];
    int pcs[TH_TONES_MAX], n = th_chord(c.root, c.q, t), best = 1 << 30, found = 0, k, m0, j;
    if (np > 16) np = 16;
    for (j = 0; j < n; j++) pcs[j] = th_pc(t[j]);
    for (k = 0; k < n; k++) {            /* inversion k: chord tone k at the bottom, the rest stacked above */
        for (m0 = lo; m0 <= hi; m0++) {
            uint8_t v[TH_TONES_MAX];
            int cost, sum = 0;
            if (m0 % 12 != pcs[k]) continue;
            v[0] = (uint8_t)m0;
            for (j = 1; j < n; j++) v[j] = (uint8_t)(v[j - 1] + th_mod(pcs[(k + j) % n] - v[j - 1], 12));
            if (v[n - 1] > hi) continue;
            if (np > 0) cost = th_movement(v, n, prev, np);
            else {
                for (j = 0; j < n; j++) sum += v[j];
                cost = th_iabs(2 * sum - n * (lo + hi));
            }
            if (cost < best) {
                best = cost;
                found = n;
                for (j = 0; j < n; j++) out[j] = v[j];
            }
        }
    }
    return found;
}
