/* SPDX-License-Identifier: GPL-3.0-or-later */
/* groove: DRUMS' gv_styles and player (see groove.h). */
#include "groove.h"

/* ------------------------------------------------------------------ gv_styles --- */
#define GV_MAX_STYLES 24
#define GV_POOL 3072

static gv_ev_t gv_pool[GV_POOL];
static uint32_t gv_pool_n;
static gv_style_t gv_styles[GV_MAX_STYLES];
static int gv_nstyles;
static const char *gv_err = "";

static const char GV_LANE_CODE[GV_LANES] = { 'K', 'S', 'R', 'C', 'H', 'O', 'D', 'X', 'T', 'M', 'L', 'B', 'Z' };
static const char *const GV_LANE_NAME[GV_LANES] = { "KICK", "SNARE", "RIM", "CLAP", "HAT", "OPEN HAT", "RIDE",
                                                 "CRASH", "TOM HI", "TOM MID", "TOM LO", "COWBELL", "SHAKER" };
static const char GV_PART_CODE[GV_PARTS] = { 'A', 'B', 'a', 'b', 'E' };

const char *gv_lane_name(int lane) { return lane >= 0 && lane < GV_LANES ? GV_LANE_NAME[lane] : "?"; }

/* fills for a style that has none of its own, by metre: a snare build (a) and a tom roll (b) */
static const char GV_FILL_44[] =
    "a K 9... .... 9... ....\n"
    "a S .... 9.7. 7.7. 9999\n"
    "a H 7.5. 7.5. .... ....\n"
    "b K 9... .... .... ....\n"
    "b S 9.7. 7... .... ....\n"
    "b T .... 9.7. .... ....\n"
    "b M .... .... 9.7. ....\n"
    "b L .... .... .... 9.79\n";
static const char GV_FILL_44T[] =               /* 4/4 in triplets (shuffle, 12/8) */
    "a K 9.. ... 9.. ...\n"
    "a S ... 9.7 7.7 979\n"
    "b S 9.7 ... ... ...\n"
    "b T ... 9.7 ... ...\n"
    "b M ... ... 9.7 ...\n"
    "b L ... ... ... 979\n";
static const char GV_FILL_34[] =
    "a K 9... .... ....\n"
    "a S .... 9.7. 9999\n"
    "b T 9.7. .... ....\n"
    "b M .... 9.7. ....\n"
    "b L .... .... 9.79\n";
static const char GV_FILL_68[] =
    "a K 9.. ...\n"
    "a S ... 979\n"
    "b T 97. ...\n"
    "b M ..9 7..\n"
    "b L ... .97\n";

static const char *gv_generic_fill(uint8_t beats, uint8_t spb)
{
    if (beats == 4 && spb == 4) return GV_FILL_44;
    if (beats == 4 && spb == 3) return GV_FILL_44T;
    if (beats == 3 && spb == 4) return GV_FILL_34;
    if (beats == 2 && spb == 3) return GV_FILL_68;
    return 0;
}

static int gv_lane_of(char c)
{
    int i;
    for (i = 0; i < GV_LANES; i++)
        if (GV_LANE_CODE[i] == c) return i;
    return -1;
}

/* the lines of src for part code pc into the gv_pool: 1 = some, 0 = none, -1 = a bad line */
static int gv_parse_part(const char *src, char pc, uint8_t steps)
{
    const char *p = src;
    int any = 0;
    while (*p) {
        const char *line = p;
        while (*p && *p != '\n') p++;
        if (line[0] == pc && line[1] == ' ') {
            const char *q = line + 2;
            int lane, s = 0;
            while (*q == ' ') q++;
            lane = gv_lane_of(*q++);
            if (lane < 0) {
                gv_err = "unknown lane";
                return -1;
            }
            for (; q < p; q++) {
                if (*q == ' ') continue;
                if (s >= steps) {
                    gv_err = "too many steps";
                    return -1;
                }
                if (*q != '.') {
                    uint8_t v = *q == 'x' ? 7 : (uint8_t)(*q - '0');
                    if (v < 1 || v > 9) {
                        gv_err = "bad step";
                        return -1;
                    }
                    if (gv_pool_n >= GV_POOL) {
                        gv_err = "gv_pool full";
                        return -1;
                    }
                    gv_pool[gv_pool_n].step = (uint8_t)s;
                    gv_pool[gv_pool_n].lane = (uint8_t)lane;
                    gv_pool[gv_pool_n].vel = v;
                    gv_pool_n++;
                }
                s++;
            }
            if (s != steps) {
                gv_err = "too few steps";
                return -1;
            }
            any = 1;
        }
        if (*p) p++;
    }
    return any;
}

static void gv_sort_events(uint32_t off, uint32_t n)   /* by step, stable */
{
    uint32_t i, j;
    for (i = off + 1; i < off + n; i++) {
        gv_ev_t e = gv_pool[i];
        for (j = i; j > off && gv_pool[j - 1].step > e.step; j--) gv_pool[j] = gv_pool[j - 1];
        gv_pool[j] = e;
    }
}

int gv_load(const gv_style_src_t *src, int n)
{
    int i, p;
    gv_pool_n = 0;
    gv_nstyles = 0;
    gv_err = "";
    for (i = 0; i < n && i < GV_MAX_STYLES; i++) {
        gv_style_t *st = &gv_styles[i];
        const gv_style_src_t *s = &src[i];
        st->name = s->name;
        st->beats = s->beats;
        st->spb = s->spb;
        st->bpm = s->bpm;
        if (!s->beats || !s->spb || GV_TPB % s->spb || s->beats * s->spb > 48) {
            gv_err = "bad metre";
            return -(i + 1);
        }
        st->steps = (uint8_t)(s->beats * s->spb);
        for (p = 0; p < GV_PARTS; p++) {
            uint32_t off = gv_pool_n;
            int r = gv_parse_part(s->src, GV_PART_CODE[p], st->steps);
            if (r < 0) return -(i + 1);
            if (!r && (p == P_FILL_A || p == P_FILL_B)) {
                const char *g = gv_generic_fill(s->beats, s->spb);
                if (!g || (r = gv_parse_part(g, GV_PART_CODE[p], st->steps)) <= 0) {
                    gv_err = "no fill";
                    return -(i + 1);
                }
            }
            if (!r && p == P_END) {          /* the ending: kick and crash on the one */
                if (gv_pool_n + 2 > GV_POOL) {
                    gv_err = "gv_pool full";
                    return -(i + 1);
                }
                gv_pool[gv_pool_n++] = (gv_ev_t){ 0, L_KICK, 9 };
                gv_pool[gv_pool_n++] = (gv_ev_t){ 0, L_CRASH, 9 };
                r = 1;
            }
            if (!r) {
                gv_err = "no main";
                return -(i + 1);
            }
            st->off[p] = (uint16_t)off;
            st->n[p] = (uint16_t)(gv_pool_n - off);
            gv_sort_events(off, gv_pool_n - off);
        }
        gv_nstyles++;
    }
    return gv_nstyles;
}

int gv_count(void) { return gv_nstyles; }
const gv_style_t *gv_style(int i) { return i >= 0 && i < gv_nstyles ? &gv_styles[i] : 0; }
const char *gv_error(void) { return gv_err; }

const gv_ev_t *gv_events(const gv_style_t *st, int part, uint32_t *n)
{
    *n = st->n[part];
    return &gv_pool[st->off[part]];
}

/* ------------------------------------------------------------------ player --- */
static uint32_t gv_step_ticks(const groove_t *g) { return GV_TPB / g->st->spb; }
static uint8_t gv_main_of(int sec) { return sec ? P_MAIN_B : P_MAIN_A; }
static uint8_t gv_fill_of(int sec) { return sec ? P_FILL_B : P_FILL_A; }
static int gv_is_fill(int part) { return part == P_FILL_A || part == P_FILL_B; }

static void gv_set_bar_len(groove_t *g) { g->bar_ticks = (uint32_t)g->st->beats * GV_TPB; }

void groove_init(groove_t *g, const gv_style_t *st)
{
    uint8_t *b = (uint8_t *)g;
    uint32_t i;
    for (i = 0; i < sizeof *g; i++) b[i] = 0;
    g->st = st;
    g->req_sec = GV_NONE;
    g->fill_at = FA_PHRASE;
    g->phrase = 4;
    g->auto_fill = 1;
    gv_set_bar_len(g);
}

void groove_start(groove_t *g, int sec)
{
    if (g->req_style) g->st = g->req_style, g->req_style = 0;
    gv_set_bar_len(g);
    g->state = GS_PLAY;
    g->sec = (uint8_t)(sec ? 1 : 0);
    g->part = gv_main_of(g->sec);
    g->fill_from = 0;
    g->crash = 0;
    g->bar_start = 0;
    g->bars = 0;
    g->bar_in_sec = 0;
    g->pos = 0;
    g->req_fill = g->req_end = g->switch_now = g->hold = 0;
    g->req_sec = GV_NONE;
}

void groove_stop(groove_t *g) { g->state = GS_STOP; }

int groove_last_of_phrase(const groove_t *g) { return g->bar_in_sec % g->phrase == (uint32_t)g->phrase - 1u; }

int groove_step(const groove_t *g) { return (int)(g->pos / gv_step_ticks(g)); }

int groove_fill_pending(const groove_t *g) { return g->req_fill || (g->req_sec != GV_NONE && g->auto_fill); }

/* the next bar: what it plays */
static void gv_bar_begin(groove_t *g)
{
    int was_fill = gv_is_fill(g->part);
    g->bars++;
    g->bar_in_sec++;
    g->fill_from = 0;
    if (g->state == GS_END) {            /* the ending played: stop */
        g->state = GS_STOP;
        return;
    }
    if (g->req_style) {
        g->st = g->req_style;
        g->req_style = 0;
        gv_set_bar_len(g);
    }
    g->crash = (uint8_t)(was_fill && !g->hold);
    if (g->req_end) {
        g->req_end = 0;
        g->state = GS_END;
        g->part = P_END;
        g->crash = 0;                    /* (the ending has its own) */
        return;
    }
    if (g->switch_now && g->req_sec != GV_NONE) {
        g->sec = g->req_sec;
        g->req_sec = GV_NONE;
        g->bar_in_sec = 0;
        g->crash = 1;
    }
    g->switch_now = 0;
    if (g->hold) {                       /* a transition: the fill again */
        g->part = gv_fill_of(g->sec);
        g->req_fill = 0;
        return;
    }
    g->part = gv_main_of(g->sec);
    if (g->req_sec != GV_NONE) {         /* a MAIN change waits for this bar (BAR) or the phrase's last (PHRASE) */
        if (g->fill_at != FA_PHRASE || groove_last_of_phrase(g)) {
            if (g->auto_fill) g->part = gv_fill_of(g->sec);
            g->req_fill = 0;
            g->switch_now = 1;
        }
        return;
    }
    if (g->req_fill && (g->fill_at != FA_PHRASE || groove_last_of_phrase(g))) {
        g->part = gv_fill_of(g->sec);
        g->req_fill = 0;
    }
}

/* the fill from the next step to the bar's end; 0 when the bar has no step left */
static int gv_start_now(groove_t *g)
{
    uint32_t s = g->pos / gv_step_ticks(g) + 1u;
    if (s >= g->st->steps) return 0;
    g->part = gv_fill_of(g->sec);
    g->fill_from = (uint8_t)s;
    return 1;
}

void groove_fill(groove_t *g)
{
    if (g->state != GS_PLAY || gv_is_fill(g->part) || g->req_end) return;
    if (g->fill_at == FA_NOW || (g->fill_at == FA_PHRASE && groove_last_of_phrase(g))) {
        if (gv_start_now(g)) return;
    }
    g->req_fill = 1;
}

void groove_main(groove_t *g, int sec)
{
    sec = sec ? 1 : 0;
    if (g->state != GS_PLAY || g->req_end) return;
    if (sec == g->sec && g->req_sec == GV_NONE) {   /* the MAIN it plays: a fill back into it */
        groove_fill(g);
        return;
    }
    if (sec == g->sec) {                 /* asked back before the change came: cancel it */
        g->req_sec = GV_NONE;
        g->switch_now = 0;
        return;
    }
    g->req_sec = (uint8_t)sec;
    g->switch_now = 0;
    if (g->fill_at == FA_PHRASE && !groove_last_of_phrase(g)) return;   /* gv_bar_begin, at the phrase's last bar */
    if (!g->auto_fill) {
        g->switch_now = 1;               /* plain: the change at this bar's end */
        return;
    }
    if (gv_is_fill(g->part) || g->fill_at != FA_BAR) {   /* NOW, or PHRASE in its last bar: the fill now, then the change */
        if (gv_is_fill(g->part) || gv_start_now(g)) {
            g->switch_now = 1;
            g->req_fill = 0;
        }
    }
    /* BAR (or no step left): gv_bar_begin plays the next bar as the fill, then changes */
}

void groove_hold(groove_t *g, int on)
{
    if (g->state != GS_PLAY) return;
    if (on) {
        g->hold = 1;
        g->req_fill = 0;
        if (!gv_is_fill(g->part)) (void)gv_start_now(g);
        return;
    }
    if (!g->hold) return;
    g->hold = 0;
    g->req_sec = (uint8_t)(g->sec ? 0 : 1);   /* released: the other MAIN after this bar */
    g->switch_now = 1;
}

void groove_end(groove_t *g)
{
    if (g->state != GS_PLAY) return;
    g->req_end = 1;
    g->hold = 0;
}

void groove_set_style(groove_t *g, const gv_style_t *st)
{
    if (!st) return;
    if (g->state == GS_STOP) {
        g->st = st;
        g->req_style = 0;
        gv_set_bar_len(g);
    } else
        g->req_style = st;
}

static void gv_emit(gv_out_t *o, uint8_t lane, uint8_t vel)
{
    if (o->n < GV_MAX_EV) {
        o->e[o->n].step = 0;
        o->e[o->n].lane = lane;
        o->e[o->n].vel = vel;
        o->n++;
    }
}

int groove_tick(groove_t *g, uint32_t t, gv_out_t *o)
{
    uint32_t pos, st, s, base, i, n;
    const gv_ev_t *ev;
    o->n = 0;
    if (g->state == GS_STOP) return 0;
    pos = t - g->bar_start;
    while (pos >= g->bar_ticks) {
        g->bar_start += g->bar_ticks;
        pos = t - g->bar_start;
        gv_bar_begin(g);
        if (g->state == GS_STOP) return 0;
    }
    g->pos = pos;
    st = gv_step_ticks(g);
    s = pos / st;
    base = s * st;
    /* the off 16ths come g->swing ticks late (4 steps a beat only) */
    if (g->st->spb == 4 && (s & 1u) && g->swing) {
        if (pos != base + g->swing) return 0;
    } else if (pos != base)
        return 0;
    if (s == 0 && g->crash && !(g->mutes >> L_CRASH & 1u)) gv_emit(o, L_CRASH, 8);
    ev = gv_events(g->st, g->part, &n);
    for (i = 0; i < n && ev[i].step <= s; i++) {
        if (ev[i].step != s) continue;
        if (gv_is_fill(g->part) && s < g->fill_from) continue;
        if (g->mutes >> ev[i].lane & 1u) continue;
        if (ev[i].lane == L_CRASH && s == 0 && g->crash) continue;   /* (already) */
        gv_emit(o, ev[i].lane, ev[i].vel);
    }
    /* a fill started mid-bar: the main part up to it */
    if (gv_is_fill(g->part) && s < g->fill_from) {
        ev = gv_events(g->st, gv_main_of(g->sec), &n);
        for (i = 0; i < n && ev[i].step <= s; i++)
            if (ev[i].step == s && !(g->mutes >> ev[i].lane & 1u)) gv_emit(o, ev[i].lane, ev[i].vel);
    }
    return o->n;
}
