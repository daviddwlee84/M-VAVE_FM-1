/* SPDX-License-Identifier: GPL-3.0-or-later */
/* groove: DRUMS' styles and player (see groove.h). */
#include "groove.h"

/* ------------------------------------------------------------------ styles --- */
#define GV_MAX_STYLES 24
#define GV_POOL 6144

static gv_ev_t gv_pool[GV_POOL];
static uint32_t gv_pool_n;
static gv_style_t gv_styles[GV_MAX_STYLES];
static int gv_nstyles;
static const char *gv_err = "";

static const char GV_LANE_CODE[GV_LANES] = { 'K', 'S', 'R', 'C', 'H', 'O', 'D', 'X', 'T', 'M', 'L', 'B', 'Z' };
static const char *const GV_LANE_NAME[GV_LANES] = { "KICK", "SNARE", "RIM", "CLAP", "HAT", "OPEN HAT", "RIDE",
                                                    "CRASH", "TOM HI", "TOM MID", "TOM LO", "COWBELL", "SHAKER" };
static const char GV_PART_CODE[GV_PARTS] = { 'A', 'B', 'C', 'D', 'a', 'b', 'c', 'd', 't', 'w', 'I', 'K', 'P', 'E' };
static const char *const GV_PART_NAME[GV_PARTS] = { "MAIN A", "MAIN B", "MAIN C", "MAIN D", "FILL", "FILL",
                                                    "FILL", "FILL", "TOM FILL", "FILL", "INTRO", "BREAK", "STOP",
                                                    "ENDING" };

const char *gv_lane_name(int lane) { return lane >= 0 && lane < GV_LANES ? GV_LANE_NAME[lane] : "?"; }
const char *gv_part_name(int part) { return part >= 0 && part < GV_PARTS ? GV_PART_NAME[part] : "?"; }
int gv_is_fill(int part) { return (part >= P_FILL_A && part <= P_FILL_D) || part == P_TOM || part == P_TWO; }

/* the parts a style leaves out, by metre: fills (a snare build, a syncopated one, a tom roll), a two-bar build
 * (w), a two-bar intro (I: the hats count in, the snare leads in) */
static const char GV_GEN_44[] =
    "a K 9... .... 9... ....\n"
    "a S .... 9.7. 7.7. 9999\n"
    "a H 7.5. 7.5. .... ....\n"
    "b K 9... ..9. 9... ....\n"
    "b S ..7. 9..7 ..9. 9797\n"
    "b O 7... .... .... ....\n"
    "t K 9... .... .... ....\n"
    "t S 9.7. 7... .... ....\n"
    "t T .... 9.7. .... ....\n"
    "t M .... .... 9.7. ....\n"
    "t L .... .... .... 9.79\n"
    "w K 9... 9... 9... 9... 9... 9... 9... 9...\n"
    "w S 7.7. 7.7. 7.7. 7.7. 7777 7777 8888 9999\n"
    "I H 9... 9... 9... 9... .... .... .... ....\n"
    "I K .... .... .... .... 9... .... 9... ....\n"
    "I S .... .... .... .... 7... 7... 7.7. 9999\n";
static const char GV_GEN_44T[] =             /* 4/4 in triplets (shuffle, 12/8) */
    "a K 9.. ... 9.. ...\n"
    "a S ... 9.7 7.7 979\n"
    "b K 9.. ..9 9.. ...\n"
    "b S ... 9.7 ..7 979\n"
    "t S 9.7 ... ... ...\n"
    "t T ... 9.7 ... ...\n"
    "t M ... ... 9.7 ...\n"
    "t L ... ... ... 979\n"
    "w K 9.. 9.. 9.. 9.. 9.. 9.. 9.. 9..\n"
    "w S 7.. 7.. 7.. 7.. 7.7 7.7 777 999\n"
    "I H 9.. 9.. 9.. 9.. ... ... ... ...\n"
    "I S ... ... ... ... ... ... 7.7 979\n";
static const char GV_GEN_34[] =
    "a K 9... .... ....\n"
    "a S .... 9.7. 9999\n"
    "b K 9... .... 9...\n"
    "b S .... 9.7. 9.7.\n"
    "t T 9.7. .... ....\n"
    "t M .... 9.7. ....\n"
    "t L .... .... 9.79\n"
    "w K 9... .... .... 9... .... ....\n"
    "w S 7.7. 7.7. 7.7. 7777 8888 9999\n"
    "I H 9... 9... 9... .... .... ....\n"
    "I S .... .... .... .... 7.7. 9999\n";
static const char GV_GEN_68[] =
    "a K 9.. ...\n"
    "a S ... 979\n"
    "b K 9.. 9..\n"
    "b S 7.7 979\n"
    "t T 97. ...\n"
    "t M ..9 7..\n"
    "t L ... .97\n"
    "w K 9.. 9.. 9.. 9..\n"
    "w S 7.. 7.. 777 999\n"
    "I H 9.. 9.. ... ...\n"
    "I S ... ... 7.7 979\n";

static const char *gv_generic(uint8_t beats, uint8_t spb)
{
    if (beats == 4 && spb == 4) return GV_GEN_44;
    if (beats == 4 && spb == 3) return GV_GEN_44T;
    if (beats == 3 && spb == 4) return GV_GEN_34;
    if (beats == 2 && spb == 3) return GV_GEN_68;
    return 0;
}

static int gv_lane_of(char c)
{
    int i;
    for (i = 0; i < GV_LANES; i++)
        if (GV_LANE_CODE[i] == c) return i;
    return -1;
}

/* the lines of src for part code pc into the pool: its length in bars (1, 2), 0 = none, -1 = a bad line */
static int gv_parse_part(const char *src, char pc, uint8_t steps)
{
    const char *p = src;
    int bars = 0;
    while (*p) {
        const char *line = p;
        while (*p && *p != '\n') p++;
        if (line[0] == pc && line[1] == ' ') {
            const char *q = line + 2;
            int lane, s = 0, b;
            while (*q == ' ') q++;
            lane = gv_lane_of(*q++);
            if (lane < 0) {
                gv_err = "unknown lane";
                return -1;
            }
            for (; q < p; q++) {
                if (*q == ' ') continue;
                if (s >= 2 * steps) {
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
                        gv_err = "pool full";
                        return -1;
                    }
                    gv_pool[gv_pool_n].step = (uint8_t)s;
                    gv_pool[gv_pool_n].lane = (uint8_t)lane;
                    gv_pool[gv_pool_n].vel = v;
                    gv_pool_n++;
                }
                s++;
            }
            b = s == steps ? 1 : s == 2 * steps ? 2 : 0;
            if (!b || (bars && b != bars)) {
                gv_err = b ? "lines of different lengths" : "a line is not 1 or 2 bars";
                return -1;
            }
            bars = b;
        }
        if (*p) p++;
    }
    return bars;
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

static int gv_hit_on_one(void)           /* kick and crash on the one (STOP, ENDING without their own) */
{
    if (gv_pool_n + 2 > GV_POOL) {
        gv_err = "pool full";
        return -1;
    }
    gv_pool[gv_pool_n++] = (gv_ev_t){ 0, L_KICK, 9 };
    gv_pool[gv_pool_n++] = (gv_ev_t){ 0, L_CRASH, 9 };
    return 1;
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
        const char *gen = gv_generic(s->beats, s->spb);
        st->name = s->name;
        st->beats = s->beats;
        st->spb = s->spb;
        st->bpm = s->bpm;
        if (!s->beats || !s->spb || GV_TPB % s->spb || s->beats * s->spb > 48 || !gen) {
            gv_err = "bad metre";
            return -(i + 1);
        }
        st->steps = (uint8_t)(s->beats * s->spb);
        for (p = 0; p < GV_PARTS; p++) {
            uint32_t off = gv_pool_n;
            int r = gv_parse_part(s->src, GV_PART_CODE[p], st->steps);
            if (r < 0) return -(i + 1);
            if (!r) {
                if (p == P_MAIN_C || p == P_MAIN_D || p == P_FILL_C || p == P_FILL_D) {
                    int from = p == P_MAIN_C || p == P_MAIN_D ? P_MAIN_B : P_FILL_B;   /* C, D: as B */
                    st->off[p] = st->off[from];
                    st->n[p] = st->n[from];
                    st->bars[p] = st->bars[from];
                    continue;
                }
                if (p == P_FILL_A || p == P_FILL_B || p == P_TOM || p == P_TWO || p == P_INTRO)
                    r = gv_parse_part(gen, GV_PART_CODE[p], st->steps);
                else if (p == P_BREAK)
                    r = 1;                   /* an empty bar */
                else if (p == P_STOP || p == P_END)
                    r = gv_hit_on_one();
                if (r < 0) return -(i + 1);
            }
            if (!r || (p == P_TWO ? r != 2 : p == P_INTRO ? r > 2 : r != 1)) {
                gv_err = !r ? "no main" : "wrong length";
                return -(i + 1);
            }
            st->off[p] = (uint16_t)off;
            st->n[p] = (uint16_t)(gv_pool_n - off);
            st->bars[p] = (uint8_t)r;
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
static uint8_t gv_main_of(int sec) { return (uint8_t)(P_MAIN_A + (sec & 3)); }
static uint8_t gv_fill_of(int sec) { return (uint8_t)(P_FILL_A + (sec & 3)); }
static void gv_set_bar_len(groove_t *g) { g->bar_ticks = (uint32_t)g->st->beats * GV_TPB; }

void groove_init(groove_t *g, const gv_style_t *st)
{
    uint8_t *b = (uint8_t *)g;
    uint32_t i;
    for (i = 0; i < sizeof *g; i++) b[i] = 0;
    g->st = st;
    g->req_sec = GV_NONE;
    g->fill_at = FA_PHRASE;
    g->fill_len = FL_ONE;
    g->phrase = 4;
    g->auto_fill = 1;
    gv_set_bar_len(g);
}

static void gv_clear_requests(groove_t *g)
{
    g->req_fill = g->req_end = g->req_break = g->req_stop = g->switch_now = g->hold = 0;
    g->req_sec = GV_NONE;
}

void groove_start(groove_t *g, int sec)
{
    if (g->req_style) g->st = g->req_style, g->req_style = 0;
    gv_set_bar_len(g);
    g->state = GS_PLAY;
    g->sec = (uint8_t)(sec & 3);
    g->part = gv_main_of(g->sec);
    g->part_bar = 0;
    g->fill_from = 0;
    g->crash = 0;
    g->bar_start = 0;
    g->bars = 0;
    g->bar_in_sec = 0;
    g->pos = 0;
    gv_clear_requests(g);
}

void groove_start_intro(groove_t *g, int sec)
{
    groove_start(g, sec);
    g->part = P_INTRO;
}

void groove_stop(groove_t *g) { g->state = GS_STOP; }

int groove_last_of_phrase(const groove_t *g) { return g->bar_in_sec % g->phrase == (uint32_t)g->phrase - 1u; }

static int gv_phrase_pos(const groove_t *g) { return (int)(g->bar_in_sec % g->phrase); }

/* where PHRASE puts a fill of this length: its first bar */
static int gv_phrase_fill_bar(const groove_t *g, int len) { return g->phrase - (len == FL_TWO ? 2 : 1); }

int groove_step(const groove_t *g) { return (int)(g->pos / gv_step_ticks(g)); }

int groove_fill_pending(const groove_t *g) { return g->req_fill || (g->req_sec != GV_NONE && g->auto_fill); }

/* a fill at a bar's start: which part, from which step */
static void gv_put_fill(groove_t *g, int kind, int len)
{
    g->part_bar = 0;
    g->fill_from = 0;
    if (kind == FK_TOM) g->part = P_TOM;
    else if (len == FL_TWO) g->part = P_TWO;
    else g->part = gv_fill_of(g->sec);
    if (len == FL_HALF) g->fill_from = (uint8_t)(g->st->steps / 2);
}

/* a fill from the next step to the bar's end (HALF: not before the middle); 0 when the bar has no step left */
static int gv_start_now(groove_t *g, int kind, int len)
{
    uint32_t s = g->pos / gv_step_ticks(g) + 1u;
    if (len == FL_HALF && s < g->st->steps / 2u) s = g->st->steps / 2u;
    if (s >= g->st->steps) return 0;
    gv_put_fill(g, kind, len);
    g->fill_from = (uint8_t)s;
    return 1;
}

/* the next bar: what it plays */
static void gv_bar_begin(groove_t *g)
{
    int prev = g->part;
    g->bars++;
    g->bar_in_sec++;
    if (g->state == GS_END) {            /* the ending played: stop */
        g->state = GS_STOP;
        return;
    }
    if (g->part_bar + 1u < g->st->bars[g->part]) {   /* a two-bar part goes on */
        g->part_bar++;
        g->fill_from = 0;
        g->crash = 0;
        return;
    }
    g->part_bar = 0;
    g->fill_from = 0;
    if (g->req_style) {
        g->st = g->req_style;
        g->req_style = 0;
        gv_set_bar_len(g);
    }
    if (prev == P_INTRO) g->bar_in_sec = 0;   /* the phrase counts from after the intro */
    g->crash = (uint8_t)((gv_is_fill(prev) || prev == P_INTRO || prev == P_BREAK) && !g->hold);
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
    if (g->hold) {                       /* a transition: the MAIN's fill again */
        gv_put_fill(g, FK_SEC, FL_ONE);
        g->req_fill = 0;
        return;
    }
    if (g->req_break || g->req_stop) {
        g->part = g->req_stop ? P_STOP : P_BREAK;
        g->req_break = g->req_stop = 0;
        g->crash = 0;
        return;
    }
    g->part = gv_main_of(g->sec);
    if (g->req_sec != GV_NONE) {         /* a MAIN change waits for this bar (BAR) or the phrase's end (PHRASE) */
        int len = g->fill_len;
        if (g->fill_at != FA_PHRASE || gv_phrase_pos(g) == gv_phrase_fill_bar(g, g->auto_fill ? len : FL_ONE)) {
            if (g->auto_fill) gv_put_fill(g, FK_SEC, len);
            g->req_fill = 0;
            g->switch_now = 1;
        }
        return;
    }
    if (g->req_fill && (g->fill_at != FA_PHRASE || gv_phrase_pos(g) == gv_phrase_fill_bar(g, g->req_len))) {
        gv_put_fill(g, g->req_kind, g->req_len);
        g->req_fill = 0;
    }
}

void groove_fill_ex(groove_t *g, int kind, int len)
{
    if (g->state != GS_PLAY || gv_is_fill(g->part) || g->part == P_INTRO || g->req_end) return;
    if (kind == FK_TOM && len == FL_TWO) len = FL_ONE;   /* (the tom fill is one bar) */
    if (g->fill_at == FA_PHRASE) {
        int pos = gv_phrase_pos(g);
        if (pos == g->phrase - 1) {      /* asked in the phrase's last bar: now (a two-bar one is too late: one bar) */
            if (gv_start_now(g, kind, len == FL_TWO ? FL_ONE : len)) return;
        } else if (len == FL_TWO && pos == g->phrase - 2) {
            if (gv_start_now(g, kind, len)) return;
        }
    } else if (g->fill_at == FA_NOW) {
        if (gv_start_now(g, kind, len)) return;
    }
    g->req_fill = 1;
    g->req_kind = (uint8_t)kind;
    g->req_len = (uint8_t)len;
}

void groove_fill(groove_t *g) { groove_fill_ex(g, FK_SEC, g->fill_len); }

void groove_main(groove_t *g, int sec)
{
    sec &= 3;
    if (g->state != GS_PLAY || g->req_end || g->part == P_INTRO) return;
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
    if (gv_is_fill(g->part)) {           /* a fill already plays: the change after it */
        g->switch_now = 1;
        g->req_fill = 0;
        return;
    }
    if (g->fill_at == FA_PHRASE && !groove_last_of_phrase(g)) {
        if (!(g->auto_fill && g->fill_len == FL_TWO && gv_phrase_pos(g) == g->phrase - 2)) return;   /* gv_bar_begin */
    }
    if (!g->auto_fill) {
        if (g->fill_at != FA_PHRASE || groove_last_of_phrase(g)) g->switch_now = 1;   /* at this bar's end */
        return;
    }
    if (g->fill_at != FA_BAR) {          /* NOW, or PHRASE at its end: the fill now, then the change */
        int len = g->fill_len;
        if (g->fill_at == FA_PHRASE && groove_last_of_phrase(g) && len == FL_TWO) len = FL_ONE;
        if (gv_start_now(g, FK_SEC, len)) {
            g->switch_now = 1;
            g->req_fill = 0;
        }
    }
    /* BAR (or no step left): gv_bar_begin plays the next bar as the fill, then changes */
}

void groove_hold(groove_t *g, int on)
{
    if (g->state != GS_PLAY || g->part == P_INTRO) return;
    if (on) {
        g->hold = 1;
        g->req_fill = 0;
        if (!gv_is_fill(g->part)) (void)gv_start_now(g, FK_SEC, FL_ONE);
        return;
    }
    if (!g->hold) return;
    g->hold = 0;
    g->req_sec = (uint8_t)((g->sec + 1) & 3);   /* let go: the next MAIN after this bar */
    g->switch_now = 1;
}

void groove_break(groove_t *g)
{
    if (g->state != GS_PLAY || g->req_end) return;
    g->req_break = 1;
    g->req_stop = 0;
}

void groove_stop_time(groove_t *g)
{
    if (g->state != GS_PLAY || g->req_end) return;
    g->req_stop = 1;
    g->req_break = 0;
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
    uint32_t pos, st, s, ps, base, i, n;
    const gv_ev_t *ev;
    int tail;
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
    /* a fill that starts mid-bar (NOW, HALF): the MAIN up to it */
    tail = gv_is_fill(g->part) && g->part_bar == 0 && s < g->fill_from;
    ps = s + (uint32_t)g->part_bar * g->st->steps;   /* the step within a two-bar part */
    ev = gv_events(g->st, tail ? gv_main_of(g->sec) : g->part, &n);
    if (tail) ps = s;
    for (i = 0; i < n && ev[i].step <= ps; i++) {
        if (ev[i].step != ps) continue;
        if (g->mutes >> ev[i].lane & 1u) continue;
        if (ev[i].lane == L_CRASH && s == 0 && g->crash) continue;   /* (already) */
        gv_emit(o, ev[i].lane, ev[i].vel);
    }
    return o->n;
}
