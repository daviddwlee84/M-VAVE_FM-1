/* SPDX-License-Identifier: GPL-3.0-or-later */
/* RHYTHM v0.1: METRO and ECHO (see app.h). The rhythms and the scoring are rhythm.c.
 *
 * Time: the audio's sample count is the clock. The render plays every click and note on its exact sample (a tick
 * of 12 a beat), not on the next block (which would wobble by up to 5.8 ms). The 1 kHz tick reads the keys and
 * dates each press on the same clock: the last block's first sample plus the ms since it began. What the speaker
 * adds on the FM-1 (the blocks queued ahead, the key's debounce) is one constant, taken off by LAT (K4).
 *
 * Part of fm1-base's unity build, after the platform: gfx.c and announce.c are in scope. */
#include "fastmath.h"
#include "rhythm.c"
#include "ledfx.c"

#define FW_MAJOR 0
#define FW_MINOR 2
#define SR 44100u

/* ---------------------------------------------------------------- settings --- */
#define REC_MAGIC 0x31304852u            /* "RH01" */
#define REC_VER 1u

enum { PG_METRO, PG_ECHO, PG_ENDLESS, PG_N };
static const char *const PG_NAME[PG_N] = { "METRO", "ECHO", "ENDLESS" };
enum { SUB_NONE, SUB_8, SUB_TRIP, SUB_16, SUB_N };
static const char *const SUB_NAME[SUB_N] = { "OFF", "8THS", "TRIPLETS", "16THS" };
static const uint8_t GAPS[][2] = { { 0, 0 }, { 1, 1 }, { 2, 1 }, { 2, 2 }, { 4, 2 }, { 4, 4 } };   /* bars on, off */
#define NGAPS 6

typedef struct {
    uint32_t magic;
    uint16_t ver, size;
    uint8_t page;                        /* PG_* */
    uint8_t bpm;                         /* 30..240 */
    uint8_t bpb;                         /* beats a bar: 2..4 */
    uint8_t sub;                         /* METRO: SUB_* */
    uint8_t gap;                         /* METRO: GAPS[] */
    uint8_t level;                       /* ECHO: 1..RH_LEVELS */
    uint8_t bars;                        /* ECHO: 1, 2 */
    int8_t lat;                          /* ms taken off every tap: -30..+90 */
    uint8_t read;                        /* ECHO: 0 HEAR (blind), 1 READ (the notes shown, nothing played) (0.2) */
    uint8_t rsv;
    uint16_t best;                       /* ENDLESS: most rounds survived (0.2) */
} rec_t;
static rec_t rec;
static uint32_t saves, dirty_at;

static void defaults(void)
{
    rec.page = PG_ECHO;
    rec.bpm = 90;
    rec.bpb = 4;
    rec.sub = SUB_NONE;
    rec.gap = 0;
    rec.level = 1;
    rec.bars = 1;
    rec.lat = 0;
}

static int rec_valid(const rec_t *r)
{
    return r->magic == REC_MAGIC && r->ver == REC_VER && r->size == sizeof *r && r->page < PG_N && r->bpm >= 30
           && r->bpm <= 240 && r->bpb >= 2 && r->bpb <= 4 && r->sub < SUB_N && r->gap < NGAPS && r->level >= 1
           && r->level <= RH_LEVELS && r->bars >= 1 && r->bars <= 2 && r->lat >= -30 && r->lat <= 90
           && r->read <= 1u;
}

static void rec_save(void)
{
    rec.magic = REC_MAGIC;
    rec.ver = REC_VER;
    rec.size = (uint16_t)sizeof rec;
    if (plat_store_save(OBJ_RHYTHM, &rec, sizeof rec) == 0) saves++;
    dirty_at = 0;
}

/* ------------------------------------------------------- audio and clock --- */
enum { EV_ACCENT = 1, EV_CLICK = 2, EV_SOFT = 4, EV_SUB = 8, EV_NOTE = 16 };
enum { TR_METRO, TR_ECHO };
#define EN_ROUNDS 4                      /* ENDLESS: call-and-response rounds a stage */
#define EV_MAX ((4 + 2 * 4 * EN_ROUNDS + 2) * RH_TPB)   /* the longest: an ENDLESS stage in 4/4 (ECHO's is shorter) */

static volatile uint32_t au_blk;         /* the first sample of the block last rendered */
static volatile uint32_t tk_since;       /* ms ticks since that block began */
static volatile uint32_t tap_snd;        /* taps to sound (the tick counts, the render plays) */

/* the transport: the main loop writes it under plat_lock, the render plays it */
static struct {
    volatile uint8_t run, mode;
    uint32_t start;                      /* sample of tick 0 */
    uint32_t tick_q8;                    /* samples a tick, x256 */
    uint32_t next, next_at;              /* render: the next tick, its sample */
    uint8_t bpb, sub, gap_on, gap_off;   /* METRO */
    uint16_t len;                        /* ECHO: ticks in ev[] */
    uint8_t ev[EV_MAX];                  /* ECHO: what sounds on each tick */
} tr;

typedef struct {
    float ph, inc, amp, dec;
} voice_t;
#define NV 6
static voice_t vo[NV];
static uint32_t vo_rr;

/* q8 steps in 32-bit arithmetic only: the device links no 64-bit division (no libgcc), and its 64-bit multiplies
 * beside loads can corrupt a register (felucca-bsp docs/CPU.md). Exact for 2^24 steps (a day at 240 BPM in ticks) */
static uint32_t q8_mul(uint32_t n, uint32_t q8) { return n * (q8 >> 8) + ((n * (q8 & 255u) + 128u) >> 8); }

static int32_t smp_ms(int32_t x) { return (x * 20 + (x < 0 ? -441 : 441)) / 882; }   /* samples -> ms, rounded */

static uint32_t tick_at(uint32_t t) { return tr.start + q8_mul(t, tr.tick_q8); }

static int metro_gap(uint32_t beat)      /* METRO: this beat is in a silent bar */
{
    uint32_t bar = beat / tr.bpb, cyc = (uint32_t)tr.gap_on + tr.gap_off;
    return tr.gap_on && bar % cyc >= tr.gap_on;
}

static uint8_t event_at(uint32_t t)
{
    if (tr.mode == TR_ECHO) return t < tr.len ? tr.ev[t] : 0;
    {
        uint32_t beat = t / RH_TPB, s = t % RH_TPB;
        if (metro_gap(beat)) return 0;
        if (!s) return beat % tr.bpb ? EV_CLICK : EV_ACCENT;
        if ((tr.sub == SUB_8 && s == 6) || (tr.sub == SUB_TRIP && s % 4 == 0) || (tr.sub == SUB_16 && s % 3 == 0))
            return EV_SUB;
    }
    return 0;
}

static void voice(float hz, float amp, float tau_ms)
{
    voice_t *v = &vo[vo_rr++ % NV];
    v->ph = 0.0f;
    v->inc = hz / (float)SR;
    v->amp = amp;
    v->dec = 1.0f - 1000.0f / (tau_ms * (float)SR);   /* e^(-1 / (tau * SR)), near enough */
}

static void sound(uint8_t e)
{
    if (e & EV_ACCENT) voice(1568.0f, 0.50f, 18.0f);
    if (e & EV_CLICK) voice(1047.0f, 0.36f, 16.0f);
    if (e & EV_SOFT) voice(1047.0f, 0.13f, 12.0f);
    if (e & EV_SUB) voice(784.0f, 0.16f, 12.0f);
    if (e & EV_NOTE) voice(523.0f, 0.60f, 70.0f);
}

static float gain_now;

void app_render(int32_t *lr, uint32_t n, uint32_t master_q12)
{
    uint32_t i, k, s0 = au_blk, taps;
    float gain = (float)master_q12 / 4096.0f * 8300000.0f;
    s0 += APP_BLOCK;                     /* (au_blk is the last block's start: this one follows it) */
    if (!tr.run) tr.next = 0, tr.next_at = 0;
    else if (tr.next == 0 && tr.next_at == 0) tr.next_at = tick_at(0);
    taps = tap_snd;
    if (taps) {
        tap_snd = 0;
        voice(330.0f, 0.30f, 30.0f);
    }
    gain_now += (gain - gain_now) * 0.5f;
    for (i = 0; i < n; i++) {
        float s = 0.0f;
        uint32_t now = s0 + i;
        while (tr.run && (int32_t)(now - tr.next_at) >= 0) {
            uint8_t e = event_at(tr.next);
            if (e) sound(e);
            tr.next++;
            tr.next_at = tick_at(tr.next);
        }
        for (k = 0; k < NV; k++) {
            voice_t *v = &vo[k];
            if (v->amp < 0.0005f) continue;
            s += v->amp * fm_sinf(v->ph * 6.2831853f);
            v->ph += v->inc;
            if (v->ph >= 1.0f) v->ph -= 1.0f;
            v->amp *= v->dec;
        }
        lr[2 * i] = lr[2 * i + 1] = (int32_t)(s * gain_now);
    }
    au_blk = s0;
    tk_since = 0;
}

void app_panic(void)
{
    uint32_t k;
    tr.run = 0;
    for (k = 0; k < NV; k++) vo[k].amp = 0.0f;
}

static uint32_t now_smp(void)            /* the clock, now (main loop or tick) */
{
    uint32_t f = plat_lock(), s = au_blk + APP_BLOCK + tk_since * (SR / 1000u);
    plat_unlock(f);
    return s;
}

/* -------------------------------------------------------------- the taps --- */
#define TAPQ 32
static volatile uint32_t tq[TAPQ], tq_w;  /* key presses, as samples (the tick writes) */
static uint32_t tq_r, tk_keys;
#ifdef PLAT_HOST
static uint32_t host_tap[RH_MAX_TAPS], host_ntap, host_itap, host_down, host_armed;   /* autotap: presses at these samples */
#endif

void app_tick(void)
{
    uint32_t keys = plat_keys(), down, at = 0;
    tk_since++;
#ifdef PLAT_HOST
    {
        uint32_t s = now_smp();
        if (host_down) {                 /* a planned press held 30 ms */
            host_down--;
            keys |= 1u;
        } else if (host_itap < host_ntap && (int32_t)(s - host_tap[host_itap]) >= 0) {
            at = host_tap[host_itap];    /* dated at the planned sample: the 1 ms tick would add up to 1 ms */
            host_itap++;
            host_down = 30;
            keys |= 1u;
        }
    }
#endif
    down = keys & ~tk_keys;
    tk_keys = keys;
    if (down) {
        tq[tq_w % TAPQ] = at ? at : now_smp();
        tq_w++;
        tap_snd++;
    }
}

/* ------------------------------------------------------------------- UI --- */
#define R_HEAD 1u
#define R_MAIN 2u
#define R_FOOT 4u
#define R_ALL 7u
#define MAIN_H 176
#define C_GOOD RGB(80, 220, 120)
#define C_OK RGB(240, 190, 60)
#define C_BAD RGB(240, 80, 70)

enum { ES_IDLE, ES_COUNT1, ES_LISTEN, ES_COUNT2, ES_TAP, ES_RESULT };
static const char *const ES_NAME[] = { "READY", "COUNT-IN", "LISTEN", "COUNT-IN", "YOUR TURN", "RESULT" };

#define MT_N 8
static struct {
    uint32_t btn, redraw, drawn_at;
    char toast[2][28];
    uint32_t toast_until;
    uint32_t seed;
    /* METRO */
    uint8_t running;
    int16_t mt_err[MT_N];                /* the last taps' errors, ms */
    uint8_t mt_gap[MT_N];                /* tapped in a silent bar */
    uint32_t mt_n;                       /* taps so far */
    uint32_t tt[4], tt_n;                /* tap tempo: REC's last presses, ms */
    /* ECHO */
    uint8_t es;
    rh_pattern_t pat;
    uint8_t have_pat;
    uint32_t t_listen, t_count2, t_tap, t_end;   /* ticks where each part starts */
    int32_t taps[RH_MAX_TAPS];
    uint32_t ntaps;
    rh_result_t res;
    int32_t cur;                         /* the tick now (-1 before the start) */
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

static void tms(char *d, int v)          /* "+12MS", "-8MS" */
{
    if (v >= 0) tcat(d, "+");
    tint(d, v);
    tcat(d, "MS");
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

/* --------------------------------------------------------------- transport --- */
static uint32_t beat_q8_at(uint32_t bpm) { return SR * 60u * 256u / bpm; }   /* (677,376,000: fits) */
static uint32_t beat_smp_q8(void) { return beat_q8_at(rec.bpm); }

static void tr_stop(void)
{
    uint32_t f = plat_lock();
    tr.run = 0;
    plat_unlock(f);
    ui.running = 0;
}

static void tr_go_at(int mode, uint32_t start, uint32_t bpm)
{
    uint32_t f = plat_lock();
    tr.mode = (uint8_t)mode;
    tr.start = start;
    tr.tick_q8 = beat_q8_at(bpm) / RH_TPB;
    tr.bpb = rec.bpb;
    tr.sub = rec.sub;
    tr.gap_on = GAPS[rec.gap][0];
    tr.gap_off = GAPS[rec.gap][1];
    tr.next = 0;
    tr.next_at = 0;
    tr.run = 1;
    plat_unlock(f);
}

static void tr_go(int mode, uint32_t start) { tr_go_at(mode, start, rec.bpm); }

static int32_t tick_now(void)            /* the tick playing now; negative before the start */
{
    int32_t d = (int32_t)(now_smp() - tr.start);
    if (!tr.run) return -1;
    return d < 0 ? -1 : (int32_t)((float)d * 256.0f / (float)tr.tick_q8);
}

/* ------------------------------------------------------------------ METRO --- */
static void metro_start(void)
{
    ui.mt_n = 0;
    tq_r = tq_w;
    tr_go(TR_METRO, now_smp() + SR / 20u);   /* 50 ms ahead: the first click whole */
    ui.running = 1;
    ui.redraw |= R_ALL;
}

/* a tap against the nearest beat (in a silent bar too: that is the point of the gap) */
static void metro_tap(uint32_t smp)
{
    uint32_t bq = beat_smp_q8();
    int32_t d = (int32_t)(smp - tr.start), b, err;
    if (!ui.running || d < -(int32_t)(bq >> 9)) return;
    b = (int32_t)((float)d * 256.0f / (float)bq + 0.5f);
    if (b < 0) b = 0;
    err = smp_ms(d - (int32_t)q8_mul((uint32_t)b, bq)) - rec.lat;
    ui.mt_err[ui.mt_n % MT_N] = (int16_t)clamp(err, -999, 999);
    ui.mt_gap[ui.mt_n % MT_N] = (uint8_t)(tr.gap_on && metro_gap((uint32_t)b));
    ui.mt_n++;
    ui.redraw |= R_MAIN;
}

static int metro_avg(int gap_only, int *n)
{
    uint32_t i, k = ui.mt_n < MT_N ? ui.mt_n : MT_N;
    int32_t s = 0;
    *n = 0;
    for (i = 0; i < k; i++)
        if (!gap_only || ui.mt_gap[i]) s += ui.mt_err[i], (*n)++;
    return *n ? (int)(s / *n) : 0;
}

/* tap tempo: REC on the beat, the last four presses */
static void tap_tempo(void)
{
    uint32_t now = plat_ms(), i;
    if (ui.tt_n && now - ui.tt[(ui.tt_n - 1) % 4] > 2000u) ui.tt_n = 0;
    ui.tt[ui.tt_n % 4] = now;
    ui.tt_n++;
    if (ui.tt_n >= 2) {
        uint32_t k = ui.tt_n < 4 ? ui.tt_n : 4, first = ui.tt[(ui.tt_n - k) % 4], span = now - first, bpm;
        char t[16];
        if (!span) return;
        bpm = (60000u * (k - 1) + span / 2) / span;
        bpm = (uint32_t)clamp((int32_t)bpm, 30, 240);
        rec.bpm = (uint8_t)bpm;
        t[0] = 0;
        tint(t, (int)bpm);
        app_say("TAP TEMPO", t);
        if (ui.running) metro_start();
        changed(R_ALL);
    }
    (void)i;
}

/* ------------------------------------------------------------------- ECHO --- */
static void echo_build(void)             /* the events: count-in, the rhythm (with soft beats), count-in, your turn */
{
    uint32_t t, i, bpb = rec.bpb, n = ui.pat.nbeats;
    zero(tr.ev, sizeof tr.ev);
    if (rec.read) {                      /* READ: the notes on the screen, one bar counted in, then yours */
        ui.t_listen = ui.t_count2 = 0;
        ui.t_tap = bpb * RH_TPB;
    } else {
        ui.t_listen = bpb * RH_TPB;
        ui.t_count2 = ui.t_listen + n * RH_TPB;
        ui.t_tap = ui.t_count2 + bpb * RH_TPB;
    }
    ui.t_end = ui.t_tap + n * RH_TPB + RH_TPB / 2;   /* half a beat after: a late last tap still counts */
    for (t = 0; t < ui.t_tap + n * RH_TPB; t += RH_TPB) {
        uint32_t beat = t / RH_TPB;
        int count = t < ui.t_listen || (t >= ui.t_count2 && t < ui.t_tap);
        tr.ev[t] = (uint8_t)(count ? (beat % bpb ? EV_CLICK : EV_ACCENT) : EV_SOFT);
    }
    if (!rec.read)
        for (i = 0; i < ui.pat.n; i++) tr.ev[ui.t_listen + ui.pat.on[i]] |= EV_NOTE;
    tr.len = (uint16_t)(ui.t_tap + n * RH_TPB);
}

static void echo_start(int fresh)
{
    if (fresh || !ui.have_pat) {
        rh_generate(&ui.seed, rec.level, rec.bpb, rec.bars, &ui.pat);
        ui.have_pat = 1;
    }
    tr_stop();
    echo_build();
    ui.ntaps = 0;
    tq_r = tq_w;
    tr_go(TR_ECHO, now_smp() + SR / 20u);
    ui.running = 1;
    ui.es = ES_COUNT1;
#ifdef PLAT_HOST
    if (host_armed) {                    /* autotap: the plan's sample times, now the start is known */
        uint32_t i;
        for (i = 0; i < host_ntap; i++) host_tap[i] += tick_at(ui.t_tap);
        host_itap = 0;
        host_armed = 0;
    } else
        host_ntap = host_itap = 0;
#endif
    ui.redraw |= R_ALL;
}

static void echo_tap(uint32_t smp)
{
    uint32_t bq = beat_smp_q8(), t0 = tick_at(ui.t_tap);
    int32_t d = (int32_t)(smp - t0), ms;
    if (ui.es != ES_COUNT2 && ui.es != ES_TAP) return;
    if (d < -(int32_t)(bq / 256u / 2u)) return;   /* more than half a beat early: still counting in */
    ms = smp_ms(d) - rec.lat;
    if (ui.ntaps < RH_MAX_TAPS) ui.taps[ui.ntaps++] = ms;
    ui.redraw |= R_MAIN;
}

static void echo_done(void)
{
    char t[24];
    tr_stop();
    rh_score(&ui.pat, rec.bpm, ui.taps, (int)ui.ntaps, &ui.res);
    ui.es = ES_RESULT;
    t[0] = 0;
    tint(t, ui.res.score);
    tcat(t, "%");
    if (ann_on()) {
        char s[64];
        s[0] = 0;
        tcat(s, t);
        tcat(s, ". ");
        tcat(s, rh_hint_text(ui.res.hint));
        ann_say(s, 0, ANN_NOW);
    }
    ui.redraw |= R_ALL;
}

static void echo_follow(void)            /* where the round is */
{
    uint8_t es = ui.es;
    int32_t c = ui.cur;
    if (es == ES_IDLE || es == ES_RESULT || !ui.running) return;
    if (c >= (int32_t)ui.t_end) {
        echo_done();
        return;
    }
    es = c >= (int32_t)ui.t_tap ? ES_TAP : c >= (int32_t)ui.t_count2 ? ES_COUNT2 : c >= (int32_t)ui.t_listen ? ES_LISTEN : ES_COUNT1;
    if (es != ui.es) {
        ui.es = es;
        ui.redraw |= R_ALL;
    }
}

/* ---------------------------------------------------------------- ENDLESS --- */
/* call and response without a break: a bar is played, you tap it back in the next bar, the next call follows at
 * once. A stage is EN_ROUNDS rounds at one tempo after a bar's count-in; each stage is faster, every second one
 * harder. Health drops for what went wrong and comes back a little for a clean round; at 0 the game is over */
#define EN_TAPS 24
#define EN_SPEEDUP 6                     /* BPM a stage */
static struct {
    uint8_t on, over;                    /* a game running; the last game ended */
    uint16_t rounds;                     /* rounds survived */
    uint8_t stage, level, bpm, scored;   /* this stage; rounds of it scored */
    int16_t health;                      /* 0..100 */
    int16_t delta;                       /* the last round's change */
    rh_pattern_t pat[EN_ROUNDS];
    int32_t taps[EN_ROUNDS][EN_TAPS];
    uint8_t ntaps[EN_ROUNDS];
    rh_result_t last;                    /* the last round scored */
    uint8_t last_k, have_last, new_best;
} en;

static uint32_t en_bar(void) { return (uint32_t)rec.bpb * RH_TPB; }
static uint32_t en_call(uint32_t k) { return en_bar() * (1u + 2u * k); }   /* tick where round k's call starts */
static uint32_t en_resp(uint32_t k) { return en_call(k) + en_bar(); }

static void en_stage(void)               /* the next stage: its rhythms, its events, the transport */
{
    uint32_t k, t, i, bar = en_bar(), len = en_call(EN_ROUNDS);
    for (k = 0; k < EN_ROUNDS; k++) {
        rh_generate(&ui.seed, en.level, rec.bpb, 1, &en.pat[k]);
        en.ntaps[k] = 0;
    }
    en.scored = 0;
    zero(tr.ev, sizeof tr.ev);
    for (t = 0; t < len; t += RH_TPB)
        tr.ev[t] = (uint8_t)(t < bar ? ((t / RH_TPB) % rec.bpb ? EV_CLICK : EV_ACCENT) : EV_SOFT);
    for (k = 0; k < EN_ROUNDS; k++) {
        tr.ev[en_resp(k)] = EV_CLICK;    /* your bar starts on a louder beat */
        for (i = 0; i < en.pat[k].n; i++) tr.ev[en_call(k) + en.pat[k].on[i]] |= EV_NOTE;
    }
    tr.len = (uint16_t)len;
    tr_stop();
    tq_r = tq_w;
    tr_go_at(TR_ECHO, now_smp() + SR / 20u, en.bpm);
    ui.running = 1;
    ui.redraw |= R_ALL;
}

static void en_start(void)
{
    zero(&en, sizeof en);
    en.on = 1;
    en.health = 100;
    en.bpm = rec.bpm;
    en.level = rec.level;
    en_stage();
}

static void en_end(void)
{
    tr_stop();
    en.on = 0;
    en.over = 1;
    if (en.rounds > rec.best) {
        rec.best = en.rounds;
        en.new_best = 1;
        changed(0);
    }
    {
        char t[16];
        t[0] = 0;
        tint(t, en.rounds);
        app_say(en.new_best ? "NEW BEST" : "ROUNDS", t);
    }
    ui.redraw |= R_ALL;
}

static void en_tap(uint32_t smp)         /* into the round whose bar (give or take half a beat) it falls in */
{
    int32_t d = (int32_t)(smp - tr.start), half = RH_TPB / 2;
    float tick;
    uint32_t k;
    if (!en.on || d < 0) return;
    tick = (float)d * 256.0f / (float)tr.tick_q8;
    for (k = 0; k < EN_ROUNDS; k++) {
        float r = (float)en_resp(k);
        if (tick >= r - (float)half && tick < r + (float)(en_bar() + half)) {
            if (en.ntaps[k] < EN_TAPS)
                en.taps[k][en.ntaps[k]++] = smp_ms((int32_t)(smp - tick_at(en_resp(k)))) - rec.lat;
            return;
        }
    }
}

static void en_follow(void)
{
    int32_t c = ui.cur;
    if (!en.on || c < 0) return;
    while (en.scored < EN_ROUNDS && c >= (int32_t)(en_resp(en.scored) + en_bar() + RH_TPB / 2)) {
        uint32_t k = en.scored++;
        rh_result_t *r = &en.last;
        int d;
        rh_score(&en.pat[k], en.bpm, en.taps[k], en.ntaps[k], r);
        d = -(12 * r->missed + 8 * r->extra + 6 * r->off + 2 * r->ok);
        if (d < -40) d = -40;            /* a lost round costs at most 40: three to lose it all */
        if (r->good == r->n && !r->extra) d += 6;
        en.delta = (int16_t)d;
        en.health = (int16_t)clamp(en.health + d, 0, 100);
        en.last_k = (uint8_t)k;
        en.have_last = 1;
        ui.redraw |= R_ALL;
        if (!en.health) {
            en_end();
            return;
        }
        en.rounds++;
    }
    if (en.scored == EN_ROUNDS) {        /* the next stage: faster, every second one harder */
        en.stage++;
        en.bpm = (uint8_t)clamp(en.bpm + EN_SPEEDUP, 30, 240);
        if (!(en.stage % 2u) && en.level < RH_LEVELS) en.level++;
        app_say("FASTER", "");
        en_stage();
    }
}

static int en_part(void)                 /* 0 counting in, 1 listening (a call), 2 your bar */
{
    int32_t c = ui.cur;
    uint32_t bar = en_bar();
    if (c < (int32_t)bar) return 0;
    return (((uint32_t)c - bar) / bar) % 2u ? 2 : 1;
}

/* ----------------------------------------------------------------- input --- */
static void set_page(int p)
{
    tr_stop();
    en.on = 0;
    rec.page = (uint8_t)p;
    ui.es = ES_IDLE;
    app_say("PAGE", PG_NAME[p]);
    changed(R_ALL);
}

static void input(void)
{
    uint32_t btn = plat_buttons(), k;
    int32_t e;
    char t[24];
    for (k = 0; k < NB; k++) {
        uint32_t bit = 1u << k;
        if (!(btn & bit) || (ui.btn & bit)) continue;
        if (k == B_HOME) set_page((rec.page + 1) % PG_N);
        else if (k == B_PLAY) {
            if (rec.page == PG_ENDLESS) {
                if (en.on) en_end();
                else en_start();
            } else if (ui.running) {
                tr_stop();
                if (rec.page == PG_ECHO) ui.es = ES_IDLE;
                ui.redraw |= R_ALL;
            } else if (rec.page == PG_METRO) metro_start();
            else echo_start(1);
        } else if (k == B_SEL && rec.page == PG_ECHO && !ui.running) {
            rec.read = (uint8_t)!rec.read;
            app_say("ECHO", rec.read ? "READ" : "HEAR");
            changed(R_ALL);
        } else if (k == B_REC) {
            if (rec.page == PG_METRO) tap_tempo();
            else if (rec.page == PG_ECHO && !ui.running && ui.have_pat) echo_start(0);   /* the same rhythm again */
        } else if (k == B_OCTDN || k == B_OCTUP) {
            if (en.on) continue;
            rec.bpm = (uint8_t)clamp(rec.bpm + (k == B_OCTUP ? 5 : -5), 30, 240);
            goto bpm_changed;
        }
        continue;
    bpm_changed:
        t[0] = 0;
        tint(t, rec.bpm);
        app_say("BPM", t);
        if (ui.running && rec.page == PG_METRO) metro_start();
        changed(R_ALL);
    }
    ui.btn = btn;
    if ((e = plat_enc(EN_SELECT)) != 0 && !en.on) {
        rec.bpm = (uint8_t)clamp(rec.bpm + e, 30, 240);
        t[0] = 0;
        tint(t, rec.bpm);
        app_say("BPM", t);
        if (ui.running && rec.page == PG_METRO) metro_start();
        changed(R_ALL);
    }
    if ((e = plat_enc(EN_K1)) != 0 && !(ui.running && rec.page != PG_METRO)) {
        rec.bpb = (uint8_t)clamp(rec.bpb + (e > 0 ? 1 : -1), 2, 4);
        t[0] = 0;
        tint(t, rec.bpb);
        tcat(t, "/4");
        app_say("TIME", t);
        ui.have_pat = 0;
        if (ui.running) metro_start();
        changed(R_ALL);
    }
    if ((e = plat_enc(EN_K2)) != 0) {
        if (rec.page == PG_METRO) {
            rec.sub = (uint8_t)clamp(rec.sub + (e > 0 ? 1 : -1), 0, SUB_N - 1);
            app_say("SUB", SUB_NAME[rec.sub]);
            if (ui.running) metro_start();
        } else if (!ui.running) {          /* ECHO and ENDLESS: the level (ENDLESS starts from it) */
            rec.level = (uint8_t)clamp(rec.level + (e > 0 ? 1 : -1), 1, RH_LEVELS);
            for (k = 0; k < RH_NCELLS; k++)
                if (rh_cell_level((int)k) == rec.level) break;
            t[0] = 0;
            tint(t, rec.level);
            tcat(t, " ");
            tcat(t, rh_cell_name((int)k));
            app_say("LEVEL", t);
            ui.have_pat = 0;
        }
        changed(R_ALL);
    }
    if ((e = plat_enc(EN_K3)) != 0) {
        if (rec.page == PG_METRO) {
            rec.gap = (uint8_t)clamp(rec.gap + (e > 0 ? 1 : -1), 0, NGAPS - 1);
            t[0] = 0;
            if (!rec.gap) tcat(t, "OFF");
            else {
                tint(t, GAPS[rec.gap][0]);
                tcat(t, " ON ");
                tint(t, GAPS[rec.gap][1]);
                tcat(t, " OFF");
            }
            app_say("GAP", t);
            if (ui.running) metro_start();
        } else if (!ui.running) {
            rec.bars = (uint8_t)clamp(rec.bars + (e > 0 ? 1 : -1), 1, 2);
            t[0] = 0;
            tint(t, rec.bars);
            app_say("BARS", t);
            ui.have_pat = 0;
        }
        changed(R_ALL);
    }
    if ((e = plat_enc(EN_K4)) != 0) {
        rec.lat = (int8_t)clamp(rec.lat + e, -30, 90);
        t[0] = 0;
        tms(t, rec.lat);
        app_say("LATENCY", t);
        changed(R_ALL);
    }
    (void)plat_enc(EN_ALGO);
    (void)plat_enc(EN_PRESET);
    while (tq_r != tq_w) {               /* the taps the tick dated */
        uint32_t s = tq[tq_r % TAPQ];
        tq_r++;
        if (rec.page == PG_METRO) metro_tap(s);
        else if (rec.page == PG_ENDLESS) en_tap(s);
        else echo_tap(s);
    }
}

void app_input_only(void) { input(); }

/* ---------------------------------------------------------------- lights --- */
static void lights(uint32_t now)
{
    ledfx_t kb = { 0, 0, 0 }, bb = { 0, 0, 0 };
    int32_t c = ui.cur;
    uint32_t k, flash = 0, half = 0;
    ledfx_set(&bb, B_HOME, FX_GLOW, now);
    ledfx_set(&bb, B_REC, FX_GLOW, now);
    ledfx_set(&bb, B_OCTDN, FX_GLOW, now);
    ledfx_set(&bb, B_OCTUP, FX_GLOW, now);
    if (ui.running && c >= 0) {
        uint32_t beat = (uint32_t)c / RH_TPB, in = (uint32_t)c % RH_TPB;
        if (rec.page == PG_METRO) {
            if (in < 2 && !metro_gap(beat)) flash = beat % rec.bpb ? 1 : 2;
        } else if ((uint32_t)c < tr.len) {
            uint32_t t;                  /* LISTEN: the notes light; counting in: the beats */
            for (t = (uint32_t)c; t + 2 > (uint32_t)c && t < tr.len; t--) {
                if (tr.ev[t] & EV_NOTE) flash = 2;
                else if (tr.ev[t] & (EV_ACCENT | EV_CLICK)) flash = flash ? flash : 1;
                if (!t) break;
            }
            half = rec.page == PG_ENDLESS ? en_part() == 2 : ui.es == ES_TAP || ui.es == ES_COUNT2;
        }
        bb.lit |= LED_PLAY_GREEN;
    }
    for (k = 0; k < NKEYS; k++) {
        int fx = flash == 2 ? FX_LIT : flash == 1 ? FX_MID : half ? FX_GLOW : FX_OFF;
        if (tk_keys >> k & 1u) fx = FX_LIT;
        ledfx_set(&kb, k, fx, now);
    }
    ui.lit = kb.lit;
    ui.mid = kb.mid;
    ledfx_show(&bb, &kb);
}

/* ---------------------------------------------------------------- screen --- */
static void text_c(int32_t cx, int32_t y, const felucca_font_t *f, const char *s, uint16_t c)
{
    cv_text(cx - text_w(f, s) / 2, y, f, s, c);
}

static uint16_t grade_col(int g) { return g == RH_G_GOOD ? C_GOOD : g == RH_G_OK ? C_OK : C_BAD; }

static void draw_header(void)
{
    char t[32];
    cv_begin(240, 32, C_BLACK);
    cv_text(8, 8, &FONT_B, PG_NAME[rec.page], C_WHITE);
    t[0] = 0;
    tint(t, rec.bpb);
    tcat(t, "/4  ");
    tint(t, en.on ? en.bpm : rec.bpm);
    tcat(t, " BPM");
    cv_text(232 - text_w(&FONT_T, t), 11, &FONT_T, t, C_HI);
    cv_rect(0, 31, 240, 1, C_LINE);
    cv_blit(0, 0);
}

/* the error scale: -100 .. +100 ms over x 20 .. 220 */
static int32_t ex(int32_t ms) { return 120 + clamp(ms, -100, 100); }

static void draw_metro(int32_t oy)
{
    char t[48];
    int32_t c = ui.cur, i, n, x0, w = 26;
    uint32_t beat = c >= 0 ? (uint32_t)c / RH_TPB : 0;
    int gap = ui.running && c >= 0 && metro_gap(beat), avg, gavg, ng;
    t[0] = 0;
    tint(t, rec.bpm);
    text_c(120, 4 - oy, &FONT_L, t, ui.running ? C_WHITE : C_HI);
    /* the beats */
    x0 = 120 - (rec.bpb * w + (rec.bpb - 1) * 8) / 2;
    for (i = 0; i < rec.bpb; i++) {
        int32_t x = x0 + i * (w + 8), on = ui.running && c >= 0 && (int32_t)(beat % rec.bpb) == i && !gap;
        cv_rect(x, 46 - oy, w, 14, on ? (i == 0 ? C_WHITE : C_HI) : C_LINE);
    }
    if (gap) text_c(120, 64 - oy, &FONT_T, "GAP: KEEP TAPPING", C_AMB);
    t[0] = 0;
    tcat(t, "SUB ");
    tcat(t, SUB_NAME[rec.sub]);
    tcat(t, "  GAP ");
    if (!rec.gap) tcat(t, "OFF");
    else {
        tint(t, GAPS[rec.gap][0]);
        tcat(t, "+");
        tint(t, GAPS[rec.gap][1]);
    }
    tcat(t, "  LAT ");
    tint(t, rec.lat);
    text_c(120, 80 - oy, &FONT_T, t, C_GRAY);
    /* your taps against the beat */
    cv_rect(20, 120 - oy, 201, 1, C_LINE);
    cv_rect(ex(-rh_tol_good(rec.bpm)), 110 - oy, 2 * rh_tol_good(rec.bpm) + 1, 21, C_LINE);
    cv_rect(120, 104 - oy, 1, 33, C_DIM);
    cv_text(20, 140 - oy, &FONT_T, "EARLY", C_DIM);
    cv_text(220 - text_w(&FONT_T, "LATE"), 140 - oy, &FONT_T, "LATE", C_DIM);
    n = (int32_t)(ui.mt_n < MT_N ? ui.mt_n : MT_N);
    for (i = 0; i < n; i++) {
        uint32_t idx = (ui.mt_n - 1u - (uint32_t)i) % MT_N;
        int32_t e = ui.mt_err[idx], a = e < 0 ? -e : e, h = i == 0 ? 26 : 14;
        uint16_t col = a <= rh_tol_good(rec.bpm) ? C_GOOD : a <= 2 * rh_tol_good(rec.bpm) ? C_OK : C_BAD;
        if (i) col = C_GRAY;
        cv_rect(ex(e) - 1, 120 - h / 2 - oy, 3, h, ui.mt_gap[idx] && !i ? C_WHITE : col);
    }
    avg = metro_avg(0, &ng);
    t[0] = 0;
    if (ui.mt_n) {
        tcat(t, "LAST ");
        tms(t, ui.mt_err[(ui.mt_n - 1u) % MT_N]);
        tcat(t, "  AVG ");
        tms(t, avg);
        gavg = metro_avg(1, &ng);
        if (ng) {
            tcat(t, "  GAP ");
            tms(t, gavg);
        }
    } else
        tcat(t, ui.running ? "TAP ANY KEY ON THE BEAT" : "PLAY: START   REC: TAP TEMPO");
    text_c(120, 156 - oy, &FONT_T, t, ui.mt_n ? C_HI : C_DIM);
}

/* -------------------------------------------------------------- notation --- */
/* a one-line percussion staff: the pattern's ticks over x0 .. x0 + w, the line at y. Heads are centred on their
 * tick; stems go up; a beat's eighths and sixteenths are beamed together, as a score groups them. With a result,
 * each head is coloured by its grade (a missed one hollow and red) */
#define ST_Y 88                          /* ECHO's staff line, in the main area */
#define ST_STEM 15
static int32_t st_x(const rh_pattern_t *p, int32_t x0, int32_t w, int32_t tick)
{
    return x0 + tick * w / (p->nbeats ? p->nbeats * RH_TPB : 1);
}

static void st_head(int32_t x, int32_t y, int32_t hw, int hollow, uint16_t c)
{
    int32_t l = x - hw / 2;
    cv_rect(l + 1, y - 2, hw - 2, 1, c);
    cv_rect(l + 1, y + 2, hw - 2, 1, c);
    if (hollow) {
        cv_rect(l, y - 1, 1, 3, c);
        cv_rect(l + hw - 1, y - 1, 1, 3, c);
    } else
        cv_rect(l, y - 1, hw, 3, c);
}

static void st_rest(int32_t x, int32_t y, uint16_t c)   /* a quarter rest */
{
    cv_line(x, y - 8, x + 3, y - 4, c);
    cv_line(x + 3, y - 4, x, y - 1, c);
    cv_line(x, y - 1, x + 3, y + 3, c);
    cv_line(x + 3, y + 3, x, y + 3, c);
    cv_line(x, y + 3, x + 2, y + 6, c);
}

static void st_beam(int32_t xa, int32_t xb, int32_t y, uint16_t c) { cv_rect(xa, y, xb - xa + 1, 2, c); }

static void draw_staff(const rh_pattern_t *p, const rh_result_t *r, int32_t x0, int32_t w, int32_t y, int show)
{
    int32_t total = p->nbeats * RH_TPB, sp = total ? 3 * w / total : 8, hw = clamp(sp - 1, 4, 6), by = y - ST_STEM;
    int32_t sx[4];
    uint16_t sc = r ? C_GRAY : C_HI;
    int beat = 0, ci, j, i = 0, b;
    cv_rect(x0 - 6, y, w + 10, 1, C_LINE);
    for (b = 0; b <= p->nbeats; b++) {   /* beats under the line, bar lines through it */
        int32_t x = st_x(p, x0, w, b * RH_TPB);
        if (b % (p->bpb ? p->bpb : 4) == 0 && b) cv_rect(x - hw / 2 - 3, y - 10, 1, 21, C_GRAY);
        else if (b < p->nbeats) cv_rect(x, y + 4, 1, 2, C_DIM);
    }
    if (!show) return;
    for (ci = 0; ci < p->ncells; ci++) {
        int cell = p->cells[ci], n = CELLS[cell].n;
        if (cell == RH_REST) st_rest(st_x(p, x0, w, beat * RH_TPB) - 1, y, C_HI);
        for (j = 0; j < n && i < p->n; j++, i++) {
            int32_t x = st_x(p, x0, w, p->on[i]);
            int miss = r && r->grade[i] == RH_G_MISS;
            uint16_t c = r ? (miss ? C_BAD : grade_col(r->grade[i])) : C_WHITE;
            st_head(x, y, hw, miss || cell == RH_TAA, c);
            sx[j] = x - hw / 2 + hw - 1;
            cv_rect(sx[j], by, 1, ST_STEM - 1, sc);
        }
        switch (cell) {
        case RH_TITI:
        case RH_TRIOLA:
            st_beam(sx[0], sx[n - 1], by, sc);
            if (cell == RH_TRIOLA) cv_text((sx[0] + sx[2]) / 2 - 2, by - 13, &FONT_T, "3", C_GRAY);
            break;
        case RH_TIKA:
            st_beam(sx[0], sx[3], by, sc);
            st_beam(sx[0], sx[3], by + 3, sc);
            break;
        case RH_TITIKA:
            st_beam(sx[0], sx[2], by, sc);
            st_beam(sx[1], sx[2], by + 3, sc);
            break;
        case RH_TIKATI:
            st_beam(sx[0], sx[2], by, sc);
            st_beam(sx[0], sx[1], by + 3, sc);
            break;
        case RH_TIMKA:                   /* a dotted eighth and a sixteenth: the dot, the short second beam */
            st_beam(sx[0], sx[1], by, sc);
            st_beam(sx[1] - 4, sx[1], by + 3, sc);
            cv_rect(st_x(p, x0, w, p->on[i - 2]) + hw / 2 + 2, y - 1, 2, 2, C_WHITE);
            break;
        case RH_SYNCO:                   /* an eighth, a quarter, an eighth: two flags */
            for (j = 0; j < 3; j += 2) {
                cv_line(sx[j], by, sx[j] + 4, by + 5, sc);
                cv_line(sx[j], by + 1, sx[j] + 4, by + 6, sc);
            }
            break;
        default:
            break;
        }
        beat += CELLS[cell].beats;
    }
}

/* the pattern's ticks and ms on the same scale as the staff; taps below it */
#define TL_X 14
#define TL_W 212
static int32_t tl_x(int32_t ms_or_tick, int is_tick)
{
    int32_t total = ui.pat.nbeats * RH_TPB;
    if (!is_tick) {                      /* ms -> ticks */
        int32_t tick_us = (int32_t)rh_tick_ms_x1000(rec.bpm);
        ms_or_tick = ms_or_tick * 1000 / tick_us;   /* (ms within a few bars: no overflow) */
    }
    return TL_X + ms_or_tick * TL_W / (total ? total : 1);
}

static void draw_echo(int32_t oy)
{
    char t[RH_TEXT_MAX + 8];
    uint32_t i;
    int k, res = ui.es == ES_RESULT, show;
    const char *big = ES_NAME[ui.es];
    uint16_t bigc = C_HI;
    if (res) {
        t[0] = 0;
        tint(t, ui.res.score);
        tcat(t, "%");
        big = t;
        bigc = ui.res.score >= 85 ? C_GOOD : ui.res.score >= 60 ? C_OK : C_BAD;
    } else if ((ui.es == ES_COUNT1 || ui.es == ES_COUNT2) && ui.cur >= 0) {
        uint32_t b = ((uint32_t)ui.cur - (ui.es == ES_COUNT2 ? ui.t_count2 : 0)) / RH_TPB;
        t[0] = 0;
        tint(t, (int)(b % rec.bpb) + 1);
        big = t;
        bigc = C_WHITE;
    } else if (ui.es == ES_TAP)
        bigc = C_WHITE;
    text_c(120, 2 - oy, ui.es == ES_IDLE || ui.es == ES_LISTEN || ui.es == ES_TAP ? &FONT_M : &FONT_L, big, bigc);
    {
        char u[40];
        for (k = 0; k < RH_NCELLS; k++)
            if (rh_cell_level(k) == rec.level) break;
        u[0] = 0;
        tcat(u, "LEVEL ");
        tint(u, rec.level);
        tcat(u, " ");
        tcat(u, rh_cell_name(k));
        tcat(u, "  ");
        tint(u, rec.bars);
        tcat(u, rec.bars > 1 ? " BARS" : " BAR");
        tcat(u, rec.read ? "  READ" : "  HEAR");
        text_c(120, 40 - oy, &FONT_T, u, C_GRAY);
    }
    if (!ui.have_pat) {
        text_c(120, 96 - oy, &FONT_T, "PLAY: A NEW RHYTHM   SEL: HEAR / READ", C_DIM);
        return;
    }
    /* the notes: hidden while you only hear them (HEAR); in the result, coloured by how close */
    show = res || rec.read;
    draw_staff(&ui.pat, res ? &ui.res : 0, TL_X, TL_W, ST_Y - oy, show);
    for (i = 0; res && i < ui.pat.n; i++)
        if (ui.res.tap_of[i] >= 0) {     /* how far: a line from the note to its tap */
            int32_t x = tl_x(ui.pat.on[i], 1), tx = tl_x(ui.taps[ui.res.tap_of[i]], 0);
            cv_rect(tx < x ? tx : x, ST_Y + 10 - oy, (tx < x ? x - tx : tx - x) + 1, 1, C_DIM);
        }
    for (i = 0; i < ui.ntaps; i++) {      /* your taps */
        int32_t x = tl_x(ui.taps[i], 0);
        if (x >= TL_X - 8 && x <= TL_X + TL_W + 8) cv_rect(x - 1, ST_Y + 12 - oy, 2, 10, C_WHITE);
    }
    /* the playhead */
    if (ui.running && ui.cur >= 0) {
        int32_t base = ui.es == ES_LISTEN ? (int32_t)ui.t_listen : ui.es == ES_TAP ? (int32_t)ui.t_tap : -1;
        if (base >= 0) cv_rect(tl_x(ui.cur - base, 1), ST_Y - 30 - oy, 2, 56, C_AMB);
    }
    if (res) {
        char u[48];
        u[0] = 0;
        tcat(u, "MEAN ");
        tms(u, ui.res.mean);
        tcat(u, "  GOOD ");
        tint(u, ui.res.good);
        tcat(u, "/");
        tint(u, ui.res.n);
        if (ui.res.missed) {
            tcat(u, "  MISS ");
            tint(u, ui.res.missed);
        }
        if (ui.res.extra) {
            tcat(u, "  EXTRA ");
            tint(u, ui.res.extra);
        }
        text_c(120, 128 - oy, &FONT_T, u, C_HI);
        u[0] = 0;
        for (i = 0; i < ui.pat.ncells; i++) {   /* what it was, by its syllables */
            if (i) tcat(u, " ");
            if (text_w(&FONT_T, u) + text_w(&FONT_T, rh_cell_name(ui.pat.cells[i])) > 230) break;
            tcat(u, rh_cell_name(ui.pat.cells[i]));
        }
        text_c(120, 146 - oy, &FONT_T, u, C_GRAY);
    } else if (ui.es == ES_IDLE)
        text_c(120, 140 - oy, &FONT_T, rec.read ? "PLAY: READ IT, TAP IT" : "PLAY: HEAR, THEN TAP IT BACK", C_DIM);
}

static void draw_endless(int32_t oy)
{
    char t[48];
    const char *big;
    uint16_t bigc = C_WHITE, hc;
    int part = en.on ? en_part() : -1, i;
    int32_t hb = 200 * en.health / 100;
    if (en.on && part == 0 && ui.cur >= 0) {
        t[0] = 0;
        tint(t, (int)((uint32_t)ui.cur / RH_TPB % rec.bpb) + 1);
        big = t;
    } else if (en.on)
        big = part == 1 ? "LISTEN" : "YOUR TURN", bigc = part == 1 ? C_HI : C_WHITE;
    else if (en.over)
        big = en.new_best ? "NEW BEST" : "GAME OVER", bigc = en.new_best ? C_GOOD : C_BAD;
    else
        big = "ENDLESS", bigc = C_HI;
    text_c(120, 2 - oy, part == 0 ? &FONT_L : &FONT_M, big, bigc);
    /* health */
    hc = en.health > 50 ? C_GOOD : en.health > 25 ? C_OK : C_BAD;
    cv_rect(20, 38 - oy, 200, 10, C_LINE);
    if (en.on || en.over) cv_rect(20, 38 - oy, hb, 10, hc);
    t[0] = 0;
    tcat(t, "ROUND ");
    tint(t, en.rounds);
    tcat(t, "   ");
    tint(t, en.on || en.over ? en.bpm : rec.bpm);
    tcat(t, " BPM   LV ");
    tint(t, en.on || en.over ? en.level : rec.level);
    tcat(t, "   BEST ");
    tint(t, rec.best);
    text_c(120, 52 - oy, &FONT_T, t, C_GRAY);
    if (!en.have_last) {
        text_c(120, 96 - oy, &FONT_T, en.on ? "TAP EACH BAR BACK, NO BREAKS" : "PLAY: START   K2: LEVEL   SELECT: BPM", C_DIM);
        return;
    }
    /* the last round: its notes coloured, your taps under them */
    {
        const rh_pattern_t *p = &en.pat[en.last_k];
        int32_t total = p->nbeats * RH_TPB, tick_us = (int32_t)rh_tick_ms_x1000(en.bpm);
        draw_staff(p, &en.last, TL_X, TL_W, ST_Y + 6 - oy, 1);
        for (i = 0; i < en.ntaps[en.last_k]; i++) {
            int32_t x = TL_X + en.taps[en.last_k][i] * 1000 / tick_us * TL_W / (total ? total : 1);
            if (x >= TL_X - 8 && x <= TL_X + TL_W + 8) cv_rect(x - 1, ST_Y + 18 - oy, 2, 10, C_WHITE);
        }
    }
    t[0] = 0;
    tcat(t, "GOOD ");
    tint(t, en.last.good);
    tcat(t, "/");
    tint(t, en.last.n);
    tcat(t, "   HEALTH ");
    if (en.delta >= 0) tcat(t, "+");
    tint(t, en.delta);
    tcat(t, "   ");
    tcat(t, rh_hint_text(en.last.hint));
    text_c(120, 136 - oy, &FONT_T, t, en.delta >= 0 ? C_GOOD : C_OK);
}

static void draw_main(void)
{
    int32_t h;
    for (h = 0; h < 2; h++) {
        cv_begin(240, MAIN_H / 2, C_BLACK);
        if (rec.page == PG_METRO) draw_metro(h * MAIN_H / 2);
        else if (rec.page == PG_ENDLESS) draw_endless(h * MAIN_H / 2);
        else draw_echo(h * MAIN_H / 2);
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
    } else if (rec.page == PG_ECHO && ui.es == ES_RESULT) {
        uint16_t col = ui.res.hint == RH_H_GOOD ? C_GOOD : ui.res.hint == RH_H_ALMOST ? C_OK : C_BAD;
        int32_t x = cv_text(8, 7, &FONT_B, rh_hint_text(ui.res.hint), col);
        const char *m = rh_hint_more(ui.res.hint);
        if (x + 8 + text_w(&FONT_T, m) <= 232) cv_text(232 - text_w(&FONT_T, m), 10, &FONT_T, m, C_GRAY);
    } else
        cv_text(8, 10, &FONT_T,
                rec.page == PG_METRO ? "HOME: ECHO   PLAY: START/STOP"
                : rec.page == PG_ECHO ? "HOME: ENDLESS   PLAY: NEW   REC: AGAIN"
                                      : "HOME: METRO   PLAY: START/STOP",
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
        tcat(t, PG_NAME[rec.page]);
        tcat(t, ". ");
        tint(t, rec.bpm);
        tcat(t, " BPM");
        ann_say(t, 0, ANN_NOW);
    }
}

/* ------------------------------------------------------------ the contract --- */
void app_init(void)
{
    zero(&ui, sizeof ui);
    zero(&tr, sizeof tr);
    saves = 0;
    dirty_at = 0;
    if (plat_store_load(OBJ_RHYTHM, &rec, sizeof rec) != (int)sizeof rec || !rec_valid(&rec)) {
        zero(&rec, sizeof rec);
        defaults();
    }
    ui.seed = 0x5EED1234u;
    app_panic();
}

void app_ui_init(void)
{
    lcd_fill(0, 0, 240, 240, C_BLACK);
    ui.redraw = R_ALL;
}

void app_frame(void)
{
    static uint32_t last_toast;
    uint32_t now = plat_ms();
    int32_t c;
    if (!ui.seed) ui.seed = 1;
    ui.seed ^= now * 2654435761u;        /* (the rhythms differ from one play to the next) */
    input();
    c = tick_now();
    if (c != ui.cur) {
        int32_t old = ui.cur;
        ui.cur = c;
        if (ui.running && (c / 3 != old / 3 || c < 0)) ui.redraw |= R_MAIN;   /* the playhead, the beat */
    }
    if (rec.page == PG_ECHO) echo_follow();
    if (rec.page == PG_ENDLESS) {
        int part = en.on ? en_part() : -1;
        static int last_part = -1;
        en_follow();
        if (part != last_part) ui.redraw |= R_MAIN;
        last_part = part;
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
int app_expect(const char *what, int *got)
{
    int n;
    if (!strcmp(what, "page")) *got = rec.page;
    else if (!strcmp(what, "bpm")) *got = rec.bpm;
    else if (!strcmp(what, "bpb")) *got = rec.bpb;
    else if (!strcmp(what, "sub")) *got = rec.sub;
    else if (!strcmp(what, "gap")) *got = rec.gap;
    else if (!strcmp(what, "level")) *got = rec.level;
    else if (!strcmp(what, "bars")) *got = rec.bars;
    else if (!strcmp(what, "lat")) *got = rec.lat;
    else if (!strcmp(what, "running")) *got = ui.running;
    else if (!strcmp(what, "echo")) *got = ui.es;
    else if (!strcmp(what, "taps")) *got = (int)ui.ntaps;
    else if (!strcmp(what, "score")) *got = ui.res.score;
    else if (!strcmp(what, "hint")) *got = ui.res.hint;
    else if (!strcmp(what, "good")) *got = ui.res.good;
    else if (!strcmp(what, "missed")) *got = ui.res.missed;
    else if (!strcmp(what, "extra")) *got = ui.res.extra;
    else if (!strcmp(what, "mean")) *got = ui.res.mean;
    else if (!strcmp(what, "notes")) *got = ui.pat.n;
    else if (!strcmp(what, "metro_taps")) *got = (int)ui.mt_n;
    else if (!strcmp(what, "metro_last")) *got = ui.mt_n ? ui.mt_err[(ui.mt_n - 1u) % MT_N] : 0;
    else if (!strcmp(what, "metro_avg")) *got = metro_avg(0, &n);
    else if (!strcmp(what, "metro_gap_taps")) {
        (void)metro_avg(1, &n);
        *got = n;
    } else if (!strcmp(what, "read")) *got = rec.read;
    else if (!strcmp(what, "best")) *got = rec.best;
    else if (!strcmp(what, "en_on")) *got = en.on;
    else if (!strcmp(what, "en_over")) *got = en.over;
    else if (!strcmp(what, "en_rounds")) *got = en.rounds;
    else if (!strcmp(what, "en_stage")) *got = en.stage;
    else if (!strcmp(what, "en_bpm")) *got = en.bpm;
    else if (!strcmp(what, "en_level")) *got = en.level;
    else if (!strcmp(what, "health")) *got = en.health;
    else if (!strcmp(what, "en_good")) *got = en.last.good;
    else if (!strcmp(what, "en_notes")) *got = en.last.n;
    else if (!strcmp(what, "lit_keys")) *got = (int)ui.lit;
    else if (!strcmp(what, "saves")) *got = (int)saves;
    else return 0;
    return 1;
}

static void host_text(const char *a, char *out, uint32_t max)   /* a script word: _ is a space */
{
    uint32_t n = 0;
    for (; a && *a && n + 1u < max; a++) out[n++] = *a == '_' ? ' ' : *a;
    out[n] = 0;
}

int app_host_cmd(const char *cmd, const char *a, const char *b, const char *c, const char *d)
{
    char want[96], got[96];
    (void)d;
    if (!strcmp(cmd, "pattern")) {       /* pattern q_ee_r_q: ECHO's next rhythm (REC plays it) */
        host_text(a, want, sizeof want);
        if (rh_parse(want, rec.bpb, &ui.pat) < 0) {
            printf("  FAIL pattern %s: not a rhythm\n", a);
            host_fails++;
        } else
            ui.have_pat = 1;
        if (!ui.running) ui.es = ES_IDLE, ui.ntaps = 0;
        ui.redraw |= R_ALL;
        return 1;
    }
    if (!strcmp(cmd, "autotap")) {       /* autotap OFFSET [SKIP] [MOVE_I MOVE_MS]: the next round tapped by the script */
        int off = atoi(a), skip = b[0] ? atoi(b) : -1, mi = c[0] ? atoi(c) : -1, mv = d[0] ? atoi(d) : 0;
        uint32_t i;
        host_ntap = 0;
        for (i = 0; i < ui.pat.n; i++) {
            int32_t ms = rh_onset_ms(&ui.pat, (int)i, rec.bpm) + off + ((int)i == mi ? mv : 0);
            if ((int)i == skip) continue;
            host_tap[host_ntap++] = (uint32_t)((int64_t)ms * SR / 1000);   /* (echo_start adds the start) */
        }
        host_itap = host_ntap;           /* armed by echo_start */
        host_armed = 1;
        return 1;
    }
    if (!strcmp(cmd, "endtap")) {        /* endtap OFFSET [SKIP_ROUND]: this ENDLESS stage tapped (one round left out) */
        int off = atoi(a), skip = b[0] ? atoi(b) : -1;
        uint32_t k, i;
        host_ntap = 0;
        for (k = 0; k < EN_ROUNDS; k++)
            for (i = 0; i < en.pat[k].n && host_ntap < RH_MAX_TAPS; i++)
                if ((int)k != skip)
                    host_tap[host_ntap++] = tick_at(en_resp(k))
                                            + (uint32_t)(((int64_t)rh_onset_ms(&en.pat[k], (int)i, en.bpm) + off) * SR / 1000);
        host_itap = 0;
        return 1;
    }
    if (!strcmp(cmd, "metrotap")) {      /* metrotap N OFFSET: the next N beats from now, tapped OFFSET ms off */
        uint32_t n = (uint32_t)atoi(a), i, bq = beat_smp_q8(), s = now_smp();
        int32_t off = atoi(b), d0 = (int32_t)(s - tr.start), b0 = (int32_t)((int64_t)d0 * 256 / (int64_t)bq) + 1;
        host_ntap = 0;
        for (i = 0; i < n && i < RH_MAX_TAPS; i++)
            host_tap[host_ntap++] = tr.start + (uint32_t)((uint64_t)(uint32_t)(b0 + (int32_t)i) * bq / 256u)
                                    + (uint32_t)((int64_t)off * SR / 1000);
        host_itap = 0;
        return 1;
    }
    if (strcmp(cmd, "expect_text")) return 0;
    /* expect_text hint|pattern TEXT (_ a space) */
    host_text(b, want, sizeof want);
    got[0] = 0;
    if (!strcmp(a, "hint")) tcat(got, rh_hint_text(ui.res.hint));
    else if (!strcmp(a, "pattern")) rh_text(&ui.pat, got);
    if (strcmp(got, want)) {
        printf("  FAIL expect_text %s: \"%s\", got \"%s\"\n", a, want, got);
        host_fails++;
    } else
        printf("  ok text %s \"%s\"\n", a, want);
    return 1;
}

int app_host_fails(void) { return host_fails; }
#endif
