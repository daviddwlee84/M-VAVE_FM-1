/* SPDX-License-Identifier: GPL-3.0-or-later */
/* view: the cube on the screen. Part of the unity build after gfx.c (cv_rect in scope).
 *
 * 3D: each sticker is a square on its face, turned with the layer being animated, then through the camera (a 3x3
 * rotation: x right, y up, z towards the eye) and an orthographic projection. Stickers facing away are dropped; the
 * rest are painted far to near, which is enough for a convex cube and for one layer part-way through a turn. The
 * quads are built once a frame (view_build) and filled band by band (view_draw), as fm1-base's canvas is a band.
 * 2D: the net (U above F, then L F R B, D below) and the last layer from above with its side stickers (OLL, PLL). */
#include "cube.h"
#include "fastmath.h"

/* sticker colours by face (U R F D L B): white, red, green, yellow, orange, blue */
static const uint16_t VIEW_RGB[6] = {RGB(240, 240, 240), RGB(220, 40, 40), RGB(30, 190, 70),
                                     RGB(250, 215, 30), RGB(255, 135, 20), RGB(40, 100, 230)};
#define VIEW_GREY RGB(70, 70, 78)
/* the colour each face's stickers show: the scramble's (white up, green front) and the formula book's (yellow up,
 * green front: the cube turned z2, as the first two layers are solved white down) */
static const uint8_t SCHEME_WHITE[6] = {0, 1, 2, 3, 4, 5};
static const uint8_t SCHEME_YELLOW[6] = {3, 4, 2, 0, 1, 5};

typedef struct {
    float m[9];                          /* row-major: screen = m * world */
} mat3_t;

static void m3_ident(mat3_t *a)
{
    int i;
    for (i = 0; i < 9; i++) a->m[i] = (i % 4 == 0) ? 1.0f : 0.0f;
}

static void m3_mul(const mat3_t *a, const mat3_t *b, mat3_t *o)   /* o = a b */
{
    int r, c;
    for (r = 0; r < 3; r++)
        for (c = 0; c < 3; c++)
            o->m[r * 3 + c] = a->m[r * 3] * b->m[c] + a->m[r * 3 + 1] * b->m[3 + c] + a->m[r * 3 + 2] * b->m[6 + c];
}

static void m3_rot(mat3_t *a, int axis, float deg)   /* a rotation about a world axis, counter-clockwise from its tip */
{
    float r = deg * 0.017453293f, c = fm_cosf(r), s = fm_sinf(r);
    m3_ident(a);
    if (axis == 0) a->m[4] = c, a->m[5] = -s, a->m[7] = s, a->m[8] = c;
    else if (axis == 1) a->m[0] = c, a->m[2] = s, a->m[6] = -s, a->m[8] = c;
    else a->m[0] = c, a->m[1] = -s, a->m[3] = s, a->m[4] = c;
}

static void m3_turn(mat3_t *cam, int axis, float deg)   /* turn the view about a screen axis */
{
    mat3_t r, o;
    float n;
    int i;
    m3_rot(&r, axis, deg);
    m3_mul(&r, cam, &o);
    /* keep it a rotation: normalise the rows, the third as the cross of the first two */
    for (i = 0; i < 2; i++) {
        float *v = &o.m[i * 3];
        if (i == 1) {                    /* row 1 minus its part along row 0 */
            float d = v[0] * o.m[0] + v[1] * o.m[1] + v[2] * o.m[2];
            v[0] -= d * o.m[0], v[1] -= d * o.m[1], v[2] -= d * o.m[2];
        }
        n = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
        n = n > 0.0f ? fm_sqrtf(n) : 1.0f;
        v[0] /= n, v[1] /= n, v[2] /= n;
    }
    o.m[6] = o.m[1] * o.m[5] - o.m[2] * o.m[4];
    o.m[7] = o.m[2] * o.m[3] - o.m[0] * o.m[5];
    o.m[8] = o.m[0] * o.m[4] - o.m[1] * o.m[3];
    *cam = o;
}

static void m3_view(mat3_t *cam, float yaw, float pitch)   /* yaw about y, then pitch about x */
{
    mat3_t a, b;
    m3_rot(&a, 1, yaw);
    m3_rot(&b, 0, pitch);
    m3_mul(&b, &a, cam);
}

/* the presets K4 steps through: from the front-right above, front-left, back-right, back-left, then from below */
#define VIEW_PRESETS 6
static const int16_t VIEW_YAW[VIEW_PRESETS] = {-35, 35, -145, 145, -35, 35};
static const int8_t VIEW_PITCH[VIEW_PRESETS] = {28, 28, 28, 28, -28, -28};
static const char *const VIEW_NAME[VIEW_PRESETS] = {"FRONT RIGHT", "FRONT LEFT", "BACK RIGHT", "BACK LEFT",
                                                    "BELOW RIGHT", "BELOW LEFT"};

typedef struct {
    int16_t x[4], y[4];                  /* screen, 1/16 pixel */
    int16_t z;                           /* depth (bigger: nearer), for the order */
    uint16_t c;
} vquad_t;

static vquad_t vq[54];
static uint8_t vq_order[54];
static int vq_n;

static void xform(const mat3_t *cam, const float *p, float *o)
{
    o[0] = cam->m[0] * p[0] + cam->m[1] * p[1] + cam->m[2] * p[2];
    o[1] = cam->m[3] * p[0] + cam->m[4] * p[1] + cam->m[5] * p[2];
    o[2] = cam->m[6] * p[0] + cam->m[7] * p[1] + cam->m[8] * p[2];
}

/* the quads of cube c seen through cam, centred at (cx, cy), unit = scale pixels; the layers of base anim turned
 * by deg degrees (anim < 0: none); scheme: the colour of each face */
static void view_build(const cube_t *c, const mat3_t *cam, int cx, int cy, float scale, int anim, float deg,
                       const uint8_t *scheme)
{
    mat3_t turn, full;
    uint64_t layer = 0;
    int i, j, axis = 0, sign = 1;
    if (anim >= 0) {
        layer = cube_move_layer(anim, &axis, &sign);
        m3_rot(&turn, axis, deg * (float)sign);
        m3_mul(cam, &turn, &full);
    }
    vq_n = 0;
    for (i = 0; i < 54; i++) {
        int8_t p[3], n[3];
        float ctr[3], u[3] = {0, 0, 0}, v[3] = {0, 0, 0}, nn[3], tn[3], pt[3], sp[3];
        const mat3_t *m = (layer >> i & 1) ? &full : cam;
        int ax, k;
        vquad_t *q;
        cube_sticker_pos(i, p, n);
        for (k = 0; k < 3; k++) {
            ctr[k] = (float)p[k] + 0.5f * (float)n[k];
            nn[k] = (float)n[k];
        }
        xform(m, nn, tn);
        if (tn[2] < 0.04f) continue;     /* faces away */
        ax = n[0] ? 0 : n[1] ? 1 : 2;
        u[(ax + 1) % 3] = 0.44f;
        v[(ax + 2) % 3] = 0.44f;
        q = &vq[vq_n];
        for (k = 0; k < 4; k++) {
            float su = (k == 0 || k == 1) ? 1.0f : -1.0f, sv = (k == 0 || k == 3) ? 1.0f : -1.0f;
            for (j = 0; j < 3; j++) pt[j] = ctr[j] + su * u[j] + sv * v[j];
            xform(m, pt, sp);
            q->x[k] = (int16_t)((float)cx * 16.0f + sp[0] * scale * 16.0f);
            q->y[k] = (int16_t)((float)cy * 16.0f - sp[1] * scale * 16.0f);
        }
        xform(m, ctr, sp);
        q->z = (int16_t)(sp[2] * 1000.0f);
        q->c = VIEW_RGB[scheme[c->f[i] % 6]];
        vq_order[vq_n] = (uint8_t)vq_n;
        vq_n++;
    }
    for (i = 1; i < vq_n; i++) {         /* far to near */
        uint8_t t = vq_order[i];
        for (j = i; j > 0 && vq[vq_order[j - 1]].z > vq[t].z; j--) vq_order[j] = vq_order[j - 1];
        vq_order[j] = t;
    }
}

/* a convex quad filled, in the band of canvas rows that starts at screen row oy and is h tall */
static void fill_quad(const vquad_t *q, int32_t oy, int32_t h)
{
    int32_t ymin = 0x7FFF, ymax = -0x7FFF, k, y;
    for (k = 0; k < 4; k++) {
        if (q->y[k] < ymin) ymin = q->y[k];
        if (q->y[k] > ymax) ymax = q->y[k];
    }
    ymin = (ymin + 15) >> 4;
    ymax = ymax >> 4;
    if (ymin < oy) ymin = oy;
    if (ymax > oy + h - 1) ymax = oy + h - 1;
    for (y = ymin; y <= ymax; y++) {
        int32_t yc = y * 16 + 8, xl = 0x7FFFFF, xr = -0x7FFFFF;
        for (k = 0; k < 4; k++) {
            int32_t x0 = q->x[k], y0 = q->y[k], x1 = q->x[(k + 1) & 3], y1 = q->y[(k + 1) & 3], x;
            if ((yc < y0) == (yc < y1)) continue;   /* the row does not cross this edge */
            x = x0 + (x1 - x0) * (yc - y0) / (y1 - y0);
            if (x < xl) xl = x;
            if (x > xr) xr = x;
        }
        if (xl > xr) continue;
        xl = (xl + 8) >> 4;
        xr = (xr + 8) >> 4;
        if (xr > xl) cv_rect(xl, y - oy, xr - xl, 1, q->c);
    }
}

static void view_draw(int32_t oy, int32_t h)
{
    int i;
    for (i = 0; i < vq_n; i++) fill_quad(&vq[vq_order[i]], oy, h);
}

/* the net, top-left at (x, y) in band coordinates, each sticker s pixels (with a 1-pixel gap) */
static void view_net(const cube_t *c, int32_t x, int32_t y, int32_t s, const uint8_t *scheme)
{
    static const int8_t AT[6][2] = {{3, 0}, {6, 3}, {3, 3}, {3, 6}, {0, 3}, {9, 3}};   /* U R F D L B, in cells */
    int f, i;
    for (f = 0; f < 6; f++)
        for (i = 0; i < 9; i++)
            cv_rect(x + (AT[f][0] + i % 3) * s, y + (AT[f][1] + i / 3) * s, s - 1, s - 1,
                    VIEW_RGB[scheme[c->f[f * 9 + i] % 6]]);
}

/* the last layer from above, the 3x3 at (x, y) of s pixels with the side stickers as thin strips around it;
 * oll: only which stickers are the top colour (yellow, the rest grey) */
static void view_ll(const cube_t *c, int32_t x, int32_t y, int32_t s, int oll, const uint8_t *scheme)
{
    static const uint8_t TOP[3] = {47, 46, 45}, RIGHT[3] = {11, 10, 9}, BOTTOM[3] = {18, 19, 20}, LEFT[3] = {36, 37, 38};
    int32_t t = s / 3 > 4 ? s / 3 : 4, i;
    uint16_t top = VIEW_RGB[scheme[CF_U]];
#define LLC(i) (oll ? (c->f[i] == c->f[4] ? top : VIEW_GREY) : VIEW_RGB[scheme[c->f[i] % 6]])
    for (i = 0; i < 9; i++) cv_rect(x + t + 1 + (i % 3) * s, y + t + 1 + (i / 3) * s, s - 2, s - 2, LLC(i));
    for (i = 0; i < 3; i++) {
        uint16_t a = LLC(TOP[i]), b = LLC(BOTTOM[i]), l = LLC(LEFT[i]), r = LLC(RIGHT[i]);
        if (!oll || a == top) cv_rect(x + t + 1 + i * s, y, s - 2, t - 1, a);
        if (!oll || b == top) cv_rect(x + t + 1 + i * s, y + t + 3 * s + 1, s - 2, t - 1, b);
        if (!oll || l == top) cv_rect(x, y + t + 1 + i * s, t - 1, s - 2, l);
        if (!oll || r == top) cv_rect(x + t + 3 * s + 1, y + t + 1 + i * s, t - 1, s - 2, r);
    }
#undef LLC
}
