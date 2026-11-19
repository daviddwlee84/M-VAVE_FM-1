/* SPDX-License-Identifier: GPL-3.0-or-later */
/* DRUMS v0.2 (see app.h). The player is groove.c, the styles styles.h, the sounds kit.c (its own unit, at -O2).
 *
 * Time: the audio's sample count is the clock (as RHYTHM's). The render walks the transport's ticks (GV_TPB a beat)
 * on their exact samples, asks groove.c what each one plays and starts those voices there, rendering the voices in
 * between. The main loop only asks for things (a fill, a MAIN, the ending), under plat_lock; groove.c decides at the
 * bar where each lands. The 1 kHz tick reads the keys: a pad is queued for the next block (at most 5.8 ms late).
 *
 * Part of fm1-base's unity build, after the platform: gfx.c and announce.c are in scope. */
#include "fastmath.h"
#include "groove.c"
#include "styles.h"
#include "kit.h"
#include "ledfx.c"

#define FW_MAJOR 0
#define FW_MINOR 2
#define SR 44100u

/* ---------------------------------------------------------------- settings --- */
#define REC_MAGIC 0x32305244u            /* "DR02" (0.1's DR01 was never installed: it falls back to the defaults) */
#define REC_VER 2u
#define LEVEL_UNITY 8                    /* a MIX level: 0..10, 8 is the kit as it is */
static const uint8_t PHRASES[] = { 4, 8, 16 };
static const char *const FA_NAME[FA_N] = { "NOW", "NEXT BAR", "PHRASE" };
static const char *const FL_NAME[FL_N] = { "1/2 BAR", "1 BAR", "2 BARS" };

typedef struct {
    uint32_t magic;
    uint16_t ver, size;
    uint8_t style, kit;
    uint8_t bpm;                         /* 30..240 */
    uint8_t swing;                       /* ticks the off 16ths come late: 0..12 */
    uint8_t fill_at;                     /* FA_* */
    uint8_t phrase;                      /* 4, 8, 16 */
    uint8_t auto_fill;
    uint8_t fill_len;                    /* FL_* (0.2) */
    uint16_t mutes;                      /* bit per lane */
    uint8_t levels[GV_LANES];            /* the MIX page: 0..10 (0.2) */
    uint8_t rsv[3];
} rec_t;
static rec_t rec;
static uint32_t saves, dirty_at;

static void defaults(void)
{
    rec.style = 0;
    rec.kit = KIT_808;
    rec.bpm = DR_STYLES[0].bpm;
    rec.swing = 0;
    rec.fill_at = FA_PHRASE;
    rec.phrase = 4;
    rec.auto_fill = 1;
    rec.fill_len = FL_ONE;
    rec.mutes = 0;
    {
        uint32_t l;
        for (l = 0; l < GV_LANES; l++) rec.levels[l] = LEVEL_UNITY;
    }
}

static int rec_valid(const rec_t *r)
{
    return r->magic == REC_MAGIC && r->ver == REC_VER && r->size == sizeof *r && r->style < DR_NSTYLES
           && r->kit < KIT_N && r->bpm >= 30 && r->bpm <= 240 && r->swing <= 12 && r->fill_at < FA_N
           && (r->phrase == 4 || r->phrase == 8 || r->phrase == 16) && r->auto_fill <= 1u
           && r->mutes < (1u << GV_LANES) && r->fill_len < FL_N;
}

static int levels_valid(const rec_t *r)
{
    uint32_t l;
    for (l = 0; l < GV_LANES; l++)
        if (r->levels[l] > 10) return 0;
    return 1;
}

static void apply_levels(void)
{
    uint32_t l;
    for (l = 0; l < GV_LANES; l++) kit_set_level((int)l, (float)rec.levels[l] / (float)LEVEL_UNITY);
}

static void rec_save(void)
{
    rec.magic = REC_MAGIC;
    rec.ver = REC_VER;
    rec.size = (uint16_t)sizeof rec;
    if (plat_store_save(OBJ_DRUMS, &rec, sizeof rec) == 0) saves++;
    dirty_at = 0;
}

/* ------------------------------------------------------- audio and clock --- */
static groove_t gv;                      /* the player: the render runs it, the main loop asks under plat_lock */
static volatile uint32_t au_blk;         /* the first sample of the block last rendered */
static volatile uint32_t tk_since;       /* ms ticks since that block began */
static volatile uint32_t lane_at[GV_LANES];   /* the sample each lane last sounded (the pads' lights) */
static volatile uint32_t hits[GV_LANES], pads_played;
static volatile uint8_t ended;           /* the render stopped the transport after the ending */

static struct {
    volatile uint8_t run;
    uint32_t start;                      /* sample of tick 0 */
    uint32_t tick_q8;                    /* samples a tick, x256 */
    uint32_t next, next_at;              /* render: the next tick, its sample */
} tr;

#define PADQ 16
static volatile uint8_t pq[PADQ];
static volatile uint32_t pq_w;
static uint32_t pq_r;

/* q8 steps in 32-bit arithmetic only: the device links no 64-bit division (no libgcc), and its 64-bit multiplies
 * beside loads can corrupt a register (felucca-bsp docs/CPU.md). As RHYTHM's */
static uint32_t q8_mul(uint32_t n, uint32_t q8) { return n * (q8 >> 8) + ((n * (q8 & 255u) + 128u) >> 8); }
static uint32_t tick_at(uint32_t t) { return tr.start + q8_mul(t, tr.tick_q8); }
static uint32_t beat_q8_at(uint32_t bpm) { return SR * 60u * 256u / bpm; }   /* (677,376,000: fits) */

static float mix[APP_BLOCK];
static float gain_now;

static void play_tick(uint32_t at)
{
    gv_out_t o;
    uint32_t k;
    groove_tick(&gv, tr.next, &o);
    for (k = 0; k < o.n; k++) {
        kit_trigger(o.e[k].lane, o.e[k].vel, rec.kit);
        lane_at[o.e[k].lane] = at;
        hits[o.e[k].lane]++;
    }
    if (gv.state == GS_STOP) {
        tr.run = 0;
        ended = 1;
    }
    tr.next++;
    tr.next_at = tick_at(tr.next);
}

void app_render(int32_t *lr, uint32_t n, uint32_t master_q12)
{
    uint32_t i = 0, s0 = au_blk + APP_BLOCK;   /* (au_blk is the last block's start: this one follows it) */
    float gain = (float)master_q12 / 4096.0f * 8300000.0f;
    if (n > APP_BLOCK) n = APP_BLOCK;
    if (!tr.run) tr.next = 0, tr.next_at = 0;
    else if (tr.next == 0 && tr.next_at == 0) tr.next_at = tick_at(0);
    while (pq_r != pq_w) {               /* the pads the tick read */
        uint8_t lane = pq[pq_r % PADQ];
        pq_r++;
        kit_trigger(lane, 7, rec.kit);
        lane_at[lane] = s0;
        pads_played++;
    }
    while (i < n) {                      /* the voices, up to each tick's sample */
        uint32_t upto = n;
        if (tr.run) {
            int32_t d = (int32_t)(tr.next_at - (s0 + i));
            if (d <= 0) {
                play_tick(s0 + i);
                continue;
            }
            if ((uint32_t)d < n - i) upto = i + (uint32_t)d;
        }
        kit_render(mix + i, upto - i);
        i = upto;
    }
    gain_now += (gain - gain_now) * 0.5f;
    for (i = 0; i < n; i++) lr[2 * i] = lr[2 * i + 1] = (int32_t)(fm_tanhf(mix[i] * 0.7f) * gain_now);
    au_blk = s0;
    tk_since = 0;
}

void app_panic(void)
{
    tr.run = 0;
    gv.state = GS_STOP;
    kit_silence();
}

static uint32_t now_smp(void)            /* the clock, now (main loop or tick) */
{
    uint32_t f = plat_lock(), s = au_blk + APP_BLOCK + tk_since * (SR / 1000u);
    plat_unlock(f);
    return s;
}

/* -------------------------------------------------------------- the keys --- */
/* F3 .. E4 play the band: MAIN A-D on F3 G3 A3 B3, the fills on F#3 (FILL LEN), G#3 (half a bar), A#3 (toms);
 * C4 BREAK, C#4 TRANSITION (held), D4 INTRO, D#4 STOP-TIME, E4 ENDING. F4 .. G5 are the pads */
#define K_MAIN_A 0
#define K_FILL 1
#define K_MAIN_B 2
#define K_SHORT 3
#define K_MAIN_C 4
#define K_TOM 5
#define K_MAIN_D 6
#define K_BREAK 7
#define K_TRANS 8
#define K_INTRO 9
#define K_STOP 10
#define K_END 11
#define K_PAD0 12
#define KQ_UP 0x80u                      /* (in kq: a key let go; only C#4's are queued) */
static const uint8_t MAIN_KEY[GV_MAINS] = { K_MAIN_A, K_MAIN_B, K_MAIN_C, K_MAIN_D };
static const uint8_t PAD_LANE[15] = { L_KICK, L_CHH, L_SNARE, L_OHH, L_RIM, L_RIDE, L_CLAP, L_TOML, L_SHAKE,
                                      L_TOMM, L_BELL, L_TOMH, L_CRASH, L_CHH, L_SNARE };
#define KEYQ 16
static volatile uint8_t kq[KEYQ];        /* the main loop's key presses: performance keys, SEL + pad */
static volatile uint32_t kq_w;
static uint32_t kq_r, tk_keys;
static volatile uint8_t ui_mix;          /* the MIX page shows (the tick reads it) */

void app_tick(void)
{
    uint32_t keys = plat_keys(), down, up, k;
    tk_since++;
    down = keys & ~tk_keys;
    up = ~keys & tk_keys;
    tk_keys = keys;
    if (up >> K_TRANS & 1u) {
        kq[kq_w % KEYQ] = (uint8_t)(K_TRANS | KQ_UP);
        kq_w++;
    }
    if (!down) return;
    for (k = 0; k < NKEYS; k++) {
        if (!(down >> k & 1u)) continue;
        if (k >= K_PAD0 && !(plat_buttons() >> B_SEL & 1u)) {
            pq[pq_w % PADQ] = PAD_LANE[k - K_PAD0];
            pq_w++;
            if (ui_mix) {                /* the MIX page: the pad also picks its lane */
                kq[kq_w % KEYQ] = (uint8_t)k;
                kq_w++;
            }
        } else {
            kq[kq_w % KEYQ] = (uint8_t)k;
            kq_w++;
        }
    }
}

/* ------------------------------------------------------------------- UI --- */
#define R_HEAD 1u
#define R_MAIN 2u
#define R_FOOT 4u
#define R_ALL 7u
#define MAIN_H 176
#define HOLD_MS 450u                     /* REC held this long: the transition */
#define DOUBLE_MS 300u                   /* a second tap within this: the ending */
#define STOP_MS 700u                     /* PLAY held this long: stop at once */

enum { PG_PLAY, PG_MIX };

static struct {
    uint32_t btn, redraw, drawn_at;
    char toast[2][28];
    uint32_t toast_until;
    uint32_t btn_at[NB];                 /* when each button went down */
    uint8_t rec_hold, tap_pending, play_long, key_hold;
    uint32_t tap_at;
    uint32_t tt[4], tt_n;                /* tap tempo: REC's last presses, ms */
    uint8_t page, mix_sel;               /* PG_*; the MIX page's lane */
    /* what the screen shows (a copy of the player, taken under the lock) */
    uint8_t running, state, sec, part, part_bar, fill_from, req_sec, fill_pending, end_pending, hold;
    uint8_t req_break, req_stop;
    uint16_t bar_in_sec;
    uint32_t beat;
    uint32_t lit, mid;                   /* the keys' lights (checks) */
} ui;
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

static void tint(char *d, int v)         /* decimal */
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

static void toast(const char *a, const char *b)
{
    uint32_t i;
    for (i = 0; i < 27u && a[i]; i++) ui.toast[0][i] = a[i];
    ui.toast[0][i] = 0;
    for (i = 0; i < 27u && b[i]; i++) ui.toast[1][i] = b[i];
    ui.toast[1][i] = 0;
    ui.toast_until = plat_ms() + 1500u;
    ui.redraw |= R_FOOT;
}

void app_say(const char *a, const char *b)
{
    char t[64];
    toast(a ? a : "", b ? b : "");
    if (ann_on()) {
        t[0] = 0;
        tcat(t, ui.toast[0]);
        tcat(t, " ");
        tcat(t, ui.toast[1]);
        ann_say(t, 0, ANN_NOW);
    }
}

static void changed(uint32_t r)
{
    ui.redraw |= r;
    dirty_at = plat_ms();
    if (!dirty_at) dirty_at = 1u;
}

static int swing_pct(int sw) { return 50 + sw * 50 / 24; }   /* the off 16th's place in its 8th: 50 % straight */
static const char *main_name(int sec)
{
    static const char *const n[GV_MAINS] = { "MAIN A", "MAIN B", "MAIN C", "MAIN D" };
    return n[sec & 3];
}

/* --------------------------------------------------------------- transport --- */
static void apply_settings(void)         /* rec -> the player (under the lock) */
{
    gv.fill_at = rec.fill_at;
    gv.fill_len = rec.fill_len;
    gv.phrase = rec.phrase;
    gv.auto_fill = rec.auto_fill;
    gv.swing = rec.swing;
    gv.mutes = rec.mutes;
}

static void start(int sec, int intro)
{
    uint32_t s = now_smp() + 2u * APP_BLOCK, f = plat_lock();
    apply_settings();
    groove_set_style(&gv, gv_style(rec.style));
    if (intro) groove_start_intro(&gv, sec);
    else groove_start(&gv, sec);
    tr.start = s;
    tr.tick_q8 = beat_q8_at(rec.bpm) / GV_TPB;
    tr.next = 0;
    tr.next_at = 0;
    ended = 0;
    tr.run = 1;
    plat_unlock(f);
    ui.running = 1;
    app_say(intro ? "INTRO" : "PLAY", main_name(sec));
    ui.redraw |= R_ALL;
}

static void stop_now(void)
{
    uint32_t f = plat_lock();
    tr.run = 0;
    groove_stop(&gv);
    plat_unlock(f);
    ui.running = 0;
    app_say("STOP", "");
    ui.redraw |= R_ALL;
}

static void retime(void)                 /* a new tempo, from the next tick on (no jump) */
{
    uint32_t f = plat_lock(), q8 = beat_q8_at(rec.bpm) / GV_TPB;
    if (tr.run && tr.next_at) tr.start = tr.next_at - q8_mul(tr.next, q8);
    tr.tick_q8 = q8;
    plat_unlock(f);
}

static void ask(void (*fn)(groove_t *), const char *what, const char *more)
{
    uint32_t f = plat_lock();
    fn(&gv);
    plat_unlock(f);
    if (what) app_say(what, more);
    ui.redraw |= R_MAIN;
}

static void ask_fill(int kind, int len, const char *what)
{
    uint32_t f = plat_lock();
    groove_fill_ex(&gv, kind, len);
    plat_unlock(f);
    app_say(what, FA_NAME[rec.fill_at]);
    ui.redraw |= R_MAIN;
}

static void ask_main(int sec)
{
    uint32_t f = plat_lock();
    groove_main(&gv, sec);
    plat_unlock(f);
    app_say(main_name(sec), rec.auto_fill ? "WITH A FILL" : "");
    ui.redraw |= R_MAIN;
}

static void ask_hold(int on)
{
    uint32_t f = plat_lock();
    groove_hold(&gv, on);
    plat_unlock(f);
    if (on) app_say("TRANSITION", "LET GO FOR NEXT");
    ui.redraw |= R_MAIN;
}

static void tap_tempo(void)
{
    uint32_t now = plat_ms(), i, span, bpm;
    if (ui.tt_n && now - ui.tt[(ui.tt_n - 1u) % 4u] > 2000u) ui.tt_n = 0;   /* a pause: a new count */
    ui.tt[ui.tt_n % 4u] = now;
    ui.tt_n++;
    if (ui.tt_n < 2) {
        app_say("TAP TEMPO", "AGAIN");
        return;
    }
    i = ui.tt_n < 4u ? ui.tt_n : 4u;
    span = now - ui.tt[(ui.tt_n - i) % 4u];
    bpm = (60000u * (i - 1u) + span / 2u) / span;
    rec.bpm = (uint8_t)clamp((int32_t)bpm, 30, 240);
    {
        char t[12];
        t[0] = 0;
        tint(t, rec.bpm);
        app_say("TAP TEMPO", t);
    }
    retime();
    changed(R_ALL);
}

/* ----------------------------------------------------------------- input --- */
static void bpm_by(int32_t d)
{
    char t[12];
    rec.bpm = (uint8_t)clamp(rec.bpm + d, 30, 240);
    t[0] = 0;
    tint(t, rec.bpm);
    app_say("BPM", t);
    retime();
    changed(R_ALL);
}

static void settings_changed(void)
{
    uint32_t f = plat_lock();
    apply_settings();
    plat_unlock(f);
    changed(R_ALL);
}

static void say_level(void)
{
    char t[24];
    t[0] = 0;
    tint(t, rec.levels[ui.mix_sel]);
    tcat(t, rec.levels[ui.mix_sel] == LEVEL_UNITY ? " (0 DB)" : "");
    app_say(gv_lane_name(ui.mix_sel), t);
}

static void key_pressed(uint32_t k)
{
    int s;
    if (k == (K_TRANS | KQ_UP)) {        /* C#4 let go */
        if (ui.key_hold) ui.key_hold = 0, ask_hold(0);
        return;
    }
    if (k >= K_PAD0) {
        uint8_t lane = PAD_LANE[k - K_PAD0];
        if (!(plat_buttons() >> B_SEL & 1u)) {   /* the MIX page: a pad picks its lane */
            ui.mix_sel = lane;
            say_level();
            ui.redraw |= R_MAIN;
            return;
        }
        rec.mutes ^= (uint16_t)(1u << lane);   /* SEL + a pad: that drum off or on */
        app_say(gv_lane_name(lane), rec.mutes >> lane & 1u ? "MUTED" : "ON");
        settings_changed();
        return;
    }
    for (s = 0; s < GV_MAINS; s++)
        if (k == MAIN_KEY[s]) {
            if (!ui.running) start(s, 0);
            else ask_main(s);
            return;
        }
    if (k == K_INTRO) {
        if (!ui.running) start(0, 1);
        return;
    }
    if (!ui.running) return;
    if (k == K_FILL) ask_fill(FK_SEC, rec.fill_len, "FILL");
    else if (k == K_SHORT) ask_fill(FK_SEC, FL_HALF, "SHORT FILL");
    else if (k == K_TOM) ask_fill(FK_TOM, FL_ONE, "TOM FILL");
    else if (k == K_BREAK) ask(groove_break, "BREAK", "NEXT BAR");
    else if (k == K_STOP) ask(groove_stop_time, "STOP-TIME", "NEXT BAR");
    else if (k == K_TRANS) ui.key_hold = 1, ask_hold(1);
    else if (k == K_END) ask(groove_end, "ENDING", "");
}

static void rec_button(uint32_t btn, uint32_t now)
{
    uint32_t bit = 1u << B_REC;
    int down = (btn & bit) && !(ui.btn & bit), up = !(btn & bit) && (ui.btn & bit);
    if (!ui.running) {
        ui.tap_pending = 0;
        ui.rec_hold = 0;
        if (down) tap_tempo();
        return;
    }
    if (down) ui.btn_at[B_REC] = now;
    if ((btn & bit) && !ui.rec_hold && now - ui.btn_at[B_REC] >= HOLD_MS) {
        ui.rec_hold = 1;
        ui.tap_pending = 0;
        ask_hold(1);
    }
    if (up) {
        if (ui.rec_hold) {
            ui.rec_hold = 0;
            ask_hold(0);
        } else if (ui.tap_pending && now - ui.tap_at <= DOUBLE_MS) {
            ui.tap_pending = 0;
            ask(groove_end, "ENDING", "");
        } else {
            ui.tap_pending = 1;
            ui.tap_at = now;
        }
    }
    if (ui.tap_pending && now - ui.tap_at > DOUBLE_MS) {   /* one tap: a fill */
        ui.tap_pending = 0;
        ask_fill(FK_SEC, rec.fill_len, "FILL");
    }
}

static void set_page(int pg)
{
    ui.page = (uint8_t)pg;
    ui_mix = (uint8_t)(pg == PG_MIX);
    app_say("PAGE", pg == PG_MIX ? "MIX" : "PLAY");
    ui.redraw |= R_ALL;
}

static void input(void)
{
    uint32_t btn = plat_buttons(), now = plat_ms(), k;
    int32_t e;
    char t[24];
    if (ended) {                         /* the ending played out */
        ended = 0;
        ui.running = 0;
        app_say("END", "");
        ui.redraw |= R_ALL;
    }
    rec_button(btn, now);
    for (k = 0; k < NB; k++) {
        uint32_t bit = 1u << k;
        if (k == B_REC) continue;
        if ((btn & bit) && !(ui.btn & bit)) {   /* down */
            ui.btn_at[k] = now;
            if (k == B_PLAY) {
                ui.play_long = 0;
                if (!ui.running) start(0, 0);
                else ui.play_long = 2;   /* decided on release (or when held) */
            } else if (k == B_OCTDN || k == B_OCTUP)
                bpm_by(k == B_OCTUP ? 1 : -1);
            else if (k == B_ARP) {
                rec.auto_fill = (uint8_t)!rec.auto_fill;
                app_say("AUTO FILL", rec.auto_fill ? "ON" : "OFF");
                settings_changed();
            } else if (k == B_HOME)
                set_page(ui.page == PG_MIX ? PG_PLAY : PG_MIX);
        } else if (!(btn & bit) && (ui.btn & bit) && k == B_PLAY && ui.play_long == 2) {   /* PLAY let go soon */
            ui.play_long = 0;
            if (ui.running) {
                if (gv.state == GS_END || gv.req_end) stop_now();   /* again during the ending: now */
                else ask(groove_end, "ENDING", "");
            }
        }
    }
    if ((btn >> B_PLAY & 1u) && ui.play_long == 2 && now - ui.btn_at[B_PLAY] >= STOP_MS) {
        ui.play_long = 1;
        if (ui.running) stop_now();
    }
    ui.btn = btn;
    if ((e = plat_enc(EN_SELECT)) != 0) {
        if (ui.page == PG_MIX) {         /* MIX: the lane */
            ui.mix_sel = (uint8_t)((ui.mix_sel + (uint32_t)(GV_LANES * 8 + e)) % GV_LANES);
            say_level();
            ui.redraw |= R_MAIN;
        } else {
            rec.style = (uint8_t)((rec.style + (uint32_t)(DR_NSTYLES * 8 + e)) % DR_NSTYLES);
            {
                uint32_t f = plat_lock();
                groove_set_style(&gv, gv_style(rec.style));
                plat_unlock(f);
            }
            if (!ui.running) {           /* stopped: the style's own tempo */
                rec.bpm = DR_STYLES[rec.style].bpm;
                retime();
            }
            app_say("STYLE", DR_STYLES[rec.style].name);
            changed(R_ALL);
        }
    }
    e = plat_enc(EN_PRESET);
    if (ui.page == PG_MIX) e += plat_enc(EN_K2);   /* MIX: K2 is the kit too */
    if (e) {
        rec.kit = (uint8_t)((rec.kit + (uint32_t)(KIT_N * 8 + e)) % KIT_N);
        app_say("KIT", kit_name(rec.kit));
        changed(R_ALL);
    }
    if ((e = plat_enc(EN_K1)) != 0) {
        if (ui.page == PG_MIX) {         /* MIX: the lane's level */
            rec.levels[ui.mix_sel] = (uint8_t)clamp(rec.levels[ui.mix_sel] + e, 0, 10);
            kit_set_level(ui.mix_sel, (float)rec.levels[ui.mix_sel] / (float)LEVEL_UNITY);
            say_level();
            changed(R_MAIN);
        } else
            bpm_by(e);
    }
    if (ui.page == PG_PLAY && (e = plat_enc(EN_K2)) != 0) {
        rec.swing = (uint8_t)clamp(rec.swing + e, 0, 12);
        t[0] = 0;
        tint(t, swing_pct(rec.swing));
        tcat(t, "%");
        app_say("SWING", t);
        settings_changed();
    }
    if ((e = plat_enc(EN_K3)) != 0) {
        rec.fill_at = (uint8_t)clamp(rec.fill_at + (e > 0 ? 1 : -1), 0, FA_N - 1);
        app_say("FILL AT", FA_NAME[rec.fill_at]);
        settings_changed();
    }
    if ((e = plat_enc(EN_K4)) != 0) {
        uint32_t i;
        for (i = 0; i < 3u && PHRASES[i] != rec.phrase; i++) {}
        i = (uint32_t)clamp((int32_t)i + (e > 0 ? 1 : -1), 0, 2);
        rec.phrase = PHRASES[i];
        t[0] = 0;
        tint(t, rec.phrase);
        tcat(t, " BARS");
        app_say("PHRASE", t);
        settings_changed();
    }
    if ((e = plat_enc(EN_ALGO)) != 0) {
        rec.fill_len = (uint8_t)clamp(rec.fill_len + (e > 0 ? 1 : -1), 0, FL_N - 1);
        app_say("FILL LEN", FL_NAME[rec.fill_len]);
        settings_changed();
    }
    while (kq_r != kq_w) {
        uint32_t k2 = kq[kq_r % KEYQ];
        kq_r++;
        key_pressed(k2);
    }
}

void app_input_only(void) { input(); }

/* ---------------------------------------------------------------- lights --- */
static int part_is(int p) { return ui.running && ui.part == p; }

static void lights(uint32_t now)
{
    ledfx_t kb = { 0, 0, 0 }, bb = { 0, 0, 0 };
    uint32_t k, s = now_smp();
    int sec, fill = ui.running && gv_is_fill(ui.part);
    ledfx_set(&bb, B_REC, ui.running ? FX_MID : FX_GLOW, now);
    ledfx_set(&bb, B_ARP, rec.auto_fill ? FX_LIT : FX_GLOW, now);
    ledfx_set(&bb, B_HOME, ui.page == PG_MIX ? FX_LIT : FX_GLOW, now);
    ledfx_set(&bb, B_OCTDN, FX_GLOW, now);
    ledfx_set(&bb, B_OCTUP, FX_GLOW, now);
    if (ui.running) {
        bb.lit |= LED_PLAY_GREEN;
        if (ui.beat == 0) ledfx_set(&bb, B_PLAY, FX_LIT, now);   /* the one */
    }
    for (sec = 0; sec < GV_MAINS; sec++)
        ledfx_set(&kb, MAIN_KEY[sec], ui.running && ui.sec == sec ? FX_LIT : ui.req_sec == sec ? FX_BLINK : FX_GLOW,
                  now);
    ledfx_set(&kb, K_FILL, fill && ui.part != P_TOM ? FX_LIT : ui.fill_pending ? FX_BLINK : ui.running ? FX_GLOW : FX_OFF,
              now);
    ledfx_set(&kb, K_SHORT, fill && ui.fill_from >= gv_style(rec.style)->steps / 2u ? FX_LIT : ui.running ? FX_GLOW : FX_OFF,
              now);
    ledfx_set(&kb, K_TOM, part_is(P_TOM) ? FX_LIT : ui.running ? FX_GLOW : FX_OFF, now);
    ledfx_set(&kb, K_BREAK, part_is(P_BREAK) ? FX_LIT : ui.req_break ? FX_BLINK : ui.running ? FX_GLOW : FX_OFF, now);
    ledfx_set(&kb, K_STOP, part_is(P_STOP) ? FX_LIT : ui.req_stop ? FX_BLINK : ui.running ? FX_GLOW : FX_OFF, now);
    ledfx_set(&kb, K_TRANS, ui.hold ? FX_LIT : ui.running ? FX_GLOW : FX_OFF, now);
    ledfx_set(&kb, K_INTRO, part_is(P_INTRO) ? FX_LIT : ui.running ? FX_OFF : FX_GLOW, now);
    ledfx_set(&kb, K_END, ui.end_pending || ui.state == GS_END ? FX_BLINK : ui.running ? FX_GLOW : FX_OFF, now);
    for (k = K_PAD0; k < NKEYS; k++) {
        uint8_t lane = PAD_LANE[k - K_PAD0];
        int fx = rec.mutes >> lane & 1u ? FX_OFF : FX_GLOW;
        if (ui.page == PG_MIX && lane == ui.mix_sel) fx = FX_MID;
        if (s - lane_at[lane] < SR / 12u) fx = FX_LIT;   /* sounded in the last 83 ms */
        if (tk_keys >> k & 1u) fx = FX_LIT;
        ledfx_set(&kb, k, fx, now);
    }
    ui.lit = kb.lit;
    ui.mid = kb.mid;
    ledfx_show(&bb, &kb);
}

/* ---------------------------------------------------------------- screen --- */
#define C_FILL RGB(80, 190, 255)
#define C_ENDC RGB(240, 90, 80)

static void text_c(int32_t cx, int32_t y, const felucca_font_t *f, const char *s, uint16_t c)
{
    cv_text(cx - text_w(f, s) / 2, y, f, s, c);
}

static void draw_header(void)
{
    char t[32];
    cv_begin(240, 32, C_BLACK);
    cv_text(8, 8, &FONT_B, ui.page == PG_MIX ? "MIX" : DR_STYLES[rec.style].name, C_WHITE);
    t[0] = 0;
    tcat(t, kit_name(rec.kit));
    tcat(t, "  ");
    tint(t, rec.bpm);
    tcat(t, " BPM");
    cv_text(232 - text_w(&FONT_T, t), 11, &FONT_T, t, C_HI);
    cv_rect(0, 31, 240, 1, C_LINE);
    cv_blit(0, 0);
}

static void box(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c)   /* an outline */
{
    cv_rect(x, y, w, 2, c);
    cv_rect(x, y + h - 2, w, 2, c);
    cv_rect(x, y, 2, h, c);
    cv_rect(x + w - 2, y, 2, h, c);
}

static const char *now_word(void)        /* what plays now */
{
    if (!ui.running) return "STOPPED";
    if (ui.state == GS_END) return "ENDING";
    if (ui.hold) return "TRANSITION";
    switch (ui.part) {
    case P_INTRO: return "INTRO";
    case P_BREAK: return "BREAK";
    case P_STOP: return "STOP-TIME";
    case P_TOM: return "TOM FILL";
    case P_TWO: return "FILL 2 BARS";
    default: break;
    }
    if (gv_is_fill(ui.part)) return ui.fill_from >= gv_style(rec.style)->steps / 2u ? "FILL 1/2" : "FILL";
    return main_name(ui.sec);
}

static void draw_top(int32_t oy)         /* the MAINs, what plays, where */
{
    static const char *const L[GV_MAINS] = { "A", "B", "C", "D" };
    char t[40];
    int32_t s;
    uint32_t blink = (plat_ms() / 250u) & 1u;
    int fill = ui.running && (gv_is_fill(ui.part) || ui.part >= P_INTRO);
    const gv_style_t *st = gv_style(rec.style);
    const char *w = now_word();
    for (s = 0; s < GV_MAINS; s++) {
        int32_t x = 12 + s * 56, y = 8 - oy;
        int cur = ui.running && ui.sec == s, want = ui.req_sec == s;
        if (cur) cv_rect(x, y, 48, 48, fill ? C_FILL : C_HI);
        else box(x, y, 48, 48, want && blink ? C_FILL : C_LINE);
        text_c(x + 24, y + 10, &FONT_L, L[s], cur ? C_BLACK : want ? C_FILL : C_GRAY);
    }
    cv_text(12, 64 - oy, &FONT_B, w,
            !ui.running ? C_DIM : ui.state == GS_END || ui.part == P_STOP ? C_ENDC : fill ? C_FILL : C_WHITE);
    t[0] = 0;
    if (ui.running) {
        tint(t, (int)(ui.bar_in_sec % rec.phrase) + 1);
        tcat(t, ".");
        tint(t, (int)ui.beat + 1);
    } else {
        tint(t, st->beats * (st->spb == 3 && st->beats == 2 ? 3 : 1));
        tcat(t, st->spb == 3 && st->beats == 2 ? "/8" : "/4");
    }
    cv_text(228 - text_w(&FONT_M, t), 60 - oy, &FONT_M, t, ui.running ? C_HI : C_GRAY);
}

static void draw_bottom(int32_t oy)      /* the phrase, the settings */
{
    char t[48];
    int32_t i, n = rec.phrase, gap = n > 8 ? 2 : 4, w = (216 - gap * (n - 1)) / n, x0 = 120 - (n * w + gap * (n - 1)) / 2;
    int32_t cur = ui.running ? (int32_t)(ui.bar_in_sec % rec.phrase) : -1;
    int fill = ui.running && gv_is_fill(ui.part);
    int fill_mark = ui.fill_pending && rec.fill_at == FA_PHRASE, two = rec.fill_len == FL_TWO;
    for (i = 0; i < n; i++) {
        int32_t x = x0 + i * (w + gap), y = 100 - oy;
        uint16_t c = C_LINE;
        if (i == cur) c = fill ? C_FILL : C_WHITE;
        else if (i < cur) c = C_DIM;
        cv_rect(x, y, w, 14, c);
        if ((i == n - 1 || (two && i == n - 2)) && i != cur)   /* where PHRASE puts a fill */
            box(x, y, w, 14, fill_mark ? C_FILL : C_GRAY);
    }
    t[0] = 0;
    tcat(t, "FILL AT ");
    tcat(t, FA_NAME[rec.fill_at]);
    tcat(t, "  ");
    tcat(t, FL_NAME[rec.fill_len]);
    tcat(t, "  AUTO ");
    tcat(t, rec.auto_fill ? "ON" : "OFF");
    text_c(120, 122 - oy, &FONT_T, t, C_GRAY);
    t[0] = 0;
    tcat(t, "SWING ");
    tint(t, swing_pct(rec.swing));
    tcat(t, "%   PHRASE ");
    tint(t, rec.phrase);
    text_c(120, 138 - oy, &FONT_T, t, C_GRAY);
    t[0] = 0;
    if (rec.mutes) {
        uint32_t l, k = 0;
        tcat(t, "MUTED:");
        for (l = 0; l < GV_LANES && k < 3; l++)
            if (rec.mutes >> l & 1u) {
                tcat(t, " ");
                tcat(t, gv_lane_name((int)l));
                k++;
            }
        text_c(120, 156 - oy, &FONT_T, t, C_AMB);
    } else if (ui.running && (ui.fill_pending || ui.req_sec != GV_NONE || ui.req_break || ui.req_stop)) {
        if (ui.req_sec != GV_NONE) tcat(t, "NEXT: "), tcat(t, main_name(ui.req_sec));
        else if (ui.req_break) tcat(t, "BREAK NEXT BAR");
        else if (ui.req_stop) tcat(t, "STOP-TIME NEXT BAR");
        else tcat(t, "FILL QUEUED");
        text_c(120, 156 - oy, &FONT_T, t, C_FILL);
    }
}

static void draw_mix(int32_t oy)         /* the MIX page: a bar a lane */
{
    uint32_t l;
    for (l = 0; l < GV_LANES; l++) {
        int32_t y = 4 + (int32_t)l * 13 - oy, lv = rec.levels[l], sel = l == ui.mix_sel;
        uint16_t c = rec.mutes >> l & 1u ? C_DIM : sel ? C_WHITE : C_GRAY;
        if (y < -13 || y > 88) continue;
        cv_text(10, y, &FONT_T, gv_lane_name((int)l), c);
        cv_rect(96, y + 3, 120, 6, C_LINE);
        cv_rect(96, y + 3, lv * 12, 6, sel ? C_HI : C_GRAY);
        cv_rect(96 + LEVEL_UNITY * 12, y + 1, 1, 10, C_DIM);   /* 0 dB */
        if (sel) cv_rect(2, y + 2, 4, 8, C_HI);
    }
}

static void draw_main(void)
{
    int32_t h;
    for (h = 0; h < 2; h++) {
        cv_begin(240, MAIN_H / 2, C_BLACK);
        if (ui.page == PG_MIX) draw_mix(h * MAIN_H / 2);
        else {
            draw_top(h * MAIN_H / 2);
            draw_bottom(h * MAIN_H / 2);
        }
        cv_blit(0, 32 + (uint32_t)h * MAIN_H / 2);
    }
}

static void draw_footer(void)
{
    cv_begin(240, 32, C_BLACK);
    cv_rect(0, 0, 240, 1, C_LINE);
    if (plat_ms() < ui.toast_until) {
        int32_t x = cv_text(8, 10, &FONT_T, ui.toast[0], C_GRAY);
        cv_text(x + 8, 7, &FONT_B, ui.toast[1], C_HI);
    } else
        cv_text(8, 10, &FONT_T,
                ui.page == PG_MIX ? "PAD: PICK   K1: LEVEL   K2: KIT   HOME: BACK"
                : ui.running      ? "REC: FILL   HOLD: NEXT   x2: END"
                                  : "PLAY: START   D4: INTRO   REC: TAP",
                C_DIM);
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
        tcat(t, DR_STYLES[rec.style].name);
        tcat(t, ". ");
        tint(t, rec.bpm);
        tcat(t, " BPM");
        ann_say(t, 0, ANN_NOW);
    }
}

/* the player, as the screen shows it */
static void follow(void)
{
    uint32_t f = plat_lock(), beat = gv.pos / GV_TPB;
    uint8_t part = gv.part, sec = gv.sec, state = gv.state, req = gv.req_sec, hold = gv.hold, pb = gv.part_bar;
    uint8_t ff = gv.fill_from, fp = (uint8_t)groove_fill_pending(&gv), ep = gv.req_end, rb = gv.req_break,
            rs = gv.req_stop;
    uint16_t bis = gv.bar_in_sec;
    plat_unlock(f);
    if (!ui.running) beat = 0, req = GV_NONE, fp = 0, ep = 0, rb = 0, rs = 0;
    if (part != ui.part || sec != ui.sec || state != ui.state || req != ui.req_sec || fp != ui.fill_pending
        || ep != ui.end_pending || bis != ui.bar_in_sec || beat != ui.beat || hold != ui.hold || pb != ui.part_bar
        || ff != ui.fill_from || rb != ui.req_break || rs != ui.req_stop)
        ui.redraw |= ui.page == PG_PLAY ? R_MAIN : 0;
    ui.part = part;
    ui.sec = sec;
    ui.state = state;
    ui.req_sec = req;
    ui.fill_pending = fp;
    ui.end_pending = ep;
    ui.bar_in_sec = bis;
    ui.beat = beat;
    ui.hold = hold;
    ui.part_bar = pb;
    ui.fill_from = ff;
    ui.req_break = rb;
    ui.req_stop = rs;
}

/* ------------------------------------------------------------ the contract --- */
void app_init(void)
{
    zero(&ui, sizeof ui);
    zero(&tr, sizeof tr);
    saves = 0;
    dirty_at = 0;
    ui_mix = 0;
    if (gv_load(DR_STYLES, DR_NSTYLES) != DR_NSTYLES) host_fails++;   /* (drums-unit checks the styles) */
    if (plat_store_load(OBJ_DRUMS, &rec, sizeof rec) != (int)sizeof rec || !rec_valid(&rec) || !levels_valid(&rec)) {
        zero(&rec, sizeof rec);
        defaults();
    }
    kit_init((float)SR);
    apply_levels();
    groove_init(&gv, gv_style(rec.style));
    apply_settings();
    ui.req_sec = GV_NONE;
    app_panic();
}

void app_ui_init(void)
{
    lcd_fill(0, 0, 240, 240, C_BLACK);
    ui.redraw = R_ALL;
}

void app_frame(void)
{
    static uint32_t last_toast, last_blink;
    uint32_t now = plat_ms();
    input();
    follow();
    if ((ui.req_sec != GV_NONE || ui.fill_pending) && (now / 250u & 1u) != last_blink) {
        last_blink = now / 250u & 1u;
        if (ui.page == PG_PLAY) ui.redraw |= R_MAIN;
    }
    if ((ui.toast_until && now >= ui.toast_until) != last_toast) {
        last_toast = ui.toast_until && now >= ui.toast_until;
        ui.redraw |= R_FOOT;
    }
    if (dirty_at && (int32_t)(now - dirty_at) > 3000) rec_save();
    lights(now);
    speak();
    ann_tick(now);
    if (ui.redraw && (!ui.running || now - ui.drawn_at >= 30u || (ui.redraw & ~R_MAIN))) draw();
}

/* --------------------------------------------------------- host simulator --- */
#ifdef PLAT_HOST
static int lane_code(const char *w)      /* styles.h's lane letter -> lane, -1 */
{
    int l;
    for (l = 0; l < GV_LANES; l++)
        if (GV_LANE_CODE[l] == w[0]) return l;
    return -1;
}

int app_expect(const char *what, int *got)
{
    if (!strncmp(what, "hits_", 5) || !strncmp(what, "level_", 6)) {   /* hits_K, level_K: lane K (styles.h's letters) */
        int l = lane_code(what + (what[0] == 'h' ? 5 : 6));
        if (l < 0) return 0;
        *got = what[0] == 'h' ? (int)hits[l] : rec.levels[l];
        return 1;
    }
    if (!strcmp(what, "running")) *got = ui.running;
    else if (!strcmp(what, "state")) *got = gv.state;
    else if (!strcmp(what, "sec")) *got = gv.sec;
    else if (!strcmp(what, "part")) *got = gv.part;
    else if (!strcmp(what, "part_bar")) *got = gv.part_bar;
    else if (!strcmp(what, "bar")) *got = gv.bar_in_sec;
    else if (!strcmp(what, "bars")) *got = (int)gv.bars;
    else if (!strcmp(what, "fill_from")) *got = gv.fill_from;
    else if (!strcmp(what, "req_fill")) *got = gv.req_fill;
    else if (!strcmp(what, "req_sec")) *got = gv.req_sec == GV_NONE ? -1 : gv.req_sec;
    else if (!strcmp(what, "req_end")) *got = gv.req_end;
    else if (!strcmp(what, "req_break")) *got = gv.req_break;
    else if (!strcmp(what, "req_stop")) *got = gv.req_stop;
    else if (!strcmp(what, "hold")) *got = gv.hold;
    else if (!strcmp(what, "style")) *got = rec.style;
    else if (!strcmp(what, "kit")) *got = rec.kit;
    else if (!strcmp(what, "bpm")) *got = rec.bpm;
    else if (!strcmp(what, "swing")) *got = rec.swing;
    else if (!strcmp(what, "fill_at")) *got = rec.fill_at;
    else if (!strcmp(what, "fill_len")) *got = rec.fill_len;
    else if (!strcmp(what, "phrase")) *got = rec.phrase;
    else if (!strcmp(what, "auto_fill")) *got = rec.auto_fill;
    else if (!strcmp(what, "mutes")) *got = rec.mutes;
    else if (!strcmp(what, "page")) *got = ui.page;
    else if (!strcmp(what, "mix_sel")) *got = ui.mix_sel;
    else if (!strcmp(what, "pads")) *got = (int)pads_played;
    else if (!strcmp(what, "lit_keys")) *got = (int)ui.lit;
    else if (!strcmp(what, "saves")) *got = (int)saves;
    else if (!strcmp(what, "styles")) *got = gv_count();
    else return 0;
    return 1;
}

int app_host_fails(void) { return host_fails; }
#endif
