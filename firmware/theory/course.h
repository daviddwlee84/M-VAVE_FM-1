/* SPDX-License-Identifier: GPL-3.0-or-later */
/* course: THEORY's flash-card items, in the order they open. APPEND ONLY: progress records, backups and the host's
 * JSON (host/theory/progress.py, which builds the same list) name an item by its place in this list.
 *
 * Keys open one at a time, spreading out from C on both sides of the circle as graded scale syllabuses do
 * (C, G, F, D, Bb, A, Eb, E, Ab, B, Db, F#). Each key brings its scale, key signature (not C), relative minor,
 * chords by degree, and degrees by ear; chord qualities by ear open with C (major, minor), D (diminished) and A
 * (augmented). Needs theory.h and srs.h first. */
#pragma once

enum { SK_SCALE, SK_KEYSIG, SK_RELMIN, SK_CHORDS, SK_EARDEG, SK_EARQ, SK_N };
enum { SEG_THEORY, SEG_EAR };

#define CO_KEYS 12
#define CO_N 63
static const int8_t CO_TONIC[CO_KEYS] = { 0, 1, -1, 2, -2, 3, -3, 4, -4, 5, -5, 6 };   /* C G F D Bb A Eb E Ab B Db F# */

static uint8_t co_skill[CO_N], co_key[CO_N], co_param[CO_N], co_seg[CO_N], co_vary[CO_N], co_group_end[CO_KEYS];
static sr_course_t course;

static void co_add(int *n, int skill, int key, int param)
{
    co_skill[*n] = (uint8_t)skill;
    co_key[*n] = (uint8_t)key;
    co_param[*n] = (uint8_t)param;
    co_seg[*n] = (uint8_t)(skill >= SK_EARDEG ? SEG_EAR : SEG_THEORY);
    co_vary[*n] = (uint8_t)(skill == SK_CHORDS || skill == SK_EARDEG || skill == SK_EARQ);   /* a new question each time */
    (*n)++;
}

static void course_init(void)
{
    int n = 0, g;
    for (g = 0; g < CO_KEYS; g++) {
        co_add(&n, SK_SCALE, g, 0);
        if (g) co_add(&n, SK_KEYSIG, g, 0);
        co_add(&n, SK_RELMIN, g, 0);
        co_add(&n, SK_CHORDS, g, 0);
        co_add(&n, SK_EARDEG, g, 0);
        if (g == 0) {
            co_add(&n, SK_EARQ, SR_NOKEY, TH_MAJ);
            co_add(&n, SK_EARQ, SR_NOKEY, TH_MIN);
        }
        if (g == 3) co_add(&n, SK_EARQ, SR_NOKEY, TH_DIM);
        if (g == 5) co_add(&n, SK_EARQ, SR_NOKEY, TH_AUG);
        co_group_end[g] = (uint8_t)n;
    }
    course.n = (uint8_t)n;
    course.seg = co_seg;
    course.key = co_key;
    course.vary = co_vary;
    course.group_end = co_group_end;
    course.ngroups = CO_KEYS;
    course.unlock_box = 2;
}
