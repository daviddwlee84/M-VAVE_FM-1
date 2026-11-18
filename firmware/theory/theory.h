/* SPDX-License-Identifier: GPL-3.0-or-later */
/* theory: the music theory a pocket trainer needs, worked out instead of stored. Scales, chords, key signatures,
 * the circle of fifths, Roman numerals and naming a chord someone plays, in every key, spelled the way a score
 * spells them (F# in D major, Gb in Db major, E# in C# major).
 *
 * Every spelled note is a position f on the line of fifths: C = 0, G = 1, D = 2, F = -1, Bb = -2, F# = 6, Gb = -6.
 * A step of +7 is a sharp, -7 a flat, +12 the next enharmonic spelling (B# = 12 sounds as C). From f:
 *   pitch class   7f mod 12
 *   letter        "FCGDAEB"[(f + 1) mod 7]
 *   accidental    floor((f + 1) / 7)   (+1 a sharp, -1 a flat)
 * Scales and chords are lists of offsets on that line from their root (a major third is +4, a minor third -3,
 * a fifth +1), so transposing is adding, and one letter per scale step comes out by itself.
 *
 * Roman numerals: the case says the quality (V major, vi minor), the accidental is measured against the major
 * scale on the same tonic, in minor keys too (pop and Wikipedia's habit: in A minor, G is bVII, C is bIII, G#dim
 * is viio). ASCII by default (o diminished, /o half-diminished, as music21 writes them); an app with its own
 * glyphs defines TH_GLYPH_SHARP, _FLAT, _DIM, _HDIM before including this file.
 *
 * No platform calls, no libc, no floating point (tests/theory_test.c runs it on a computer: just theory-unit). */
#pragma once
#include <stdint.h>

#ifndef TH_GLYPH_SHARP
#define TH_GLYPH_SHARP "#"
#endif
#ifndef TH_GLYPH_FLAT
#define TH_GLYPH_FLAT "b"
#endif
#ifndef TH_GLYPH_DIM
#define TH_GLYPH_DIM "o"
#endif
#ifndef TH_GLYPH_HDIM
#define TH_GLYPH_HDIM "/o"
#endif

#define TH_NAME_MAX 16                   /* a note name, with room for multi-byte accidental glyphs */
#define TH_CHORD_MAX 40                  /* a chord name: root, symbol, slash bass */
#define TH_ROMAN_MAX 40                  /* a Roman numeral with a secondary target */
#define TH_TONES_MAX 6                   /* notes in a chord at most (a 13th: R 3 5 b7 9 13) */

enum {                                   /* th_scales[] */
    TH_MAJOR, TH_DORIAN, TH_PHRYGIAN, TH_LYDIAN, TH_MIXOLYDIAN, TH_MINOR, TH_LOCRIAN,
    TH_HARMONIC,                         /* harmonic minor */
    TH_MELODIC,                          /* melodic minor, ascending (jazz minor) */
    TH_PENTA,                            /* major pentatonic */
    TH_MPENTA,                           /* minor pentatonic */
    TH_BLUES,                            /* minor pentatonic with the flat fifth */
    TH_NSCALES
};

enum {                                   /* th_quals[]; the order is the order of preference when naming */
    TH_MAJ, TH_MIN, TH_DIM, TH_AUG, TH_SUS4, TH_SUS2,
    TH_7, TH_MAJ7, TH_M7, TH_M7B5, TH_DIM7, TH_MMAJ7, TH_6, TH_M6, TH_7SUS4, TH_ADD9, TH_MADD9,
    TH_9, TH_MAJ9, TH_M9, TH_AUG7, TH_AUGMAJ7,
    TH_69, TH_7B9, TH_7S9,               /* 6/9, 7b9, 7#9 */
    TH_13,                               /* R 3 5 b7 9 13, the 11th left out as it usually is */
    TH_5,                                /* power chord: root and fifth */
    TH_NQUAL
};

typedef struct {
    const char *name;
    uint8_t n;                           /* notes: 7, 6 or 5 */
    int8_t sig;                          /* key signature relative to the tonic: dorian -2 (D dorian has none) */
    int8_t f[7];                         /* fifths from the tonic, by scale step */
} th_scale_t;

typedef struct {
    const char *sym;                     /* chord symbol after the root: "m7", "maj7", "dim" */
    const char *rn;                      /* after a Roman numeral: "7", "M7", TH_GLYPH_DIM */
    uint8_t lower;                       /* the numeral is lower case (minor, diminished) */
    uint8_t n;
    int8_t f[TH_TONES_MAX];              /* fifths from the root: root, third, fifth, seventh, ninth, 13th */
} th_qual_t;

typedef struct { int8_t tonic; uint8_t scale; } th_key_t;
typedef struct { int8_t root; uint8_t q; int8_t bass; } th_chord_t;   /* bass == root in root position */

typedef struct {
    th_chord_t c;
    uint8_t inv;                         /* which chord tone is in the bass: 0 root, 1 third, 2 fifth, 3 seventh */
    uint8_t no5;                         /* a seventh or ninth chord played without its fifth */
    uint8_t inkey;                       /* every note belongs to the key (minor keys: the raised 7th too) */
} th_ident_t;

extern const th_scale_t th_scales[TH_NSCALES];
extern const th_qual_t th_quals[TH_NQUAL];

int th_mod(int a, int m);                /* 0..m-1 for negative a too */
int th_fdiv(int a, int m);               /* floor division */

/* ---------------------------------------------------------------- notes --- */
int th_pc(int f);                        /* pitch class 0..11, C = 0 */
int th_letter(int f);                    /* 0..6 = C D E F G A B */
int th_acc(int f);                       /* sharps (+) or flats (-) */
int th_from(int letter, int acc);        /* the f of a letter and accidental */
int th_name(int f, char *buf);           /* "F#", "Bb", "F##"; returns the length */
int th_parse_name(const char *s, int *f);    /* "Eb", "f#", "Bbb", "Cx": characters read, 0 if none */
int th_spell_pc(int pc, int tonic);      /* a pitch class spelled for a key: Db Eb F# Ab Bb around C */
int th_midi(int f, int octave);          /* C4 = 60; B#3 = 60, Cb4 = 59 */
int th_octave(int f, int midi);          /* the octave number of that spelling of that note */
int th_note_name(int midi, int tonic, char *buf);    /* "C#4", spelled for the key */
int th_interval_name(int d, char *buf);  /* "M3", "d5", "A4", "P1" for d fifths apart (simple intervals) */

/* ---------------------------------------------------------- scales, keys --- */
int th_scale(int tonic, int scale, int8_t *out);     /* the spelled notes; returns how many */
int th_keysig(int tonic, int scale, int8_t *out);    /* +sharps / -flats; out: the accidentals in order (at most 7) */
int th_key_acc(th_key_t key, int letter);            /* that letter's accidental in the key signature */
int th_key_mask(th_key_t key);                       /* pitch classes of the key, bit 0 = C (minor: + raised 7th) */
int th_minorish(th_key_t key);                       /* the key's third is minor */
int th_circle_pos(int f);                            /* 0..11 clockwise from C at the top */
int th_circle_tonic(int pos, int minor);             /* the key at a circle position: F# major, D# minor at 6 */
int th_key_for_pc(int pc, int scale);                /* the usual tonic for a pitch class: Db, F#; Eb minor, G# minor */

/* ---------------------------------------------------------------- chords --- */
int th_chord(int root, int q, int8_t *out);          /* the spelled notes, root first; returns how many */
int th_chord_name(th_chord_t c, char *buf);          /* "Am7", "C/E", "F#m7b5" */
int th_qual_find(const char *sym);                   /* TH_* for a chord symbol ("m7"), -1 if unknown */
int th_diatonic(th_key_t key, int degree, int sevenths, th_chord_t *out);    /* degree 0..6; 0 if not one */
int th_degree_name(th_key_t key, int f, char *buf);  /* "3", "b7", "#4": a note as a degree of the major scale */
int th_roman_name(th_key_t key, th_chord_t c, char *buf);    /* "IVM7", "viio", "bVII", "V7/V" (ignores the bass) */
int th_nashville_name(th_key_t key, th_chord_t c, char *buf);    /* "4", "6m", "b7", "2m7" */
int th_roman_parse(const char *s, th_key_t key, th_chord_t *out);    /* one numeral; characters read, 0 on error */
int th_prog_parse(const char *s, th_key_t key, th_chord_t *out, int max);    /* chords read, -1 on error */

/* What chord is this? Notes as MIDI numbers in any order. 1 and *out when one is found, 0 when not (one note,
 * a lone third). Prefers, in order: every note fits without leaving the fifth out, the root in the bass, a root
 * in the key, the simpler chord. So C E G A is C6 over C and Am7 over A. */
int th_identify(const uint8_t *midi, int n, th_key_t key, th_ident_t *out);
/* Every reading of the notes, best first (th_identify's is out[0]): C E G A gives C6, then Am7/C; a dim7 chord its
 * four roots. Returns how many were written, at most max. */
int th_identify_all(const uint8_t *midi, int n, th_key_t key, th_ident_t *out, int max);
/* What a chord tone is in its chord, from its fifths above the root: "R", "b3", "3", "5", "b7", "9", "#11", "13";
 * "2" in a sus2 chord, "4" in a sus4. */
int th_tone_label(int q, int d, char *buf);

/* A close voicing of c between lo and hi (MIDI) that moves least from prev (np notes, any order); np = 0 picks
 * the one nearest the middle. Returns how many notes, 0 if none fits. */
int th_voice_near(const uint8_t *prev, int np, th_chord_t c, int lo, int hi, uint8_t *out);
