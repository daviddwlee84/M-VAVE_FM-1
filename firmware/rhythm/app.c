/* SPDX-License-Identifier: GPL-3.0-or-later */
/* RHYTHM v0.3: METRO, ECHO, ENDLESS, and PULSE, JUDGE, QUIZ (see app.h). The rhythms and the scoring are rhythm.c.
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
#define FW_MINOR 3
#define SR 44100u

/* ---------------------------------------------------------------- settings --- */
#define REC_MAGIC 0x31304852u            /* "RH01" */
#define REC_VER 2u                       /* 2: 0.3's pages (a v1 record is read and kept) */

enum { PG_METRO, PG_ECHO, PG_ENDLESS, PG_PULSE, PG_JUDGE, PG_QUIZ, PG_N };
static const char *const PG_NAME[PG_N] = { "METRO", "ECHO", "ENDLESS", "PULSE", "JUDGE", "QUIZ" };
static const uint8_t PULSE_BARS[] = { 2, 4, 8, 16 }, PULSE_COUNT[] = { 1, 2, 4 };
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
    /* v2 (0.3) */
    uint8_t pmode;                       /* PULSE: 0 GUIDED (a count-in), 1 COLD (the BPM alone) */
    uint8_t pbars;                       /* PULSE: PULSE_BARS[] tapped blind */
    uint8_t pcount;                      /* PULSE: PULSE_COUNT[] bars of count-in (GUIDED) */
    uint8_t jtype;                       /* JUDGE: RH_J_* */
    uint8_t jrand;                       /* JUDGE: each trial at a tempo near the BPM, not at it */
    uint8_t qtype;                       /* QUIZ: RH_Q_* */
    uint8_t qlevel;                      /* QUIZ: 1..3 */
    uint8_t rsv2;
    uint16_t pbest[2];                   /* PULSE: the closest tempo, |%| x10 + 1 (0 none), per mode */
    uint16_t jbest[3];                   /* JUDGE: the smallest threshold, x10 (0 none), per type */
    uint16_t qbest;                      /* QUIZ: the longest streak */
} rec_t;
#define REC_V1_SIZE 20u                  /* (v1 ended at best) */
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

static void defaults_v2(void)            /* the fields 0.3 added (a v1 record keeps the rest) */
{
    rec.pmode = 0;
    rec.pbars = 1;
    rec.pcount = 0;
    rec.jtype = RH_J_TEMPO;
    rec.jrand = 0;
    rec.qtype = RH_Q_TERM_BPM;
    rec.qlevel = 1;
    rec.rsv2 = 0;
    rec.pbest[0] = rec.pbest[1] = 0;
    rec.jbest[0] = rec.jbest[1] = rec.jbest[2] = 0;
    rec.qbest = 0;
}

static int rec_valid(const rec_t *r)
{
    return r->magic == REC_MAGIC && r->ver == REC_VER && r->size == sizeof *r && r->page < PG_N && r->bpm >= 30
           && r->bpm <= 240 && r->bpb >= 2 && r->bpb <= 4 && r->sub < SUB_N && r->gap < NGAPS && r->level >= 1
           && r->level <= RH_LEVELS && r->bars >= 1 && r->bars <= 2 && r->lat >= -30 && r->lat <= 90
           && r->read <= 1u && r->pmode <= 1u && r->pbars < sizeof PULSE_BARS && r->pcount < sizeof PULSE_COUNT
           && r->jtype < RH_J_N && r->jrand <= 1u && r->qtype < RH_Q_N && r->qlevel >= 1 && r->qlevel <= 3;
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
enum { EV_ACCENT = 1, EV_CLICK = 2, EV_SOFT = 4, EV_SUB = 8, EV_NOTE = 16, EV_LOW = 32 };   /* = RH_S_* */
enum { TR_METRO, TR_ECHO, TR_SEQ };      /* TR_SEQ: a list of sounds at their own samples (rh_seq_t) */
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
    uint8_t sn, si;                      /* TR_SEQ: sounds, the next one */
    uint32_t sq_at[RH_SEQ_MAX];          /* TR_SEQ: samples from start */
    uint8_t sq_ev[RH_SEQ_MAX];
} tr;
static volatile uint32_t snd_at;         /* the sample of the last click or note (the keys' lights follow it) */

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
    if (e & EV_LOW) voice(262.0f, 0.70f, 90.0f);
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
        if (tr.mode == TR_SEQ)
            while (tr.run && tr.si < tr.sn && (int32_t)(now - (tr.start + tr.sq_at[tr.si])) >= 0) {
                sound(tr.sq_ev[tr.si++]);
                snd_at = now;
            }
        else
            while (tr.run && (int32_t)(now - tr.next_at) >= 0) {
                uint8_t e = event_at(tr.next);
                if (e) sound(e), snd_at = now;
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
static volatile uint8_t tq_k[TAPQ];      /* and which key (the lowest pressed in that tick) */
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
        uint8_t k = 0;
        while (!(down >> k & 1u)) k++;
        tq[tq_w % TAPQ] = at ? at : now_smp();
        tq_k[tq_w % TAPQ] = k;
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
} rh_en;

static uint32_t en_bar(void) { return (uint32_t)rec.bpb * RH_TPB; }
static uint32_t en_call(uint32_t k) { return en_bar() * (1u + 2u * k); }   /* tick where round k's call starts */
static uint32_t en_resp(uint32_t k) { return en_call(k) + en_bar(); }

static void en_stage(void)               /* the next stage: its rhythms, its events, the transport */
{
    uint32_t k, t, i, bar = en_bar(), len = en_call(EN_ROUNDS);
    for (k = 0; k < EN_ROUNDS; k++) {
        rh_generate(&ui.seed, rh_en.level, rec.bpb, 1, &rh_en.pat[k]);
        rh_en.ntaps[k] = 0;
    }
    rh_en.scored = 0;
    zero(tr.ev, sizeof tr.ev);
    for (t = 0; t < len; t += RH_TPB)
        tr.ev[t] = (uint8_t)(t < bar ? ((t / RH_TPB) % rec.bpb ? EV_CLICK : EV_ACCENT) : EV_SOFT);
    for (k = 0; k < EN_ROUNDS; k++) {
        tr.ev[en_resp(k)] = EV_CLICK;    /* your bar starts on a louder beat */
        for (i = 0; i < rh_en.pat[k].n; i++) tr.ev[en_call(k) + rh_en.pat[k].on[i]] |= EV_NOTE;
    }
    tr.len = (uint16_t)len;
    tr_stop();
    tq_r = tq_w;
    tr_go_at(TR_ECHO, now_smp() + SR / 20u, rh_en.bpm);
    ui.running = 1;
    ui.redraw |= R_ALL;
}

static void en_start(void)
{
    zero(&rh_en, sizeof rh_en);
    rh_en.on = 1;
    rh_en.health = 100;
    rh_en.bpm = rec.bpm;
    rh_en.level = rec.level;
    en_stage();
}

static void en_end(void)
{
    tr_stop();
    rh_en.on = 0;
    rh_en.over = 1;
    if (rh_en.rounds > rec.best) {
        rec.best = rh_en.rounds;
        rh_en.new_best = 1;
        changed(0);
    }
    {
        char t[16];
        t[0] = 0;
        tint(t, rh_en.rounds);
        app_say(rh_en.new_best ? "NEW BEST" : "ROUNDS", t);
    }
    ui.redraw |= R_ALL;
}

static void en_tap(uint32_t smp)         /* into the round whose bar (give or take half a beat) it falls in */
{
    int32_t d = (int32_t)(smp - tr.start), half = RH_TPB / 2;
    float tick;
    uint32_t k;
    if (!rh_en.on || d < 0) return;
    tick = (float)d * 256.0f / (float)tr.tick_q8;
    for (k = 0; k < EN_ROUNDS; k++) {
        float r = (float)en_resp(k);
        if (tick >= r - (float)half && tick < r + (float)(en_bar() + half)) {
            if (rh_en.ntaps[k] < EN_TAPS)
                rh_en.taps[k][rh_en.ntaps[k]++] = smp_ms((int32_t)(smp - tick_at(en_resp(k)))) - rec.lat;
            return;
        }
    }
}

static void en_follow(void)
{
    int32_t c = ui.cur;
    if (!rh_en.on || c < 0) return;
    while (rh_en.scored < EN_ROUNDS && c >= (int32_t)(en_resp(rh_en.scored) + en_bar() + RH_TPB / 2)) {
        uint32_t k = rh_en.scored++;
        rh_result_t *r = &rh_en.last;
        int d;
        rh_score(&rh_en.pat[k], rh_en.bpm, rh_en.taps[k], rh_en.ntaps[k], r);
        d = -(12 * r->missed + 8 * r->extra + 6 * r->off + 2 * r->ok);
        if (d < -40) d = -40;            /* a lost round costs at most 40: three to lose it all */
        if (r->good == r->n && !r->extra) d += 6;
        rh_en.delta = (int16_t)d;
        rh_en.health = (int16_t)clamp(rh_en.health + d, 0, 100);
        rh_en.last_k = (uint8_t)k;
        rh_en.have_last = 1;
        ui.redraw |= R_ALL;
        if (!rh_en.health) {
            en_end();
            return;
        }
        rh_en.rounds++;
    }
    if (rh_en.scored == EN_ROUNDS) {        /* the next stage: faster, every second one harder */
        rh_en.stage++;
        rh_en.bpm = (uint8_t)clamp(rh_en.bpm + EN_SPEEDUP, 30, 240);
        if (!(rh_en.stage % 2u) && rh_en.level < RH_LEVELS) rh_en.level++;
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
/* -------------------------------------------------------------- TR_SEQ --- */
#define LEAD_SMP 2205u                   /* rh_*_seq start their first sound 50 ms in */

/* sounds at these samples from start (at most RH_SEQ_MAX) */
static void seq_load(uint32_t start, uint32_t n, const uint32_t *at, const uint8_t *ev, uint32_t bpm)
{
    uint32_t f = plat_lock(), i;
    tr.mode = TR_SEQ;
    tr.start = start;
    tr.tick_q8 = beat_q8_at(bpm) / RH_TPB;
    tr.sn = (uint8_t)n;
    for (i = 0; i < n; i++) tr.sq_at[i] = at[i], tr.sq_ev[i] = ev[i];
    tr.si = 0;
    tr.next = 0;
    tr.next_at = 0;
    tr.run = 1;
    plat_unlock(f);
    ui.running = 1;
}

static void seq_go(uint32_t start, const rh_seq_t *q, uint32_t bpm)
{
    uint32_t at[RH_SEQ_MAX], i;
    for (i = 0; i < q->n; i++) at[i] = (uint32_t)((float)q->us[i] * 0.0441f + 0.5f);   /* us -> samples, 44.1 kHz */
    seq_load(start, q->n, at, q->ev, bpm);
}

static int seq_over(void) { return tr.mode == TR_SEQ && tr.run && tr.si >= tr.sn; }

static uint32_t seq_end(void)            /* the sample of the last sound */
{
    return tr.start + (tr.sn ? tr.sq_at[tr.sn - 1] : 0);
}

/* ------------------------------------------------------------------ PULSE --- */
/* tap every beat with no click. GUIDED: a count-in you hear, then the click stops and the grid goes on without it.
 * COLD: nothing to hear, only the BPM; your first tap is beat 1. After the last beat the click comes back for a bar
 * on the true grid (GUIDED) or from your first tap (COLD), so you hear where it would have been */
enum { PS_IDLE, PS_COUNT, PS_WAIT, PS_BLIND, PS_RESULT };
static struct {
    uint8_t st, cold, have;
    uint8_t beats, count;                /* blind beats, count-in beats */
    uint32_t anchor;                     /* sample of the first blind beat */
    uint32_t end;                        /* sample where it is scored */
    int32_t taps[RH_PULSE_TAPS];
    uint16_t ntaps;
    rh_pulse_t res;
    uint8_t new_best;
} pl;

static uint32_t pl_beat(uint32_t k) { return pl.anchor + q8_mul(k, beat_smp_q8()); }
static uint32_t pl_grid_end(void) { return pl_beat(pl.beats); }   /* a beat after the last: no tap for it yet */

static void pl_start(void)
{
    rh_seq_t q;
    uint32_t now = now_smp();
    zero(&pl.res, sizeof pl.res);
    pl.cold = rec.pmode;
    pl.beats = (uint8_t)(PULSE_BARS[rec.pbars] * rec.bpb);
    if (pl.beats > RH_PULSE_MAX) pl.beats = RH_PULSE_MAX;
    pl.ntaps = 0;
    pl.have = 0;
    pl.new_best = 0;
    tq_r = tq_w;
    tr_stop();
    if (pl.cold) {
        pl.count = 0;
        pl.st = PS_WAIT;
    } else {
        pl.count = (uint8_t)(PULSE_COUNT[rec.pcount] * rec.bpb);
        rh_clicks_seq(rec.bpm, rec.bpb, PULSE_COUNT[rec.pcount], &q);
        seq_go(now, &q, rec.bpm);
        pl.anchor = now + LEAD_SMP + q8_mul(pl.count, beat_smp_q8());
        pl.end = pl_grid_end();
        pl.st = PS_COUNT;
    }
    ui.redraw |= R_ALL;
}

static void pl_tap(uint32_t smp)
{
    if (pl.st == PS_WAIT) {              /* COLD: the first tap is beat 1 */
        pl.anchor = smp;
        pl.end = pl_grid_end();
        pl.st = PS_BLIND;
        ui.running = 1;
    }
    if (pl.st != PS_COUNT && pl.st != PS_BLIND) return;
    if ((int32_t)(smp - pl.anchor) < -(int32_t)(beat_smp_q8() / 512u)) return;   /* counting in: tap along, not scored */
    if (pl.ntaps < RH_PULSE_TAPS)
        pl.taps[pl.ntaps++] = smp_ms((int32_t)(smp - pl.anchor)) - (pl.cold ? 0 : rec.lat);
    /* the end: a quarter beat after the tap that answers the last beat, by your own pulse (late if you drag); with
     * no such tap, a beat after the grid's last, held on while taps keep coming (up to a quarter of the length) */
    rh_pulse(rec.bpm, pl.beats, pl.taps, pl.ntaps, &pl.res);
    if (pl.res.tap_of[pl.beats - 1u] >= 0) pl.end = smp + beat_smp_q8() / 1024u;
    else {
        uint32_t later = smp + q8_mul(5u, beat_smp_q8()) / 4u, cap = pl_beat(pl.beats + pl.beats / 4u + 1u);
        if ((int32_t)(later - pl.end) > 0) pl.end = (int32_t)(later - cap) > 0 ? cap : later;
    }
    ui.redraw |= R_MAIN;
}

static void pl_done(void)
{
    uint32_t k, bq = beat_smp_q8(), at[4];
    uint8_t ev[4];
    rh_pulse(rec.bpm, pl.beats, pl.taps, pl.ntaps, &pl.res);
    pl.st = PS_RESULT;
    pl.have = 1;
    tr_stop();
    if (pl.res.answered * 10u >= pl.beats * 9u && pl.res.bpm_x10) {   /* a best: nine beats in ten answered */
        int a = pl.res.tempo_x10 < 0 ? -pl.res.tempo_x10 : pl.res.tempo_x10;
        uint16_t *b = &rec.pbest[pl.cold];
        if (!*b || a + 1 < *b) {
            *b = (uint16_t)(a + 1);
            pl.new_best = 1;
            changed(0);
        }
    }
    /* the click again for a bar, where the grid is now (a beat from the last one) */
    for (k = 0; k < rec.bpb; k++) {
        at[k] = q8_mul(pl.beats + 1u + k, bq);
        ev[k] = (uint8_t)(k ? EV_CLICK : EV_ACCENT);
    }
    seq_load(pl.anchor, rec.bpb, at, ev, rec.bpm);
    {
        char t[48];
        t[0] = 0;
        tcat(t, rh_pulse_hint(pl.res.hint));
        if (ann_on()) ann_say(t, 0, ANN_NOW);
    }
    ui.redraw |= R_ALL;
}

static void pl_follow(void)
{
    uint32_t now = now_smp();
    if (pl.st == PS_COUNT && (int32_t)(now - pl.anchor) >= -(int32_t)(beat_smp_q8() / 512u)) {
        pl.st = PS_BLIND;
        ui.redraw |= R_ALL;
    }
    if (pl.st == PS_BLIND && (int32_t)(now - pl.end) >= 0) pl_done();
    if (pl.st == PS_RESULT && seq_over() && (int32_t)(now - seq_end()) > (int32_t)(SR / 5u)) tr_stop();
}

static void pl_stop(void)
{
    if (pl.st == PS_BLIND && pl.ntaps) pl_done();   /* scored with what there is */
    else {
        tr_stop();
        pl.st = pl.have ? PS_RESULT : PS_IDLE;
    }
    tr_stop();
    ui.redraw |= R_ALL;
}

/* ------------------------------------------------------------------ JUDGE --- */
/* hear a trial, answer on the low keys (slower, dragging) or the high ones (faster, rushing). The staircase makes
 * the change smaller after two right in a row and bigger after a wrong one, and ends with your threshold */
enum { JS_IDLE, JS_PLAY, JS_ASK, JS_FEED, JS_DONE };
static struct {
    uint8_t st, ok, new_best, type;
    int8_t sign, said;                   /* the trial's direction; your answer */
    uint16_t bpm;
    rh_stair_t s;
    rh_seq_t q;
    uint32_t until;                      /* JS_FEED ends (ms) */
    float thr;
} jd;

static void jd_play(void)                /* the trial (again) */
{
    tr_stop();
    seq_go(now_smp(), &jd.q, jd.bpm);
    jd.st = JS_PLAY;
    ui.redraw |= R_ALL;
}

static void jd_trial(void)
{
    jd.sign = (int8_t)(rnd(&ui.seed) & 1u ? 1 : -1);
    jd.bpm = rec.bpm;
    if (rec.jrand) jd.bpm = (uint16_t)clamp((int32_t)rec.bpm * (85 + (int32_t)(rnd(&ui.seed) % 31u)) / 100, 30, 240);
    rh_judge_seq(jd.type, jd.bpm, rec.bpb, jd.s.d, jd.sign, &jd.q);
    jd.said = 0;
    jd_play();
}

static void jd_start(void)
{
    jd.type = rec.jtype;
    rh_stair_init(&jd.s, jd.type);
    jd.new_best = 0;
    jd.thr = 0.0f;
    jd_trial();
}

static void jd_answer(int dir)
{
    int done;
    if (jd.st != JS_PLAY && jd.st != JS_ASK) return;
    tr_stop();
    jd.said = (int8_t)dir;
    jd.ok = (uint8_t)(dir == jd.sign);
    done = rh_stair_answer(&jd.s, jd.ok);
    jd.st = JS_FEED;
    jd.until = plat_ms() + (jd.ok ? 700u : 1400u);
    if (done) {
        uint16_t t;
        jd.thr = rh_stair_threshold(&jd.s);
        t = (uint16_t)(jd.thr * 10.0f + 0.5f);
        if (!t) t = 1;
        if (!rec.jbest[jd.type] || t < rec.jbest[jd.type]) {
            rec.jbest[jd.type] = t;
            jd.new_best = 1;
            changed(0);
        }
    }
    app_say(jd.ok ? "RIGHT" : "WRONG", "");
    ui.redraw |= R_ALL;
}

static void jd_key(uint32_t k)
{
    if (k == NKEYS / 2) return;          /* (the middle key is neither) */
    jd_answer(k < NKEYS / 2 ? -1 : 1);
}

static void jd_follow(void)
{
    if (jd.st == JS_PLAY && seq_over() && (int32_t)(now_smp() - seq_end()) > (int32_t)(SR / 4u)) {
        tr_stop();
        jd.st = JS_ASK;
        ui.redraw |= R_ALL;
    }
    if (jd.st == JS_FEED && (int32_t)(plat_ms() - jd.until) >= 0) {
        if (jd.s.done) {
            char t[24];
            jd.st = JS_DONE;
            t[0] = 0;
            tint(t, (int)(jd.thr * 10.0f + 0.5f) / 10);
            tcat(t, ".");
            tint(t, (int)(jd.thr * 10.0f + 0.5f) % 10);
            tcat(t, rh_judge_unit(jd.type));
            app_say(jd.new_best ? "NEW BEST" : "THRESHOLD", t);
            ui.redraw |= R_ALL;
        } else
            jd_trial();
    }
}

/* ------------------------------------------------------------------- QUIZ --- */
/* four choices on four zones of the keyboard (A lowest .. D highest). HEAR and METER play their question (REC
 * again). A term or meter you miss comes back more often for the rest of the session */
enum { QS_IDLE, QS_ASK, QS_FEED };
static struct {
    uint8_t st, type, level, chosen, ok;
    rh_quiz_t q;
    uint16_t asked, right, streak;
    uint8_t miss[RH_NTERMS];
    int prev;
    uint32_t until;
    uint8_t new_best;
} qz;

static void qz_sound(void)
{
    rh_seq_t s;
    tr_stop();
    if (qz.q.type == RH_Q_HEAR) rh_clicks_seq(qz.q.bpm, 4, 2, &s);
    else if (qz.q.type == RH_Q_METER) rh_meter_seq(qz.q.ask, qz.q.bpm, qz.level, 3, &s);
    else return;
    seq_go(now_smp(), &s, qz.q.bpm);
}

static void qz_next(void)
{
    rh_quiz_make(&ui.seed, qz.type, qz.level, qz.miss, qz.prev, &qz.q);
    qz.prev = qz.q.ask;
    qz.st = QS_ASK;
    qz_sound();
    ui.redraw |= R_ALL;
}

static void qz_start(void)
{
    if (qz.type != rec.qtype || qz.level != rec.qlevel) zero(qz.miss, sizeof qz.miss);
    qz.type = rec.qtype;
    qz.level = rec.qlevel;
    qz.asked = qz.right = qz.streak = 0;
    qz.prev = -1;
    qz.new_best = 0;
    qz_next();
}

static void qz_answer(int z)
{
    if (qz.st != QS_ASK) return;
    tr_stop();
    qz.chosen = (uint8_t)z;
    qz.ok = (uint8_t)(z == qz.q.answer);
    qz.asked++;
    if (qz.ok) {
        qz.right++;
        qz.streak++;
        if (qz.miss[qz.q.ask]) qz.miss[qz.q.ask]--;
        if (qz.streak > rec.qbest) {
            rec.qbest = qz.streak;
            qz.new_best = 1;
            changed(0);
        }
    } else {
        qz.streak = 0;
        if (qz.miss[qz.q.ask] < 4) qz.miss[qz.q.ask] += 2;
    }
    qz.st = QS_FEED;
    qz.until = plat_ms() + (qz.ok ? 900u : 2400u);
    app_say(qz.ok ? "RIGHT" : "WRONG", "");
    ui.redraw |= R_ALL;
}

static void qz_key(uint32_t k) { qz_answer((int)(k * 4u / NKEYS)); }

static void qz_follow(void)
{
    if (qz.st == QS_FEED && (int32_t)(plat_ms() - qz.until) >= 0) qz_next();
    if (seq_over() && (int32_t)(now_smp() - seq_end()) > (int32_t)(SR / 4u)) tr_stop();
}

static int pl_busy(void) { return pl.st == PS_COUNT || pl.st == PS_WAIT || pl.st == PS_BLIND; }

static void set_page(int p)
{
    tr_stop();
    rh_en.on = 0;
    rec.page = (uint8_t)p;
    ui.es = ES_IDLE;
    if (pl.st != PS_RESULT) pl.st = PS_IDLE;
    if (jd.st != JS_DONE) jd.st = JS_IDLE;
    qz.st = QS_IDLE;
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
        else if (k == B_PLAY && rec.page == PG_PULSE) {
            if (pl.st == PS_COUNT || pl.st == PS_WAIT || pl.st == PS_BLIND) pl_stop();
            else pl_start();
        } else if (k == B_PLAY && rec.page == PG_JUDGE) {
            if (jd.st == JS_IDLE || jd.st == JS_DONE) jd_start();
            else {
                tr_stop();
                jd.st = JS_IDLE;
                ui.redraw |= R_ALL;
            }
        } else if (k == B_PLAY && rec.page == PG_QUIZ) {
            if (qz.st == QS_IDLE) qz_start();
            else {
                tr_stop();
                qz.st = QS_IDLE;
                ui.redraw |= R_ALL;
            }
        } else if (k == B_PLAY) {
            if (rec.page == PG_ENDLESS) {
                if (rh_en.on) en_end();
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
        } else if (k == B_SEL && rec.page == PG_PULSE && !pl_busy()) {
            rec.pcount = (uint8_t)((rec.pcount + 1u) % sizeof PULSE_COUNT);
            t[0] = 0;
            tint(t, PULSE_COUNT[rec.pcount]);
            tcat(t, PULSE_COUNT[rec.pcount] > 1 ? " BARS" : " BAR");
            app_say("COUNT-IN", t);
            changed(R_ALL);
        } else if (k == B_SEL && rec.page == PG_JUDGE) {
            rec.jrand = (uint8_t)!rec.jrand;
            app_say("TEMPO", rec.jrand ? "VARIED" : "FIXED");
            changed(R_ALL);
        } else if (k == B_REC) {
            if (rec.page == PG_METRO) tap_tempo();
            else if (rec.page == PG_ECHO && !ui.running && ui.have_pat) echo_start(0);   /* the same rhythm again */
            else if (rec.page == PG_JUDGE && (jd.st == JS_PLAY || jd.st == JS_ASK)) jd_play();
            else if (rec.page == PG_QUIZ && qz.st == QS_ASK) qz_sound();
        } else if (k == B_OCTDN || k == B_OCTUP) {
            if (rh_en.on || pl_busy()) continue;
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
    if ((e = plat_enc(EN_SELECT)) != 0 && !rh_en.on && !pl_busy()) {
        rec.bpm = (uint8_t)clamp(rec.bpm + e, 30, 240);
        t[0] = 0;
        tint(t, rec.bpm);
        app_say("BPM", t);
        if (ui.running && rec.page == PG_METRO) metro_start();
        changed(R_ALL);
    }
    if ((e = plat_enc(EN_K1)) != 0 && !(ui.running && rec.page != PG_METRO) && !pl_busy()) {
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
        if (rec.page == PG_PULSE) {
            if (!pl_busy()) {
                rec.pmode = (uint8_t)(e > 0);
                app_say("PULSE", rec.pmode ? "COLD" : "GUIDED");
            }
        } else if (rec.page == PG_JUDGE) {
            if (jd.st == JS_IDLE || jd.st == JS_DONE) {
                rec.jtype = (uint8_t)clamp(rec.jtype + (e > 0 ? 1 : -1), 0, RH_J_N - 1);
                app_say("JUDGE", rh_judge_name(rec.jtype));
                jd.st = JS_IDLE;
            }
        } else if (rec.page == PG_QUIZ) {
            rec.qtype = (uint8_t)clamp(rec.qtype + (e > 0 ? 1 : -1), 0, RH_Q_N - 1);
            app_say("QUIZ", rh_quiz_name(rec.qtype));
            if (qz.st != QS_IDLE) qz_start();
        } else if (rec.page == PG_METRO) {
            rec.sub = (uint8_t)clamp(rec.sub + (e > 0 ? 1 : -1), 0, SUB_N - 1);
            app_say("SUB", SUB_NAME[rec.sub]);
            if (ui.running) metro_start();
        } else if (!ui.running && rec.page != PG_PULSE) {   /* ECHO and ENDLESS: the level (ENDLESS starts from it) */
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
        if (rec.page == PG_PULSE) {
            if (!pl_busy()) {
                rec.pbars = (uint8_t)clamp(rec.pbars + (e > 0 ? 1 : -1), 0, (int)sizeof PULSE_BARS - 1);
                t[0] = 0;
                tint(t, PULSE_BARS[rec.pbars]);
                tcat(t, " BARS");
                app_say("BLIND", t);
            }
        } else if (rec.page == PG_QUIZ) {
            rec.qlevel = (uint8_t)clamp(rec.qlevel + (e > 0 ? 1 : -1), 1, 3);
            t[0] = 0;
            tint(t, rec.qlevel);
            app_say("LEVEL", t);
            if (qz.st != QS_IDLE) qz_start();
        } else if (rec.page == PG_JUDGE) {
        } else if (rec.page == PG_METRO) {
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
        uint32_t s = tq[tq_r % TAPQ], key = tq_k[tq_r % TAPQ];
        tq_r++;
        if (rec.page == PG_PULSE) pl_tap(s);
        else if (rec.page == PG_JUDGE) jd_key(key);
        else if (rec.page == PG_QUIZ) qz_key(key);
        else if (rec.page == PG_METRO) metro_tap(s);
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
        if (tr.mode == TR_SEQ) {
            if (now_smp() - snd_at < SR * 6u / 100u) flash = 2;   /* 60 ms after each sound */
        } else if (rec.page == PG_METRO) {
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
        if (!flash && rec.page == PG_JUDGE && (jd.st == JS_PLAY || jd.st == JS_ASK) && k != NKEYS / 2)
            fx = k < NKEYS / 2 ? FX_GLOW : FX_MID;   /* the two answers: low keys slower, high keys faster */
        if (!flash && rec.page == PG_QUIZ && qz.st == QS_ASK) fx = (k * 4u / NKEYS) % 2u ? FX_MID : FX_GLOW;   /* A..D */
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
    tint(t, rh_en.on ? rh_en.bpm : rec.bpm);
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
#define STAFF_Y 88                          /* ECHO's staff line, in the main area */
#define STAFF_STEM 15
static int32_t staff_x(const rh_pattern_t *p, int32_t x0, int32_t w, int32_t tick)
{
    return x0 + tick * w / (p->nbeats ? p->nbeats * RH_TPB : 1);
}

static void staff_head(int32_t x, int32_t y, int32_t hw, int hollow, uint16_t c)
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

static void staff_rest(int32_t x, int32_t y, uint16_t c)   /* a quarter rest */
{
    cv_line(x, y - 8, x + 3, y - 4, c);
    cv_line(x + 3, y - 4, x, y - 1, c);
    cv_line(x, y - 1, x + 3, y + 3, c);
    cv_line(x + 3, y + 3, x, y + 3, c);
    cv_line(x, y + 3, x + 2, y + 6, c);
}

static void staff_beam(int32_t xa, int32_t xb, int32_t y, uint16_t c) { cv_rect(xa, y, xb - xa + 1, 2, c); }

static void draw_staff(const rh_pattern_t *p, const rh_result_t *r, int32_t x0, int32_t w, int32_t y, int show)
{
    int32_t total = p->nbeats * RH_TPB, sp = total ? 3 * w / total : 8, hw = clamp(sp - 1, 4, 6), by = y - STAFF_STEM;
    int32_t sx[4];
    uint16_t sc = r ? C_GRAY : C_HI;
    int beat = 0, ci, j, i = 0, b;
    cv_rect(x0 - 6, y, w + 10, 1, C_LINE);
    for (b = 0; b <= p->nbeats; b++) {   /* beats under the line, bar lines through it */
        int32_t x = staff_x(p, x0, w, b * RH_TPB);
        if (b % (p->bpb ? p->bpb : 4) == 0 && b) cv_rect(x - hw / 2 - 3, y - 10, 1, 21, C_GRAY);
        else if (b < p->nbeats) cv_rect(x, y + 4, 1, 2, C_DIM);
    }
    if (!show) return;
    for (ci = 0; ci < p->ncells; ci++) {
        int cell = p->cells[ci], n = CELLS[cell].n;
        if (cell == RH_REST) staff_rest(staff_x(p, x0, w, beat * RH_TPB) - 1, y, C_HI);
        for (j = 0; j < n && i < p->n; j++, i++) {
            int32_t x = staff_x(p, x0, w, p->on[i]);
            int miss = r && r->grade[i] == RH_G_MISS;
            uint16_t c = r ? (miss ? C_BAD : grade_col(r->grade[i])) : C_WHITE;
            staff_head(x, y, hw, miss || cell == RH_TAA, c);
            sx[j] = x - hw / 2 + hw - 1;
            cv_rect(sx[j], by, 1, STAFF_STEM - 1, sc);
        }
        switch (cell) {
        case RH_TITI:
        case RH_TRIOLA:
            staff_beam(sx[0], sx[n - 1], by, sc);
            if (cell == RH_TRIOLA) cv_text((sx[0] + sx[2]) / 2 - 2, by - 13, &FONT_T, "3", C_GRAY);
            break;
        case RH_TIKA:
            staff_beam(sx[0], sx[3], by, sc);
            staff_beam(sx[0], sx[3], by + 3, sc);
            break;
        case RH_TITIKA:
            staff_beam(sx[0], sx[2], by, sc);
            staff_beam(sx[1], sx[2], by + 3, sc);
            break;
        case RH_TIKATI:
            staff_beam(sx[0], sx[2], by, sc);
            staff_beam(sx[0], sx[1], by + 3, sc);
            break;
        case RH_TIMKA:                   /* a dotted eighth and a sixteenth: the dot, the short second beam */
            staff_beam(sx[0], sx[1], by, sc);
            staff_beam(sx[1] - 4, sx[1], by + 3, sc);
            cv_rect(staff_x(p, x0, w, p->on[i - 2]) + hw / 2 + 2, y - 1, 2, 2, C_WHITE);
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
    draw_staff(&ui.pat, res ? &ui.res : 0, TL_X, TL_W, STAFF_Y - oy, show);
    for (i = 0; res && i < ui.pat.n; i++)
        if (ui.res.tap_of[i] >= 0) {     /* how far: a line from the note to its tap */
            int32_t x = tl_x(ui.pat.on[i], 1), tx = tl_x(ui.taps[ui.res.tap_of[i]], 0);
            cv_rect(tx < x ? tx : x, STAFF_Y + 10 - oy, (tx < x ? x - tx : tx - x) + 1, 1, C_DIM);
        }
    for (i = 0; i < ui.ntaps; i++) {      /* your taps */
        int32_t x = tl_x(ui.taps[i], 0);
        if (x >= TL_X - 8 && x <= TL_X + TL_W + 8) cv_rect(x - 1, STAFF_Y + 12 - oy, 2, 10, C_WHITE);
    }
    /* the playhead */
    if (ui.running && ui.cur >= 0) {
        int32_t base = ui.es == ES_LISTEN ? (int32_t)ui.t_listen : ui.es == ES_TAP ? (int32_t)ui.t_tap : -1;
        if (base >= 0) cv_rect(tl_x(ui.cur - base, 1), STAFF_Y - 30 - oy, 2, 56, C_AMB);
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
    int part = rh_en.on ? en_part() : -1, i;
    int32_t hb = 200 * rh_en.health / 100;
    if (rh_en.on && part == 0 && ui.cur >= 0) {
        t[0] = 0;
        tint(t, (int)((uint32_t)ui.cur / RH_TPB % rec.bpb) + 1);
        big = t;
    } else if (rh_en.on)
        big = part == 1 ? "LISTEN" : "YOUR TURN", bigc = part == 1 ? C_HI : C_WHITE;
    else if (rh_en.over)
        big = rh_en.new_best ? "NEW BEST" : "GAME OVER", bigc = rh_en.new_best ? C_GOOD : C_BAD;
    else
        big = "ENDLESS", bigc = C_HI;
    text_c(120, 2 - oy, part == 0 ? &FONT_L : &FONT_M, big, bigc);
    /* health */
    hc = rh_en.health > 50 ? C_GOOD : rh_en.health > 25 ? C_OK : C_BAD;
    cv_rect(20, 38 - oy, 200, 10, C_LINE);
    if (rh_en.on || rh_en.over) cv_rect(20, 38 - oy, hb, 10, hc);
    t[0] = 0;
    tcat(t, "ROUND ");
    tint(t, rh_en.rounds);
    tcat(t, "   ");
    tint(t, rh_en.on || rh_en.over ? rh_en.bpm : rec.bpm);
    tcat(t, " BPM   LV ");
    tint(t, rh_en.on || rh_en.over ? rh_en.level : rec.level);
    tcat(t, "   BEST ");
    tint(t, rec.best);
    text_c(120, 52 - oy, &FONT_T, t, C_GRAY);
    if (!rh_en.have_last) {
        text_c(120, 96 - oy, &FONT_T, rh_en.on ? "TAP EACH BAR BACK, NO BREAKS" : "PLAY: START   K2: LEVEL   SELECT: BPM", C_DIM);
        return;
    }
    /* the last round: its notes coloured, your taps under them */
    {
        const rh_pattern_t *p = &rh_en.pat[rh_en.last_k];
        int32_t total = p->nbeats * RH_TPB, tick_us = (int32_t)rh_tick_ms_x1000(rh_en.bpm);
        draw_staff(p, &rh_en.last, TL_X, TL_W, STAFF_Y + 6 - oy, 1);
        for (i = 0; i < rh_en.ntaps[rh_en.last_k]; i++) {
            int32_t x = TL_X + rh_en.taps[rh_en.last_k][i] * 1000 / tick_us * TL_W / (total ? total : 1);
            if (x >= TL_X - 8 && x <= TL_X + TL_W + 8) cv_rect(x - 1, STAFF_Y + 18 - oy, 2, 10, C_WHITE);
        }
    }
    t[0] = 0;
    tcat(t, "GOOD ");
    tint(t, rh_en.last.good);
    tcat(t, "/");
    tint(t, rh_en.last.n);
    tcat(t, "   HEALTH ");
    if (rh_en.delta >= 0) tcat(t, "+");
    tint(t, rh_en.delta);
    tcat(t, "   ");
    tcat(t, rh_hint_text(rh_en.last.hint));
    text_c(120, 136 - oy, &FONT_T, t, rh_en.delta >= 0 ? C_GOOD : C_OK);
}

static void tx10(char *d, int v, int sign)   /* x10 -> "+2.4" (sign: always show it) */
{
    int a = v < 0 ? -v : v;
    if (v < 0) tcat(d, "-");
    else if (sign) tcat(d, "+");
    tint(d, a / 10);
    tcat(d, ".");
    tint(d, a % 10);
}

static void draw_pulse(int32_t oy)
{
    char t[48];
    const rh_pulse_t *r = &pl.res;
    int32_t i;
    if (pl.st != PS_RESULT) {
        const char *big = "PULSE", *sub = rec.pmode ? "COLD: ONLY THE BPM, NO CLICK" : "GUIDED: COUNT-IN, THEN BLIND";
        uint16_t bc = C_HI;
        t[0] = 0;
        if (pl.st == PS_COUNT && ui.cur >= 0) {
            int32_t d = (int32_t)(now_smp() - tr.start - LEAD_SMP);
            uint32_t b = d < 0 ? 0 : (uint32_t)((float)d * 256.0f / (float)beat_smp_q8());
            tint(t, (int)(b % rec.bpb) + 1);
            big = t, bc = C_WHITE, sub = "LISTEN, THEN KEEP IT GOING";
        } else if (pl.st == PS_WAIT) {
            tint(t, rec.bpm);
            big = t, bc = C_WHITE, sub = "YOUR FIRST TAP IS BEAT 1";
        } else if (pl.st == PS_BLIND)
            big = "KEEP TAPPING", bc = C_WHITE, sub = "";
        text_c(120, 6 - oy, &FONT_L, big, bc);
        text_c(120, 46 - oy, &FONT_T, sub, C_GRAY);
        if (pl.st == PS_BLIND) {          /* how far, never where: no beat shown, it is blind */
            int32_t w = 200 * (pl.ntaps < pl.beats ? pl.ntaps : pl.beats) / (pl.beats ? pl.beats : 1);
            cv_rect(20, 80 - oy, 200, 6, C_LINE);
            cv_rect(20, 80 - oy, w, 6, C_HI);
            t[0] = 0;
            tint(t, pl.ntaps);
            tcat(t, " / ");
            tint(t, pl.beats);
            tcat(t, " TAPS");
            text_c(120, 92 - oy, &FONT_T, t, C_GRAY);
        } else if (pl.st == PS_IDLE) {
            t[0] = 0;
            tcat(t, rec.pmode ? "COLD  " : "GUIDED  ");
            if (!rec.pmode) {
                tint(t, PULSE_COUNT[rec.pcount]);
                tcat(t, " IN + ");
            }
            tint(t, PULSE_BARS[rec.pbars]);
            tcat(t, " BARS BLIND");
            text_c(120, 80 - oy, &FONT_T, t, C_HI);
            t[0] = 0;
            tcat(t, "BEST  GUIDED ");
            if (rec.pbest[0]) tcat(t, "+-"), tx10(t, rec.pbest[0] - 1, 0), tcat(t, "%");
            else tcat(t, "-");
            tcat(t, "  COLD ");
            if (rec.pbest[1]) tcat(t, "+-"), tx10(t, rec.pbest[1] - 1, 0), tcat(t, "%");
            else tcat(t, "-");
            text_c(120, 100 - oy, &FONT_T, t, C_GRAY);
            text_c(120, 130 - oy, &FONT_T, "K2: GUIDED/COLD  K3: BARS  SEL: COUNT-IN", C_DIM);
        }
        return;
    }
    /* the result: your tempo, then where every beat fell, then how they spread */
    t[0] = 0;
    if (r->bpm_x10) {
        int a = r->tempo_x10 < 0 ? -r->tempo_x10 : r->tempo_x10;
        tx10(t, r->bpm_x10, 0);
        tcat(t, " BPM");
        cv_text(8, 2 - oy, &FONT_M, t, a <= 10 ? C_GOOD : a <= 20 ? C_OK : C_BAD);
        t[0] = 0;
        tx10(t, r->tempo_x10, 1);
        tcat(t, "%");
        cv_text(232 - text_w(&FONT_M, t), 2 - oy, &FONT_M, t, a <= 10 ? C_GOOD : a <= 20 ? C_OK : C_BAD);
    } else
        cv_text(8, 2 - oy, &FONT_M, "TOO FEW TAPS", C_BAD);
    t[0] = 0;
    tcat(t, "GOOD ");
    tint(t, r->good);
    tcat(t, "/");
    tint(t, r->n);
    tcat(t, "  JITTER ");
    tint(t, r->jitter);
    tcat(t, "MS  TREND ");
    tx10(t, r->trend_x10, 1);
    tcat(t, "%");
    text_c(120, 30 - oy, &FONT_T, t, C_HI);
    t[0] = 0;
    if (!pl.cold) {
        tcat(t, "MEAN ");
        tms(t, r->mean);
        tcat(t, "  END ");
        tms(t, r->end_ms);
    }
    if (r->missed) tcat(t, "  MISS "), tint(t, r->missed);
    if (r->extra) tcat(t, "  EXTRA "), tint(t, r->extra);
    if (pl.new_best) tcat(t, "  NEW BEST");
    text_c(120, 45 - oy, &FONT_T, t, pl.new_best ? C_GOOD : C_GRAY);
    /* each beat's error over time: +-150 ms over 24 px either side of the line */
    {
        int32_t y0 = 86, x0 = 10, w = 220, tg = r->tol_good;
        cv_rect(x0, y0 - oy, w, 1, C_LINE);
        cv_rect(x0, y0 - tg * 24 / 150 - oy, w, 2 * tg * 24 / 150 + 1, C_LINE);
        cv_text(x0, 60 - oy, &FONT_T, "LATE", C_DIM);
        cv_text(x0, 100 - oy, &FONT_T, "EARLY", C_DIM);
        for (i = 0; i < r->n; i++) {
            int32_t x = x0 + (r->n > 1 ? i * (w - 3) / (r->n - 1) : w / 2), y;
            if (r->tap_of[i] < 0) {
                cv_rect(x, y0 - 1 - oy, 3, 3, C_BAD);
                continue;
            }
            y = y0 - clamp(r->err[i], -150, 150) * 24 / 150;
            cv_rect(x, y - 1 - oy, 3, 3, grade_col(r->grade[i]));
        }
    }
    /* the spread: one bar a GOOD window wide */
    {
        int32_t mx = 1, bw = 18, x0 = 120 - RH_HIST * bw / 2, yb = 164;
        for (i = 0; i < RH_HIST; i++) mx = r->hist[i] > mx ? r->hist[i] : mx;
        for (i = 0; i < RH_HIST; i++) {
            int32_t h = r->hist[i] * 44 / mx, d = i - RH_HIST / 2;
            uint16_t c = !d ? C_GOOD : d == 1 || d == -1 ? C_OK : C_BAD;
            if (h) cv_rect(x0 + i * bw + 2, yb - h - oy, bw - 4, h, c);
            cv_rect(x0 + i * bw + 2, yb - oy, bw - 4, 1, C_LINE);
        }
        cv_text(x0 - 2, yb + 2 - oy, &FONT_T, "EARLY", C_DIM);
        cv_text(x0 + RH_HIST * bw - text_w(&FONT_T, "LATE") + 2, yb + 2 - oy, &FONT_T, "LATE", C_DIM);
    }
}

static float lg2(float x)                /* log2, near enough to place a dot (no libm) */
{
    float e = 0.0f;
    if (x <= 0.0f) return -10.0f;
    while (x >= 2.0f) x *= 0.5f, e += 1.0f;
    while (x < 1.0f) x *= 2.0f, e -= 1.0f;
    return e + (x - 1.0f) * (1.3466f - 0.3466f * x);   /* (within 0.01 on 1..2) */
}

static void tval(char *t, float v, int type)   /* "2.4%", "18MS" */
{
    int x = (int)(v * 10.0f + 0.5f);
    if (type == RH_J_FEEL && x >= 100) tint(t, (x + 5) / 10);
    else tx10(t, x, 0);
    tcat(t, rh_judge_unit(type));
}

static void draw_judge(int32_t oy)
{
    char t[48];
    const char *lo = jd.type == RH_J_FEEL || (jd.st == JS_IDLE && rec.jtype == RH_J_FEEL) ? "DRAGGING" : "SLOWER",
               *hi = lo[0] == 'D' ? "RUSHING" : "FASTER", *big;
    int type = jd.st == JS_IDLE ? rec.jtype : jd.type, i;
    uint16_t bc = C_HI;
    switch (jd.st) {
    case JS_PLAY: big = "LISTEN"; break;
    case JS_ASK: big = lo[0] == 'D' ? "RUSHING OR DRAGGING?" : "FASTER OR SLOWER?", bc = C_WHITE; break;
    case JS_FEED: big = jd.ok ? "RIGHT" : "WRONG", bc = jd.ok ? C_GOOD : C_BAD; break;
    case JS_DONE: big = jd.new_best ? "NEW BEST" : "DONE", bc = jd.new_best ? C_GOOD : C_HI; break;
    default: big = rh_judge_name(type); break;
    }
    text_c(120, 4 - oy, &FONT_M, big, bc);
    t[0] = 0;
    if (jd.st == JS_FEED) {
        tcat(t, "IT WAS ");
        tcat(t, jd.sign > 0 ? hi : lo);
        tcat(t, ", BY ");
        tval(t, jd.s.hist_x10[jd.s.trials - 1] / 10.0f, jd.type);
    } else if (jd.st == JS_DONE) {
        tcat(t, "YOU HEAR ");
        tval(t, jd.thr, jd.type);
        tcat(t, " (71% RIGHT)");
    } else if (jd.st == JS_IDLE)
        tcat(t, type == RH_J_TEMPO ? "TWO BARS, THEN TWO MORE: FASTER?"
                : type == RH_J_FEEL ? "THE PLAYER AGAINST THE CLICK"
                                    : "DOES IT SPEED UP OR SLOW DOWN?");
    else {
        tcat(t, "TRIAL ");
        tint(t, jd.s.trials + 1);
        tcat(t, "  STEP ");
        tval(t, jd.s.d, jd.type);
        tcat(t, "  TURNS ");
        tint(t, jd.s.revs);
        tcat(t, "/");
        tint(t, RH_ST_REVS);
    }
    text_c(120, 34 - oy, &FONT_T, t, jd.st == JS_DONE ? C_WHITE : C_GRAY);
    /* the staircase so far: each trial's step (log scale), green right, red wrong */
    if (jd.st != JS_IDLE && jd.s.trials) {
        float top = lg2(jd.type == RH_J_FEEL ? 150.0f : 40.0f), bot = lg2(rh_judge_min(jd.type));
        int32_t x0 = 16, w = 208, y0 = 56, h = 72;
        cv_rect(x0, y0 + h - oy, w, 1, C_LINE);
        cv_rect(x0, y0 - oy, 1, h, C_LINE);
        if (jd.st == JS_DONE) {
            int32_t y = y0 + (int32_t)((top - lg2(jd.thr)) / (top - bot) * (float)h);
            cv_rect(x0, y - oy, w, 1, C_AMB);
        }
        for (i = 0; i < jd.s.trials; i++) {
            int32_t x = x0 + 2 + i * (w - 6) / (RH_ST_TRIALS - 1),
                    y = y0 + (int32_t)((top - lg2((float)jd.s.hist_x10[i] / 10.0f)) / (top - bot) * (float)h);
            cv_rect(x, clamp(y, y0, y0 + h) - 1 - oy, 4, 3, jd.s.right[i] ? C_GOOD : C_BAD);
        }
    } else if (jd.st == JS_IDLE) {
        t[0] = 0;
        tcat(t, "BEST  TEMPO ");
        if (rec.jbest[0]) tval(t, rec.jbest[0] / 10.0f, RH_J_TEMPO);
        else tcat(t, "-");
        tcat(t, "  FEEL ");
        if (rec.jbest[1]) tval(t, rec.jbest[1] / 10.0f, RH_J_FEEL);
        else tcat(t, "-");
        tcat(t, "  DRIFT ");
        if (rec.jbest[2]) tval(t, rec.jbest[2] / 10.0f, RH_J_DRIFT);
        else tcat(t, "-");
        text_c(120, 64 - oy, &FONT_T, t, C_GRAY);
        text_c(120, 84 - oy, &FONT_T, rec.jrand ? "TEMPO VARIES A LITTLE (SEL)" : "TEMPO FIXED AT THE BPM (SEL)", C_DIM);
        text_c(120, 104 - oy, &FONT_T, "K2: TEMPO / FEEL / DRIFT   PLAY: START", C_DIM);
    }
    /* the two answers, where they are on the keyboard */
    {
        int on = jd.st == JS_PLAY || jd.st == JS_ASK;
        uint16_t cl = on ? C_HI : C_DIM;
        if (jd.st == JS_FEED) cl = jd.said < 0 ? (jd.ok ? C_GOOD : C_BAD) : C_DIM;
        cv_rect(8, 140 - oy, 108, 30, C_LINE);
        text_c(62, 142 - oy, &FONT_B, lo, cl);
        text_c(62, 157 - oy, &FONT_T, "LOW KEYS", C_DIM);
        cl = on ? C_HI : C_DIM;
        if (jd.st == JS_FEED) cl = jd.said > 0 ? (jd.ok ? C_GOOD : C_BAD) : C_DIM;
        cv_rect(124, 140 - oy, 108, 30, C_LINE);
        text_c(178, 142 - oy, &FONT_B, hi, cl);
        text_c(178, 157 - oy, &FONT_T, "HIGH KEYS", C_DIM);
    }
}

static void qz_opt_text(int i, char *t)  /* choice i as shown */
{
    const rh_quiz_t *q = &qz.q;
    t[0] = 0;
    switch (q->type) {
    case RH_Q_TERM_BPM: tint(t, q->val[i]); tcat(t, " BPM"); break;
    case RH_Q_CHANGE: tcat(t, q->dir ? RH_CHANGES[q->opt[i]].name : RH_CHANGES[q->opt[i]].meaning); break;
    case RH_Q_METER: tcat(t, RH_METERS[q->opt[i]].name); break;
    default: tcat(t, RH_TERMS[q->opt[i]].name); break;
    }
}

static void draw_quiz(int32_t oy)
{
    char t[48];
    const rh_quiz_t *q = &qz.q;
    int i;
    if (qz.st == QS_IDLE) {
        text_c(120, 6 - oy, &FONT_M, rh_quiz_name(rec.qtype), C_HI);
        t[0] = 0;
        tcat(t, "LEVEL ");
        tint(t, rec.qlevel);
        tcat(t, "  ");
        tint(t, rh_quiz_items(rec.qtype, rec.qlevel));
        tcat(t, rec.qtype == RH_Q_METER ? " METERS" : " TERMS");
        tcat(t, "   BEST STREAK ");
        tint(t, rec.qbest);
        text_c(120, 40 - oy, &FONT_T, t, C_GRAY);
        text_c(120, 64 - oy,
               &FONT_T,
               rec.qtype == RH_Q_TERM_BPM   ? "A TEMPO MARKING: WHICH BPM FITS IT?"
               : rec.qtype == RH_Q_BPM_TERM ? "A BPM: WHICH MARKING HOLDS IT?"
               : rec.qtype == RH_Q_HEAR     ? "HEAR A TEMPO: WHICH MARKING?"
               : rec.qtype == RH_Q_CHANGE   ? "ACCEL., RIT., A TEMPO: WHAT IT MEANS"
                                            : "HEAR A FEW BARS: WHICH METER?",
               C_HI);
        text_c(120, 84 - oy, &FONT_T, "BPM: EN.WIKIPEDIA, BASIC TEMPO MARKINGS", C_DIM);
        text_c(120, 112 - oy, &FONT_T, "K2: KIND  K3: LEVEL  PLAY: START", C_DIM);
        text_c(120, 128 - oy, &FONT_T, "ANSWER ON FOUR KEY ZONES, A LOW .. D HIGH", C_DIM);
        return;
    }
    /* the question */
    t[0] = 0;
    switch (q->type) {
    case RH_Q_TERM_BPM: tcat(t, RH_TERMS[q->ask].name); break;
    case RH_Q_BPM_TERM: tint(t, q->bpm); tcat(t, " BPM"); break;
    case RH_Q_HEAR: tcat(t, qz.st == QS_FEED ? "" : "WHICH MARKING?"); if (qz.st == QS_FEED) tint(t, q->bpm), tcat(t, " BPM"); break;
    case RH_Q_CHANGE: tcat(t, q->dir ? RH_CHANGES[q->ask].meaning : RH_CHANGES[q->ask].name); break;
    default: tcat(t, "WHICH METER?"); break;
    }
    text_c(120, 2 - oy, &FONT_M, t, C_WHITE);
    t[0] = 0;
    if (q->type == RH_Q_TERM_BPM && qz.st == QS_FEED) tcat(t, "IS "), rh_term_range(q->ask, t + 3), tcat(t, " BPM");
    else if (q->type == RH_Q_HEAR || q->type == RH_Q_METER) tcat(t, "REC: HEAR IT AGAIN");
    else if (q->type == RH_Q_CHANGE) tcat(t, q->dir ? "WHICH TERM?" : "MEANS?");
    else tcat(t, q->type == RH_Q_TERM_BPM ? "WHICH BPM?" : "WHICH MARKING?");
    text_c(120, 30 - oy, &FONT_T, t, C_GRAY);
    /* the choices, A at the top = the lowest keys */
    for (i = 0; i < 4; i++) {
        int32_t y = 48 + i * 26;
        uint16_t c = C_HI, box = C_LINE;
        char lab[2] = { (char)('A' + i), 0 };
        if (qz.st == QS_FEED) {
            if (i == q->answer) c = C_GOOD, box = C_GOOD;
            else if (i == qz.chosen) c = C_BAD, box = C_BAD;
            else c = C_DIM;
        }
        cv_rect(8, y - oy, 20, 22, box);
        text_c(18, y + 3 - oy, &FONT_B, lab, qz.st == QS_FEED && (i == q->answer || i == qz.chosen) ? C_BLACK : C_WHITE);
        qz_opt_text(i, t);
        cv_text(36, y + 3 - oy, &FONT_B, t, c);
        if (qz.st == QS_FEED && (q->type == RH_Q_BPM_TERM || q->type == RH_Q_HEAR)) {   /* every term's range */
            char r[12];
            rh_term_range(q->opt[i], r);
            cv_text(232 - text_w(&FONT_T, r), y + 6 - oy, &FONT_T, r, c == C_DIM ? C_DIM : C_GRAY);
        }
    }
    t[0] = 0;
    tint(t, qz.right);
    tcat(t, "/");
    tint(t, qz.asked);
    tcat(t, "  STREAK ");
    tint(t, qz.streak);
    tcat(t, "  BEST ");
    tint(t, rec.qbest);
    text_c(120, 158 - oy, &FONT_T, t, qz.new_best && qz.streak == rec.qbest ? C_GOOD : C_GRAY);
}

static void draw_main(void)
{
    int32_t h;
    for (h = 0; h < 2; h++) {
        cv_begin(240, MAIN_H / 2, C_BLACK);
        if (rec.page == PG_METRO) draw_metro(h * MAIN_H / 2);
        else if (rec.page == PG_ENDLESS) draw_endless(h * MAIN_H / 2);
        else if (rec.page == PG_PULSE) draw_pulse(h * MAIN_H / 2);
        else if (rec.page == PG_JUDGE) draw_judge(h * MAIN_H / 2);
        else if (rec.page == PG_QUIZ) draw_quiz(h * MAIN_H / 2);
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
    } else if (rec.page == PG_PULSE && pl.st == PS_RESULT) {
        uint16_t col = pl.res.hint == RH_P_GOOD ? C_GOOD : pl.res.hint == RH_P_CLOSE ? C_OK : C_BAD;
        int32_t x = cv_text(8, 7, &FONT_B, rh_pulse_hint(pl.res.hint), col);
        const char *m = rh_pulse_more(pl.res.hint);
        if (x + 8 + text_w(&FONT_T, m) <= 232) cv_text(232 - text_w(&FONT_T, m), 10, &FONT_T, m, C_GRAY);
    } else
        cv_text(8, 10, &FONT_T,
                rec.page == PG_METRO     ? "HOME: ECHO   PLAY: START/STOP"
                : rec.page == PG_ECHO    ? "HOME: ENDLESS   PLAY: NEW   REC: AGAIN"
                : rec.page == PG_ENDLESS ? "HOME: PULSE   PLAY: START/STOP"
                : rec.page == PG_PULSE   ? "HOME: JUDGE   PLAY: START/STOP"
                : rec.page == PG_JUDGE   ? "HOME: QUIZ   PLAY: START   REC: AGAIN"
                                         : "HOME: METRO   PLAY: START   REC: AGAIN",
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
    {
        int n = plat_store_load(OBJ_RHYTHM, &rec, sizeof rec);
        if (n == (int)REC_V1_SIZE && rec.magic == REC_MAGIC && rec.ver == 1u && rec.size == REC_V1_SIZE) {
            defaults_v2();               /* 0.1 / 0.2's settings: kept, 0.3's fields from their defaults */
            rec.ver = REC_VER;
            rec.size = (uint16_t)sizeof rec;
            n = (int)sizeof rec;
        }
        if (n != (int)sizeof rec || !rec_valid(&rec)) {
            zero(&rec, sizeof rec);
            defaults();
            defaults_v2();
        }
    }
    zero(&pl, sizeof pl);
    zero(&jd, sizeof jd);
    zero(&qz, sizeof qz);
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
    if (rec.page == PG_PULSE) pl_follow();
    if (rec.page == PG_JUDGE) jd_follow();
    if (rec.page == PG_QUIZ) qz_follow();
    if (rec.page == PG_ENDLESS) {
        int part = rh_en.on ? en_part() : -1;
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
    else if (!strcmp(what, "en_on")) *got = rh_en.on;
    else if (!strcmp(what, "en_over")) *got = rh_en.over;
    else if (!strcmp(what, "en_rounds")) *got = rh_en.rounds;
    else if (!strcmp(what, "en_stage")) *got = rh_en.stage;
    else if (!strcmp(what, "en_bpm")) *got = rh_en.bpm;
    else if (!strcmp(what, "en_level")) *got = rh_en.level;
    else if (!strcmp(what, "health")) *got = rh_en.health;
    else if (!strcmp(what, "en_good")) *got = rh_en.last.good;
    else if (!strcmp(what, "en_notes")) *got = rh_en.last.n;
    else if (!strcmp(what, "lit_keys")) *got = (int)ui.lit;
    else if (!strcmp(what, "saves")) *got = (int)saves;
    else if (!strcmp(what, "ver")) *got = rec.ver;
    else if (!strcmp(what, "seq_n")) *got = tr.mode == TR_SEQ && tr.run ? tr.sn : 0;
    else if (!strcmp(what, "pmode")) *got = rec.pmode;
    else if (!strcmp(what, "pbars")) *got = rec.pbars;
    else if (!strcmp(what, "pcount")) *got = rec.pcount;
    else if (!strcmp(what, "pbest0")) *got = rec.pbest[0];
    else if (!strcmp(what, "pbest1")) *got = rec.pbest[1];
    else if (!strcmp(what, "pulse_st")) *got = pl.st;
    else if (!strcmp(what, "pulse_beats")) *got = pl.beats;
    else if (!strcmp(what, "pulse_taps")) *got = pl.ntaps;
    else if (!strcmp(what, "pulse_good")) *got = pl.res.good;
    else if (!strcmp(what, "pulse_missed")) *got = pl.res.missed;
    else if (!strcmp(what, "pulse_extra")) *got = pl.res.extra;
    else if (!strcmp(what, "pulse_hint")) *got = pl.res.hint;
    else if (!strcmp(what, "pulse_mean")) *got = pl.res.mean;
    else if (!strcmp(what, "pulse_bpm10")) *got = pl.res.bpm_x10;
    else if (!strcmp(what, "pulse_tempo10")) *got = pl.res.tempo_x10;
    else if (!strcmp(what, "pulse_trend10")) *got = pl.res.trend_x10;
    else if (!strcmp(what, "pulse_jitter")) *got = pl.res.jitter;
    else if (!strcmp(what, "jtype")) *got = rec.jtype;
    else if (!strcmp(what, "jrand")) *got = rec.jrand;
    else if (!strcmp(what, "jbest0")) *got = rec.jbest[0];
    else if (!strcmp(what, "jbest1")) *got = rec.jbest[1];
    else if (!strcmp(what, "jbest2")) *got = rec.jbest[2];
    else if (!strcmp(what, "judge_st")) *got = jd.st;
    else if (!strcmp(what, "judge_trials")) *got = jd.s.trials;
    else if (!strcmp(what, "judge_revs")) *got = jd.s.revs;
    else if (!strcmp(what, "judge_d10")) *got = (int)(jd.s.d * 10.0f + 0.5f);
    else if (!strcmp(what, "judge_thr10")) *got = (int)(jd.thr * 10.0f + 0.5f);
    else if (!strcmp(what, "judge_ok")) *got = jd.ok;
    else if (!strcmp(what, "qtype")) *got = rec.qtype;
    else if (!strcmp(what, "qlevel")) *got = rec.qlevel;
    else if (!strcmp(what, "qbest")) *got = rec.qbest;
    else if (!strcmp(what, "quiz_st")) *got = qz.st;
    else if (!strcmp(what, "quiz_asked")) *got = qz.asked;
    else if (!strcmp(what, "quiz_right")) *got = qz.right;
    else if (!strcmp(what, "quiz_streak")) *got = qz.streak;
    else if (!strcmp(what, "quiz_type")) *got = qz.q.type;
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
            for (i = 0; i < rh_en.pat[k].n && host_ntap < RH_MAX_TAPS; i++)
                if ((int)k != skip)
                    host_tap[host_ntap++] = tick_at(en_resp(k))
                                            + (uint32_t)(((int64_t)rh_onset_ms(&rh_en.pat[k], (int)i, rh_en.bpm) + off) * SR / 1000);
        host_itap = 0;
        return 1;
    }
    if (!strcmp(cmd, "pulsetap")) {      /* pulsetap OFFSET [DRIFT_X10 [SKIP]]: PULSE's beats, each DRIFT/10 ms more off */
        int off = atoi(a), dr = b[0] ? atoi(b) : 0, skip = c[0] ? atoi(c) : -1;
        uint32_t k, base = pl.st == PS_WAIT ? now_smp() + SR * 3u / 10u : pl.anchor;   /* COLD: 300 ms from now */
        host_ntap = 0;
        for (k = 0; k < pl.beats && host_ntap < RH_MAX_TAPS; k++)
            if ((int)k != skip)
                host_tap[host_ntap++] = base + q8_mul(k, beat_smp_q8())
                                        + (uint32_t)(((int64_t)off * 10 + (int64_t)dr * (int64_t)k) * SR / 10000);
        host_itap = 0;
        return 1;
    }
    if (!strcmp(cmd, "judgeans")) {      /* judgeans right|wrong: JUDGE's answer, as the keys give it */
        jd_answer(!strcmp(a, "right") ? jd.sign : -jd.sign);
        return 1;
    }
    if (!strcmp(cmd, "judgeseq")) {      /* judgeseq RRW...: a run answered at once (R right, W wrong) */
        for (; *a; a++) {
            if (jd.st == JS_DONE || jd.s.done) break;
            if (jd.st == JS_FEED) jd_trial();
            jd_answer(*a == 'R' ? jd.sign : -jd.sign);
        }
        return 1;
    }
    if (!strcmp(cmd, "quizans")) {       /* quizans right|wrong */
        qz_answer(!strcmp(a, "right") ? qz.q.answer : (qz.q.answer + 1) % 4);
        return 1;
    }
    if (!strcmp(cmd, "v1record")) {      /* v1record BPM: 0.2's settings record (ECHO page), as an old unit has it */
        rec_t o = rec;
        o.magic = REC_MAGIC;
        o.ver = 1u;
        o.size = (uint16_t)REC_V1_SIZE;
        o.page = PG_ECHO;
        o.bpm = (uint8_t)atoi(a);
        o.best = 9;
        plat_store_save(OBJ_RHYTHM, &o, REC_V1_SIZE);
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
