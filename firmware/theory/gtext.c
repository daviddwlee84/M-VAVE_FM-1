/* SPDX-License-Identifier: GPL-3.0-or-later */
/* gtext: see gtext.h. */
#include "gtext.h"

typedef struct {
    uint8_t w, h, top;                    /* size, and rows down from the top of a 16 px line */
    uint8_t rows[11];                     /* bit (w-1-x) of row y lit */
} gt_glyph_t;

static const gt_glyph_t GT_GLYPHS[4] = {
    /* sharp: two upright strokes, two rising bars */
    { 7, 11, 2, { 0x14, 0x14, 0x17, 0x7C, 0x14, 0x14, 0x17, 0x7C, 0x14, 0x14, 0x14 } },
    /* flat: a stem and a bowl below */
    { 5, 11, 2, { 0x10, 0x10, 0x10, 0x10, 0x16, 0x19, 0x11, 0x11, 0x12, 0x14, 0x18 } },
    /* degree sign */
    { 5, 5, 2, { 0x0E, 0x11, 0x11, 0x11, 0x0E } },
    /* o with a stroke */
    { 6, 6, 2, { 0x0D, 0x12, 0x15, 0x19, 0x12, 0x2C } },
};

static int gt_scale(const felucca_font_t *f) { return f->h >= 24u ? 2 : 1; }

static int32_t gt_glyph(int32_t x, int32_t y, const felucca_font_t *f, int g, uint16_t c)
{
    const gt_glyph_t *d = &GT_GLYPHS[g];
    int s = gt_scale(f), r, b, i, j;
    int32_t y0 = y + (int32_t)d->top * s + (f->h >= 24u ? 1 : (f->h < 15u ? -2 : 0));
    for (r = 0; r < d->h; r++)
        for (b = 0; b < d->w; b++)
            if (d->rows[r] >> (d->w - 1 - b) & 1u)
                for (j = 0; j < s; j++)
                    for (i = 0; i < s; i++) cv_pset(x + b * s + i, y0 + r * s + j, c);
    return x + (d->w + 1) * s;
}

static int gt_marker(char ch) { return ch >= 1 && ch <= 4 ? ch - 1 : -1; }

static int32_t gtext(int32_t x, int32_t y, const felucca_font_t *f, const char *s, uint16_t c)
{
    char run[48];
    uint32_t n = 0;
    for (;; s++) {
        int g = gt_marker(*s);
        if (g >= 0 || !*s || n == sizeof run - 1u) {
            if (n) {
                run[n] = 0;
                x = cv_text(x, y, f, run, c);
                n = 0;
            }
            if (!*s) return x;
            if (g >= 0) x = gt_glyph(x, y, f, g, c);
            else run[n++] = *s;
        } else
            run[n++] = *s;
    }
}

static int32_t gtext_w(const felucca_font_t *f, const char *s)
{
    char run[48];
    uint32_t n = 0;
    int32_t w = 0;
    for (;; s++) {
        int g = gt_marker(*s);
        if (g >= 0 || !*s || n == sizeof run - 1u) {
            if (n) {
                run[n] = 0;
                w += text_w(f, run);
                n = 0;
            }
            if (!*s) return w;
            if (g >= 0) w += (GT_GLYPHS[g].w + 1) * gt_scale(f);
            else run[n++] = *s;
        } else
            run[n++] = *s;
    }
}

/* the same text in ASCII (# b o /o), for the screen reader's words and the simulator's checks */
static void gt_ascii(const char *s, char *out, uint32_t max)
{
    static const char *const SUB[4] = { "#", "b", "o", "/o" };
    uint32_t n = 0;
    for (; *s && n + 3u < max; s++) {
        int g = gt_marker(*s);
        const char *p = g >= 0 ? SUB[g] : 0;
        if (p) while (*p) out[n++] = *p++;
        else out[n++] = *s;
    }
    out[n] = 0;
}
