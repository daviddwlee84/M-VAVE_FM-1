/* SPDX-License-Identifier: GPL-3.0-or-later */
/* theory.c on the computer: spelling, scales, keys, chords, Roman numerals, naming what is played, voicing.
 * The hand-written answers come first; tests/vectors.h (music21's spellings, host/theory/make_vectors.py) after.
 *   just theory-unit */
#include <stdio.h>
#include <string.h>
#include "../theory.c"
#include "vectors.h"

static int fails, checks;
#define CHECK(c, ...) do { checks++; if (!(c)) { printf("FAIL %s:%d ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); fails++; } } while (0)

static const th_key_t C_MAJOR = { 0, TH_MAJOR };

static int F(const char *name)           /* "F#" -> its f */
{
    int f = 0;
    int n = th_parse_name(name, &f);
    if (n != (int)strlen(name)) { printf("bad name in a test: %s\n", name); fails++; }
    return f;
}

static const char *join(const int8_t *f, int n)  /* "C D E" */
{
    static char b[128];
    char t[TH_NAME_MAX];
    int i;
    b[0] = 0;
    for (i = 0; i < n; i++) {
        th_name(f[i], t);
        if (i) strcat(b, " ");
        strcat(b, t);
    }
    return b;
}

/* the same notes, in any order (chord vectors list them low to high) */
static int same_set(const int8_t *f, int n, const char *names)
{
    char buf[128], *p;
    int seen = 0, i, count = 0;
    strcpy(buf, names);
    for (p = strtok(buf, " "); p; p = strtok(NULL, " ")) {
        int g = F(p);
        count++;
        for (i = 0; i < n; i++) if (f[i] == g) break;
        if (i == n) return 0;
        seen |= 1 << i;
    }
    return count == n && seen == (1 << n) - 1;
}

static const char *chord_str(th_chord_t c)
{
    static char b[TH_CHORD_MAX];
    th_chord_name(c, b);
    return b;
}

static const char *roman_str(th_key_t k, th_chord_t c)
{
    static char b[TH_ROMAN_MAX];
    th_roman_name(k, c, b);
    return b;
}

/* -------------------------------------------------------------------- notes --- */
static void test_notes(void)
{
    static const char *const NAMES[] = { "Bbb", "Fb", "Cb", "Gb", "Db", "Ab", "Eb", "Bb", "F", "C", "G", "D", "A",
                                         "E", "B", "F#", "C#", "G#", "D#", "A#", "E#", "B#" };
    char b[TH_NAME_MAX];
    int f, i;
    for (f = -9, i = 0; f <= 12; f++, i++) {
        th_name(f, b);
        CHECK(!strcmp(b, NAMES[i]), "name of %d: %s, not %s", f, b, NAMES[i]);
        CHECK(F(b) == f, "%s reads back as %d", b, f);
    }
    for (f = -21; f <= 21; f++) {
        CHECK(th_pc(f) == th_mod(f * 7, 12), "pc of %d", f);
        CHECK(th_from(th_letter(f), th_acc(f)) == f, "letter and accidental of %d", f);
        CHECK(th_pc(f + 12) == th_pc(f) && th_letter(f + 12) != th_letter(f), "%d and %d are enharmonic", f, f + 12);
    }
    CHECK(F("Bbb") == -9 && F("Cx") == 14 && F("eb") == -3 && F("F\xE2\x99\xAF") == 6, "other ways to write them");
    CHECK(th_parse_name("H", &f) == 0, "H is not a note here");
    CHECK(th_pc(F("B#")) == 0 && th_pc(F("Cb")) == 11 && th_pc(F("E#")) == 5, "B# = C, Cb = B, E# = F");

    /* spelling a pitch class for a key */
    CHECK(th_spell_pc(6, F("D")) == F("F#"), "pc 6 in D is F#");
    CHECK(th_spell_pc(6, F("Db")) == F("Gb"), "pc 6 in Db is Gb");
    CHECK(th_spell_pc(0, F("C#")) == F("B#"), "pc 0 in C# is B#");
    CHECK(th_spell_pc(11, F("Cb")) == F("Cb"), "pc 11 in Cb is Cb");
    {
        static const char *const IN_C[12] = { "C", "Db", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
        for (i = 0; i < 12; i++) CHECK(th_spell_pc(i, 0) == F(IN_C[i]), "pc %d in C is %s", i, IN_C[i]);
    }

    /* octaves */
    CHECK(th_midi(F("C"), 4) == 60 && th_midi(F("A"), 4) == 69, "C4 = 60, A4 = 69");
    CHECK(th_midi(F("B#"), 3) == 60 && th_midi(F("Cb"), 4) == 59, "B#3 = 60, Cb4 = 59");
    CHECK(th_octave(F("B#"), 60) == 3 && th_octave(F("Cb"), 59) == 4, "and back");
    th_note_name(60, 0, b);  CHECK(!strcmp(b, "C4"), "60 in C: %s", b);
    th_note_name(61, 0, b);  CHECK(!strcmp(b, "Db4"), "61 in C: %s", b);
    th_note_name(61, 2, b);  CHECK(!strcmp(b, "C#4"), "61 in D: %s", b);
    th_note_name(60, 7, b);  CHECK(!strcmp(b, "B#3"), "60 in C#: %s", b);
    th_note_name(59, -7, b); CHECK(!strcmp(b, "Cb4"), "59 in Cb: %s", b);
    th_note_name(53, 0, b);  CHECK(!strcmp(b, "F3"), "the lowest key: %s", b);
    th_note_name(79, 0, b);  CHECK(!strcmp(b, "G5"), "the highest key: %s", b);

    /* intervals */
    {
        static const struct { int d; const char *n; } IV[] = {
            { 0, "P1" }, { -5, "m2" }, { 2, "M2" }, { -3, "m3" }, { 4, "M3" }, { -1, "P4" }, { 6, "A4" },
            { -6, "d5" }, { 1, "P5" }, { 8, "A5" }, { -4, "m6" }, { 3, "M6" }, { -9, "d7" }, { -2, "m7" },
            { 5, "M7" }, { 7, "A1" }, { -7, "d1" }, { -10, "d3" }, { 9, "A2" }, { 13, "AA4" },
        };
        for (i = 0; i < (int)(sizeof IV / sizeof IV[0]); i++) {
            th_interval_name(IV[i].d, b);
            CHECK(!strcmp(b, IV[i].n), "interval %d: %s, not %s", IV[i].d, b, IV[i].n);
        }
    }
}

/* -------------------------------------------------------------- scales, keys --- */
static const char *const MAJ_KEYS[15] = { "Cb", "Gb", "Db", "Ab", "Eb", "Bb", "F", "C", "G", "D", "A", "E", "B",
                                          "F#", "C#" };
static const char *const MIN_KEYS[15] = { "Ab", "Eb", "Bb", "F", "C", "G", "D", "A", "E", "B", "F#", "C#", "G#",
                                          "D#", "A#" };

static void test_scales(void)
{
    static const char *const MAJ[15] = {
        "Cb Db Eb Fb Gb Ab Bb", "Gb Ab Bb Cb Db Eb F", "Db Eb F Gb Ab Bb C", "Ab Bb C Db Eb F G",
        "Eb F G Ab Bb C D", "Bb C D Eb F G A", "F G A Bb C D E", "C D E F G A B", "G A B C D E F#",
        "D E F# G A B C#", "A B C# D E F# G#", "E F# G# A B C# D#", "B C# D# E F# G# A#",
        "F# G# A# B C# D# E#", "C# D# E# F# G# A# B#",
    };
    static const char *const MIN[15] = {
        "Ab Bb Cb Db Eb Fb Gb", "Eb F Gb Ab Bb Cb Db", "Bb C Db Eb F Gb Ab", "F G Ab Bb C Db Eb",
        "C D Eb F G Ab Bb", "G A Bb C D Eb F", "D E F G A Bb C", "A B C D E F G", "E F# G A B C D",
        "B C# D E F# G A", "F# G# A B C# D E", "C# D# E F# G# A B", "G# A# B C# D# E F#",
        "D# E# F# G# A# B C#", "A# B# C# D# E# F# G#",
    };
    int8_t s[7], sig[7];
    int i, j, k;
    for (i = 0; i < 15; i++) {
        int t = F(MAJ_KEYS[i]), m = F(MIN_KEYS[i]), c;
        th_scale(t, TH_MAJOR, s);
        CHECK(!strcmp(join(s, 7), MAJ[i]), "%s major: %s", MAJ_KEYS[i], join(s, 7));
        th_scale(m, TH_MINOR, s);
        CHECK(!strcmp(join(s, 7), MIN[i]), "%s minor: %s", MIN_KEYS[i], join(s, 7));
        {
            int nat7 = s[6];
            th_scale(m, TH_HARMONIC, s);
            CHECK(s[6] == nat7 + 7, "%s harmonic minor raises the seventh", MIN_KEYS[i]);
        }
        c = th_keysig(t, TH_MAJOR, sig);
        CHECK(c == i - 7, "%s major has %d accidentals (%d)", MAJ_KEYS[i], i - 7, c);
        CHECK(th_keysig(m, TH_MINOR, sig) == c, "%s minor shares %s major's signature", MIN_KEYS[i], MAJ_KEYS[i]);
        CHECK(th_scale(t, TH_MAJOR, s) == 7, "seven notes");
        for (j = 0; j < 7; j++)          /* the signature's accidentals are the scale's */
            CHECK(th_key_acc((th_key_t){ (int8_t)t, TH_MAJOR }, th_letter(s[j])) == th_acc(s[j]),
                  "%s major: the signature agrees on %d", MAJ_KEYS[i], j);
    }
    {   /* harmonic minor raises the seventh; the odd spellings */
        th_scale(F("G#"), TH_HARMONIC, s);
        CHECK(!strcmp(join(s, 7), "G# A# B C# D# E F##"), "G# harmonic minor: %s", join(s, 7));
        th_scale(F("Ab"), TH_HARMONIC, s);
        CHECK(!strcmp(join(s, 7), "Ab Bb Cb Db Eb Fb G"), "Ab harmonic minor: %s", join(s, 7));
        th_scale(F("A"), TH_MELODIC, s);
        CHECK(!strcmp(join(s, 7), "A B C D E F# G#"), "A melodic minor: %s", join(s, 7));
        th_scale(F("D"), TH_DORIAN, s);
        CHECK(!strcmp(join(s, 7), "D E F G A B C"), "D dorian: %s", join(s, 7));
        th_scale(F("F"), TH_LYDIAN, s);
        CHECK(!strcmp(join(s, 7), "F G A B C D E"), "F lydian: %s", join(s, 7));
        th_scale(F("A"), TH_MPENTA, s);
        CHECK(!strcmp(join(s, 5), "A C D E G"), "A minor pentatonic: %s", join(s, 5));
        th_scale(F("A"), TH_BLUES, s);
        CHECK(!strcmp(join(s, 6), "A C D Eb E G"), "A blues: %s", join(s, 6));
        th_scale(F("Eb"), TH_PENTA, s);
        CHECK(!strcmp(join(s, 5), "Eb F G Bb C"), "Eb major pentatonic: %s", join(s, 5));
    }
    /* every heptatonic scale on every tonic: seven letters in a row, one each */
    for (k = 0; k < TH_NSCALES; k++) {
        if (th_scales[k].n != 7) continue;
        for (i = -7; i <= 7; i++) {
            th_scale(i, k, s);
            for (j = 0; j < 7; j++)
                CHECK(th_letter(s[j]) == (th_letter(i) + j) % 7, "%s on %d: step %d's letter", th_scales[k].name, i, j);
        }
        /* modes of the major scale share their parent's signature */
        if (k <= TH_LOCRIAN) {
            th_scale(0, TH_MAJOR, s);
            CHECK(th_keysig(s[k], k, sig) == 0, "%s on the %d step of C has no accidentals", th_scales[k].name, k + 1);
        }
    }
    th_keysig(F("D"), TH_MAJOR, sig);
    CHECK(!strcmp(join(sig, 2), "F# C#"), "D major: %s", join(sig, 2));
    th_keysig(F("Eb"), TH_MAJOR, sig);
    CHECK(!strcmp(join(sig, 3), "Bb Eb Ab"), "Eb major: %s", join(sig, 3));
    th_keysig(F("C#"), TH_MAJOR, sig);
    CHECK(!strcmp(join(sig, 7), "F# C# G# D# A# E# B#"), "C# major: %s", join(sig, 7));
    th_keysig(F("Cb"), TH_MAJOR, sig);
    CHECK(!strcmp(join(sig, 7), "Bb Eb Ab Db Gb Cb Fb"), "Cb major: %s", join(sig, 7));

    /* the circle */
    CHECK(th_circle_tonic(0, 0) == F("C") && th_circle_tonic(0, 1) == F("A"), "top: C and A minor");
    CHECK(th_circle_tonic(1, 0) == F("G") && th_circle_tonic(11, 0) == F("F"), "G clockwise, F anticlockwise");
    CHECK(th_circle_tonic(6, 0) == F("F#") && th_circle_tonic(6, 1) == F("D#"), "bottom: F# and D# minor");
    CHECK(th_circle_tonic(7, 0) == F("Db") && th_circle_tonic(7, 1) == F("Bb"), "then Db and Bb minor");
    for (i = 0; i < 12; i++) CHECK(th_circle_pos(th_circle_tonic(i, 0)) == i, "position %d round trip", i);
    CHECK(th_circle_pos(F("Gb")) == 6 && th_circle_pos(F("Cb")) == 5, "enharmonic keys share a place");
    CHECK(th_key_for_pc(1, TH_MAJOR) == F("Db") && th_key_for_pc(6, TH_MAJOR) == F("F#"), "Db, F# major");
    CHECK(th_key_for_pc(11, TH_MAJOR) == F("B") && th_key_for_pc(8, TH_MAJOR) == F("Ab"), "B, Ab major");
    CHECK(th_key_for_pc(3, TH_MINOR) == F("Eb") && th_key_for_pc(8, TH_MINOR) == F("G#"), "Eb, G# minor");
    CHECK(th_key_for_pc(10, TH_MINOR) == F("Bb") && th_key_for_pc(1, TH_MINOR) == F("C#"), "Bb, C# minor");
    CHECK(th_key_mask(C_MAJOR) == 0xAB5, "C major's pitch classes");
    CHECK(th_key_mask((th_key_t){ 3, TH_MINOR }) == (0xAB5 | 1 << 8), "A minor's, with G#");
    CHECK(!th_minorish(C_MAJOR) && th_minorish((th_key_t){ 2, TH_DORIAN }), "dorian is minor-ish");
}

/* ------------------------------------------------------------------- chords --- */
static void test_chords(void)
{
    static const char *const C_TRIADS = "C Dm Em F G Am Bdim";
    static const char *const C_SEVENTHS = "Cmaj7 Dm7 Em7 Fmaj7 G7 Am7 Bm7b5";
    static const char *const C_RN3 = "I ii iii IV V vi viio";
    static const char *const C_RN7 = "IM7 ii7 iii7 IVM7 V7 vi7 vii/o7";
    static const char *const A_RN3 = "i iio bIII iv v bVI bVII";
    static const char *const A_RN7 = "i7 ii/o7 bIIIM7 iv7 v7 bVIM7 bVII7";
    static const char *const AH_RN3 = "i iio bIII+ iv V bVI viio";
    static const char *const AH_RN7 = "iM7 ii/o7 bIII+M7 iv7 V7 bVIM7 viio7";
    char names[160], rns[160], nash[160], b[TH_ROMAN_MAX];
    th_chord_t c;
    int8_t t[TH_TONES_MAX];
    int d, s, i, k;
    struct { th_key_t key; int sev; const char *names, *rns; } rows[] = {
        { { 0, TH_MAJOR }, 0, C_TRIADS, C_RN3 }, { { 0, TH_MAJOR }, 1, C_SEVENTHS, C_RN7 },
        { { 3, TH_MINOR }, 0, 0, A_RN3 }, { { 3, TH_MINOR }, 1, 0, A_RN7 },
        { { 3, TH_HARMONIC }, 0, 0, AH_RN3 }, { { 3, TH_HARMONIC }, 1, 0, AH_RN7 },
    };
    for (k = 0; k < (int)(sizeof rows / sizeof rows[0]); k++) {
        names[0] = rns[0] = 0;
        for (d = 0; d < 7; d++) {
            CHECK(th_diatonic(rows[k].key, d, rows[k].sev, &c), "row %d degree %d", k, d);
            if (d) { strcat(names, " "); strcat(rns, " "); }
            strcat(names, chord_str(c));
            strcat(rns, roman_str(rows[k].key, c));
        }
        if (rows[k].names) CHECK(!strcmp(names, rows[k].names), "row %d: %s", k, names);
        CHECK(!strcmp(rns, rows[k].rns), "row %d: %s", k, rns);
    }
    nash[0] = 0;
    for (d = 0; d < 7; d++) {
        th_diatonic(C_MAJOR, d, 0, &c);
        th_nashville_name(C_MAJOR, c, b);
        if (d) strcat(nash, " ");
        strcat(nash, b);
    }
    CHECK(!strcmp(nash, "1 2m 3m 4 5 6m 7o"), "Nashville in C: %s", nash);

    /* chord spellings */
    th_chord(F("F#"), TH_M7B5, t); CHECK(!strcmp(join(t, 4), "F# A C E"), "F#m7b5: %s", join(t, 4));
    th_chord(F("C#"), TH_DIM7, t); CHECK(!strcmp(join(t, 4), "C# E G Bb"), "C#dim7: %s", join(t, 4));
    th_chord(F("Db"), TH_MAJ7, t); CHECK(!strcmp(join(t, 4), "Db F Ab C"), "Dbmaj7: %s", join(t, 4));
    th_chord(F("E"), TH_AUG, t);   CHECK(!strcmp(join(t, 3), "E G# B#"), "Eaug: %s", join(t, 3));
    th_chord(F("G"), TH_9, t);     CHECK(!strcmp(join(t, 5), "G B D F A"), "G9: %s", join(t, 5));
    CHECK(th_qual_find("m7") == TH_M7 && th_qual_find("") == TH_MAJ && th_qual_find("M7") == TH_MAJ7, "symbols");
    CHECK(th_qual_find("o7") == TH_DIM7 && th_qual_find("nope") == -1, "aliases");
    for (i = 0; i < TH_NQUAL; i++) {
        CHECK(th_qual_find(th_quals[i].sym) == i, "symbol %s finds itself", th_quals[i].sym);
        for (s = 0; s < i; s++) CHECK(th_quals[s].n != th_quals[i].n || memcmp(th_quals[s].f, th_quals[i].f,
                                      (size_t)th_quals[i].n) != 0, "qualities %d and %d differ", s, i);
    }
    c.root = c.bass = (int8_t)F("C");
    c.q = TH_MAJ;
    c.bass = (int8_t)F("E");
    CHECK(!strcmp(chord_str(c), "C/E"), "slash: %s", chord_str(c));
}

/* ----------------------------------------------------------- Roman numerals --- */
static const char *prog_str(const char *src, th_key_t key)
{
    static char b[256];
    th_chord_t c[32];
    int n = th_prog_parse(src, key, c, 32), i;
    if (n < 0) return "(error)";
    b[0] = 0;
    for (i = 0; i < n; i++) {
        if (i) strcat(b, " ");
        strcat(b, chord_str(c[i]));
    }
    return b;
}

static void test_roman(void)
{
    static const struct { const char *src; int key; int scale; const char *want; } P[] = {
        { "IVM7 V7 iii7 vi", 0, TH_MAJOR, "Fmaj7 G7 Em7 Am" },          /* the royal road, 王道進行 */
        { "IVM7 V7 iii7 vi", -3, TH_MAJOR, "Abmaj7 Bb7 Gm7 Cm" },
        { "IVM7 V7 iii7 vi", 6, TH_MAJOR, "Bmaj7 C#7 A#m7 D#m" },
        { "4536251", 0, TH_MAJOR, "F G Em Am Dm G C" },
        { "4536251", -3, TH_MAJOR, "Ab Bb Gm Cm Fm Bb Eb" },
        { "4536251", 6, TH_MAJOR, "B C# A#m D#m G#m C# F#" },
        { "1 5 6 3 4 1 4 5", 0, TH_MAJOR, "C G Am Em F C F G" },        /* Canon */
        { "I-V-vi-IV", 2, TH_MAJOR, "D A Bm G" },
        { "vi IV I V", 0, TH_MAJOR, "Am F C G" },
        { "1645", 0, TH_MAJOR, "C Am F G" },
        { "ii7 V7 IM7", -2, TH_MAJOR, "Cm7 F7 Bbmaj7" },
        { "i-bVII-bVI-V", 3, TH_MINOR, "Am G F E" },                     /* Andalusian */
        { "i bVI bIII bVII", 4, TH_MINOR, "Em C G D" },
        { "V7/V V7 I", 0, TH_MAJOR, "D7 G7 C" },
        { "I V7/vi vi V/ii ii", 0, TH_MAJOR, "C E7 Am A Dm" },
        { "viio7/V V", 0, TH_MAJOR, "F#dim7 G" },
        { "vii/o7 iii", 0, TH_MAJOR, "Bm7b5 Em" },
        { "viiø7 vii°7", 0, TH_MAJOR, "Bm7b5 Bdim7" },
        { "b7 4 1", 0, TH_MAJOR, "Bb F C" },
        { "2m7 5 1maj7", 0, TH_MAJOR, "Dm7 G Cmaj7" },
        { "6m 4 1 5", 0, TH_MAJOR, "Am F C G" },
        { "Isus4 I Vsus2 Iadd9", 0, TH_MAJOR, "Csus4 C Gsus2 Cadd9" },
        { "bII bIII iv", 0, TH_MAJOR, "Db Eb Fm" },
        { "I+ i iM7", 0, TH_MAJOR, "Caug Cm Cm(maj7)" },
        { "I, IV | V", 0, TH_MAJOR, "C F G" },
    };
    static const char *const BAD[] = { "VX", "Vi", "IIII", "8", "Q", "V/", "V7x" };
    th_chord_t c, back;
    int i, k, d, s;
    for (i = 0; i < (int)(sizeof P / sizeof P[0]); i++) {
        th_key_t key = { (int8_t)P[i].key, (uint8_t)P[i].scale };
        const char *got = prog_str(P[i].src, key);
        CHECK(!strcmp(got, P[i].want), "%s: %s, not %s", P[i].src, got, P[i].want);
    }
    for (i = 0; i < (int)(sizeof BAD / sizeof BAD[0]); i++) {
        th_chord_t x[4];
        CHECK(th_prog_parse(BAD[i], C_MAJOR, x, 4) == -1, "%s is not a numeral", BAD[i]);
    }
    /* naming then reading gives the chord back: diatonic chords of every key, triads and sevenths */
    for (k = -7; k <= 7; k++)
        for (s = 0; s < 3; s++) {
            static const uint8_t SC[3] = { TH_MAJOR, TH_MINOR, TH_HARMONIC };
            th_key_t key = { (int8_t)k, SC[s] };
            for (d = 0; d < 7; d++) {
                int sev;
                for (sev = 0; sev < 2; sev++) {
                    char b[TH_ROMAN_MAX];
                    if (!th_diatonic(key, d, sev, &c)) continue;
                    th_roman_name(key, c, b);
                    CHECK(th_roman_parse(b, key, &back) == (int)strlen(b) && back.root == c.root && back.q == c.q,
                          "key %d %s step %d: %s reads back as %s", k, th_scales[key.scale].name, d, b, chord_str(back));
                }
            }
        }
    /* secondary dominants are named as such, and read back */
    {
        static const struct { const char *chord; const char *rn; } S[] = {
            { "D7", "V7/V" }, { "D", "V/V" }, { "E7", "V7/vi" }, { "E", "V/vi" }, { "A7", "V7/ii" },
            { "B7", "V7/iii" }, { "C7", "V7/IV" }, { "Bb", "bVII" }, { "Bb7", "bVII7" }, { "Ab", "bVI" },
            { "Fm", "iv" }, { "G7", "V7" }, { "F#m7b5", "#iv/o7" }, { "Db", "bII" },
        };
        for (i = 0; i < (int)(sizeof S / sizeof S[0]); i++) {
            const char *p = S[i].chord;
            int f, n = th_parse_name(p, &f);
            c.root = c.bass = (int8_t)f;
            c.q = (uint8_t)th_qual_find(p + n);
            CHECK(!strcmp(roman_str(C_MAJOR, c), S[i].rn), "%s in C: %s, not %s", S[i].chord,
                  roman_str(C_MAJOR, c), S[i].rn);
            CHECK(th_roman_parse(S[i].rn, C_MAJOR, &back) == (int)strlen(S[i].rn) && back.root == c.root
                  && back.q == c.q, "%s reads back as %s", S[i].rn, chord_str(back));
        }
        /* minor keys: E is V (the tonic is not a target), D is IV (not V/bVII) */
        {
            th_key_t am = { 3, TH_MINOR };
            c.root = c.bass = (int8_t)F("E"); c.q = TH_MAJ;
            CHECK(!strcmp(roman_str(am, c), "V"), "E in A minor: %s", roman_str(am, c));
            c.root = c.bass = (int8_t)F("D");
            CHECK(!strcmp(roman_str(am, c), "IV"), "D in A minor: %s", roman_str(am, c));
            c.root = c.bass = (int8_t)F("B");
            CHECK(!strcmp(roman_str(am, c), "V/v"), "B in A minor: %s", roman_str(am, c));
            c.root = c.bass = (int8_t)F("G#"); c.q = TH_DIM7;
            CHECK(!strcmp(roman_str(am, c), "viio7"), "G#dim7 in A minor: %s", roman_str(am, c));
        }
    }
}

/* ----------------------------------------------------------------- identify --- */
static const char *ident_str(const uint8_t *m, int n, th_key_t key)
{
    static char b[96];
    char r[TH_ROMAN_MAX];
    th_ident_t id;
    if (!th_identify(m, n, key, &id)) return "-";
    th_roman_name(key, id.c, r);
    snprintf(b, sizeof b, "%s %s inv%d%s%s", chord_str(id.c), r, id.inv, id.no5 ? " no5" : "", id.inkey ? "" : " out");
    return b;
}

static void test_identify(void)
{
    static const struct { int key, scale; uint8_t m[6]; int n; const char *want; } I[] = {
        { 0, TH_MAJOR, { 60, 64, 67 }, 3, "C I inv0" },
        { 0, TH_MAJOR, { 67, 60, 64 }, 3, "C I inv0" },                     /* order does not matter */
        { 0, TH_MAJOR, { 64, 67, 72 }, 3, "C/E I inv1" },
        { 0, TH_MAJOR, { 67, 72, 76 }, 3, "C/G I inv2" },
        { 0, TH_MAJOR, { 57, 60, 64, 67 }, 4, "Am7 vi7 inv0" },
        { 0, TH_MAJOR, { 60, 64, 67, 69 }, 4, "C6 Iadd6 inv0" },             /* same notes: the bass decides */
        { 0, TH_MAJOR, { 55, 59, 62, 65 }, 4, "G7 V7 inv0" },
        { 0, TH_MAJOR, { 59, 62, 65, 67 }, 4, "G7/B V7 inv1" },
        { 0, TH_MAJOR, { 62, 66, 69, 72 }, 4, "D7 V7/V inv0 out" },
        { 0, TH_MAJOR, { 58, 62, 65 }, 3, "Bb bVII inv0 out" },
        { 0, TH_MAJOR, { 59, 62, 65, 68 }, 4, "Bdim7 viio7 inv0 out" },
        { 0, TH_MAJOR, { 68, 71, 74, 77 }, 4, "G#dim7 #vo7 inv0 out" },
        { 0, TH_MAJOR, { 60, 63, 66, 69 }, 4, "Cdim7 io7 inv0 out" },
        { 0, TH_MAJOR, { 60, 64, 68 }, 3, "Caug I+ inv0 out" },
        { 0, TH_MAJOR, { 60, 64, 70 }, 3, "C7 V7/IV inv0 no5 out" },
        { 0, TH_MAJOR, { 59, 62, 65, 69 }, 4, "Bm7b5 vii/o7 inv0" },
        { 0, TH_MAJOR, { 53, 57, 60, 64 }, 4, "Fmaj7 IVM7 inv0" },
        { 0, TH_MAJOR, { 65, 68, 72 }, 3, "Fm iv inv0 out" },
        { 0, TH_MAJOR, { 56, 60, 63 }, 3, "Ab bVI inv0 out" },
        { 0, TH_MAJOR, { 61, 65, 68 }, 3, "Db bII inv0 out" },
        { 0, TH_MAJOR, { 66, 70, 73 }, 3, "F# #IV inv0 out" },
        { 0, TH_MAJOR, { 60, 65, 67 }, 3, "Csus4 Isus4 inv0" },
        { 0, TH_MAJOR, { 65, 67, 72 }, 3, "Fsus2 IVsus2 inv0" },             /* the same notes over F */
        { 0, TH_MAJOR, { 60, 62, 64, 67 }, 4, "Cadd9 Iadd9 inv0" },
        { 0, TH_MAJOR, { 55, 59, 62, 65, 69 }, 5, "G9 V9 inv0" },
        { 0, TH_MAJOR, { 60, 67 }, 2, "C5 I5 inv0" },
        { 0, TH_MAJOR, { 48, 60, 64, 67, 72 }, 5, "C I inv0" },              /* doubled notes */
        { 0, TH_MAJOR, { 60, 64 }, 2, "-" },                                 /* a third alone */
        { 0, TH_MAJOR, { 60 }, 1, "-" },
        { 0, TH_MAJOR, { 60, 61, 62 }, 3, "-" },
        { 2, TH_MAJOR, { 66, 69, 73 }, 3, "F#m iii inv0" },
        { -5, TH_MAJOR, { 66, 70, 73 }, 3, "Gb IV inv0" },                   /* same keys, spelled for Db */
        { -3, TH_MAJOR, { 62, 65, 68, 72 }, 4, "Dm7b5 vii/o7 inv0" },
        { 5, TH_MAJOR, { 61, 65, 68 }, 3, "C# V/V inv0 out" },               /* C# in B major, not Db */
        { 3, TH_MINOR, { 64, 68, 71 }, 3, "E V inv0" },
        { 3, TH_MINOR, { 68, 71, 74, 77 }, 4, "G#dim7 viio7 inv0" },          /* the raised 7th is in the key */
        { 3, TH_MINOR, { 60, 64, 67 }, 3, "C bIII inv0" },
        { 3, TH_MINOR, { 67, 71, 74 }, 3, "G bVII inv0" },
    };
    int i;
    for (i = 0; i < (int)(sizeof I / sizeof I[0]); i++) {
        th_key_t key = { (int8_t)I[i].key, (uint8_t)I[i].scale };
        const char *got = ident_str(I[i].m, I[i].n, key);
        CHECK(!strcmp(got, I[i].want), "identify %d: %s, not %s", i, got, I[i].want);
    }
    /* every diatonic triad and seventh of every major and minor key, in every inversion, is named as itself */
    {
        int k, s, d, sev, inv;
        for (k = -7; k <= 7; k++)
            for (s = 0; s < 2; s++) {
                th_key_t key = { (int8_t)(s ? k + 3 : k), (uint8_t)(s ? TH_MINOR : TH_MAJOR) };
                for (d = 0; d < 7; d++)
                    for (sev = 0; sev < 2; sev++) {
                        th_chord_t c;
                        int8_t t[4];
                        int n;
                        th_diatonic(key, d, sev, &c);
                        n = th_chord(c.root, c.q, t);
                        for (inv = 0; inv < n; inv++) {
                            uint8_t m[4];
                            th_ident_t id;
                            int j;
                            m[0] = (uint8_t)(48 + th_pc(t[inv]));    /* tone inv at the bottom, the rest above */
                            for (j = 1; j < n; j++) {
                                int x = m[j - 1] + 1;
                                while (x % 12 != th_pc(t[(inv + j) % n])) x++;
                                m[j] = (uint8_t)x;
                            }
                            CHECK(th_identify(m, n, key, &id), "key %d step %d sev %d inv %d found", key.tonic, d, sev, inv);
                            if (inv == 1 && (c.q == TH_M7 || c.q == TH_M7B5)) {  /* Am7/C is C6: the bass wins */
                                CHECK(id.c.root == c.root + th_quals[c.q].f[1] && id.c.q == (c.q == TH_M7 ? TH_6 : TH_M6)
                                      && id.inv == 0, "key %d step %d: %s", key.tonic, d, chord_str(id.c));
                                continue;
                            }
                            CHECK(id.c.root == c.root && id.c.q == c.q && id.inv == inv && id.inkey,
                                  "key %d %s step %d sev %d inv %d: %s", key.tonic, th_scales[key.scale].name, d, sev,
                                  inv, chord_str(id.c));
                        }
                    }
            }
    }
}

/* every reading, best first; chord-tone labels */
static void test_readings(void)
{
    static const uint8_t C6[4] = { 60, 64, 67, 69 }, BD7[4] = { 59, 62, 65, 68 }, AUG[3] = { 60, 64, 68 };
    static const uint8_t C7B9[5] = { 48, 52, 55, 58, 61 }, G7S9[5] = { 55, 59, 62, 65, 70 }, C13[5] = { 48, 52, 58, 62, 69 };
    static const uint8_t C69[5] = { 48, 52, 55, 57, 62 };
    th_ident_t r[16];
    char b[TH_NAME_MAX];
    int n, i, roots = 0;
    n = th_identify_all(C6, 4, C_MAJOR, r, 16);
    CHECK(n == 2 && !strcmp(chord_str(r[0].c), "C6"), "C E G A: C6 first (%d, %s)", n, chord_str(r[0].c));
    CHECK(n == 2 && !strcmp(chord_str(r[1].c), "Am7/C") && r[1].inv == 1, "then Am7/C (%s)", chord_str(r[1].c));
    n = th_identify_all(BD7, 4, C_MAJOR, r, 16);
    CHECK(n == 4 && !strcmp(chord_str(r[0].c), "Bdim7"), "B D F Ab: Bdim7 first (%d, %s)", n, chord_str(r[0].c));
    for (i = 0; i < n; i++) roots |= 1 << th_pc(r[i].c.root);
    CHECK(roots == (1 << 11 | 1 << 2 | 1 << 5 | 1 << 8), "and its four roots");
    CHECK(th_identify_all(AUG, 3, C_MAJOR, r, 16) == 3, "an augmented triad: three readings");
    CHECK(th_identify_all(AUG, 3, C_MAJOR, r, 1) == 1, "max is kept to");
    CHECK(th_identify(C7B9, 5, C_MAJOR, &r[0]) && !strcmp(chord_str(r[0].c), "C7b9"), "C7b9 (%s)", chord_str(r[0].c));
    CHECK(th_identify(G7S9, 5, C_MAJOR, &r[0]) && !strcmp(chord_str(r[0].c), "G7#9"), "G7#9 (%s)", chord_str(r[0].c));
    CHECK(th_identify(C13, 5, C_MAJOR, &r[0]) && !strcmp(chord_str(r[0].c), "C13") && r[0].no5, "C13 without its fifth (%s)",
          chord_str(r[0].c));
    CHECK(th_identify(C69, 5, C_MAJOR, &r[0]) && !strcmp(chord_str(r[0].c), "C6/9"), "C6/9 (%s)", chord_str(r[0].c));
    {
        static const struct { int q, d; const char *s; } L[] = {
            { TH_MAJ, 0, "R" }, { TH_MAJ, 4, "3" }, { TH_MIN, -3, "b3" }, { TH_MAJ, 1, "5" }, { TH_DIM, -6, "b5" },
            { TH_AUG, 8, "#5" }, { TH_7, -2, "b7" }, { TH_MAJ7, 5, "7" }, { TH_DIM7, -9, "bb7" }, { TH_9, 2, "9" },
            { TH_SUS2, 2, "2" }, { TH_SUS4, -1, "4" }, { TH_7B9, -5, "b9" }, { TH_7S9, 9, "#9" }, { TH_13, 3, "13" },
            { TH_6, 3, "6" },
        };
        for (i = 0; i < (int)(sizeof L / sizeof L[0]); i++) {
            th_tone_label(L[i].q, L[i].d, b);
            CHECK(!strcmp(b, L[i].s), "tone %d in %s: %s, not %s", L[i].d, th_quals[L[i].q].sym, b, L[i].s);
        }
    }
    /* the new qualities read back from Roman numerals */
    CHECK(!strcmp(prog_str("V7b9 V7#9 V13 I69", C_MAJOR), "G7b9 G7#9 G13 C6/9"), "%s", prog_str("V7b9 V7#9 V13 I69", C_MAJOR));
}

/* -------------------------------------------------------------------- voice --- */
static int all_moves(const uint8_t *prev, int np, th_chord_t c, int lo, int hi)  /* the least, found another way */
{
    int8_t t[TH_TONES_MAX];
    int n = th_chord(c.root, c.q, t), best = 1 << 30, bottom, inv;
    for (inv = 0; inv < n; inv++)
        for (bottom = lo; bottom <= hi; bottom++) {
            uint8_t v[TH_TONES_MAX];
            int j, ok = 1;
            if (th_pc(t[inv]) != bottom % 12) continue;
            v[0] = (uint8_t)bottom;
            for (j = 1; j < n; j++) {
                int x = v[j - 1] + 1;
                while (x % 12 != th_pc(t[(inv + j) % n])) x++;
                v[j] = (uint8_t)x;
            }
            if (v[n - 1] > hi) ok = 0;
            if (ok && th_movement(v, n, prev, np) < best) best = th_movement(v, n, prev, np);
        }
    return best;
}

static void test_voice(void)
{
    static const uint8_t C4[3] = { 60, 64, 67 };
    uint8_t out[TH_TONES_MAX];
    th_chord_t c;
    int n, i, k, lo = 53, hi = 79;
    uint32_t rng = 1;
    c.root = c.bass = (int8_t)F("F"); c.q = TH_MAJ;
    n = th_voice_near(C4, 3, c, lo, hi, out);
    CHECK(n == 3 && out[0] == 60 && out[1] == 65 && out[2] == 69, "C to F: C F A (%d %d %d)", out[0], out[1], out[2]);
    c.root = c.bass = (int8_t)F("G");
    n = th_voice_near(C4, 3, c, lo, hi, out);
    CHECK(n == 3 && out[0] == 59 && out[1] == 62 && out[2] == 67, "C to G: B D G (%d %d %d)", out[0], out[1], out[2]);
    c.root = c.bass = (int8_t)F("C");
    n = th_voice_near(0, 0, c, lo, hi, out);
    CHECK(n == 3 && out[0] >= lo && out[2] <= hi && out[0] + out[1] + out[2] - 3 * 66 <= 6
          && out[0] + out[1] + out[2] - 3 * 66 >= -6, "C on its own, near the middle (%d %d %d)", out[0], out[1], out[2]);
    CHECK(th_voice_near(0, 0, c, 60, 62, out) == 0, "no room for a chord");
    for (i = 0; i < 2000; i++) {         /* random chords after random voicings: never worse than another */
        uint8_t prev[TH_TONES_MAX];
        int np, j;
        rng = rng * 1103515245u + 12345u;
        c.root = c.bass = (int8_t)((int)(rng >> 8) % 15 - 7);
        c.q = (uint8_t)((rng >> 16) % TH_NQUAL);
        np = 2 + (int)((rng >> 24) % 3);
        for (j = 0; j < np; j++) { rng = rng * 1103515245u + 12345u; prev[j] = (uint8_t)(55 + (rng >> 16) % 20); }
        n = th_voice_near(prev, np, c, lo, hi, out);
        CHECK(n == th_quals[c.q].n, "random %d: a voicing", i);
        for (j = 0; j < n; j++) CHECK(out[j] >= lo && out[j] <= hi && (j == 0 || out[j] > out[j - 1]), "random %d: in range, rising", i);
        {
            int8_t t[TH_TONES_MAX];
            th_chord(c.root, c.q, t);
            for (k = 0; k < n; k++) {
                int found = 0;
                for (j = 0; j < n; j++) found |= th_pc(t[j]) == out[k] % 12;
                CHECK(found, "random %d: note %d is a chord tone", i, k);
            }
        }
        CHECK(th_movement(out, n, prev, np) == all_moves(prev, np, c, lo, hi), "random %d: the least movement", i);
    }
}

/* ------------------------------------------------------------ music21 vectors --- */
static void test_vectors(void)
{
    int8_t s[7], t[TH_TONES_MAX];
    unsigned i;
    for (i = 0; i < sizeof VEC_SCALES / sizeof VEC_SCALES[0]; i++) {
        int k, n;
        for (k = 0; k < TH_NSCALES && strcmp(th_scales[k].name, VEC_SCALES[i].scale); k++) {}
        CHECK(k < TH_NSCALES, "scale %s", VEC_SCALES[i].scale);
        if (k == TH_NSCALES) continue;
        n = th_scale(F(VEC_SCALES[i].tonic), k, s);
        CHECK(!strcmp(join(s, n), VEC_SCALES[i].notes), "%s %s: %s, music21 %s", VEC_SCALES[i].tonic,
              VEC_SCALES[i].scale, join(s, n), VEC_SCALES[i].notes);
    }
    for (i = 0; i < sizeof VEC_KEYSIGS / sizeof VEC_KEYSIGS[0]; i++) {
        int8_t sig[7];
        int c = th_keysig(F(VEC_KEYSIGS[i].tonic), VEC_KEYSIGS[i].minor ? TH_MINOR : TH_MAJOR, sig);
        CHECK(c == VEC_KEYSIGS[i].sharps, "%s %s: %d, music21 %d", VEC_KEYSIGS[i].tonic,
              VEC_KEYSIGS[i].minor ? "minor" : "major", c, VEC_KEYSIGS[i].sharps);
    }
    for (i = 0; i < sizeof VEC_CHORDS / sizeof VEC_CHORDS[0]; i++) {
        int q = th_qual_find(VEC_CHORDS[i].sym), n;
        CHECK(q >= 0, "symbol %s", VEC_CHORDS[i].sym);
        if (q < 0) continue;
        n = th_chord(F(VEC_CHORDS[i].root), q, t);
        CHECK(same_set(t, n, VEC_CHORDS[i].notes), "%s%s: %s, music21 %s", VEC_CHORDS[i].root, VEC_CHORDS[i].sym,
              join(t, n), VEC_CHORDS[i].notes);
    }
    for (i = 0; i < sizeof VEC_INTERVALS / sizeof VEC_INTERVALS[0]; i++) {
        char b[8];
        th_interval_name(F(VEC_INTERVALS[i].note), b);
        CHECK(!strcmp(b, VEC_INTERVALS[i].name), "C up to %s: %s, music21 %s", VEC_INTERVALS[i].note, b,
              VEC_INTERVALS[i].name);
    }
    for (i = 0; i < sizeof VEC_ROMAN / sizeof VEC_ROMAN[0]; i++) {
        th_key_t key = { (int8_t)F(VEC_ROMAN[i].key), TH_MAJOR };
        th_chord_t c;
        int n = th_roman_parse(VEC_ROMAN[i].rn, key, &c);
        CHECK(n == (int)strlen(VEC_ROMAN[i].rn), "%s in %s reads", VEC_ROMAN[i].rn, VEC_ROMAN[i].key);
        n = th_chord(c.root, c.q, t);
        CHECK(same_set(t, n, VEC_ROMAN[i].notes), "%s in %s: %s, music21 %s", VEC_ROMAN[i].rn, VEC_ROMAN[i].key,
              join(t, n), VEC_ROMAN[i].notes);
    }
}

int main(void)
{
    test_notes();
    test_scales();
    test_chords();
    test_roman();
    test_identify();
    test_readings();
    test_voice();
    test_vectors();
    printf("%s: %d checks, %d failed\n", fails ? "FAIL" : "ok", checks, fails);
    return fails != 0;
}
