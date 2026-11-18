/* SPDX-License-Identifier: GPL-3.0-or-later */
/* gtext: text with music signs. fm1-base's fonts are ASCII, so theory.c is built with its glyph strings set to
 * marker bytes (GT_SHARP .. GT_HDIM, below); gtext draws the text between them with cv_text and each marker as a
 * small bitmap sized to the font: 1x for the 12-16 px fonts, 2x for M and L. Part of the unity build (gfx.c in
 * scope). */
#pragma once
#include <stdint.h>

#define GT_SHARP "\x01"
#define GT_FLAT "\x02"
#define GT_DIM "\x03"                     /* the degree sign of a diminished chord */
#define GT_HDIM "\x04"                    /* o with a stroke: half-diminished */

/* gtext(x, y, &FONT_B, "F\x01m7", colour) -> the x after it; gtext_w the width it takes; gtext_wrap(x, y, w, lh, ...)
 * the same broken into lines w wide at the spaces -> the y below it */
