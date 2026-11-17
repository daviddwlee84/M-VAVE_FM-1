/* SPDX-License-Identifier: MIT */
/* Voice Lab: see app.h. The audio interrupt decodes the word as it plays (LPC at 8 kHz, ADPCM at 11.025 kHz) and
 * steps it up to the FM-1's 44.1 kHz by linear interpolation; nothing is buffered beyond 64 samples. Part of the
 * unity build, after the platform: gfx.c and announce.c are in scope. */
#include "fastmath.h"
#include "lpc.c"
#include "adpcm.c"

typedef struct {
    const char *text;
    const uint8_t *lpc;              /* python_wizard's LPC stream, 25 ms frames */
    uint32_t lpc_len;
    const uint8_t *adp;              /* IMA ADPCM, 11025 Hz */
    uint32_t adp_len, adp_samples;
} vl_word_t;

#if __has_include("gen/voicedata.h")
#include "gen/voicedata.h"           /* host/voicelab/fwdata.py: VL_N, vl_words[] */
#else
#define VL_N 0u
static const vl_word_t vl_words[1] = {{"", 0, 0, 0, 0, 0}};
#endif

enum { ENG_LPC, ENG_ADPCM, ENG_N };
static const char *const ENG_NAME[ENG_N] = {"LPC 25 MS", "ADPCM 11 KHZ"};
static const float ENG_RATE[ENG_N] = {8000.0f, 11025.0f};

/* ------------------------------------------------------------------ voice --- */
static volatile int32_t req_word = -1;   /* main loop -> audio interrupt: say this word */
static volatile uint8_t req_eng, playing;
static volatile int32_t now_word = -1;

static uint8_t eng;                      /* the audio interrupt's own copies */
static lpc_t lpc;
static adpcm_t adp;
static int16_t src[64];
static uint32_t src_n, src_i;
static float pos, step, s0, s1, gain_now;

static int next_sample(float *s)
{
    if (src_i >= src_n) {
        src_n = eng == ENG_LPC ? lpc_render(&lpc, src, 64) : adpcm_render(&adp, src, 64);
        src_i = 0;
        if (!src_n)
            return 0;
    }
    *s = (float)src[src_i++];
    return 1;
}

void app_render(int32_t *lr, uint32_t n, uint32_t master_q12)
{
    uint32_t i;
    float gain = (float)master_q12 / 4096.0f * 180.0f;   /* int16 x 256 is the render's full scale; a little under */
    int32_t w = req_word;
    if (w >= 0 && (uint32_t)w < VL_N) {
        const vl_word_t *v = &vl_words[w];
        req_word = -1;
        eng = req_eng;
        if (eng == ENG_LPC)
            lpc_begin(&lpc, v->lpc, v->lpc_len, 25);
        else
            adpcm_begin(&adp, v->adp, v->adp_len, v->adp_samples);
        src_n = src_i = 0;
        pos = 0.0f;
        step = ENG_RATE[eng] / 44100.0f;
        s0 = s1 = 0.0f;
        now_word = w;
        playing = 1;
    }
    for (i = 0; i < n; i++) {
        float s = 0.0f;
        gain_now += (gain - gain_now) * 0.004f;
        if (playing) {
            pos += step;
            while (pos >= 1.0f) {
                pos -= 1.0f;
                s0 = s1;
                if (!next_sample(&s1)) {
                    playing = 0;
                    s1 = 0.0f;
                    break;
                }
            }
            s = (s0 + (s1 - s0) * pos) * gain_now;
        }
        lr[2 * i] = (int32_t)s;
        lr[2 * i + 1] = (int32_t)s;
    }
}

void app_panic(void)
{
    playing = 0;
    req_word = -1;
}

void app_init(void) {}

/* --------------------------------------------------------------------- UI --- */
static struct {
    uint8_t engine, redraw;
    uint32_t keys, btn;
    int32_t last;
    char msg[2][24];
    uint32_t msg_until;
} ui;

void app_say(const char *a, const char *b)
{
    uint32_t i;
    for (i = 0; i < 23u && a && a[i]; i++)
        ui.msg[0][i] = a[i];
    ui.msg[0][i] = 0;
    for (i = 0; i < 23u && b && b[i]; i++)
        ui.msg[1][i] = b[i];
    ui.msg[1][i] = 0;
    ui.msg_until = plat_ms() + 1200u;
    ui.redraw = 1;
    if (ann_on())
        ann_say(ui.msg[0], 0, ANN_NOW);
}

static void say_word(int32_t k)
{
    if (k < 0 || (uint32_t)k >= VL_N)
        return;
    req_eng = ui.engine;
    req_word = k;
    ui.last = k;
    ui.redraw = 1;
}

static void engine_step(int32_t d)
{
    ui.engine = (uint8_t)((ui.engine + ENG_N + d) % ENG_N);
    app_say("ENGINE", ENG_NAME[ui.engine]);
}

static void input(void)
{
    uint32_t keys = plat_keys(), btn = plat_buttons(), k;
    int32_t e;
    for (k = 0; k < NKEYS; k++)
        if ((keys >> k & 1u) && !(ui.keys >> k & 1u))
            say_word((int32_t)k);
    for (k = 0; k < NB; k++) {
        if (!(btn >> k & 1u) || (ui.btn >> k & 1u))
            continue;
        if (k == B_OCTDN)
            engine_step(-1);
        else if (k == B_OCTUP)
            engine_step(1);
        else if (k == B_PLAY)
            say_word(ui.last);
    }
    if ((e = plat_enc(EN_PRESET)) != 0)
        engine_step(e);
    ui.keys = keys;
    ui.btn = btn;
    plat_glow(1);
    plat_leds(1u << B_PLAY, playing && now_word >= 0 ? 1u << now_word : 0u);
}

void app_input_only(void) { input(); }

static void u_text(uint32_t v, char *b)     /* unsigned, no padding */
{
    char t[12];
    int n = 0;
    do
        t[n++] = (char)('0' + v % 10u);
    while ((v /= 10u) && n < 11);
    while (n)
        *b++ = t[--n];
    *b = 0;
}

static void upper(const char *s, char *d, uint32_t max)
{
    uint32_t i;
    for (i = 0; s[i] && i + 1 < max; i++)
        d[i] = s[i] >= 'a' && s[i] <= 'z' ? (char)(s[i] - 32) : s[i];
    d[i] = 0;
}

static void draw(void)
{
    static const char *const NOTE[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    char t[32];
    uint32_t i, now = plat_ms();
    cv_begin(240, 32, C_BLACK);
    if (now < ui.msg_until) {
        int32_t x = cv_text(10, 6, &FONT_B, ui.msg[0], C_WHITE);
        cv_text(x + 8, 6, &FONT_B, ui.msg[1], C_HI);
    } else {
        cv_text(10, 6, &FONT_B, APP_NAME, C_WHITE);
        cv_text(236 - text_w(&FONT_XS, ENG_NAME[ui.engine]), 10, &FONT_XS, ENG_NAME[ui.engine], C_HI);
    }
    cv_blit(0, 0);
    cv_begin(240, 60, C_BLACK);
    if (!VL_N) {
        cv_text(10, 8, &FONT_B, "NO WORDS", C_HI);
        cv_text(10, 32, &FONT_XS, "just voice-lab, then just voice-fw", C_GRAY);
    } else if (ui.last >= 0) {
        const vl_word_t *v = &vl_words[ui.last];
        const felucca_font_t *f = &APP_BIG_FONT;
        uint32_t b = ui.engine == ENG_LPC ? v->lpc_len : v->adp_len;
        upper(v->text, t, sizeof t);
        if (text_w(f, t) > 228)
            f = &FONT_B;
        cv_text(10, 4, f, t, C_WHITE);
        u_text(b, t);
        cv_text(10, 42, &FONT_XS, t, C_HI);
        cv_text(10 + text_w(&FONT_XS, t) + 4, 42, &FONT_XS, "BYTES", C_GRAY);
    } else {
        cv_text(10, 8, &FONT_B, "PRESS A KEY", C_GRAY);
    }
    cv_blit(0, 34);
    for (i = 0; i < 2u; i++) {           /* the keys and their words, two columns, in two strips */
        uint32_t r;
        cv_begin(240, 72, C_BLACK);
        for (r = 0; r < 10u; r++) {
            uint32_t k = i * 5u + (r % 5u) + (r / 5u) * 10u;
            int32_t x = (int32_t)(r / 5u) * 120 + 8, y = (int32_t)(r % 5u) * 14;
            if (k >= VL_N)
                continue;
            t[0] = 0;
            {
                uint32_t m = 53u + k, o = 0;
                const char *nn = NOTE[m % 12u];
                while (nn[o]) {
                    t[o] = nn[o];
                    o++;
                }
                t[o++] = (char)('0' + m / 12u - 1u);
                t[o] = 0;
            }
            cv_text(x, y, &FONT_XS, t, C_GRAY);
            cv_text(x + 30, y, &FONT_XS, vl_words[k].text, (int32_t)k == ui.last ? C_HI : C_WHITE);
        }
        cv_blit(0, 96 + i * 72);
    }
}

void app_ui_init(void)
{
    lcd_fill(0, 0, 240, 240, C_BLACK);
    ui.last = -1;
    ui.redraw = 1;
}

void app_frame(void)
{
    static uint8_t was_playing;
    input();
    ann_tick(plat_ms());
    if (ui.msg_until && plat_ms() >= ui.msg_until) {
        ui.msg_until = 0;
        ui.redraw = 1;
    }
    if (playing != was_playing) {
        was_playing = playing;
        ui.redraw = 1;
    }
    if (ui.redraw) {
        ui.redraw = 0;
        draw();
    }
}

#ifdef PLAT_HOST
int app_expect(const char *what, int *got)
{
    if (!strcmp(what, "playing"))
        *got = playing;
    else if (!strcmp(what, "engine"))
        *got = ui.engine;
    else if (!strcmp(what, "word"))
        *got = ui.last;
    else if (!strcmp(what, "nwords"))
        *got = (int)VL_N;
    else
        return 0;
    return 1;
}
#endif
