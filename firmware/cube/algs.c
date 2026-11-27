/* SPDX-License-Identifier: GPL-3.0-or-later */
/* algs: see algs.h. The formulas are the widely taught ones (OLL and PLL by their usual numbers and names); every
 * one is checked by tests/cube_test.c: it parses, it keeps what its set keeps, and the cases of a set are all
 * different, so a set of 57 OLL, 21 PLL or 41 F2L covers every case. */
#include "algs.h"

static const alg_t ALGS[] = {
    /* ---- beginner's layer by layer: one formula a step */
    {ALG_LBL, "CORNER: WHITE RIGHT", "R U R'", 0},
    {ALG_LBL, "CORNER: WHITE FRONT", "F' U' F", 0},
    {ALG_LBL, "CORNER: WHITE UP", "R U2 R' U' R U R'", 0},
    {ALG_LBL, "EDGE TO THE RIGHT", "U R U' R' U' F' U F", 0},
    {ALG_LBL, "EDGE TO THE LEFT", "U' L' U L U F U' F'", 0},
    {ALG_LBL, "YELLOW CROSS", "F R U R' U' F'", 0},
    {ALG_LBL, "YELLOW FACE (SUNE)", "R U R' U R U2 R'", 0},
    {ALG_LBL, "YELLOW CORNERS: PLACE", "U R U' L' U R' U' L", 0},
    {ALG_LBL, "YELLOW CORNERS: TURN", "R' D' R D R' D' R D", 0},
    {ALG_LBL, "YELLOW EDGES: CYCLE", "R U' R U R U R U' R' U' R2", 0},

    /* ---- 2-look OLL: the edges (3), then the corners (7) */
    {ALG_OLL2, "EDGES: LINE", "F R U R' U' F'", 0},
    {ALG_OLL2, "EDGES: L", "f R U R' U' f'", 0},
    {ALG_OLL2, "EDGES: DOT", "F R U R' U' F' f R U R' U' f'", 0},
    {ALG_OLL2, "SUNE", "R U R' U R U2 R'", 0},
    {ALG_OLL2, "ANTISUNE", "R U2 R' U' R U' R'", 0},
    {ALG_OLL2, "H", "R U2 R' U' R U R' U' R U' R'", "F R U R' U' R U R' U' R U R' U' F'"},
    {ALG_OLL2, "PI", "R U2 R2 U' R2 U' R2 U2 R", "f R U R' U' f' F R U R' U' F'"},
    {ALG_OLL2, "HEADLIGHTS", "R2 D' R U2 R' D R U2 R", 0},
    {ALG_OLL2, "CHAMELEON", "r U R' U' r' F R F'", 0},
    {ALG_OLL2, "BOWTIE", "F' r U R' U' r' F R", "y' R' F R B' R' F' R B"},

    /* ---- 2-look PLL: the corners (2), then the edges (4) */
    {ALG_PLL2, "CORNERS: ADJACENT", "R U R' U' R' F R2 U' R' U' R U R' F'", "y x R' U R' D2 R U' R' D2 R2 x'"},
    {ALG_PLL2, "CORNERS: DIAGONAL", "F R U' R' U' R U R' F' R U R' U' R' F R F'", "R U R' U' R' F R2 U' R' U' R U R' F' U2 R U R' U' R' F R2 U' R' U' R U R' F'"},
    {ALG_PLL2, "EDGES: UA", "M2 U M U2 M' U M2", "R U' R U R U R U' R' U' R2"},
    {ALG_PLL2, "EDGES: UB", "M2 U' M U2 M' U' M2", "R2 U R U R' U' R' U' R' U R'"},
    {ALG_PLL2, "EDGES: H", "M2 U M2 U2 M2 U M2", "R2 U2 R U2 R2 U2 R2 U2 R U2 R2"},
    {ALG_PLL2, "EDGES: Z", "M' U M2 U M2 U M' U2 M2", "R' U' R U' R U R U' R' U R U R2 U' R'"},

    /* ---- OLL 1 .. 57 */
    {ALG_OLL, "OLL 1", "R U2 R2 F R F' U2 R' F R F'", "R U B' R B R2 U' R' F R F'"},
    {ALG_OLL, "OLL 2", "F R U R' U' F' f R U R' U' f'", "y r U r' U2 r U2 R' U2 R U' r'"},
    {ALG_OLL, "OLL 3", "f R U R' U' f' U' F R U R' U' F'", "y r' R2 U R' U r U2 r' U M'"},
    {ALG_OLL, "OLL 4", "f R U R' U' f' U F R U R' U' F'", "y M U' r U2 r' U' R U' R' M'"},
    {ALG_OLL, "OLL 5", "r' U2 R U R' U r", "y2 l' U2 L U L' U l"},
    {ALG_OLL, "OLL 6", "r U2 R' U' R U' r'", "y2 l U2 L' U' L U' l'"},
    {ALG_OLL, "OLL 7", "r U R' U R U2 r'", "F R' F' R U2 R U2 R'"},
    {ALG_OLL, "OLL 8", "r' U' R U' R' U2 r", "y2 l' U' L U' L' U2 l"},
    {ALG_OLL, "OLL 9", "R U R' U' R' F R2 U R' U' F'", "y R' U' R U' R' U R' F R F' U R"},
    {ALG_OLL, "OLL 10", "R U R' U R' F R F' R U2 R'", 0},
    {ALG_OLL, "OLL 11", "r U R' U R' F R F' R U2 r'", "y2 M R U R' U R U2 R' U M'"},
    {ALG_OLL, "OLL 12", "F R U R' U' F' U F R U R' U' F'", "y' M' R' U' R U' R' U2 R U' M"},
    {ALG_OLL, "OLL 13", "F U R U' R2 F' R U R U' R'", "r U' r' U' r U r' F' U F"},
    {ALG_OLL, "OLL 14", "R' F R U R' F' R F U' F'", 0},
    {ALG_OLL, "OLL 15", "r' U' r R' U' R U r' U r", "y2 l' U' l L' U' L U l' U l"},
    {ALG_OLL, "OLL 16", "r U r' R U R' U' r U' r'", 0},
    {ALG_OLL, "OLL 17", "R U R' U R' F R F' U2 R' F R F'", 0},
    {ALG_OLL, "OLL 18", "r U R' U R U2 r2 U' R U' R' U2 r", 0},
    {ALG_OLL, "OLL 19", "r' R U R U R' U' M' R' F R F'", "M U R U R' U' M' R' F R F'"},
    {ALG_OLL, "OLL 20", "r U R' U' M2 U R U' R' U' M'", 0},
    {ALG_OLL, "OLL 21", "R U2 R' U' R U R' U' R U' R'", "y R U R' U R U' R' U R U2 R' | F R U R' U' R U R' U' R U R' U' F'"},
    {ALG_OLL, "OLL 22", "R U2 R2 U' R2 U' R2 U2 R", "f R U R' U' f' F R U R' U' F'"},
    {ALG_OLL, "OLL 23", "R2 D' R U2 R' D R U2 R", "y2 R2 D R' U2 R D' R' U2 R'"},
    {ALG_OLL, "OLL 24", "r U R' U' r' F R F'", 0},
    {ALG_OLL, "OLL 25", "F' r U R' U' r' F R", "y' R' F R B' R' F' R B"},
    {ALG_OLL, "OLL 26", "R U2 R' U' R U' R'", "y L' U' L U' L' U2 L"},
    {ALG_OLL, "OLL 27", "R U R' U R U2 R'", 0},
    {ALG_OLL, "OLL 28", "r U R' U' r' R U R U' R'", "r U R' U' M U R U' R'"},
    {ALG_OLL, "OLL 29", "R U R' U' R U' R' F' U' F R U R'", "y' r2 D' r U r' D r2 U' r' U' r"},
    {ALG_OLL, "OLL 30", "F R' F R2 U' R' U' R U R' F2", "F U R U2 R' U' R U2 R' U' F'"},
    {ALG_OLL, "OLL 31", "R' U' F U R U' R' F' R", 0},
    {ALG_OLL, "OLL 32", "L U F' U' L' U L F L'", "y2 S R U R' U' R' F R f'"},
    {ALG_OLL, "OLL 33", "R U R' U' R' F R F'", "F R U' R' U R U R' F'"},
    {ALG_OLL, "OLL 34", "R U R2 U' R' F R U R U' F'", 0},
    {ALG_OLL, "OLL 35", "R U2 R2 F R F' R U2 R'", 0},
    {ALG_OLL, "OLL 36", "L' U' L U' L' U L U L F' L' F", 0},
    {ALG_OLL, "OLL 37", "F R' F' R U R U' R'", "F R U' R' U' R U R' F'"},
    {ALG_OLL, "OLL 38", "R U R' U R U' R' U' R' F R F'", 0},
    {ALG_OLL, "OLL 39", "L F' L' U' L U F U' L'", "y2 R U R' F' U' F U R U2 R'"},
    {ALG_OLL, "OLL 40", "R' F R U R' U' F' U R", 0},
    {ALG_OLL, "OLL 41", "R U R' U R U2 R' F R U R' U' F'", 0},
    {ALG_OLL, "OLL 42", "R' U' R U' R' U2 R F R U R' U' F'", "y R' F R F' R' F R F' R U R' U' R U R'"},
    {ALG_OLL, "OLL 43", "f' L' U' L U f", "y R' U' F' U F R"},
    {ALG_OLL, "OLL 44", "f R U R' U' f'", "y2 F U R U' R' F'"},
    {ALG_OLL, "OLL 45", "F R U R' U' F'", 0},
    {ALG_OLL, "OLL 46", "R' U' R' F R F' U R", 0},
    {ALG_OLL, "OLL 47", "R' U' R' F R F' R' F R F' U R", "F' L' U' L U L' U' L U F"},
    {ALG_OLL, "OLL 48", "F R U R' U' R U R' U' F'", 0},
    {ALG_OLL, "OLL 49", "r U' r2 U r2 U r2 U' r", "y2 R B' R2 F R2 B R2 F' R"},
    {ALG_OLL, "OLL 50", "r' U r2 U' r2 U' r2 U r'", 0},
    {ALG_OLL, "OLL 51", "f R U R' U' R U R' U' f'", "y2 F U R U' R' U R U' R' F'"},
    {ALG_OLL, "OLL 52", "R U R' U R U' B U' B' R'", "R' U' R U' R' U F' U F R"},
    {ALG_OLL, "OLL 53", "r' U' R U' R' U R U' R' U2 r", "y' l' U2 L U L' U' L U L' U l"},
    {ALG_OLL, "OLL 54", "r U R' U R U' R' U R U2 r'", "y' r U2 R' U' R U R' U' R U' r'"},
    {ALG_OLL, "OLL 55", "R U2 R2 U' R U' R' U2 F R F'", "y R' F R U R U' R2 F' R2 U' R' U R U R'"},
    {ALG_OLL, "OLL 56", "r U r' U R U' R' U R U' R' r U' r'", "F R U R' U' R F' r U R' U' r'"},
    {ALG_OLL, "OLL 57", "R U R' U' M' U R U' r'", "R U R' U' r R' U R U' r'"},

    /* ---- PLL: the 21 */
    {ALG_PLL, "AA", "x R' U R' D2 R U' R' D2 R2 x'", "y' x L2 D2 L' U' L D2 L' U L' x' | R' F R' B2 R F' R' B2 R2"},
    {ALG_PLL, "AB", "x R2 D2 R U R' D2 R U' R x'", "y2 x' L2 D2 L U L' D2 L U' L x | y R B' R F2 R' B R F2 R2"},
    {ALG_PLL, "E", "x' R U' R' D R U R' D' R U R' D R U' R' D' x", "y R2 U R' U' y R U R' U' R U R' U' R U R' y' R U' R2"},
    {ALG_PLL, "F", "R' U' F' R U R' U' R' F R2 U' R' U' R U R' U R", "y' R' U R U' R2 F' U' F U R F R' F' R2"},
    {ALG_PLL, "GA", "R2 U R' U R' U' R U' R2 U' D R' U R D'", "R2 u R' U R' U' R u' R2 y' R' U R"},
    {ALG_PLL, "GB", "R' U' R U D' R2 U R' U R U' R U' R2 D", "R' U' R y R2 u R' U R U' R u' R2"},
    {ALG_PLL, "GC", "R2 U' R U' R U R' U R2 U D' R U' R' D", "R2 u' R U' R U R' u R2 y R U' R'"},
    {ALG_PLL, "GD", "R U R' U' D R2 U' R U' R' U R' U R2 D'", "R U R' y' R2 u' R U' R' U R' u R2"},
    {ALG_PLL, "H", "M2 U M2 U2 M2 U M2", "R2 U2 R U2 R2 U2 R2 U2 R U2 R2"},
    {ALG_PLL, "JA", "x R2 F R F' R U2 r' U r U2 x'", "y2 L' U' L F L' U' L U L F' L2 U L | y' R' U L' U2 R U' R' U2 R L"},
    {ALG_PLL, "JB", "R U R' F' R U R' U' R' F R2 U' R'", "R U2 R' U' R U2 L' U R' U' L"},
    {ALG_PLL, "NA", "R U R' U R U R' F' R U R' U' R' F R2 U' R' U2 R U' R'", "L U' R U2 L' U R' L U' R U2 L' U R'"},
    {ALG_PLL, "NB", "R' U R U' R' F' U' F R U R' F R' F' R U' R", "R' U L' U2 R U' L R' U L' U2 R U' L"},
    {ALG_PLL, "RA", "R U' R' U' R U R D R' U' R D' R' U2 R'", "R U R' F' R U2 R' U2 R' F R U R U2 R'"},
    {ALG_PLL, "RB", "R2 F R U R U' R' F' R U2 R' U2 R", "y' R' U2 R U2 R' F R U R' U' R' F' R2"},
    {ALG_PLL, "T", "R U R' U' R' F R2 U' R' U' R U R' F'", "F R U' R' U R U R2 F' R U R U' R'"},
    {ALG_PLL, "UA", "M2 U M U2 M' U M2", "R U' R U R U R U' R' U' R2 | y2 R2 U' R' U' R U R U R U' R"},
    {ALG_PLL, "UB", "M2 U' M U2 M' U' M2", "R2 U R U R' U' R' U' R' U R' | y2 R' U R' U' R' U' R' U R U R2"},
    {ALG_PLL, "V", "R' U R' U' B' R' B2 U' B' U B' R B R", "R' U R' U' y R' F' R2 U' R' U R' F R F"},
    {ALG_PLL, "Y", "F R U' R' U' R U R' F' R U R' U' R' F R F'", "R2 U' R2 U' R2 U R' F' R U R2 U' R' F R"},
    {ALG_PLL, "Z", "M' U M2 U M2 U M' U2 M2", "y M2 U M2 U M' U2 M2 U2 M' | R' U' R U' R U R U' R' U R U R2 U' R'"},

    /* ---- F2L: the front-right pair, every case */
    {ALG_F2L, "F2L 1", "U R U' R'", "R' F R F'"},
    {ALG_F2L, "F2L 2", "U' F' U F", "F R' F' R"},
    {ALG_F2L, "F2L 3", "F' U' F", 0},
    {ALG_F2L, "F2L 4", "R U R'", 0},
    {ALG_F2L, "F2L 5", "U' R U R' U2 R U' R'", 0},
    {ALG_F2L, "F2L 6", "U F' U' F U2 F' U F", 0},
    {ALG_F2L, "F2L 7", "U' R U2 R' U2 R U' R'", 0},
    {ALG_F2L, "F2L 8", "U F' U2 F U2 F' U F", 0},
    {ALG_F2L, "F2L 9", "U F' U F U' F' U' F", 0},
    {ALG_F2L, "F2L 10", "U' R U R' U R U R'", 0},
    {ALG_F2L, "F2L 11", "U' R U2 R' U F' U' F", 0},
    {ALG_F2L, "F2L 12", "R U' R' U R U' R' U2 R U' R'", 0},
    {ALG_F2L, "F2L 13", "R U' R' F' U' F", 0},
    {ALG_F2L, "F2L 14", "U' R U' R' U R U R'", 0},
    {ALG_F2L, "F2L 15", "F' U F U2 R U R'", 0},
    {ALG_F2L, "F2L 16", "R U' R' U2 F' U' F", 0},
    {ALG_F2L, "F2L 17", "R U2 R' U' R U R'", 0},
    {ALG_F2L, "F2L 18", "F' U2 F U F' U' F", 0},
    {ALG_F2L, "F2L 19", "U R U2 R' U R U' R'", 0},
    {ALG_F2L, "F2L 20", "U' F' U2 F U' F' U F", 0},
    {ALG_F2L, "F2L 21", "U2 R U R' U R U' R'", 0},
    {ALG_F2L, "F2L 22", "U2 F' U' F U' F' U F", 0},
    {ALG_F2L, "F2L 23", "U R U' R' U' R U' R' U R U' R'", 0},
    {ALG_F2L, "F2L 24", "U' F' U F U F' U F U' F' U F", 0},
    {ALG_F2L, "F2L 25", "U' R' F R F' R U R'", 0},
    {ALG_F2L, "F2L 26", "U R U' R' U' F' U F", 0},
    {ALG_F2L, "F2L 27", "R U' R' U R U' R'", 0},
    {ALG_F2L, "F2L 28", "F' U F U' F' U F", 0},
    {ALG_F2L, "F2L 29", "F' U' F U R U' R'", 0},
    {ALG_F2L, "F2L 30", "R U R' U' R U R'", 0},
    {ALG_F2L, "F2L 31", "R U' R' U F' U F", 0},
    {ALG_F2L, "F2L 32", "R U R' U' R U R' U' R U R'", 0},
    {ALG_F2L, "F2L 33", "U' R U' R' U2 R U' R'", 0},
    {ALG_F2L, "F2L 34", "U R U R' U2 R U R'", 0},
    {ALG_F2L, "F2L 35", "U2 R U R' F R' F' R", 0},
    {ALG_F2L, "F2L 36", "F' U' F U' F' U' F", 0},
    {ALG_F2L, "F2L 37", "R2 U2 F R2 F' U2 R' U R'", 0},
    {ALG_F2L, "F2L 38", "R U' R' U' R U R' U2 R U' R'", 0},
    {ALG_F2L, "F2L 39", "R U' R' U R U2 R' U R U' R'", 0},
    {ALG_F2L, "F2L 40", "R F U R U' R' F' U' R'", 0},
    {ALG_F2L, "F2L 41", "R U F R U R' U' F' R'", 0},

    /* ---- the cross: the white-green edge home, the other three cross edges kept (found by a search for the
     * shortest, so the case pictures come out right) */
    {ALG_CROSS, "TOP, WHITE UP", "F2", 0},
    {ALG_CROSS, "TOP, WHITE FRONT", "U' R' F R", 0},
    {ALG_CROSS, "TOP RIGHT, WHITE RIGHT", "R' F R", 0},
    {ALG_CROSS, "MIDDLE RIGHT, WHITE RIGHT", "F", 0},
    {ALG_CROSS, "MIDDLE LEFT, WHITE LEFT", "F'", 0},
    {ALG_CROSS, "MIDDLE RIGHT, WHITE FRONT", "D R' D'", 0},
    {ALG_CROSS, "BOTTOM, FLIPPED", "D' L' D F'", 0},

    /* ---- patterns: the formula done on a solved cube (not undone) */
    {ALG_PAT, "CHECKERBOARD", "M2 E2 S2", "R2 L2 U2 D2 F2 B2"},
    {ALG_PAT, "SIX SPOTS", "U D' R L' F B' U D'", 0},
    {ALG_PAT, "CUBE IN A CUBE", "F L F U' R U F2 L2 U' L' B D' B' L2 U", 0},
    {ALG_PAT, "SUPERFLIP", "U R2 F B R B2 R U2 L B2 R U' D' R2 F R' L B2 U2 F2", 0},
    {ALG_PAT, "TETRIS", "L R F B U' D' L' R'", 0},
    {ALG_PAT, "ANACONDA", "L U B' U' R L' B R' F B' D R D' F'", 0},
    {ALG_PAT, "CROSSES AND STRIPES", "U2 R2 L2 U2 R2 L2", 0},
};

#define NALGS ((int)(sizeof ALGS / sizeof ALGS[0]))
static const char *const SET_NAME[ALG_SETS] = {"LAYER BY LAYER", "2-LOOK OLL", "2-LOOK PLL", "OLL", "PLL", "F2L",
                                              "CROSS", "PATTERNS"};
static const char *const SET_SHORT[ALG_SETS] = {"LBL", "2LOOK OLL", "2LOOK PLL", "OLL", "PLL", "F2L", "CROSS", "PATTERN"};

int alg_total(void) { return NALGS; }
const alg_t *alg_get(int i) { return &ALGS[i < 0 || i >= NALGS ? 0 : i]; }
const char *alg_set_name(int s) { return SET_NAME[s >= 0 && s < ALG_SETS ? s : 0]; }
const char *alg_set_short(int s) { return SET_SHORT[s >= 0 && s < ALG_SETS ? s : 0]; }

int alg_count(int set)
{
    int i, n = 0;
    for (i = 0; i < NALGS; i++) n += ALGS[i].set == set;
    return n;
}

int alg_index(int set, int k)
{
    int i;
    for (i = 0; i < NALGS; i++)
        if (ALGS[i].set == set && k-- == 0) return i;
    return -1;
}

int alg_pos(int i)
{
    int j, k = 0;
    for (j = 0; j < i; j++) k += ALGS[j].set == ALGS[i].set;
    return k;
}

int alg_moves(int i, uint8_t *out) { return cube_parse(alg_get(i)->alg, out, ALG_MAX); }

int alg_forms(int i)
{
    const char *p = alg_get(i)->alt;
    int n = 1;
    if (!p || !*p) return 1;
    for (n = 2; *p; p++) n += *p == '|';
    return n > ALG_FORMS ? ALG_FORMS : n;
}

int alg_form_text(int i, int k, char *out, int max)
{
    const char *p = k == 0 ? alg_get(i)->alg : alg_get(i)->alt;
    int n = 0;
    if (!p) return out[0] = 0, 0;
    for (; k > 1 && *p; p++)             /* skip k - 1 bars */
        if (*p == '|') k--;
    if (k > 1) return out[0] = 0, 0;
    while (*p == ' ') p++;
    while (*p && *p != '|' && n < max - 1) out[n++] = *p++;
    while (n && out[n - 1] == ' ') n--;
    out[n] = 0;
    return n;
}

int alg_form(int i, int k, uint8_t *out)
{
    char t[160];
    if (k >= alg_forms(i) || !alg_form_text(i, k, t, sizeof t)) return -1;
    return cube_parse(t, out, ALG_MAX);
}

int alg_setup(int i, uint8_t *out, int auf)
{
    int n = alg_moves(i, out);
    if (n < 0) return -1;
    if (alg_get(i)->set == ALG_PAT) return n;   /* a pattern: the formula itself, from solved */
    cube_invert(out, n, out);
    if (auf & 3) out[n++] = MV(MB_U, auf & 3);
    return cube_merge(out, n);
}

int alg_case(int i, cube_t *c, int auf)
{
    uint8_t m[ALG_MAX + 1];
    int n = alg_setup(i, m, auf);
    cube_reset(c);
    if (n < 0) return -1;
    cube_apply(c, m, n);
    return n;
}

/* ------------------------------------------------------------------ checks --- */
static int home(const cube_t *c, int i) { return c->f[i] == i / 9; }

int alg_f2l_kept(const cube_t *c) { return cube_f2l_solved(c); }

int alg_cross_kept(const cube_t *c)
{
    static const uint8_t KEEP[] = {31, 28, 30, 32, 34, 25, 16, 52, 43,   /* centre D, the cross */
                                   27, 44, 24, 21, 41,                   /* the FL pair */
                                   33, 53, 42, 50, 39,                   /* BL */
                                   35, 17, 51, 48, 14,                   /* BR */
                                   4, 13, 22, 40, 49};                   /* the other centres */
    int i;
    for (i = 0; i < (int)sizeof KEEP; i++)
        if (!home(c, KEEP[i])) return 0;
    return 1;
}

int alg_ll_oriented(const cube_t *c)
{
    int i;
    for (i = 0; i < 9; i++)
        if (c->f[i] != c->f[4]) return 0;
    return 1;
}

int alg_solved_auf(const cube_t *c) { return cube_solved_turn(c); }

static int upright(const cube_t *c, cube_t *o)   /* c turned whole so that U and F are where they belong */
{
    static const char *const TILT[6] = {"", "x", "x2", "x'", "z", "z'"};
    int t, y;
    uint8_t m[4];
    for (t = 0; t < 6; t++)
        for (y = 0; y < 4; y++) {
            int n = cube_parse(TILT[t], m, 4);
            *o = *c;
            cube_apply(o, m, n);
            if (y) cube_move(o, MV(MB_y, y));
            if (o->f[4] == CF_U && o->f[22] == CF_F) return 1;
        }
    return 0;
}

static int corners_placed(const cube_t *c)   /* the last layer's corners where they belong, up to a U turn */
{
    static const uint8_t SIDE_CORNER[8] = {18, 20, 9, 11, 45, 47, 36, 38};
    int a, i;
    for (a = 0; a < 4; a++) {
        cube_t t = *c;
        if (a) cube_move(&t, MV(MB_U, a));
        for (i = 0; i < 8 && t.f[SIDE_CORNER[i]] == SIDE_CORNER[i] / 9; i++) {}
        if (i == 8) return 1;
    }
    return 0;
}

int alg_goal_met(int i, const cube_t *c)
{
    cube_t u;
    int set = alg_get(i)->set, a;
    if (!upright(c, &u)) return 0;
    switch (set) {
    case ALG_OLL:
    case ALG_OLL2:
        for (a = 0; a < 4; a++) {        /* (a last U turn changes nothing here, D or a slice turn might) */
            cube_t t = u;
            if (a) cube_move(&t, MV(MB_U, a));
            if (cube_f2l_solved(&t) && alg_ll_oriented(&t)) return 1;
        }
        return cube_solved_turn(&u);
    case ALG_F2L: return cube_f2l_solved(&u) || cube_solved_turn(&u);
    case ALG_CROSS: {
        static const uint8_t CROSS[] = {28, 30, 32, 34, 25, 16, 52, 43};
        int k;
        for (k = 0; k < 8 && u.f[CROSS[k]] == CROSS[k] / 9; k++) {}
        return k == 8;
    }
    case ALG_PAT: return 1;
    case ALG_PLL2:
        if (alg_pos(i) < 2) return cube_f2l_solved(&u) && alg_ll_oriented(&u) && corners_placed(&u);
        return cube_solved_turn(&u);
    default: return cube_solved_turn(&u);
    }
}

/* a case's fingerprint, the same from any side and with any last-layer turn */
static const uint8_t LL_SIDE[12] = {18, 19, 20, 9, 10, 11, 45, 46, 47, 36, 37, 38};   /* F R B L top rows */
static const uint8_t SIDE_OF[6] = {0xFF, 1, 0, 0xFF, 3, 2};   /* U R F D L B -> F 0, R 1, B 2, L 3 */
static const uint8_t AC_CORNER[8][3] = {{8, 9, 20}, {6, 18, 38}, {0, 36, 47}, {2, 45, 11},
                                        {29, 26, 15}, {27, 44, 24}, {33, 53, 42}, {35, 17, 51}};
static const uint8_t AC_EDGE[12][2] = {{5, 10}, {7, 19}, {3, 37}, {1, 46}, {32, 16}, {28, 25},
                                       {30, 43}, {34, 52}, {23, 12}, {21, 41}, {50, 39}, {48, 14}};

static uint32_t print_oll(const cube_t *c)
{
    uint32_t v = 0;
    int i;
    for (i = 0; i < 9; i++) v = v << 1 | (c->f[i] == CF_U);
    for (i = 0; i < 12; i++) v = v << 1 | (c->f[LL_SIDE[i]] == CF_U);
    return v;
}

static uint32_t print_pll(const cube_t *c, int shift)
{
    uint32_t v = 0;
    int i;
    for (i = 0; i < 12; i++) {
        int s = SIDE_OF[c->f[LL_SIDE[i]]], f = i / 3;
        v = v * 4 + (s == 0xFF ? 0 : (uint32_t)((s - f + shift + 8) % 4));
    }
    return v;
}

static uint32_t print_f2l(const cube_t *c)
{
    int i, o, cp = 0, co = 0, ep = 0, eo = 0;
    for (i = 0; i < 8; i++)
        for (o = 0; o < 3; o++)
            if (c->f[AC_CORNER[i][o]] == CF_D && c->f[AC_CORNER[i][(o + 1) % 3]] == CF_F)
                cp = i, co = o;
    for (i = 0; i < 12; i++)
        for (o = 0; o < 2; o++)
            if (c->f[AC_EDGE[i][o]] == CF_F && c->f[AC_EDGE[i][1 - o]] == CF_R) ep = i, eo = o;
    return (uint32_t)(((cp * 3 + co) * 12 + ep) * 2 + eo);
}

uint32_t alg_print_state(int set, const cube_t *c0)   /* a state's case fingerprint for set */
{
    cube_t c = *c0;
    uint32_t best = 0xFFFFFFFFu;
    int a, s;
    if (set == ALG_LBL || set >= ALG_CROSS) {   /* the whole cube, as it is */
        uint32_t h = 2166136261u;
        for (a = 0; a < 54; a++) h = (h ^ c.f[a]) * 16777619u;
        return h;
    }
    for (a = 0; a < 4; a++) {
        uint32_t v;
        if (set == ALG_OLL || set == ALG_OLL2) v = print_oll(&c);
        else if (set == ALG_PLL || set == ALG_PLL2)
            for (s = 0, v = 0xFFFFFFFFu; s < 4; s++) {
                uint32_t w = print_pll(&c, s);
                if (w < v) v = w;
            }
        else v = print_f2l(&c);
        if (v < best) best = v;
        cube_move(&c, MV(MB_U, 1));
    }
    return best;
}

static uint32_t fingerprint(int i, uint32_t *lbl)
{
    cube_t c;
    alg_case(i, &c, 0);
    *lbl = alg_print_state(alg_get(i)->set, &c);
    return *lbl;
}

int alg_same_case(int a, int b)
{
    uint32_t x, y;
    if (alg_get(a)->set != alg_get(b)->set) return 0;
    return fingerprint(a, &x) == fingerprint(b, &y);
}

/* ------------------------------------------------------- groups and order --- */
/* OLL and PLL by the shapes they are recognised by (the usual groups; OLL by number, PLL by position in the set) */
static const uint8_t G_DOT[] = {1, 2, 3, 4, 17, 18, 19, 20}, G_LINE[] = {51, 52, 55, 56},
                     G_CROSS[] = {21, 22, 23, 24, 25, 26, 27}, G_CORN[] = {28, 57}, G_SQUARE[] = {5, 6},
                     G_SLIGHT[] = {7, 8, 11, 12}, G_FISH[] = {9, 10, 35, 37}, G_KNIGHT[] = {13, 14, 15, 16},
                     G_AWK[] = {29, 30, 41, 42}, G_P[] = {31, 32, 43, 44}, G_T[] = {33, 45}, G_C[] = {34, 46},
                     G_W[] = {36, 38}, G_BLIGHT[] = {39, 40}, G_L[] = {47, 48, 49, 50, 53, 54};
/* PLL positions: AA 0 AB 1 E 2 F 3 GA 4 GB 5 GC 6 GD 7 H 8 JA 9 JB 10 NA 11 NB 12 RA 13 RB 14 T 15 UA 16 UB 17 V 18
 * Y 19 Z 20 */
static const uint8_t P_EDGE[] = {17, 18, 9, 21}, P_CORN[] = {1, 2, 3}, P_ADJ[] = {16, 4, 10, 11, 14, 15},
                     P_DIAG[] = {19, 20, 12, 13}, P_G[] = {5, 6, 7, 8};   /* (1-based) */
typedef struct {
    uint8_t set;
    const char *name;
    const uint8_t *items;                /* 1-based positions in the set; 0: the whole set */
    uint8_t n;
} alg_group_t;
#define GRP(set, name, a) {set, name, a, (uint8_t)sizeof a}
static const alg_group_t GROUPS[] = {
    {ALG_LBL, "BEGINNER: LAYER BY LAYER", 0, 0},
    {ALG_CROSS, "1 CROSS", 0, 0},
    {ALG_F2L, "2 F2L", 0, 0},
    {ALG_OLL2, "3 OLL: 2-LOOK", 0, 0},
    GRP(ALG_OLL, "3 OLL: CROSS (OCLL)", G_CROSS), GRP(ALG_OLL, "3 OLL: T SHAPES", G_T),
    GRP(ALG_OLL, "3 OLL: P SHAPES", G_P), GRP(ALG_OLL, "3 OLL: FISH", G_FISH),
    GRP(ALG_OLL, "3 OLL: SQUARES", G_SQUARE), GRP(ALG_OLL, "3 OLL: C SHAPES", G_C),
    GRP(ALG_OLL, "3 OLL: W SHAPES", G_W), GRP(ALG_OLL, "3 OLL: L SHAPES", G_L),
    GRP(ALG_OLL, "3 OLL: LINES", G_LINE), GRP(ALG_OLL, "3 OLL: SMALL LIGHTNING", G_SLIGHT),
    GRP(ALG_OLL, "3 OLL: BIG LIGHTNING", G_BLIGHT), GRP(ALG_OLL, "3 OLL: KNIGHT MOVE", G_KNIGHT),
    GRP(ALG_OLL, "3 OLL: AWKWARD", G_AWK), GRP(ALG_OLL, "3 OLL: CORNERS DONE", G_CORN),
    GRP(ALG_OLL, "3 OLL: DOT", G_DOT),
    {ALG_PLL2, "4 PLL: 2-LOOK", 0, 0},
    GRP(ALG_PLL, "4 PLL: EDGES ONLY", P_EDGE), GRP(ALG_PLL, "4 PLL: CORNERS ONLY", P_CORN),
    GRP(ALG_PLL, "4 PLL: SIDE SWAP", P_ADJ), GRP(ALG_PLL, "4 PLL: DIAGONAL SWAP", P_DIAG),
    GRP(ALG_PLL, "4 PLL: G PERMS", P_G),
    {ALG_PAT, "PATTERNS", 0, 0},
};
#define NGROUPS ((int)(sizeof GROUPS / sizeof GROUPS[0]))

int alg_groups(void) { return NGROUPS; }
const char *alg_group_name(int g) { return GROUPS[g < 0 || g >= NGROUPS ? 0 : g].name; }
int alg_group_set(int g) { return GROUPS[g < 0 || g >= NGROUPS ? 0 : g].set; }
int alg_group_count(int g)
{
    const alg_group_t *a = &GROUPS[g < 0 || g >= NGROUPS ? 0 : g];
    return a->items ? a->n : alg_count(a->set);
}
int alg_group_item(int g, int k)         /* the k-th case of group g: its entry index */
{
    const alg_group_t *a = &GROUPS[g < 0 || g >= NGROUPS ? 0 : g];
    if (k < 0 || k >= alg_group_count(g)) return -1;
    return alg_index(a->set, a->items ? a->items[k] - 1 : k);
}
int alg_group_of(int i)                  /* the group entry i is shown in */
{
    int g, k;
    for (g = 0; g < NGROUPS; g++)
        for (k = 0; k < alg_group_count(g); k++)
            if (alg_group_item(g, k) == i) return g;
    return 0;
}

/* how often a case comes up after the step before it, out of den: every last-layer state counted
 * (tests/cube_test.c counts them again). 0: not counted (F2L, the cross, patterns, the beginner's steps) */
int alg_freq(int i, int *den)
{
    const alg_t *a = alg_get(i);
    int p = alg_pos(i);
    *den = 1;
    if (a->set == ALG_OLL) {
        *den = 216;
        if (p == 19) return 1;           /* OLL 20 */
        if (p == 0 || p == 20 || p == 54 || p == 55 || p == 56) return 2;   /* OLL 1, 21, 55, 56, 57 */
        return 4;
    }
    if (a->set == ALG_PLL) {
        *den = 288;
        if (p == 8 || p == 11 || p == 12) return 4;   /* H, Na, Nb */
        if (p == 2 || p == 20) return 8;              /* E, Z */
        return 16;
    }
    return 0;
}

/* the order to learn a set in: the most frequent first, the shortest of equals first (positions in the set) */
int alg_learn_order(int set, uint8_t *out)
{
    int n = alg_count(set), k, j;
    int key[SR_ORDER_MAX];
    for (k = 0; k < n && k < SR_ORDER_MAX; k++) {
        int i = alg_index(set, k), den, f = alg_freq(i, &den), len;
        uint8_t m[ALG_MAX];
        len = alg_moves(i, m);
        key[k] = (f ? (1000 - f * 1000 / den) : 0) * 64 + len;   /* (frequency down, length up) */
        out[k] = (uint8_t)k;
    }
    for (k = 1; k < n; k++) {            /* insertion sort, stable */
        uint8_t t = out[k];
        for (j = k; j > 0 && key[out[j - 1]] > key[t]; j--) out[j] = out[j - 1];
        out[j] = t;
    }
    return n;
}
