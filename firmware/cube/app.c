/* SPDX-License-Identifier: GPL-3.0-or-later */
/* CUBE v0.1 (see app.h). Pages: MENU (HOME), TIMER, CUBE, ALGS, DRILL, STATS, SETTINGS.
 *
 * Time: the 1 kHz tick reads the keys and buttons and queues each edge with its millisecond, so the timer starts
 * and stops on the edge itself, not on the next frame. Scrambles: the next random-state scramble is searched in the
 * main loop a few milliseconds a frame (solve.c, in slices), while you solve; until the first one is found (power-on)
 * one comes from a bank made on the computer (gen/cube_tables.h), which is random-state too.
 *
 * Part of fm1-base's unity build, after the platform: gfx.c and announce.c are in scope. */
#include "fastmath.h"
#include "cube.c"
#include "solve.h"                       /* solve.c is its own unit, at -O2 (fm1.toml o2) */
#include "stats.c"
#include "keymap.c"
#include "algs.c"
#include "srs.c"
#include "gtext.c"
#include "ledfx.c"
#include "chime.c"
#include "view.c"

#define FW_MAJOR 0
#define FW_MINOR 1
#define SCR_MAX 30                       /* moves in a scramble at most */

static int host_fails;

static void zero(void *p, uint32_t n)
{
    uint8_t *b = (uint8_t *)p;
    while (n--) *b++ = 0;
}

static void tcat(char *d, const char *s)
{
    while (*d) d++;
    while (*s) *d++ = *s++;
    *d = 0;
}

static void tint(char *d, int v)
{
    char t[12];
    int i = 0, neg = v < 0;
    unsigned u = (unsigned)(neg ? -v : v);
    do t[i++] = (char)('0' + u % 10u), u /= 10u;
    while (u);
    if (neg) t[i++] = '-';
    while (*d) d++;
    while (i) *d++ = t[--i];
    *d = 0;
}

/* ---------------------------------------------------------------- settings --- */
#define SET_MAGIC 0x31425543u            /* "CUB1" */
#define SET_VER 1u
enum { SCR_STATE, SCR_MOVE };
enum { START_ANY, START_ENDS };
typedef struct {
    uint32_t magic;
    uint16_t ver, size;
    uint8_t scr_type, scr_len, inspect, start, layout, anim, hints, view;
    uint8_t alg_set, alg_pos, drill_set, drill_mode;
    uint8_t custom[NKEYS_CUBE];
    uint8_t rsv[5];
} set_t;
static set_t set;
static uint8_t keymap[NKEYS_CUBE];       /* the layout in use */
static uint32_t set_dirty_at, saves;

static void set_defaults(void)
{
    zero(&set, sizeof set);
    set.scr_type = SCR_STATE;
    set.scr_len = 25;
    set.inspect = 1;
    set.start = START_ANY;
    set.layout = KM_BLACK;
    set.anim = 1;
    set.hints = 1;
    km_preset(KM_BLACK, set.custom);
}

static int set_valid(const set_t *s)
{
    int k;
    if (s->magic != SET_MAGIC || s->ver != SET_VER || s->size != sizeof *s) return 0;
    if (s->scr_type > 1 || s->scr_len < 15 || s->scr_len > SCR_MAX || s->inspect > 1 || s->start > 1
        || s->layout > KM_CUSTOM || s->anim > 1 || s->hints > 1 || s->view >= VIEW_PRESETS || s->alg_set >= ALG_SETS
        || s->drill_set >= ALG_SETS || s->drill_mode > 2)
        return 0;
    for (k = 0; k < NKEYS_CUBE; k++)
        if (s->custom[k] != KA_NONE && s->custom[k] != KA_UNDO && s->custom[k] >= MV_N) return 0;
    return 1;
}

static void keymap_load(void)
{
    int k;
    if (set.layout == KM_CUSTOM)
        for (k = 0; k < NKEYS_CUBE; k++) keymap[k] = set.custom[k];
    else km_preset(set.layout, keymap);
}

static void set_save(void)
{
    set.magic = SET_MAGIC;
    set.ver = SET_VER;
    set.size = (uint16_t)sizeof set;
    if (plat_store_save(OBJ_CUBE, &set, sizeof set) == 0) saves++;
    set_dirty_at = 0;
}

static void set_changed(void)
{
    set_dirty_at = plat_ms();
    if (!set_dirty_at) set_dirty_at = 1;
}

/* ------------------------------------------------------------------ solves --- */
#define SOLVES_MAX 100
#define SOL_MAGIC 0x31564C53u            /* "SLV1" */
typedef struct {
    uint32_t t;                          /* ms in bits 0..27, the penalty (ST_*) in 28..29 */
    uint8_t n;
    uint8_t m[SCR_MAX];
    uint8_t rsv;
} solve_rec_t;
typedef struct {
    uint32_t magic;
    uint16_t ver, n;
    solve_rec_t s[SOLVES_MAX];           /* oldest first */
} solves_t;
static solves_t sol;

#define SOL_MS(r) ((int32_t)((r)->t & 0x0FFFFFFFu))
#define SOL_PEN(r) ((int)((r)->t >> 28 & 3u))
static int32_t sol_time(int i) { return st_with_penalty(SOL_MS(&sol.s[i]), SOL_PEN(&sol.s[i])); }

static void sol_save(void)
{
    sol.magic = SOL_MAGIC;
    sol.ver = 1;
    if (plat_store_save(OBJ_SOLVES, &sol, sizeof sol) == 0) saves++;
}

static void sol_add(int32_t ms, int pen, const uint8_t *m, int n)
{
    solve_rec_t *r;
    int i;
    if (sol.n >= SOLVES_MAX) {
        for (i = 1; i < SOLVES_MAX; i++) sol.s[i - 1] = sol.s[i];
        sol.n = SOLVES_MAX - 1;
    }
    r = &sol.s[sol.n++];
    if (ms > 0x0FFFFFFF) ms = 0x0FFFFFFF;
    r->t = (uint32_t)ms | (uint32_t)pen << 28;
    r->n = (uint8_t)(n > SCR_MAX ? SCR_MAX : n);
    for (i = 0; i < r->n; i++) r->m[i] = m[i];
    sol_save();
}

/* the newest k times (penalties in), newest last; returns how many */
static int sol_last(int k, int32_t *out)
{
    int i, n = sol.n < k ? sol.n : k;
    for (i = 0; i < n; i++) out[i] = sol_time(sol.n - n + i);
    return n;
}

static int32_t sol_avg(int k)
{
    int32_t t[ST_AVG_MAX];
    int n = sol_last(k, t);
    if (n < k) return ST_NONE;
    return k == 3 ? st_mean(t, 3) : st_average(t, k);
}

/* ---------------------------------------------------------------- drills --- */
#define DRILL_MAGIC 0x31524C44u          /* "DLR1" */
#define ALG_ROOM 160                     /* entries the record has room for */
#define BLOB_ROOM SR_BLOB_N(64)
typedef struct {
    uint32_t magic;
    uint16_t ver, size;
    uint16_t blob_n[ALG_SETS];
    uint8_t blob[ALG_SETS][BLOB_ROOM];
    uint16_t real_cs[ALG_ROOM];          /* REAL: each case's running average, 0.01 s (0: none yet) */
} drill_rec_t;
static drill_rec_t drec;
static sr_state_t dst[ALG_SETS];
static sr_course_t dcourse[ALG_SETS];
static uint8_t d_seg[SR_MAX], d_key[SR_MAX], d_groups[ALG_SETS][SR_MAX / 4 + 1];

static void drill_courses(void)
{
    int s, i;
    for (i = 0; i < (int)SR_MAX; i++) d_seg[i] = 0, d_key[i] = SR_NOKEY;
    for (s = 0; s < ALG_SETS; s++) {
        int n = alg_count(s), g = 0;
        for (i = 4; i < n; i += 4) d_groups[s][g++] = (uint8_t)i;
        d_groups[s][g++] = (uint8_t)n;
        dcourse[s].n = (uint8_t)n;
        dcourse[s].seg = d_seg;
        dcourse[s].key = d_key;
        dcourse[s].vary = 0;
        dcourse[s].group_end = d_groups[s];
        dcourse[s].ngroups = (uint8_t)g;
        dcourse[s].unlock_box = 2;
    }
}

static void drill_load(void)
{
    int s;
    int ok = plat_store_load(OBJ_DRILL, &drec, sizeof drec) == (int)sizeof drec && drec.magic == DRILL_MAGIC
             && drec.size == sizeof drec;
    if (!ok) zero(&drec, sizeof drec);
    for (s = 0; s < ALG_SETS; s++)
        if (!ok || !sr_unblob(&dst[s], &dcourse[s], drec.blob[s], drec.blob_n[s])) sr_init(&dst[s], &dcourse[s]);
}

static void drill_save(void)
{
    int s;
    drec.magic = DRILL_MAGIC;
    drec.ver = 1;
    drec.size = (uint16_t)sizeof drec;
    for (s = 0; s < ALG_SETS; s++) drec.blob_n[s] = (uint16_t)sr_blob(&dst[s], drec.blob[s]);
    if (plat_store_save(OBJ_DRILL, &drec, sizeof drec) == 0) saves++;
}

/* --------------------------------------------------------- keys, with time --- */
#define EVQ 64
enum { EV_KEY, EV_BTN };
typedef struct {
    uint8_t kind, idx, down;
    uint32_t t;
} ev_t;
static volatile ev_t evq[EVQ];
static volatile uint32_t ev_w;
static uint32_t ev_r, tk_keys, tk_btn;

void app_tick(void)
{
    uint32_t keys = plat_keys(), btn = plat_buttons(), now = plat_ms(), ch, k;
    for (ch = keys ^ tk_keys, k = 0; ch; k++, ch >>= 1)
        if (ch & 1u) {
            volatile ev_t *e = &evq[ev_w % EVQ];
            e->kind = EV_KEY, e->idx = (uint8_t)k, e->down = (uint8_t)(keys >> k & 1u), e->t = now;
            ev_w++;
        }
    for (ch = btn ^ tk_btn, k = 0; ch; k++, ch >>= 1)
        if (ch & 1u) {
            volatile ev_t *e = &evq[ev_w % EVQ];
            e->kind = EV_BTN, e->idx = (uint8_t)k, e->down = (uint8_t)(btn >> k & 1u), e->t = now;
            ev_w++;
        }
    tk_keys = keys;
    tk_btn = btn;
}

static uint32_t crng = 0x6A09E667u;
static void entropy(uint32_t x)
{
    crng ^= x * 2654435761u + 0x9E3779B9u;
    cube_rand(&crng);
}

/* ------------------------------------------------------------ scrambles --- */
enum { SK_STATE, SK_BANK, SK_MOVE };
typedef struct {
    uint8_t n, kind;
    uint8_t m[SCR_MAX];
} scr_t;
static scr_t scr_next;
static uint8_t next_ok, sv_busy;
static solve_t sv;
static uint32_t sv_found, sv_started_at, sv_last_ms, sv_nodes_last;

static void sv_begin(void)
{
    cc_t c;
    entropy(plat_ms());
    solve_random_cc(&c, &crng);
    solve_start(&sv, &c, 21);
    sv_busy = 1;
    sv_started_at = plat_ms();
}

static void sv_work(uint32_t budget_ms, uint32_t node_cap)   /* the search, a slice a frame */
{
    uint32_t t0 = plat_ms(), nodes = 0;
    int r = 0, i;
    if (!sv_busy) return;
    while (nodes < node_cap) {
        r = solve_step(&sv, 400);
        nodes += 400;
        if (r != 0 || plat_ms() - t0 >= budget_ms) break;
    }
    if (r > 0) {
        scr_next.n = (uint8_t)sv.len;
        scr_next.kind = SK_STATE;
        for (i = 0; i < sv.len; i++) scr_next.m[i] = MV_INV(sv.sol[sv.len - 1 - i]);
        next_ok = 1;
        sv_busy = 0;
        sv_found++;
        sv_last_ms = plat_ms() - sv_started_at;
        sv_nodes_last = sv.nodes;
    } else if (r < 0)
        sv_busy = 0;                     /* (never seen: start again next frame) */
}

static void scr_take(scr_t *o)
{
    if (set.scr_type == SCR_MOVE) {
        o->n = (uint8_t)cube_scramble_moves(o->m, set.scr_len, &crng);
        o->kind = SK_MOVE;
        return;
    }
    if (next_ok) {
        *o = scr_next;
        next_ok = 0;
    } else {
        int n = solve_bank(cube_rand(&crng), o->m);
        o->n = (uint8_t)(n > 0 ? n : 0);
        o->kind = SK_BANK;
    }
    if (!sv_busy) sv_begin();
}

static void scr_text(const scr_t *s, char *t, int max) { cube_print(s->m, s->n, t, max); }

/* ------------------------------------------------------------------ audio --- */
static float gain_now;
void app_render(int32_t *lr, uint32_t n, uint32_t master_q12)
{
    uint32_t i;
    float gain = (float)master_q12 / 4096.0f * 8300000.0f;
    gain_now += (gain - gain_now) * 0.5f;
    for (i = 0; i < 2u * n; i++) lr[i] = 0;
    chime_render(lr, n, gain_now);
}

void app_panic(void) { chime_stop(); }

/* --------------------------------------------------------------------- UI --- */
enum { P_MENU, P_TIMER, P_CUBE, P_ALGS, P_DRILL, P_STATS, P_SET, P_KEYEDIT, P_N };
static const char *const PAGE_NAME[P_N] = {"MENU", "TIMER", "CUBE", "ALGS", "DRILL", "STATS", "SETTINGS", "KEYS"};
#define MENU_N 6                         /* TIMER .. SETTINGS */

#define R_HEAD 1u
#define R_MAIN 2u
#define R_FOOT 4u
#define R_ALL 7u
#define MAIN_Y 32
#define MAIN_H 176

/* the timer (TIMER, and DRILL on a real cube) */
enum { T_IDLE, T_INSPECT, T_HOLD, T_READY, T_RUN, T_DONE };
#define HOLD_MS 300u

typedef struct {
    uint8_t st;
    uint8_t pen;                         /* from inspection */
    uint8_t wait_up;                     /* every key let go first */
    uint8_t beeped;
    uint32_t t_insp, t_hold, t_start, t_stop;
} tmr_t;

/* a virtual cube on the screen, with its turn being drawn */
typedef struct {
    cube_t c, prev;                      /* now, and before the move being drawn */
    int8_t anim_base, anim_turns;
    uint32_t anim_t0;
    uint8_t hist[200];
    uint8_t hn;
} vcube_t;

static struct {
    uint8_t page, menu_at, redraw;
    char toast[2][32];
    uint32_t toast_until, drawn_at;
    uint32_t keys, btn;                  /* held (from the queued edges) */
    uint32_t btn_at[NB];
    uint8_t sel_used;                    /* SEL was held with a key: not a press of its own */
    /* TIMER */
    tmr_t tm;
    scr_t scr;
    uint8_t del_armed;
    uint32_t del_at;
    /* CUBE */
    vcube_t v;                           /* CUBE's */
    vcube_t dv;                          /* ALGS' and DRILL's */
    mat3_t cam, dcam;
    scr_t fscr;                          /* the scramble being followed */
    uint8_t follow, fi, fhalf, fdev;     /* following it: the next move, a half turn half done, moves off it */
    uint8_t vt_armed, vt_run;            /* the virtual solve's timer: waiting for the first turn, running */
    uint32_t vt_start, vt_ms;
    uint8_t show_map;
    /* ALGS */
    uint8_t play;                        /* the formula being played */
    uint8_t pm[ALG_MAX], pn, pi;
    uint32_t p_next;
    /* DRILL */
    uint8_t d_state, d_mode, d_alg, d_auf;
    sr_round_t d_round;
    uint8_t d_setup[ALG_MAX + 1], d_setup_n;
    uint32_t d_ms;
    uint8_t d_moves;
    /* SETTINGS */
    uint8_t set_row, key_sel, clear_armed, stats_at;
    uint8_t lit_keys_dbg;
    uint32_t lit;
} ui;

enum { D_SETUP, D_CARD, D_ANSWER, D_DONE };
enum { DM_RECOGNIZE, DM_REAL, DM_VIRTUAL };
static const char *const DM_NAME[3] = {"RECOGNIZE", "REAL CUBE", "VIRTUAL"};

static void toast(const char *a, const char *b)
{
    uint32_t i;
    for (i = 0; i < 31u && a[i]; i++) ui.toast[0][i] = a[i];
    ui.toast[0][i] = 0;
    for (i = 0; i < 31u && b[i]; i++) ui.toast[1][i] = b[i];
    ui.toast[1][i] = 0;
    ui.toast_until = plat_ms() + 1500u;
    ui.redraw |= R_FOOT;
}

void app_say(const char *a, const char *b)
{
    char t[72];
    toast(a ? a : "", b ? b : "");
    if (ann_on()) {
        t[0] = 0;
        tcat(t, ui.toast[0]);
        tcat(t, " ");
        tcat(t, ui.toast[1]);
        ann_say(t, 0, ANN_NOW);
    }
}

static void go(int page)
{
    if (page == P_MENU && ui.page >= P_TIMER && ui.page <= P_SET) ui.menu_at = (uint8_t)(ui.page - 1);   /* where you were */
    if (page == P_DRILL) ui.d_state = D_SETUP;
    ui.page = (uint8_t)page;
    ui.play = 0;
    ui.show_map = 0;
    ui.redraw = R_ALL;
    app_say(PAGE_NAME[page], "");
}

/* ------------------------------------------------------- the virtual cube --- */
static void vc_reset(vcube_t *v)
{
    cube_reset(&v->c);
    v->prev = v->c;
    v->anim_base = -1;
    v->hn = 0;
}

static void vc_move(vcube_t *v, int m, int record)
{
    v->prev = v->c;
    cube_move(&v->c, m);
    v->anim_base = set.anim ? (int8_t)MV_BASE(m) : -1;
    v->anim_turns = (int8_t)MV_TURNS(m);
    v->anim_t0 = plat_ms();
    if (record) {
        if (v->hn >= sizeof v->hist) {
            uint32_t i;
            for (i = 1; i < sizeof v->hist; i++) v->hist[i - 1] = v->hist[i];
            v->hn--;
        }
        v->hist[v->hn++] = (uint8_t)m;
    }
}

#define ANIM_MS 110u
static int vc_animating(const vcube_t *v) { return v->anim_base >= 0 && plat_ms() - v->anim_t0 < ANIM_MS * (v->anim_turns == 2 ? 2u : 1u); }

static void vc_build(const vcube_t *v, const mat3_t *cam, int cx, int cy, float scale, const uint8_t *scheme)
{
    if (vc_animating(v)) {
        uint32_t len = ANIM_MS * (v->anim_turns == 2 ? 2u : 1u);
        float p = (float)(plat_ms() - v->anim_t0) / (float)len;
        float deg = (v->anim_turns == 3 ? -90.0f : v->anim_turns == 2 ? 180.0f : 90.0f) * p;
        view_build(&v->prev, cam, cx, cy, scale, v->anim_base, deg, scheme);
    } else
        view_build(&v->c, cam, cx, cy, scale, -1, 0.0f, scheme);
}

static void cam_preset(void) { m3_view(&ui.cam, VIEW_YAW[set.view], VIEW_PITCH[set.view]); }

/* the key a move wants, lit (LED hints) */
static int hint_key = -1, hint_sel;

static void set_hint(int mv)
{
    int twice;
    hint_key = mv < 0 ? -1 : km_key_for(keymap, mv, &hint_sel, &twice);
    if (hint_key < 0) hint_sel = 0;
}

/* ----------------------------------------------------------------- TIMER --- */
static void timer_new_scramble(void)
{
    scr_take(&ui.scr);
    ui.redraw |= R_ALL;
}

static int timer_holding(void)           /* the start condition is held */
{
    if (set.start == START_ENDS) return (ui.keys & 1u) && (ui.keys >> 26 & 1u);
    return ui.keys != 0 || (ui.btn >> B_PLAY & 1u);
}

static void timer_finish(tmr_t *tm, uint32_t t, int dnf)
{
    int32_t ms = (int32_t)(t - tm->t_start);
    int pen = dnf ? ST_DNF_PEN : tm->pen;
    tm->t_stop = t;
    tm->st = T_DONE;
    tm->wait_up = ui.keys != 0 || (ui.btn >> B_PLAY & 1u);   /* the key that stopped it lets go first */
    if (ui.page == P_TIMER) {
        sol_add(dnf ? 0 : ms, pen, ui.scr.m, ui.scr.n);
        timer_new_scramble();
        {
            char b[16];
            st_format(st_with_penalty(dnf ? 0 : ms, pen), b);
            app_say(pen == ST_PLUS2 ? "TIME +2" : "TIME", b);
        }
    }
    ui.redraw |= R_ALL;
}

/* an edge for the timer: key or button (kind), down or up, at t. Returns 1 if it took it */
static int timer_edge(tmr_t *tm, int kind, int idx, int down, uint32_t t)
{
    int startish = kind == EV_KEY || idx == B_PLAY;
    switch (tm->st) {
    case T_RUN:
        if (down) {
            timer_finish(tm, t, 0);
            return 1;
        }
        return 0;
    case T_IDLE:
    case T_DONE:
    case T_INSPECT:
        if (!startish) return 0;
        if (tm->wait_up) {
            if (!down && !ui.keys && !(ui.btn >> B_PLAY & 1u)) tm->wait_up = 0;
            return 1;
        }
        if (down && timer_holding()) {
            if (set.inspect && tm->st != T_INSPECT && ui.page == P_TIMER) {
                tm->st = T_INSPECT;      /* a press starts the inspection; the next hold starts the solve */
                tm->t_insp = t;
                tm->pen = ST_OK;
                tm->beeped = 0;
                tm->wait_up = 1;
                app_say("INSPECTION", "15");
            } else {
                if (tm->st != T_INSPECT) tm->pen = ST_OK;
                tm->st = T_HOLD;
                tm->t_hold = t;
            }
            ui.redraw |= R_ALL;
        }
        return 1;
    case T_HOLD:
    case T_READY:
        if (!startish) return 0;
        if (!down && !timer_holding()) {
            if (tm->st == T_READY) {
                tm->st = T_RUN;
                tm->t_start = t;
                if (set.inspect && tm->t_insp && ui.page == P_TIMER) {
                    uint32_t used = t - tm->t_insp;
                    if (used > 17000u) {
                        timer_finish(tm, t, 1);
                        return 1;
                    }
                    if (used > 15000u) tm->pen = ST_PLUS2;
                }
            } else
                tm->st = tm->t_insp && set.inspect && ui.page == P_TIMER ? T_INSPECT : T_IDLE;
            ui.redraw |= R_ALL;
        }
        return 1;
    }
    return 0;
}

static void timer_poll(tmr_t *tm, uint32_t now)   /* the hold turning ready, the inspection's beeps and end */
{
    if (tm->st == T_HOLD && timer_holding() && now - tm->t_hold >= HOLD_MS) {
        tm->st = T_READY;
        ui.redraw |= R_ALL;
    }
    if ((tm->st == T_INSPECT || tm->st == T_HOLD || tm->st == T_READY) && set.inspect && tm->t_insp
        && ui.page == P_TIMER) {
        uint32_t used = now - tm->t_insp;
        if (used >= 8000u && !(tm->beeped & 1u)) tm->beeped |= 1u, chime_play(CHIME_TICK), app_say("8 SECONDS", "");
        if (used >= 12000u && !(tm->beeped & 2u)) tm->beeped |= 2u, chime_play(CHIME_TICK), app_say("12 SECONDS", "");
        if (used > 17000u && tm->st != T_READY) {
            tm->t_start = now;
            timer_finish(tm, now, 1);
        }
    }
}

static void timer_input_enc(int e, int32_t d)
{
    if (e == EN_K1 && sol.n && ui.tm.st == T_DONE) {   /* the last solve: OK, +2, DNF */
        solve_rec_t *r = &sol.s[sol.n - 1];
        int pen = (SOL_PEN(r) + (d > 0 ? 1 : 2)) % 3;
        char b[16];
        r->t = (r->t & 0x0FFFFFFFu) | (uint32_t)pen << 28;
        sol_save();
        st_format(sol_time(sol.n - 1), b);
        app_say(pen == ST_OK ? "OK" : pen == ST_PLUS2 ? "+2" : "DNF", b);
        ui.redraw |= R_ALL;
    }
}

/* ------------------------------------------------------------------ CUBE --- */
static void follow_start(void)
{
    scr_take(&ui.fscr);
    vc_reset(&ui.v);
    ui.follow = 1;
    ui.fi = 0;
    ui.fhalf = 0;
    ui.fdev = 0;
    ui.vt_armed = 0;
    ui.vt_run = 0;
    ui.redraw = R_ALL;
    app_say("SCRAMBLE", "FOLLOW THE LIT KEYS");
}

static void follow_done(void)
{
    ui.follow = 0;
    ui.vt_armed = 1;                     /* the clock starts at the first turn */
    ui.v.hn = 0;
    app_say("SCRAMBLED", "SOLVE IT");
}

static void cube_apply_scramble_now(void)
{
    int i;
    scr_take(&ui.fscr);
    vc_reset(&ui.v);
    for (i = 0; i < ui.fscr.n; i++) cube_move(&ui.v.c, ui.fscr.m[i]);
    ui.v.prev = ui.v.c;
    ui.follow = 0;
    follow_done();
    ui.redraw = R_ALL;
}

static void cube_turn(int m)
{
    vc_move(&ui.v, m, 1);
    if (ui.follow) {
        int e = ui.fscr.m[ui.fi];
        if (ui.fdev) ui.fdev++;
        else if (MV_BASE(m) == MV_BASE(e) && MV_TURNS(e) == 2 && MV_TURNS(m) != 2) {
            if (ui.fhalf && ui.fhalf != MV_TURNS(m)) ui.fdev = 2, ui.fhalf = 0;   /* R then R': off */
            else if (ui.fhalf) ui.fhalf = 0, ui.fi++;
            else ui.fhalf = (uint8_t)MV_TURNS(m);
        } else if (m == e && !ui.fhalf)
            ui.fi++;
        else
            ui.fdev = 1;
        if (ui.fi >= ui.fscr.n) follow_done();
    } else if (ui.vt_armed && MV_BASE(m) < MB_x) {
        ui.vt_armed = 0;
        ui.vt_run = 1;
        ui.vt_start = plat_ms();
    }
    if (ui.vt_run && cube_solved(&ui.v.c)) {
        char b[16];
        ui.vt_run = 0;
        ui.vt_ms = plat_ms() - ui.vt_start;
        st_format((int32_t)ui.vt_ms, b);
        chime_play(CHIME_DONE);
        app_say("SOLVED", b);
    }
    ui.redraw |= R_ALL;
}

static void cube_undo(void)
{
    if (!ui.v.hn) return;
    vc_move(&ui.v, MV_INV(ui.v.hist[ui.v.hn - 1]), 0);
    ui.v.hn--;
    if (ui.follow) {
        if (ui.fdev) ui.fdev--;
        else if (ui.fhalf) ui.fhalf = 0;
        else if (ui.fi) {
            ui.fi--;
            if (MV_TURNS(ui.fscr.m[ui.fi]) == 2) ui.fhalf = (uint8_t)MV_TURNS(ui.v.hist[ui.v.hn]);
        }
    }
    ui.redraw |= R_ALL;
}

static int key_action_move(int a)        /* SEL held: the other way */
{
    int m = km_move(a);
    if ((ui.btn >> B_SEL & 1u) && MV_TURNS(m) != 2) m = MV_INV(m);
    return m;
}

static void cube_hints(void)
{
    if (!ui.follow) {
        set_hint(-1);
        return;
    }
    if (ui.fdev) {                       /* off the scramble: undo */
        int k;
        hint_key = -1;
        hint_sel = 0;
        for (k = 0; k < NKEYS_CUBE; k++)
            if (keymap[k] == KA_UNDO) hint_key = k;
        return;
    }
    set_hint(ui.fhalf ? MV(MV_BASE(ui.fscr.m[ui.fi]), ui.fhalf) : ui.fscr.m[ui.fi]);
}

/* ------------------------------------------------------------------ ALGS --- */
static int alg_cur(void)
{
    int i = alg_index(set.alg_set, set.alg_pos);
    return i < 0 ? 0 : i;
}

static void algs_play_start(void)
{
    int n = alg_moves(alg_cur(), ui.pm);
    if (n <= 0) return;
    ui.pn = (uint8_t)n;
    ui.pi = 0;
    alg_case(alg_cur(), &ui.dv.c, 0);
    ui.dv.prev = ui.dv.c;
    ui.dv.anim_base = -1;
    ui.dv.hn = 0;
    ui.play = 1;
    ui.p_next = plat_ms() + 700u;
    m3_view(&ui.dcam, VIEW_YAW[0], VIEW_PITCH[0]);
    ui.redraw = R_ALL;
}

static void algs_play_poll(uint32_t now)
{
    if (!ui.play || (int32_t)(now - ui.p_next) < 0) return;
    if (ui.pi < ui.pn) {
        vc_move(&ui.dv, ui.pm[ui.pi], 0);
        ui.pi++;
        ui.p_next = now + 650u;
    } else {
        ui.play = 0;
        app_say("DONE", alg_get(alg_cur())->name);
    }
    ui.redraw |= R_ALL;
}

/* ----------------------------------------------------------------- DRILL --- */
static void drill_card(void)
{
    const sr_card_t *c = sr_card(&ui.d_round);
    if (!c) {
        sr_finish(&dst[set.drill_set], &ui.d_round, &dcourse[set.drill_set]);
        drill_save();
        ui.d_state = D_DONE;
        chime_play(CHIME_DONE);
        ui.redraw = R_ALL;
        return;
    }
    ui.d_alg = (uint8_t)alg_index(set.drill_set, c->item);
    ui.d_auf = (uint8_t)(cube_rand(&crng) & 3u);
    if (set.drill_set == ALG_LBL || set.drill_set == ALG_F2L) ui.d_auf = 0;
    ui.d_setup_n = (uint8_t)alg_setup(ui.d_alg, ui.d_setup, ui.d_auf);
    ui.d_state = c->kind == SR_SHOW ? D_ANSWER : D_CARD;
    ui.tm.st = T_IDLE;
    ui.tm.t_insp = 0;
    ui.tm.wait_up = ui.keys != 0;
    ui.d_moves = 0;
    vc_reset(&ui.dv);
    alg_case(ui.d_alg, &ui.dv.c, ui.d_auf);
    ui.dv.prev = ui.dv.c;
    m3_view(&ui.dcam, VIEW_YAW[0], VIEW_PITCH[0]);
    ui.redraw = R_ALL;
}

static void drill_start(void)
{
    entropy(plat_ms());
    sr_build(&dst[set.drill_set], &ui.d_round, &dcourse[set.drill_set], 12, SR_FOCUS_SEGS, cube_rand(&crng));
    ui.d_mode = set.drill_mode;
    drill_card();
}

static void drill_grade(int grade)
{
    const sr_card_t *c = sr_card(&ui.d_round);
    if (!c) return;
    sr_answer(&dst[set.drill_set], &ui.d_round, grade);
    chime_play(grade == SR_WRONG ? CHIME_ERROR : CHIME_UP);
    drill_card();
}

static void drill_real_done(void)        /* REAL: the time into the case's average */
{
    uint32_t cs = (ui.tm.t_stop - ui.tm.t_start) / 10u;
    uint16_t *a = &drec.real_cs[ui.d_alg < ALG_ROOM ? ui.d_alg : 0];
    if (cs > 60000u) cs = 60000u;
    *a = (uint16_t)(*a ? (*a * 3u + cs) / 4u : cs);
    ui.d_ms = ui.tm.t_stop - ui.tm.t_start;
    ui.d_state = D_ANSWER;
    ui.redraw = R_ALL;
}

static void drill_virtual_turn(int m)
{
    vc_move(&ui.dv, m, 1);
    if (!ui.d_moves) ui.tm.t_start = plat_ms();
    if (MV_BASE(m) < MB_x) ui.d_moves++;
    if (cube_solved_turn(&ui.dv.c)) {
        uint8_t mv[ALG_MAX];
        int n = alg_moves(ui.d_alg, mv);
        ui.d_ms = plat_ms() - ui.tm.t_start;
        ui.d_state = D_ANSWER;
        app_say("SOLVED", ui.d_moves <= n + 2 ? "RIGHT" : "SLOW");
        ui.redraw = R_ALL;
    }
    ui.redraw |= R_ALL;
}

static void drill_hints(void)
{
    uint8_t mv[ALG_MAX];
    int n, i;
    if (ui.page != P_DRILL || ui.d_state != D_CARD || ui.d_mode != DM_VIRTUAL || !set.hints) {
        if (ui.page == P_DRILL) set_hint(-1);
        return;
    }
    n = alg_moves(ui.d_alg, mv);
    if (ui.d_auf) {                      /* the case came turned: a U turn first */
        for (i = n; i > 0; i--) mv[i] = mv[i - 1];
        mv[0] = MV(MB_U, 4 - ui.d_auf);
        n++;
    }
    {                                    /* the moves so far, merged (U U is U2), against the formula */
        uint8_t h[200];
        int hn = ui.dv.hn, j;
        for (j = 0; j < hn; j++) h[j] = ui.dv.hist[j];
        hn = cube_merge(h, hn);
        for (i = 0; i < hn && i < n && h[i] == mv[i]; i++) {}
        if (i == hn) set_hint(i < n ? mv[i] : -1);
        else if (i == hn - 1 && i < n && MV_BASE(h[i]) == MV_BASE(mv[i]) && MV_TURNS(mv[i]) == 2 && MV_TURNS(h[i]) != 2)
            set_hint(h[i]);              /* half of a half turn: the same again */
        else
            set_hint(-1);
    }
}

/* -------------------------------------------------------------- SETTINGS --- */
enum { SR_SCR, SR_LEN, SR_INSPECT, SR_START, SR_KEYS, SR_EDIT, SR_ANIM, SR_HINTS, SR_CLEAR, SR_N };
static const char *const SROW_NAME[SR_N] = {"SCRAMBLE", "MOVES", "INSPECTION", "START", "KEYS", "EDIT KEYS",
                                            "TURNS DRAWN", "HINT LIGHTS", "CLEAR SESSION"};

static void setting_value(int row, char *t)
{
    t[0] = 0;
    switch (row) {
    case SR_SCR: tcat(t, set.scr_type == SCR_STATE ? "RANDOM STATE" : "RANDOM MOVE"); break;
    case SR_LEN: tint(t, set.scr_len); break;
    case SR_INSPECT: tcat(t, set.inspect ? "15 S" : "OFF"); break;
    case SR_START: tcat(t, set.start == START_ANY ? "ANY KEY" : "BOTH ENDS"); break;
    case SR_KEYS: tcat(t, km_name(set.layout)); break;
    case SR_EDIT: tcat(t, "PLAY"); break;
    case SR_ANIM: tcat(t, set.anim ? "ON" : "OFF"); break;
    case SR_HINTS: tcat(t, set.hints ? "ON" : "OFF"); break;
    case SR_CLEAR: tint(t, sol.n), tcat(t, ui.clear_armed ? " - PLAY AGAIN" : sol.n == 1 ? " SOLVE" : " SOLVES"); break;
    }
}

static void setting_step(int row, int d)
{
    char t[24];
    switch (row) {
    case SR_SCR: set.scr_type ^= 1u; next_ok = 0; break;
    case SR_LEN: set.scr_len = (uint8_t)clamp(set.scr_len + d, 15, SCR_MAX); break;
    case SR_INSPECT: set.inspect ^= 1u; break;
    case SR_START: set.start ^= 1u; break;
    case SR_KEYS: set.layout = (uint8_t)((set.layout + KM_CUSTOM + 1 + (d > 0 ? 1 : -1)) % (KM_CUSTOM + 1)); keymap_load(); break;
    case SR_ANIM: set.anim ^= 1u; break;
    case SR_HINTS: set.hints ^= 1u; break;
    default: return;
    }
    if (row == SR_SCR) timer_new_scramble();
    setting_value(row, t);
    app_say(SROW_NAME[row], t);
    set_changed();
    ui.redraw |= R_ALL;
}

/* ------------------------------------------------------------------ input --- */
static void key_down(int k, uint32_t t)
{
    int a = keymap[k];
    entropy(t ^ (uint32_t)k << 20);
    if (ui.btn >> B_SEL & 1u) ui.sel_used = 1;
    switch (ui.page) {
    case P_CUBE:
        if (a == KA_UNDO) {
            if (ui.btn >> B_SEL & 1u) {
                vc_reset(&ui.v);
                ui.follow = 0;
                ui.vt_armed = ui.vt_run = 0;
                app_say("RESET", "SOLVED");
                ui.redraw = R_ALL;
            } else
                cube_undo();
        } else if (a != KA_NONE)
            cube_turn(key_action_move(a));
        break;
    case P_DRILL:
        if (ui.d_state == D_CARD && ui.d_mode == DM_VIRTUAL) {
            if (a == KA_UNDO) {
                if (ui.dv.hn) {
                    vc_move(&ui.dv, MV_INV(ui.dv.hist[ui.dv.hn - 1]), 0);
                    ui.dv.hn--;
                    ui.redraw |= R_ALL;
                }
            } else if (a != KA_NONE)
                drill_virtual_turn(key_action_move(a));
        }
        break;
    case P_KEYEDIT:
        ui.key_sel = (uint8_t)k;
        ui.redraw |= R_ALL;
        {
            char b[8];
            if (keymap[k] == KA_UNDO) app_say("KEY", "UNDO");
            else if (keymap[k] == KA_NONE) app_say("KEY", "NOTHING");
            else cube_mv_name(keymap[k], b), app_say("KEY", b);
        }
        break;
    default: break;
    }
}

static void button_down(int b, uint32_t t)
{
    entropy(t ^ (uint32_t)b << 24);
    ui.btn_at[b] = t;
    if (b == B_SEL) ui.sel_used = 0;
    if (b == B_HOME) {
        if (ui.page == P_KEYEDIT) {
            set_save();
            go(P_SET);
        } else if (ui.page == P_MENU)
            go(ui.menu_at + 1);
        else
            go(P_MENU);
        return;
    }
    switch (ui.page) {
    case P_MENU:
        if (b == B_PLAY) go(ui.menu_at + 1);
        else if (b == B_OCTUP || b == B_OCTDN) {
            ui.menu_at = (uint8_t)((ui.menu_at + MENU_N + (b == B_OCTUP ? 1 : -1)) % MENU_N);
            ui.redraw |= R_ALL;
        }
        break;
    case P_TIMER:
        if (ui.tm.st == T_RUN || ui.tm.st == T_HOLD || ui.tm.st == T_READY) break;
        if (b == B_REC) {
            timer_new_scramble();
            app_say("SCRAMBLE", "NEW");
        } else if (b == B_SEL && sol.n && ui.tm.st == T_DONE) {
            if (ui.del_armed && t - ui.del_at < 2500u) {
                sol.n--;
                sol_save();
                ui.del_armed = 0;
                ui.tm.st = T_IDLE;
                app_say("DELETED", "");
            } else {
                ui.del_armed = 1;
                ui.del_at = t;
                app_say("DELETE?", "SEL AGAIN");
            }
            ui.redraw |= R_ALL;
        }
        break;
    case P_CUBE:
        if (b == B_REC) ui.btn_at[B_REC] = t;   /* tap: follow, hold: at once (on release) */
        else if (b == B_PLAY) {
            ui.vt_run = 0;
            ui.vt_armed = !cube_solved(&ui.v.c);
            app_say("TIMER", ui.vt_armed ? "AT THE FIRST TURN" : "SCRAMBLE FIRST (REC)");
            ui.redraw |= R_ALL;
        }
        break;
    case P_ALGS:
        if (b == B_PLAY) {
            if (ui.play) ui.play = 0, ui.redraw = R_ALL;
            else algs_play_start();
        }
        break;
    case P_DRILL:
        if (ui.d_state == D_SETUP || ui.d_state == D_DONE) {
            if (b == B_PLAY) drill_start();
        } else if (ui.d_state == D_CARD) {
            if (ui.d_mode == DM_RECOGNIZE && b == B_PLAY) ui.d_state = D_ANSWER, ui.redraw = R_ALL;
            else if (b == B_REC) ui.d_state = D_ANSWER, ui.d_ms = 0, ui.redraw = R_ALL, ui.d_moves = 0xFF;   /* give up */
        } else if (ui.d_state == D_ANSWER) {
            const sr_card_t *c = sr_card(&ui.d_round);
            int show = c && c->kind == SR_SHOW;
            if (b == B_PLAY) {
                uint8_t mv[ALG_MAX];
                int n = alg_moves(ui.d_alg, mv);
                drill_grade(show ? SR_RIGHT
                            : ui.d_moves == 0xFF ? SR_WRONG
                            : ui.d_mode == DM_VIRTUAL && ui.d_moves > n + 2 ? SR_SLOW
                                                                          : SR_RIGHT);
            } else if (b == B_REC && !show)
                drill_grade(SR_WRONG);
            else if (b == B_SEL && !show)
                drill_grade(SR_SLOW);
        }
        break;
    case P_STATS:
        break;
    case P_SET:
        if (b == B_PLAY) {
            if (ui.set_row == SR_EDIT) {
                int k;
                if (set.layout != KM_CUSTOM) {
                    for (k = 0; k < NKEYS_CUBE; k++) set.custom[k] = keymap[k];
                    set.layout = KM_CUSTOM;
                    keymap_load();
                }
                ui.key_sel = 0;
                go(P_KEYEDIT);
            } else if (ui.set_row == SR_CLEAR) {
                if (ui.clear_armed) {
                    sol.n = 0;
                    sol_save();
                    ui.clear_armed = 0;
                    app_say("SESSION", "CLEARED");
                } else {
                    ui.clear_armed = 1;
                    app_say("CLEAR?", "PLAY AGAIN");
                }
                ui.redraw |= R_ALL;
            } else
                setting_step(ui.set_row, 1);
        } else if (b == B_OCTUP || b == B_OCTDN)
            setting_step(ui.set_row, b == B_OCTUP ? 1 : -1);
        break;
    default: break;
    }
}

static void button_up(int b, uint32_t t)
{
    if (ui.page == P_CUBE && b == B_REC) {
        if (t - ui.btn_at[B_REC] >= 600u) cube_apply_scramble_now();
        else follow_start();
    }
    if (b == B_SEL && ui.page == P_CUBE && !ui.sel_used && t - ui.btn_at[B_SEL] < 600u) {
        ui.show_map ^= 1u;               /* SEL alone: the key map */
        ui.redraw |= R_ALL;
    }
}

static void enc_turn(int e, int32_t d)
{
    char t[40];
    switch (ui.page) {
    case P_MENU:
        if (e == EN_SELECT) {
            ui.menu_at = (uint8_t)((ui.menu_at + MENU_N * 8 + d) % MENU_N);
            app_say(PAGE_NAME[ui.menu_at + 1], "");
            ui.redraw |= R_ALL;
        }
        break;
    case P_TIMER: timer_input_enc(e, d); break;
    case P_CUBE:
        if (e == EN_K1) m3_turn(&ui.cam, 0, 15.0f * (float)d);
        else if (e == EN_K2) m3_turn(&ui.cam, 1, 15.0f * (float)d);
        else if (e == EN_K3) m3_turn(&ui.cam, 2, 15.0f * (float)d);
        else if (e == EN_K4) {
            set.view = (uint8_t)((set.view + VIEW_PRESETS * 8 + d) % VIEW_PRESETS);
            cam_preset();
            app_say("VIEW", VIEW_NAME[set.view]);
            set_changed();
        }
        ui.redraw |= R_MAIN;
        break;
    case P_ALGS:
        if (e == EN_SELECT) {
            set.alg_set = (uint8_t)((set.alg_set + ALG_SETS * 8 + d) % ALG_SETS);
            set.alg_pos = 0;
            app_say(alg_set_name(set.alg_set), "");
        } else if (e == EN_PRESET || e == EN_K1) {
            int n = alg_count(set.alg_set);
            set.alg_pos = (uint8_t)((set.alg_pos + n * 8 + d) % n);
            app_say(alg_get(alg_cur())->name, "");
        } else
            break;
        ui.play = 0;
        set_changed();
        ui.redraw = R_ALL;
        break;
    case P_DRILL:
        if (ui.d_state != D_SETUP && ui.d_state != D_DONE) break;
        if (e == EN_SELECT) {
            set.drill_set = (uint8_t)((set.drill_set + ALG_SETS * 8 + d) % ALG_SETS);
            app_say(alg_set_name(set.drill_set), "");
        } else if (e == EN_K1 || e == EN_PRESET) {
            set.drill_mode = (uint8_t)((set.drill_mode + 3 * 8 + d) % 3);
            app_say(DM_NAME[set.drill_mode], "");
        } else
            break;
        ui.d_state = D_SETUP;
        set_changed();
        ui.redraw = R_ALL;
        break;
    case P_STATS:
        if (e == EN_SELECT && sol.n) {
            int at = (int)ui.stats_at + d;
            ui.stats_at = (uint8_t)clamp(at, 0, sol.n - 1);
            ui.redraw |= R_ALL;
        }
        break;
    case P_SET:
        if (e == EN_SELECT) {
            ui.set_row = (uint8_t)((ui.set_row + SR_N * 8 + d) % SR_N);
            ui.clear_armed = 0;
            setting_value(ui.set_row, t);
            app_say(SROW_NAME[ui.set_row], t);
            ui.redraw |= R_ALL;
        } else if (e == EN_K1 || e == EN_PRESET)
            setting_step(ui.set_row, d);
        break;
    case P_KEYEDIT:
        if (e == EN_SELECT || e == EN_K1) {
            int a = set.custom[ui.key_sel], i;
            for (i = 0; i < (d > 0 ? d : -d); i++) a = km_action_next(a, d);
            set.custom[ui.key_sel] = (uint8_t)a;
            keymap_load();
            if (a == KA_UNDO) app_say("KEY", "UNDO");
            else if (a == KA_NONE) app_say("KEY", "NOTHING");
            else cube_mv_name(a, t), app_say("KEY", t);
            set_changed();
            ui.redraw |= R_ALL;
        }
        break;
    }
}

static void input(void)
{
    uint32_t now = plat_ms();
    int e;
    while (ev_r != ev_w) {
        ev_t ev;
        ev.kind = evq[ev_r % EVQ].kind;
        ev.idx = evq[ev_r % EVQ].idx;
        ev.down = evq[ev_r % EVQ].down;
        ev.t = evq[ev_r % EVQ].t;
        ev_r++;
        if (ev.kind == EV_KEY) {
            if (ev.down) ui.keys |= 1u << ev.idx;
            else ui.keys &= ~(1u << ev.idx);
        } else {
            if (ev.down) ui.btn |= 1u << ev.idx;
            else ui.btn &= ~(1u << ev.idx);
        }
        /* the timer takes its edges first */
        if (ui.page == P_TIMER && !(ev.kind == EV_BTN && ev.idx == B_HOME)
            && timer_edge(&ui.tm, ev.kind, ev.idx, ev.down, ev.t))
            continue;
        if (ui.page == P_DRILL && ui.d_state == D_CARD && ui.d_mode == DM_REAL && !(ev.kind == EV_BTN && ev.idx == B_HOME)
            && !(ev.kind == EV_BTN && ev.idx == B_REC)) {
            uint8_t before = ui.tm.st;
            timer_edge(&ui.tm, ev.kind, ev.idx, ev.down, ev.t);
            if (before == T_RUN && ui.tm.st == T_DONE) drill_real_done();
            continue;
        }
        if (ev.kind == EV_KEY) {
            if (ev.down) key_down(ev.idx, ev.t);
        } else if (ev.down)
            button_down(ev.idx, ev.t);
        else
            button_up(ev.idx, ev.t);
    }
    for (e = 0; e < NE; e++) {
        int32_t d = plat_enc(e);
        if (d) enc_turn(e, d);
    }
    if (ui.page == P_TIMER || (ui.page == P_DRILL && ui.d_mode == DM_REAL)) timer_poll(&ui.tm, now);
    if (ui.del_armed && now - ui.del_at > 2500u) ui.del_armed = 0;
}

void app_input_only(void) { input(); }

/* ----------------------------------------------------------------- lights --- */
static void lights(uint32_t now)
{
    ledfx_t kb = {0, 0, 0}, bb = {0, 0, 0};
    int k;
    ledfx_set(&bb, B_HOME, FX_GLOW, now);
    if (ui.page == P_TIMER) {
        if (ui.tm.st == T_READY) bb.lit |= LED_PLAY_GREEN;
        else if (ui.tm.st == T_HOLD || ui.tm.st == T_INSPECT) ledfx_set(&bb, B_PLAY, ui.tm.st == T_HOLD ? FX_LIT : FX_BLINK, now);
        else if (ui.tm.st == T_RUN) ledfx_set(&bb, B_PLAY, FX_PULSE, now);
        ledfx_set(&bb, B_REC, FX_GLOW, now);
        if (set.start == START_ENDS && ui.tm.st != T_RUN) {
            ledfx_set(&kb, 0, FX_GLOW, now);
            ledfx_set(&kb, 26, FX_GLOW, now);
        }
    } else if (ui.page == P_CUBE || (ui.page == P_DRILL && ui.d_mode == DM_VIRTUAL && ui.d_state == D_CARD)
               || (ui.page == P_ALGS && ui.play)) {
        for (k = 0; k < NKEYS_CUBE; k++)
            if (keymap[k] != KA_NONE) ledfx_set(&kb, (uint32_t)k, FX_GLOW, now);
        if (ui.page == P_ALGS && ui.play && ui.pi < ui.pn) set_hint(ui.pm[ui.pi]);
        if (hint_key >= 0 && (set.hints || ui.page != P_DRILL)) {
            ledfx_set(&kb, (uint32_t)hint_key, FX_LIT, now);
            if (hint_sel) ledfx_set(&bb, B_SEL, FX_BLINK, now);
        }
        if (ui.page == P_CUBE) ledfx_set(&bb, B_REC, FX_GLOW, now);
        if (ui.vt_run && ui.page == P_CUBE) bb.lit |= LED_PLAY_GREEN;
    } else if (ui.page == P_ALGS || ui.page == P_DRILL || ui.page == P_MENU || ui.page == P_SET)
        ledfx_set(&bb, B_PLAY, FX_GLOW, now);
    if (ui.page == P_KEYEDIT) ledfx_set(&kb, ui.key_sel, FX_BLINK, now);
    for (k = 0; k < NKEYS_CUBE; k++)
        if (ui.keys >> k & 1u) ledfx_set(&kb, (uint32_t)k, FX_LIT, now);
    ui.lit = kb.lit;
    ledfx_show(&bb, &kb);
}

/* ----------------------------------------------------------------- screen --- */
#define C_GO RGB(80, 220, 120)
#define C_WAIT RGB(240, 90, 80)
#define C_OK RGB(90, 210, 120)
#define C_BAD RGB(235, 90, 80)

static void text_c(int32_t cx, int32_t y, const felucca_font_t *f, const char *s, uint16_t c)
{
    cv_text(cx - text_w(f, s) / 2, y, f, s, c);
}

static void draw_header(void)
{
    char t[40];
    cv_begin(240, 32, C_BLACK);
    t[0] = 0;
    switch (ui.page) {
    case P_TIMER:
        tint(t, sol.n);
        tcat(t, sol.n == 1 ? " SOLVE" : " SOLVES");
        break;
    case P_CUBE:
        if (ui.vt_run || ui.vt_ms) st_format(ui.vt_run ? (int32_t)(plat_ms() - ui.vt_start) : (int32_t)ui.vt_ms, t);
        else tcat(t, km_name(set.layout));
        break;
    case P_ALGS:
        tcat(t, alg_set_short(set.alg_set));
        tcat(t, "  ");
        tint(t, set.alg_pos + 1);
        tcat(t, "/");
        tint(t, alg_count(set.alg_set));
        break;
    case P_DRILL:
        tcat(t, alg_set_short(set.drill_set));
        tcat(t, "  ");
        tcat(t, DM_NAME[ui.d_state == D_SETUP || ui.d_state == D_DONE ? set.drill_mode : ui.d_mode]);
        break;
    case P_STATS:
        tint(t, sol.n);
        tcat(t, " SOLVES");
        break;
    default: break;
    }
    cv_text(8, 8, &FONT_B, PAGE_NAME[ui.page], C_WHITE);
    cv_text(232 - text_w(&FONT_T, t), 11, &FONT_T, t, C_HI);
    cv_rect(0, 31, 240, 1, C_LINE);
    cv_blit(0, 0);
}

static void draw_menu(int32_t oy)
{
    static const char *const SUB[MENU_N] = {"HOLD, LET GO, SOLVE", "TURN IT ON THE KEYS", "THE FORMULA BOOK",
                                            "LEARN THE CASES", "YOUR TIMES", "HOW IT WORKS"};
    int i;
    for (i = 0; i < MENU_N; i++) {
        int32_t y = 6 + i * 28 - oy;
        int on = i == ui.menu_at;
        if (on) cv_rect(4, y - 2, 232, 26, C_LINE);
        cv_text(14, y + 3, &FONT_B, PAGE_NAME[i + 1], on ? C_WHITE : C_GRAY);
        cv_text(232 - text_w(&FONT_XS, SUB[i]), y + 7, &FONT_XS, SUB[i], on ? C_HI : C_DIM);
    }
}

static int32_t timer_shown(uint32_t now)
{
    const tmr_t *tm = &ui.tm;
    if (tm->st == T_RUN) return (int32_t)(now - tm->t_start);
    if (tm->st == T_DONE && sol.n) return sol_time(sol.n - 1);
    return 0;
}

static void draw_stat_line(int32_t x, int32_t y, const char *name, int32_t v)
{
    char b[16];
    cv_text(x, y, &FONT_T, name, C_GRAY);
    if (v == ST_NONE) b[0] = '-', b[1] = 0;
    else st_format(st_round_cs(v), b);
    cv_text(x + 44, y, &FONT_T, b, C_AMB);
}

static void draw_timer(int32_t oy)
{
    char t[200];
    uint32_t now = plat_ms();
    const tmr_t *tm = &ui.tm;
    uint16_t tc = C_WHITE;
    int32_t ms = timer_shown(now);
    if (tm->st == T_RUN) {               /* only the time while solving */
        st_format(ms, t);
        text_c(120, 64 - oy, &FONT_L, t, C_WHITE);
        return;
    }
    if (tm->st == T_INSPECT || ((tm->st == T_HOLD || tm->st == T_READY) && tm->t_insp && set.inspect)) {
        int32_t left = 15 - (int32_t)((now - tm->t_insp) / 1000u);
        t[0] = 0;
        if (left > 0) tint(t, left);
        else tcat(t, left > -2 ? "+2" : "DNF");
        text_c(120, 10 - oy, &FONT_T, "INSPECTION", C_GRAY);
        text_c(120, 34 - oy, &FONT_L, t, left > 3 ? C_HI : C_WAIT);
        if (tm->st != T_INSPECT) text_c(120, 96 - oy, &FONT_B, tm->st == T_READY ? "READY" : "HOLD", tm->st == T_READY ? C_GO : C_WAIT);
        return;
    }
    scr_text(&ui.scr, t, sizeof t);
    gtext_wrap(8, 4 - oy, 224, 18, &FONT_B, t, C_AMB);
    if (tm->st == T_HOLD) tc = C_WAIT;
    else if (tm->st == T_READY) tc = C_GO;
    if (tm->st == T_DONE && sol.n && SOL_PEN(&sol.s[sol.n - 1]) == ST_DNF_PEN) {
        t[0] = 0;
        tcat(t, "DNF");
    } else
        st_format(tm->st == T_HOLD || tm->st == T_READY ? 0 : ms, t);
    text_c(120, 66 - oy, &FONT_L, t, tc);
    if (tm->st == T_DONE && sol.n && SOL_PEN(&sol.s[sol.n - 1]) == ST_PLUS2) cv_text(196, 74 - oy, &FONT_B, "+2", C_WAIT);
    draw_stat_line(8, 120 - oy, "AO5", sol_avg(5));
    draw_stat_line(8, 136 - oy, "AO12", sol_avg(12));
    {
        int32_t all[SOLVES_MAX];
        int n = sol_last(SOLVES_MAX, all);
        draw_stat_line(8, 152 - oy, "BEST", n ? st_best(all, n) : ST_NONE);
    }
    {
        cube_t c;
        cube_reset(&c);
        cube_apply(&c, ui.scr.m, ui.scr.n);
        view_net(&c, 160, 116 - oy, 6, SCHEME_WHITE);
    }
}

static void draw_keymap(int32_t oy, int32_t y0)   /* the keys and what each turns */
{
    static const uint8_t WK[16] = PLAT_WHITE_KEYS, BK[11] = PLAT_BLACK_KEYS;
    static const uint8_t AT[11] = {0, 1, 2, 4, 5, 7, 8, 9, 11, 12, 14};
    int i;
    char b[8];
    for (i = 0; i < 16; i++) {
        int32_t x = 1 + i * 15, y = y0 + 34 - oy;
        int k = WK[i], a = keymap[k];
        int on = ui.page == P_KEYEDIT && ui.key_sel == k;
        cv_rect(x, y, 14, 30, on ? C_HI : RGB(210, 210, 214));
        if (a == KA_UNDO) b[0] = '<', b[1] = 0;
        else if (a == KA_NONE) b[0] = 0;
        else cube_mv_name(a, b);
        text_c(x + 7, y + 16, &FONT_XS, b, C_BLACK);
    }
    for (i = 0; i < 11; i++) {
        int32_t x = 1 + AT[i] * 15 + 9, y = y0 - oy;
        int k = BK[i], a = keymap[k];
        int on = ui.page == P_KEYEDIT && ui.key_sel == k;
        cv_rect(x, y, 12, 32, on ? C_HI : RGB(40, 40, 46));
        if (a == KA_UNDO) b[0] = '<', b[1] = 0;
        else if (a == KA_NONE) b[0] = 0;
        else cube_mv_name(a, b);
        text_c(x + 6, y + 4, &FONT_XS, b, on ? C_BLACK : C_WHITE);
    }
}

static void draw_cube_page(int32_t oy)
{
    if (ui.show_map) {
        text_c(120, 20 - oy, &FONT_T, km_name(set.layout), C_HI);
        text_c(120, 40 - oy, &FONT_XS, "SEL + KEY: THE OTHER WAY.  < UNDO", C_GRAY);
        draw_keymap(oy, 70);
        return;
    }
    view_draw(oy, MAIN_H / 2);
}

static void draw_algs(int32_t oy)
{
    const alg_t *a = alg_get(alg_cur());
    char t[120];
    if (ui.play) {
        view_draw(oy, MAIN_H / 2);
        if (oy) {
            cube_print(ui.pm, ui.pi, t, sizeof t);
            gtext_wrap(8, 150 - oy, 224, 13, &FONT_XS, t[0] ? t : "...", C_HI);
        }
        return;
    }
    if (a->set == ALG_OLL || a->set == ALG_PLL || a->set == ALG_OLL2 || a->set == ALG_PLL2) {
        cube_t c;
        alg_case(alg_cur(), &c, 0);
        view_ll(&c, 8, 8 - oy, 22, a->set == ALG_OLL || a->set == ALG_OLL2, SCHEME_YELLOW);
    } else
        view_draw(oy, MAIN_H / 2);
    {
        int32_t y;
        gtext_wrap(100, 10 - oy, 136, 18, &FONT_B, a->name, C_WHITE);
        y = gtext_wrap(8, 102 - oy, 224, 17, &FONT_B, a->alg, C_HI);
        if (a->alt) {
            t[0] = 0;
            tcat(t, "OR ");
            tcat(t, a->alt);
            gtext_wrap(8, y + 4, 224, 14, &FONT_XS, t, C_GRAY);
        }
    }
}

static void draw_drill(int32_t oy)
{
    const alg_t *a = alg_get(ui.d_alg);
    char t[160];
    int ll = a->set == ALG_OLL || a->set == ALG_PLL || a->set == ALG_OLL2 || a->set == ALG_PLL2;
    if (ui.d_state == D_SETUP || ui.d_state == D_DONE) {
        sr_state_t *s = &dst[set.drill_set];
        text_c(120, 8 - oy, &FONT_B, alg_set_name(set.drill_set), C_WHITE);
        text_c(120, 30 - oy, &FONT_T, DM_NAME[set.drill_mode], C_HI);
        t[0] = 0;
        tint(t, s->open);
        tcat(t, " OF ");
        tint(t, alg_count(set.drill_set));
        tcat(t, " CASES OPEN, ");
        tint(t, sr_due_count(s, &dcourse[set.drill_set], SR_FOCUS_SEGS));
        tcat(t, " DUE");
        text_c(120, 56 - oy, &FONT_XS, t, C_GRAY);
        if (ui.d_state == D_DONE) {
            t[0] = 0;
            tint(t, ui.d_round.ok);
            tcat(t, " RIGHT, ");
            tint(t, ui.d_round.miss);
            tcat(t, " WRONG");
            text_c(120, 84 - oy, &FONT_B, t, C_OK);
        }
        gtext_wrap(12, 112 - oy, 216, 14, &FONT_XS,
                   set.drill_mode == DM_RECOGNIZE ? "SEE THE CASE, SAY ITS FORMULA, PLAY TO CHECK."
                   : set.drill_mode == DM_REAL    ? "SCRAMBLE YOUR CUBE AS SHOWN, HOLD A KEY AND LET GO, SOLVE THE CASE."
                                                  : "TURN THE CASE BACK ON THE KEYS.",
                   C_DIM);
        return;
    }
    if (ui.d_mode == DM_VIRTUAL && ui.d_state == D_CARD) {
        view_draw(oy, MAIN_H / 2);
        return;
    }
    if (ll) {
        cube_t c;
        alg_case(ui.d_alg, &c, ui.d_auf);
        view_ll(&c, 8, 8 - oy, 20, a->set == ALG_OLL || a->set == ALG_OLL2, SCHEME_YELLOW);
    } else
        view_draw(oy, MAIN_H / 2);
    if (ui.d_mode == DM_REAL && ui.d_state == D_CARD) {
        cube_print(ui.d_setup, ui.d_setup_n, t, sizeof t);
        if (ui.tm.st == T_RUN) {
            st_format((int32_t)(plat_ms() - ui.tm.t_start), t);
            text_c(120, 60 - oy, &FONT_L, t, C_WHITE);
            return;
        }
        cv_text(96, 8 - oy, &FONT_XS, "SCRAMBLE YELLOW UP:", C_GRAY);
        gtext_wrap(96, 24 - oy, 140, 15, &FONT_T, t, C_HI);
        text_c(120, 120 - oy, &FONT_B, ui.tm.st == T_READY ? "READY" : ui.tm.st == T_HOLD ? "HOLD" : "HOLD A KEY",
               ui.tm.st == T_READY ? C_GO : ui.tm.st == T_HOLD ? C_WAIT : C_DIM);
        return;
    }
    if (ui.d_state == D_ANSWER) {
        const sr_card_t *c = sr_card(&ui.d_round);
        gtext_wrap(ll ? 92 : 8, 10 - oy, ll ? 144 : 224, 18, &FONT_B, a->name, c && c->kind == SR_SHOW ? C_HI : C_WHITE);
        gtext_wrap(8, 108 - oy, 224, 17, &FONT_B, a->alg, C_HI);
        if (ui.d_mode != DM_RECOGNIZE && ui.d_moves != 0xFF && ui.d_ms) {
            st_format((int32_t)ui.d_ms, t);
            cv_text(ll ? 92 : 150, 52 - oy, &FONT_B, t, C_OK);
        }
        if (ui.d_mode == DM_REAL && drec.real_cs[ui.d_alg]) {
            t[0] = 0;
            tcat(t, "AVG ");
            {
                char b[12];
                st_format(drec.real_cs[ui.d_alg] * 10, b);
                tcat(t, b);
            }
            cv_text(ll ? 92 : 150, 76 - oy, &FONT_T, t, C_GRAY);
        }
    } else
        text_c(ll ? 166 : 120, 40 - oy, &FONT_B, "WHICH ONE?", C_GRAY);
}

static void draw_stats(int32_t oy)
{
    char t[160];
    int i, rows = 10, first;
    if (!sol.n) {
        text_c(120, 60 - oy, &FONT_T, "NO SOLVES YET", C_DIM);
        return;
    }
    if (ui.stats_at >= sol.n) ui.stats_at = 0;
    first = (int)ui.stats_at - rows / 2;
    if (first > sol.n - rows) first = sol.n - rows;
    if (first < 0) first = 0;
    for (i = first; i < sol.n && i < first + rows; i++) {   /* newest at the top: index sol.n - 1 - i */
        int idx = sol.n - 1 - i;
        int32_t y = 2 + (i - first) * 13 - oy;
        int on = i == ui.stats_at;
        t[0] = 0;
        tint(t, idx + 1);
        tcat(t, ".");
        if (on) cv_rect(0, y - 1, 118, 13, C_LINE);
        cv_text(4, y, &FONT_XS, t, C_DIM);
        st_format(sol_time(idx), t);
        if (SOL_PEN(&sol.s[idx]) == ST_PLUS2) tcat(t, "+");
        cv_text(40, y, &FONT_XS, t, on ? C_WHITE : C_AMB);
    }
    draw_stat_line(124, 2 - oy, "MO3", sol_avg(3));
    draw_stat_line(124, 18 - oy, "AO5", sol_avg(5));
    draw_stat_line(124, 34 - oy, "AO12", sol_avg(12));
    draw_stat_line(124, 50 - oy, "AO50", sol_avg(50));
    draw_stat_line(124, 66 - oy, "AO100", sol_avg(100));
    {
        int32_t all[SOLVES_MAX];
        int n = sol_last(SOLVES_MAX, all);
        draw_stat_line(124, 82 - oy, "BEST", st_best(all, n));
        draw_stat_line(124, 98 - oy, "MEAN", st_mean(all, n));
    }
    {
        const solve_rec_t *r = &sol.s[sol.n - 1 - ui.stats_at];
        cube_print(r->m, r->n, t, sizeof t);
        gtext_wrap(4, 136 - oy, 232, 12, &FONT_XS, t, C_GRAY);
    }
}

static void draw_settings(int32_t oy)
{
    char t[40];
    int i;
    for (i = 0; i < SR_N; i++) {
        int32_t y = 4 + i * 19 - oy;
        int on = i == ui.set_row;
        if (on) cv_rect(0, y - 2, 240, 18, C_LINE);
        cv_text(8, y, &FONT_T, SROW_NAME[i], on ? C_WHITE : C_GRAY);
        setting_value(i, t);
        cv_text(232 - text_w(&FONT_T, t), y, &FONT_T, t,
                i == SR_LEN && set.scr_type == SCR_STATE ? C_DIM : on ? C_HI : C_AMB);   /* (moves: random-move only) */
    }
}

static void draw_keyedit(int32_t oy)
{
    char b[8];
    int a = keymap[ui.key_sel];
    text_c(120, 8 - oy, &FONT_T, "PRESS A KEY, TURN SELECT", C_GRAY);
    if (a == KA_UNDO) b[0] = 0, tcat(b, "UNDO");
    else if (a == KA_NONE) b[0] = 0, tcat(b, "-");
    else cube_mv_name(a, b);
    text_c(120, 30 - oy, &FONT_M, b, C_HI);
    draw_keymap(oy, 76);
}

static void build_views(void)            /* the 3D quads, once a frame (both bands draw them) */
{
    if (ui.page == P_CUBE && !ui.show_map) vc_build(&ui.v, &ui.cam, 120, MAIN_H / 2, 30.0f, SCHEME_WHITE);
    else if (ui.page == P_ALGS) {
        const alg_t *a = alg_get(alg_cur());
        if (ui.play) vc_build(&ui.dv, &ui.dcam, 120, 72, 24.0f, SCHEME_YELLOW);
        else if (!(a->set == ALG_OLL || a->set == ALG_PLL || a->set == ALG_OLL2 || a->set == ALG_PLL2)) {
            cube_t c;
            mat3_t cam;
            alg_case(alg_cur(), &c, 0);
            m3_view(&cam, VIEW_YAW[0], VIEW_PITCH[0]);
            view_build(&c, &cam, 50, 50, 16.0f, -1, 0.0f, SCHEME_YELLOW);
        }
    } else if (ui.page == P_DRILL) {
        if (ui.d_mode == DM_VIRTUAL && ui.d_state == D_CARD) vc_build(&ui.dv, &ui.dcam, 120, MAIN_H / 2, 30.0f, SCHEME_YELLOW);
        else {
            cube_t c;
            mat3_t cam;
            alg_case(ui.d_alg, &c, ui.d_auf);
            m3_view(&cam, VIEW_YAW[0], VIEW_PITCH[0]);
            view_build(&c, &cam, 46, 50, 15.0f, -1, 0.0f, SCHEME_YELLOW);
        }
    }
}

static void draw_main(void)
{
    int32_t h;
    build_views();
    for (h = 0; h < 2; h++) {
        int32_t oy = h * MAIN_H / 2;
        cv_begin(240, MAIN_H / 2, C_BLACK);
        switch (ui.page) {
        case P_MENU: draw_menu(oy); break;
        case P_TIMER: draw_timer(oy); break;
        case P_CUBE: draw_cube_page(oy); break;
        case P_ALGS: draw_algs(oy); break;
        case P_DRILL: draw_drill(oy); break;
        case P_STATS: draw_stats(oy); break;
        case P_SET: draw_settings(oy); break;
        case P_KEYEDIT: draw_keyedit(oy); break;
        }
        cv_blit(0, (uint32_t)(MAIN_Y + oy));
    }
}

static const char *footer_hint(void)
{
    switch (ui.page) {
    case P_MENU: return "SELECT: CHOOSE   PLAY: OPEN";
    case P_TIMER:
        if (ui.tm.st == T_RUN) return "ANY KEY: STOP";
        if (ui.tm.st == T_DONE) return "K1: OK +2 DNF   SEL: DELETE   REC: NEW";
        return set.start == START_ENDS ? "HOLD F3 + G5, LET GO   REC: NEW" : "HOLD A KEY, LET GO   REC: NEW";
    case P_CUBE:
        if (ui.follow) return ui.fdev ? "OFF THE SCRAMBLE: UNDO" : "FOLLOW THE LIT KEYS";
        return "REC: SCRAMBLE (HOLD: AT ONCE)  K1-4: VIEW";
    case P_ALGS: return ui.play ? "PLAY: STOP" : "SELECT: SET   K1: CASE   PLAY: SHOW";
    case P_DRILL:
        if (ui.d_state == D_SETUP || ui.d_state == D_DONE) return "SELECT: SET   K1: MODE   PLAY: GO";
        if (ui.d_state == D_ANSWER) {
            const sr_card_t *c = sr_card(&ui.d_round);
            return c && c->kind == SR_SHOW ? "NEW CASE.  PLAY: NEXT" : "PLAY: RIGHT   SEL: SLOW   REC: WRONG";
        }
        return ui.d_mode == DM_RECOGNIZE ? "PLAY: SHOW THE FORMULA" : "REC: GIVE UP";
    case P_STATS: return "SELECT: SCROLL";
    case P_SET:
        if (ui.set_row == SR_SCR && sv_found) {   /* how long the FM-1 took to find the last scramble */
            static char t[64];
            t[0] = 0;
            tcat(t, "LAST: ");
            tint(t, scr_next.n ? scr_next.n : ui.scr.n);
            tcat(t, " MOVES IN ");
            tint(t, (int)(sv_last_ms / 1000u));
            tcat(t, ".");
            tint(t, (int)(sv_last_ms / 100u % 10u));
            tcat(t, " S, ");
            tint(t, (int)(sv_nodes_last / 1000u));
            tcat(t, "K NODES (");
            tint(t, (int)sv_found);
            tcat(t, ")");
            return t;
        }
        return "SELECT: ROW   K1 / OCT: CHANGE";
    case P_KEYEDIT: return "HOME: DONE";
    }
    return "";
}

static void draw_footer(void)
{
    cv_begin(240, 32, C_BLACK);
    cv_rect(0, 0, 240, 1, C_LINE);
    if (plat_ms() < ui.toast_until) {
        int32_t x = cv_text(8, 10, &FONT_T, ui.toast[0], C_GRAY);
        cv_text(x + 8, 7, &FONT_B, ui.toast[1], C_HI);
    } else if (ui.page == P_CUBE && ui.follow && !ui.fdev) {   /* the scramble's next moves */
        char t[80];
        int n = ui.fscr.n - ui.fi;
        cube_print(ui.fscr.m + ui.fi, n > 8 ? 8 : n, t, sizeof t);
        cv_text(8, 8, &FONT_B, t, C_HI);
    } else if (ui.page == P_CUBE && ui.v.hn && !ui.follow) {   /* the last moves */
        char t[80];
        uint8_t m[12];
        int n = ui.v.hn > 10 ? 10 : ui.v.hn, i;
        for (i = 0; i < n; i++) m[i] = ui.v.hist[ui.v.hn - n + i];
        n = cube_merge(m, n);
        cube_print(m, n, t, sizeof t);
        cv_text(232 - text_w(&FONT_T, t), 10, &FONT_T, t, C_AMB);
    } else
        cv_text(8, 10, &FONT_XS, footer_hint(), C_DIM);
    cv_blit(0, 208);
}

static void draw(void)
{
    uint32_t r = ui.redraw;
    ui.redraw = 0;
    ui.drawn_at = plat_ms();
    if (r & R_HEAD) draw_header();
    if (r & R_MAIN) draw_main();
    if (r & R_FOOT) draw_footer();
}

static void speak(void)
{
    if (ann_on() && ann_where()) {
        char t[64];
        t[0] = 0;
        tcat(t, APP_NAME ". ");
        tcat(t, PAGE_NAME[ui.page]);
        ann_say(t, 0, ANN_NOW);
    }
}

/* ------------------------------------------------------------ the contract --- */
void app_init(void)
{
    int r;
    zero(&ui, sizeof ui);
    saves = 0;
    set_dirty_at = 0;
    r = solve_init();
    if (r) host_fails++;
    if (plat_store_load(OBJ_CUBE, &set, sizeof set) != (int)sizeof set || !set_valid(&set)) set_defaults();
    keymap_load();
    if (plat_store_load(OBJ_SOLVES, &sol, sizeof sol) != (int)sizeof sol || sol.magic != SOL_MAGIC
        || sol.n > SOLVES_MAX)
        zero(&sol, sizeof sol);
    drill_courses();
    drill_load();
    vc_reset(&ui.v);
    vc_reset(&ui.dv);
    cam_preset();
    m3_view(&ui.dcam, VIEW_YAW[0], VIEW_PITCH[0]);
    ui.d_state = D_SETUP;
    ui.page = P_TIMER;
    entropy(plat_ms());
    timer_new_scramble();
    set_hint(-1);
}

void app_ui_init(void)
{
    lcd_fill(0, 0, 240, 240, C_BLACK);
    ui.redraw = R_ALL;
}

void app_frame(void)
{
    static uint32_t last_toast, last_sec;
    uint32_t now = plat_ms();
    int busy_ui;
    input();
    algs_play_poll(now);
    if (ui.page == P_CUBE) cube_hints();
    drill_hints();
    if ((ui.toast_until && now >= ui.toast_until) != last_toast) {
        last_toast = ui.toast_until && now >= ui.toast_until;
        ui.redraw |= R_FOOT;
    }
    /* what moves on its own: the running clocks, the inspection, a turn being drawn */
    if ((ui.page == P_TIMER && (ui.tm.st == T_RUN || ui.tm.st == T_INSPECT || ui.tm.st == T_HOLD))
        || (ui.page == P_DRILL && ui.d_mode == DM_REAL && ui.tm.st == T_RUN))
        ui.redraw |= R_MAIN;
    {
        vcube_t *v = ui.page == P_CUBE ? &ui.v : &ui.dv;
        if ((ui.page == P_CUBE || ui.page == P_DRILL || ui.page == P_ALGS) && vc_animating(v)) ui.redraw |= R_MAIN;
        if ((ui.page == P_CUBE || ui.page == P_DRILL || ui.page == P_ALGS) && v->anim_base >= 0 && !vc_animating(v)) {
            v->anim_base = -1;           /* the last frame of a turn */
            ui.redraw |= R_MAIN;
        }
    }
    if (ui.page == P_CUBE && ui.vt_run && now / 100u != last_sec) last_sec = now / 100u, ui.redraw |= R_HEAD;
    if (set_dirty_at && (int32_t)(now - set_dirty_at) > 3000) set_save();
    /* the scramble search: a few milliseconds a frame, more while the timer runs (little to draw). The keys are
     * timed by the tick, not by the frame, so a longer frame does not move a start or a stop */
    busy_ui = ui.redraw != 0;
    sv_work(ui.tm.st == T_RUN ? 10u : busy_ui ? 3u : 8u, 40000u);
    lights(now);
    speak();
    ann_tick(now);
    if (ui.redraw && (now - ui.drawn_at >= 16u || (ui.redraw & ~R_MAIN))) draw();
}

/* --------------------------------------------------------- host simulator --- */
#ifdef PLAT_HOST
static int host_press_hint(void)         /* the lit key pressed (SEL with it if lit): 0 if none is lit */
{
    int a, m;
    if (hint_key < 0) return 0;
    a = keymap[hint_key];
    if (a == KA_UNDO) {
        if (ui.page == P_CUBE) cube_undo();
        return 1;
    }
    m = km_move(a);
    if (hint_sel && MV_TURNS(m) != 2) m = MV_INV(m);
    if (ui.page == P_CUBE) cube_turn(m);
    else drill_virtual_turn(m);
    return 1;
}

int app_expect(const char *what, int *got)
{
    if (!strcmp(what, "follow_all")) {   /* press the lit keys until the scramble is done: 1 if it is */
        int n = 0;
        cube_hints();
        while (ui.follow && n++ < 200 && host_press_hint()) cube_hints();
        *got = !ui.follow && ui.vt_armed && !cube_solved(&ui.v.c);
        return 1;
    }
    if (!strncmp(what, "d_case", 6)) {  /* d_caseN: the card on the screen made formula N (a known case) */
        ui.d_alg = (uint8_t)atoi(what + 6);
        ui.d_auf = 0;
        vc_reset(&ui.dv);
        alg_case(ui.d_alg, &ui.dv.c, 0);
        ui.dv.prev = ui.dv.c;
        ui.d_state = D_CARD;
        *got = ui.d_alg;
        return 1;
    }
    if (!strcmp(what, "d_follow_all")) { /* the drill's case turned back by the lit keys: 1 if solved */
        int n = 0;
        drill_hints();
        while (ui.d_state == D_CARD && n++ < 60 && host_press_hint()) drill_hints();
        *got = ui.d_state == D_ANSWER && cube_solved_turn(&ui.dv.c);
        return 1;
    }
    if (!strcmp(what, "page")) *got = ui.page;
    else if (!strcmp(what, "timer")) *got = ui.tm.st;
    else if (!strcmp(what, "solves")) *got = sol.n;
    else if (!strcmp(what, "last_ms")) *got = sol.n ? SOL_MS(&sol.s[sol.n - 1]) : -1;
    else if (!strcmp(what, "last_pen")) *got = sol.n ? SOL_PEN(&sol.s[sol.n - 1]) : -1;
    else if (!strcmp(what, "last_scr")) *got = sol.n ? sol.s[sol.n - 1].n : -1;
    else if (!strcmp(what, "scr_n")) *got = ui.scr.n;
    else if (!strcmp(what, "scr_kind")) *got = ui.scr.kind;
    else if (!strcmp(what, "next_ok")) *got = next_ok;
    else if (!strcmp(what, "found")) *got = (int)sv_found;
    else if (!strcmp(what, "solved")) *got = cube_solved(&ui.v.c);
    else if (!strcmp(what, "moves")) *got = ui.v.hn;
    else if (!strcmp(what, "d_solved")) *got = cube_solved_turn(&ui.dv.c);
    else if (!strcmp(what, "d_moves")) *got = ui.dv.hn;
    else if (!strcmp(what, "follow")) *got = ui.follow;
    else if (!strcmp(what, "fi")) *got = ui.fi;
    else if (!strcmp(what, "fdev")) *got = ui.fdev;
    else if (!strcmp(what, "hint")) *got = hint_key;
    else if (!strcmp(what, "vt_run")) *got = ui.vt_run;
    else if (!strcmp(what, "vt_ms")) *got = (int)ui.vt_ms;
    else if (!strcmp(what, "layout")) *got = set.layout;
    else if (!strcmp(what, "scr_type")) *got = set.scr_type;
    else if (!strcmp(what, "inspect")) *got = set.inspect;
    else if (!strcmp(what, "start")) *got = set.start;
    else if (!strcmp(what, "view")) *got = set.view;
    else if (!strcmp(what, "alg_set")) *got = set.alg_set;
    else if (!strcmp(what, "alg_pos")) *got = set.alg_pos;
    else if (!strcmp(what, "playing")) *got = ui.play;
    else if (!strcmp(what, "d_state")) *got = ui.d_state;
    else if (!strcmp(what, "d_mode")) *got = ui.d_mode;
    else if (!strcmp(what, "d_ok")) *got = ui.d_round.ok;
    else if (!strcmp(what, "d_miss")) *got = ui.d_round.miss;
    else if (!strcmp(what, "d_rounds")) *got = dst[set.drill_set].round;
    else if (!strncmp(what, "d_rounds", 8)) *got = dst[atoi(what + 8) % ALG_SETS].round;   /* d_roundsN: set N's */
    else if (!strcmp(what, "d_real")) *got = drec.real_cs[ui.d_alg];
    else if (!strcmp(what, "d_alg")) *got = ui.d_alg;
    else if (!strcmp(what, "d_auf")) *got = ui.d_auf;
    else if (!strcmp(what, "custom0")) *got = set.custom[0];
    else if (!strcmp(what, "key0")) *got = keymap[0];
    else if (!strcmp(what, "lit_keys")) *got = (int)ui.lit;
    else if (!strcmp(what, "saves")) *got = (int)saves;
    else if (!strcmp(what, "algs")) *got = alg_total();
    else return 0;
    return 1;
}

int app_host_fails(void) { return host_fails; }
#endif
