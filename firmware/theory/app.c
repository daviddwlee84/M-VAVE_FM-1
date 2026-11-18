/* SPDX-License-Identifier: GPL-3.0-or-later */
/* THEORY v0.1: the WHEEL page (see app.h). The theory is theory.c, the voice synth.c, the music signs gtext.c.
 * Part of fm1-base's unity build, after the platform: gfx.c and announce.c are in scope. */
#include "fastmath.h"
#include "gtext.h"
#define TH_GLYPH_SHARP GT_SHARP
#define TH_GLYPH_FLAT GT_FLAT
#define TH_GLYPH_DIM GT_DIM
#define TH_GLYPH_HDIM GT_HDIM
#include "theory.c"
#include "synth.c"
#include "gtext.c"
#include "ledfx.c"

#define FW_MAJOR 0
#define FW_MINOR 1

/* ---------------------------------------------------------------- settings --- */
#define REC_MAGIC 0x31304854u            /* "TH01" */
#define REC_VER 1u

static const uint8_t MODES[] = { TH_MAJOR, TH_MINOR, TH_HARMONIC, TH_MELODIC, TH_DORIAN, TH_PHRYGIAN, TH_LYDIAN,
                                 TH_MIXOLYDIAN, TH_LOCRIAN };
static const char *const MODE_NAME[] = { "major", "minor", "harm. minor", "mel. minor", "dorian", "phrygian",
                                         "lydian", "mixolydian", "locrian" };
#define NMODES 9
enum { KM_NOTES, KM_CHORDS };
enum { LB_ROMAN, LB_NASH, LB_N };
enum { MAP_LETTER, MAP_C, MAP_N };       /* chord keys: a white key's letter is the chord's root, or C is I */

typedef struct {
    uint32_t magic;
    uint16_t ver, size;
    int8_t tonic;                        /* on the line of fifths (theory.h) */
    uint8_t mode;                        /* MODES[] */
    uint8_t sevenths;                    /* the key's chords as sevenths */
    uint8_t labels;                      /* LB_*: how the chords are numbered */
    uint8_t mapping;                     /* MAP_* */
    uint8_t keymode;                     /* KM_* */
    int8_t oct;                          /* -2 .. +2: where the keys sound */
    uint8_t rsv[5];
} rec_t;
static rec_t rec;
static uint32_t saves;                   /* records written since boot */
static uint32_t dirty_at;                /* a setting changed at (0: saved) */

static void defaults(void)
{
    rec.tonic = 0;
    rec.mode = 0;
    rec.sevenths = 0;
    rec.labels = LB_ROMAN;
    rec.mapping = MAP_LETTER;
    rec.keymode = KM_NOTES;
    rec.oct = 0;
}

static int rec_valid(const rec_t *r)
{
    return r->magic == REC_MAGIC && r->ver == REC_VER && r->size == sizeof *r && r->tonic >= -9 && r->tonic <= 12
           && r->mode < NMODES && r->sevenths <= 1u && r->labels < LB_N && r->mapping < MAP_N && r->keymode <= 1u
           && r->oct >= -2 && r->oct <= 2;
}

static void rec_save(void)
{
    rec.magic = REC_MAGIC;
    rec.ver = REC_VER;
    rec.size = (uint16_t)sizeof rec;
    if (plat_store_save(OBJ_THEORY, &rec, sizeof rec) == 0)
        saves++;
    dirty_at = 0;
}

static th_key_t cur_key(void)
{
    th_key_t k;
    k.tonic = rec.tonic;
    k.scale = MODES[rec.mode];
    return k;
}

/* --------------------------------------------------------------------- UI --- */
#define R_HEAD 1u
#define R_MAIN 2u
#define R_FOOT 4u
#define R_ALL 7u
#define MAIN_H 176
#define KEY_MIDI(k) (53 + (int)(k))      /* key 0 is F3 */

enum { SH_NONE, SH_NOTE, SH_INTERVAL, SH_CHORD, SH_UNKNOWN };

static struct {
    uint32_t keys, btn;
    uint8_t redraw;
    uint8_t knotes[NKEYS][TH_TONES_MAX + 1];  /* the notes each held key started */
    uint8_t kn[NKEYS];
    uint8_t prev[TH_TONES_MAX], nprev;   /* CHORDS: the last voicing, for the next to move least from */
    uint8_t held_changed, best_n;        /* NOTES: the held keys changed; most notes held in this gesture */
    uint8_t sh;                          /* SH_*: what the footer shows */
    th_ident_t id;
    uint8_t notes[12], nnotes;           /* the notes behind it (MIDI) */
    char toast[2][24];
    uint32_t toast_until;
    uint32_t lit, mid, glow;             /* the keys' lights, last shown (the simulator's checks) */
    int16_t ox[12], oy[12], ix[12], iy[12];  /* label centres on the circle, outer and inner ring */
    int16_t rx[96], ry[96];              /* the ring between them */
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

static void tint(char *d, int v)
{
    char t[12];
    th_put_int(t, 0, v);
    tcat(d, t);
}

static void key_text(char *t)            /* "D major" */
{
    th_name(rec.tonic, t);
    tcat(t, " ");
    tcat(t, MODE_NAME[rec.mode]);
}

static void sig_text(char *t)            /* "2#", "3b", "0" */
{
    int8_t s[7];
    int c = th_keysig(rec.tonic, MODES[rec.mode], s);
    t[0] = 0;
    tint(t, c < 0 ? -c : c);
    if (c) tcat(t, c > 0 ? GT_SHARP : GT_FLAT);
}

static void numeral_text(th_chord_t c, char *t)
{
    th_key_t k = cur_key();
    if (rec.labels == LB_NASH) th_nashville_name(k, c, t);
    else th_roman_name(k, c, t);
}

static void toast(const char *a, const char *b)
{
    uint32_t i;
    for (i = 0; i < 23u && a[i]; i++) ui.toast[0][i] = a[i];
    ui.toast[0][i] = 0;
    for (i = 0; i < 23u && b[i]; i++) ui.toast[1][i] = b[i];
    ui.toast[1][i] = 0;
    ui.toast_until = plat_ms() + 1500u;
    ui.redraw |= R_FOOT;
}

void app_say(const char *a, const char *b)
{
    char t[64], s[64];
    toast(a ? a : "", b ? b : "");
    if (ann_on()) {                      /* and to a screen reader (announce.c, fm1-reader) */
        gt_ascii(ui.toast[0], t, sizeof t);
        if (ui.toast[1][0]) {
            tcat(t, " ");
            gt_ascii(ui.toast[1], s, sizeof s);
            tcat(t, s);
        }
        ann_say(t, 0, ANN_NOW);
    }
}

static void changed(uint8_t r)           /* a setting changed: redraw, and save once the knobs rest */
{
    ui.redraw |= r;
    dirty_at = plat_ms();
    if (!dirty_at) dirty_at = 1u;
}

/* -------------------------------------------------------------- the notes --- */
static void midi_out(int on, int note, int vel)
{
    if (note < 0 || note > 127) return;
    if (on) plat_midi_send(0x09u | 0x90u << 8 | (uint32_t)note << 16 | (uint32_t)vel << 24, TO_USB);
    else plat_midi_send(0x08u | 0x80u << 8 | (uint32_t)note << 16, TO_USB);
}

static void key_release(uint32_t k)
{
    uint32_t i;
    for (i = 0; i < ui.kn[k]; i++) {
        synth_note_off(ui.knotes[k][i]);
        midi_out(0, ui.knotes[k][i], 0);
    }
    ui.kn[k] = 0;
}

static void release_all(void)
{
    uint32_t k;
    for (k = 0; k < NKEYS; k++) key_release(k);
    synth_all_off();
    ui.nprev = 0;
}

static int white_letter(int pc)          /* C D E F G A B = 0..6; -1 a black key */
{
    static const int8_t L[12] = { 0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6 };
    return L[th_mod(pc, 12)];
}

static void show_chord(th_chord_t c, const uint8_t *notes, int n)
{
    int i;
    ui.sh = SH_CHORD;
    ui.id.c = c;
    ui.id.inv = 0;
    ui.id.no5 = 0;
    ui.id.inkey = 1;
    ui.nnotes = (uint8_t)(n > 12 ? 12 : n);
    for (i = 0; i < ui.nnotes; i++) ui.notes[i] = notes[i];
    ui.redraw |= R_MAIN | R_FOOT;
}

/* CHORDS: a white key plays the key's chord on its degree, voiced near the last one, with the root below */
static void chord_key(uint32_t k, int vel)
{
    th_key_t key = cur_key();
    th_chord_t c;
    uint8_t v[TH_TONES_MAX];
    int letter = white_letter(KEY_MIDI(k) % 12), degree, n, lo = 55 + 12 * rec.oct, hi = 76 + 12 * rec.oct, base, i;
    if (letter < 0) return;              /* black keys: nothing yet */
    degree = rec.mapping == MAP_C ? letter : th_mod(letter - th_letter(rec.tonic), 7);
    if (!th_diatonic(key, degree, rec.sevenths, &c)) return;
    n = th_voice_near(ui.prev, ui.nprev, c, lo, hi, v);
    if (!n) return;
    base = 41 + 12 * rec.oct;            /* the bass: the root between F2 and E3 */
    ui.knotes[k][0] = (uint8_t)(base + th_mod(th_pc(c.root) - base, 12));
    for (i = 0; i < n; i++) {
        ui.knotes[k][i + 1] = v[i];
        ui.prev[i] = v[i];
    }
    ui.nprev = (uint8_t)n;
    ui.kn[k] = (uint8_t)(n + 1);
    for (i = 0; i <= n; i++) {
        synth_note_on(ui.knotes[k][i], vel);
        midi_out(1, ui.knotes[k][i], vel);
    }
    show_chord(c, ui.knotes[k], n + 1);
}

/* NOTES: name what is held. While a chord is let go, the fullest one held stays on the screen */
static void identify_held(void)
{
    th_key_t key = cur_key();
    uint8_t m[NKEYS];
    int n = 0, i;
    uint32_t k;
    for (k = 0; k < NKEYS; k++)
        if (ui.keys >> k & 1u) m[n++] = (uint8_t)KEY_MIDI(k);
    if (n == 0 || n < ui.best_n) return;
    ui.best_n = (uint8_t)n;
    ui.nnotes = (uint8_t)(n > 12 ? 12 : n);
    for (i = 0; i < ui.nnotes; i++) ui.notes[i] = m[i];
    if (n == 1) ui.sh = SH_NOTE;
    else if (th_identify(m, n, key, &ui.id)) ui.sh = SH_CHORD;
    else ui.sh = n == 2 ? SH_INTERVAL : SH_UNKNOWN;
    ui.redraw |= R_MAIN | R_FOOT;
}

/* ----------------------------------------------------------------- input --- */
static void set_tonic(int f)
{
    f = th_key_for_pc(th_pc(f), MODES[rec.mode]);
    if (f == rec.tonic) return;
    rec.tonic = (int8_t)f;
    ui.nprev = 0;
    changed(R_ALL);
}

static void input(void)
{
    uint32_t keys, btn = plat_buttons(), k;
    int32_t e;
    char t[32];
    touch_poll();
    keys = touch_keys();
    for (k = 0; k < NKEYS; k++) {
        uint32_t bit = 1u << k;
        int vel;
        if ((keys & bit) && !(ui.keys & bit)) {
            if (btn >> B_SEL & 1u) {     /* SEL held: this note becomes the tonic */
                set_tonic(th_spell_pc(KEY_MIDI(k) % 12, 0));
                key_text(t);
                app_say("KEY", t);
                continue;
            }
            vel = touch_vel(k);
            if (!ui.keys) ui.best_n = 0; /* a new gesture */
            if (rec.keymode == KM_CHORDS) chord_key(k, vel);
            else {
                int note = KEY_MIDI(k) + 12 * rec.oct;
                ui.knotes[k][0] = (uint8_t)note;
                ui.kn[k] = 1;
                synth_note_on(note, vel);
                midi_out(1, note, vel);
                ui.held_changed = 1;
            }
        } else if (!(keys & bit) && (ui.keys & bit)) {
            key_release(k);
            if (rec.keymode == KM_NOTES) ui.held_changed = 1;
        }
    }
    for (k = 0; k < NB; k++) {
        uint32_t bit = 1u << k;
        if (!(btn & bit) || (ui.btn & bit)) continue;
        if (k == B_PLAY) {
            release_all();
            rec.keymode = (uint8_t)!rec.keymode;
            app_say("KEYS", rec.keymode ? "CHORDS" : "NOTES");
            changed(R_ALL);
        } else if (k == B_OCTDN || k == B_OCTUP) {
            int o = clamp(rec.oct + (k == B_OCTUP ? 1 : -1), -2, 2);
            if (o != rec.oct) {
                rec.oct = (int8_t)o;
                ui.nprev = 0;
                t[0] = 0;
                tcat(t, o < 0 ? "-" : "+");
                tint(t, o < 0 ? -o : o);
                app_say("OCTAVE", t);
                changed(R_HEAD);
            }
        }
    }
    if ((e = plat_enc(EN_SELECT)) != 0) {    /* around the circle */
        set_tonic(rec.tonic + e);
        key_text(t);
        app_say("KEY", t);
    }
    if ((e = plat_enc(EN_K1)) != 0) {        /* the mode, the tonic kept (respelled if it must be) */
        rec.mode = (uint8_t)th_mod(rec.mode + e, NMODES);
        rec.tonic = (int8_t)th_key_for_pc(th_pc(rec.tonic), MODES[rec.mode]);
        ui.nprev = 0;
        app_say("MODE", MODE_NAME[rec.mode]);
        changed(R_ALL);
    }
    if ((e = plat_enc(EN_K2)) != 0 && rec.sevenths != (e > 0)) {
        rec.sevenths = (uint8_t)(e > 0);
        app_say("CHORDS", rec.sevenths ? "SEVENTHS" : "TRIADS");
        changed(R_MAIN | R_FOOT);
    }
    if ((e = plat_enc(EN_K3)) != 0) {
        rec.labels = (uint8_t)th_mod(rec.labels + e, LB_N);
        app_say("NUMBERS", rec.labels == LB_NASH ? "NASHVILLE" : "ROMAN");
        changed(R_MAIN | R_FOOT);
    }
    if ((e = plat_enc(EN_K4)) != 0) {
        rec.mapping = (uint8_t)th_mod(rec.mapping + e, MAP_N);
        app_say("CHORD KEYS", rec.mapping == MAP_C ? "C IS I" : "LETTERS");
        changed(R_FOOT);
    }
    (void)plat_enc(EN_ALGO);             /* not used yet: their turns thrown away */
    (void)plat_enc(EN_PRESET);
    ui.keys = keys;
    ui.btn = btn;
}

void app_input_only(void) { input(); }

/* ---------------------------------------------------------------- lights --- */
static void lights(uint32_t now)
{
    ledfx_t kb = { 0, 0, 0 }, bb = { 0, 0, 0 };
    th_key_t key = cur_key();
    int mask = th_key_mask(key), tpc = th_pc(rec.tonic), k;
    for (k = 0; k < NB; k++)
        if (k != B_PLAY) ledfx_set(&bb, (uint32_t)k, FX_GLOW, now);
    if (rec.oct < 0) ledfx_set(&bb, B_OCTDN, FX_LIT, now);
    if (rec.oct > 0) ledfx_set(&bb, B_OCTUP, FX_LIT, now);
    for (k = 0; k < NKEYS; k++) {
        int pc = KEY_MIDI(k) % 12, fx = FX_OFF;
        if (rec.keymode == KM_CHORDS) {
            int letter = white_letter(pc);
            if (letter >= 0) {
                int degree = rec.mapping == MAP_C ? letter : th_mod(letter - th_letter(rec.tonic), 7);
                fx = degree == 0 ? FX_MID : FX_GLOW;
            }
        } else if (mask >> pc & 1)
            fx = pc == tpc ? FX_MID : FX_GLOW;
        if (ui.keys >> k & 1u) fx = FX_LIT;
        ledfx_set(&kb, (uint32_t)k, fx, now);
    }
    if (rec.keymode == KM_CHORDS) bb.lit |= LED_PLAY_GREEN;
    ui.lit = kb.lit;
    ui.mid = kb.mid;
    ui.glow = kb.dim;
    ledfx_show(&bb, &kb);
}

/* ---------------------------------------------------------------- screen --- */
#define WX 86                            /* the circle's centre in the main area */
#define WY 88
#define R_OUT 68
#define R_IN 42
#define R_RING 55

static void layout(void)
{
    int i;
    for (i = 0; i < 12; i++) {
        float a = (float)i * (6.2831853f / 12.0f), s = fm_sinf(a), c = fm_cosf(a);
        ui.ox[i] = (int16_t)(WX + (int)((float)R_OUT * s + (s < 0.0f ? -0.5f : 0.5f)));
        ui.oy[i] = (int16_t)(WY - (int)((float)R_OUT * c + (c < 0.0f ? -0.5f : 0.5f)));
        ui.ix[i] = (int16_t)(WX + (int)((float)R_IN * s + (s < 0.0f ? -0.5f : 0.5f)));
        ui.iy[i] = (int16_t)(WY - (int)((float)R_IN * c + (c < 0.0f ? -0.5f : 0.5f)));
    }
    for (i = 0; i < 96; i++) {
        float a = (float)i * (6.2831853f / 96.0f);
        ui.rx[i] = (int16_t)(WX + (int)((float)R_RING * fm_sinf(a)));
        ui.ry[i] = (int16_t)(WY - (int)((float)R_RING * fm_cosf(a)));
    }
}

static void ring(int32_t x, int32_t y, int32_t r, int32_t oy, uint16_t c)  /* a thin circle around a label */
{
    int i;
    for (i = 0; i < 48; i++) {
        float a = (float)i * (6.2831853f / 48.0f);
        cv_pset(x + (int32_t)((float)r * fm_sinf(a)), y - oy - (int32_t)((float)r * fm_cosf(a)), c);
    }
}

static int family(int q)                 /* 1 major, 2 minor (where a chord sits on the circle), 0 neither */
{
    if (q == TH_MAJ || q == TH_7 || q == TH_MAJ7 || q == TH_6 || q == TH_ADD9 || q == TH_9 || q == TH_MAJ9) return 1;
    if (q == TH_MIN || q == TH_M7 || q == TH_M6 || q == TH_M9 || q == TH_MADD9 || q == TH_MMAJ7) return 2;
    return 0;
}

static void label(int32_t x, int32_t y, int32_t oy, const felucca_font_t *f, const char *s, int mark, int inner)
{
    int32_t w = gtext_w(f, s), tx = x - w / 2, ty = y - oy - (int32_t)f->h / 2;
    uint16_t c = mark == 2 ? C_BLACK : mark == 1 ? C_HI : inner ? C_DIM : C_GRAY;
    if (mark == 2) cv_rect(tx - 3, ty, w + 6, (int32_t)f->h, C_HI);
    gtext(tx, ty, f, s, c);
}

static void draw_main_at(int32_t oy)
{
    th_key_t key = cur_key();
    uint8_t out[12], in[12];
    int8_t sig[7];
    char t[TH_ROMAN_MAX], u[TH_CHORD_MAX];
    int i, d, c, n;
    zero(out, sizeof out);
    zero(in, sizeof in);
    for (d = 0; d < 7; d++) {            /* where the key's own major and minor triads sit */
        th_chord_t ch;
        int fam;
        if (!th_diatonic(key, d, 0, &ch)) continue;
        fam = family(ch.q);
        if (fam) {
            uint8_t *m = fam == 1 ? &out[th_circle_pos(ch.root)] : &in[th_circle_pos(ch.root - 3)];
            if (d == 0) *m = 2;          /* the tonic's chord */
            else if (!*m) *m = 1;
        }
    }
    for (i = 0; i < 96; i++) cv_pset(ui.rx[i], ui.ry[i] - oy, C_LINE);
    for (i = 0; i < 12; i++) {
        th_name(th_circle_tonic(i, 0), t);
        label(ui.ox[i], ui.oy[i], oy, &FONT_B, t, out[i], 0);
        th_name(th_circle_tonic(i, 1), t);
        t[0] = (char)(t[0] + 32);        /* minor keys in lower case */
        label(ui.ix[i], ui.iy[i], oy, &FONT_XS, t, in[i], 1);
    }
    if (ui.sh == SH_CHORD) {             /* the chord just played, where it sits */
        int fam = family(ui.id.c.q);
        if (fam == 1) ring(ui.ox[th_circle_pos(ui.id.c.root)], ui.oy[th_circle_pos(ui.id.c.root)], 14, oy, C_WHITE);
        if (fam == 2) ring(ui.ix[th_circle_pos(ui.id.c.root - 3)], ui.iy[th_circle_pos(ui.id.c.root - 3)], 12, oy, C_WHITE);
    }
    /* the centre: the key signature, its count and its letters */
    c = th_keysig(rec.tonic, MODES[rec.mode], sig);
    sig_text(t);
    gtext(WX - gtext_w(&FONT_M, t) / 2, WY - 22 - oy, &FONT_M, t, C_WHITE);
    n = c < 0 ? -c : c;
    if (n > 7) n = 7;
    for (i = 0; i < 2; i++) {
        int j, a = i ? 4 : 0, b = i ? n : (n < 4 ? n : 4);
        t[0] = 0;
        for (j = a; j < b; j++) {
            char one[2];
            one[0] = "CDEFGAB"[th_letter(sig[j])];
            one[1] = 0;
            if (j > a) tcat(t, " ");
            tcat(t, one);
        }
        if (t[0]) cv_text(WX - text_w(&FONT_T, t) / 2, WY + 6 + 12 * i - oy, &FONT_T, t, C_GRAY);
    }
    /* the key's chords, down the right */
    for (d = 0; d < 7; d++) {
        th_chord_t ch;
        int32_t y = 3 + 25 * d - oy;
        int played;
        if (!th_diatonic(key, d, rec.sevenths, &ch)) continue;
        played = ui.sh == SH_CHORD && ui.id.c.root == ch.root && ui.id.c.q == ch.q;
        numeral_text(ch, t);
        th_chord_name(ch, u);
        if (played) cv_rect(176, y, 64, 25, C_LINE);
        gtext(180, y, &FONT_T, t, played ? C_WHITE : C_GRAY);
        gtext(180, y + 10, &FONT_XS, u, played ? C_WHITE : C_HI);
    }
}

static void draw_main(void)
{
    cv_begin(240, MAIN_H / 2, C_BLACK);
    draw_main_at(0);
    cv_blit(0, 32);
    cv_begin(240, MAIN_H / 2, C_BLACK);
    draw_main_at(MAIN_H / 2);
    cv_blit(0, 32 + MAIN_H / 2);
}

static void draw_header(void)
{
    char t[40];
    int32_t x;
    cv_begin(240, 32, C_BLACK);
    key_text(t);
    gtext(8, 7, &FONT_B, t, C_WHITE);
    x = 232;
    t[0] = 0;
    tcat(t, rec.keymode == KM_CHORDS ? "CHORDS" : "NOTES");
    x -= text_w(&FONT_T, t);
    cv_text(x, 10, &FONT_T, t, rec.keymode == KM_CHORDS ? C_HI : C_GRAY);
    if (rec.oct) {
        t[0] = 0;
        tcat(t, rec.oct < 0 ? "OCT-" : "OCT+");
        tint(t, rec.oct < 0 ? -rec.oct : rec.oct);
        x -= 8 + text_w(&FONT_T, t);
        cv_text(x, 10, &FONT_T, t, C_AMB);
    }
    cv_rect(0, 31, 240, 1, C_LINE);
    cv_blit(0, 0);
}

/* the footer's two texts: the big one (a chord's name, a note) and the small one (its number, inversion) */
static void footer_text(char *big, char *small)
{
    th_key_t key = cur_key();
    char t[TH_ROMAN_MAX];
    int i;
    big[0] = small[0] = 0;
    switch (ui.sh) {
    case SH_NOTE: {
        int f = th_spell_pc(ui.notes[0] % 12, rec.tonic);
        th_note_name(ui.notes[0], rec.tonic, big);
        th_degree_name(key, f, small);
        break;
    }
    case SH_INTERVAL: {
        int lo = ui.notes[0] < ui.notes[1] ? ui.notes[0] : ui.notes[1], hi = ui.notes[0] ^ ui.notes[1] ^ lo;
        int f0 = th_spell_pc(lo % 12, rec.tonic), f1 = th_spell_pc(hi % 12, rec.tonic);
        if (lo % 12 == hi % 12) tcat(big, "P8");
        else th_interval_name(f1 - f0, big);
        th_name(f0, t);
        tcat(small, t);
        tcat(small, " - ");
        th_name(f1, t);
        tcat(small, t);
        break;
    }
    case SH_CHORD:
        th_chord_name(ui.id.c, big);
        numeral_text(ui.id.c, small);
        if (ui.id.inv) {
            tcat(small, " INV ");
            tint(small, ui.id.inv);
        }
        if (ui.id.no5) tcat(small, " NO 5");
        break;
    case SH_UNKNOWN:
        tcat(big, "?");
        for (i = 0; i < ui.nnotes && i < 6; i++) {
            th_name(th_spell_pc(ui.notes[i] % 12, rec.tonic), t);
            if (i) tcat(small, " ");
            tcat(small, t);
        }
        break;
    default:
        break;
    }
}

static void draw_footer(void)
{
    char big[TH_CHORD_MAX], small[TH_CHORD_MAX + 16];
    cv_begin(240, 32, C_BLACK);
    cv_rect(0, 0, 240, 1, C_LINE);
    if (plat_ms() < ui.toast_until) {
        int32_t x = gtext(8, 9, &FONT_T, ui.toast[0], C_GRAY);
        gtext(x + 8, 7, &FONT_B, ui.toast[1], C_HI);
    } else {
        footer_text(big, small);
        if (big[0]) {
            gtext(8, 3, &FONT_M, big, ui.id.inkey || ui.sh != SH_CHORD ? C_WHITE : C_AMB);
            gtext(232 - gtext_w(&FONT_B, small), 8, &FONT_B, small, C_HI);
        } else
            cv_text(8, 10, &FONT_T, rec.keymode == KM_CHORDS ? "WHITE KEYS PLAY THE KEY'S CHORDS" : "PLAY A CHORD",
                    C_DIM);
    }
    cv_blit(0, 208);
}

static void draw(void)
{
    uint8_t r = ui.redraw;
    ui.redraw = 0;
    if (r & R_HEAD) draw_header();
    if (r & R_MAIN) draw_main();
    if (r & R_FOOT) draw_footer();
}

/* a screen reader that has just started listening (or asks) hears the screen */
static void speak(void)
{
    char t[96], k[40], a[48];
    if (ann_on() && ann_where()) {
        t[0] = 0;
        tcat(t, APP_NAME ". ");
        key_text(k);
        gt_ascii(k, a, sizeof a);
        tcat(t, a);
        tcat(t, ". ");
        tcat(t, rec.keymode == KM_CHORDS ? "CHORDS" : "NOTES");
        ann_say(t, 0, ANN_NOW);
    }
}

/* ------------------------------------------------------------------ audio --- */
static float gain_now;

void app_render(int32_t *lr, uint32_t n, uint32_t master_q12)
{
    float gain = (float)master_q12 / 4096.0f * 8300000.0f;
    gain_now += (gain - gain_now) * 0.5f;
    synth_render(lr, n, gain_now);
}

void app_panic(void) { synth_stop(); }

/* ------------------------------------------------------------ the contract --- */
void app_init(void)
{
    /* everything from nothing: the host simulator's reboot runs this again over the old RAM */
    zero(&ui, sizeof ui);
    saves = 0;
    dirty_at = 0;
    if (plat_store_load(OBJ_THEORY, &rec, sizeof rec) != (int)sizeof rec || !rec_valid(&rec)) {
        zero(&rec, sizeof rec);
        defaults();
    }
    synth_init();
    synth_stop();
    layout();
}

void app_ui_init(void)
{
    touch_on(1);                         /* velocity from the keys */
    lcd_fill(0, 0, 240, 240, C_BLACK);
    ui.redraw = R_ALL;
}

void app_frame(void)
{
    static uint32_t last_toast;
    uint32_t now = plat_ms();
    input();
    if (ui.held_changed) {
        ui.held_changed = 0;
        identify_held();
    }
    if ((ui.toast_until && now >= ui.toast_until) != last_toast) {
        last_toast = ui.toast_until && now >= ui.toast_until;
        ui.redraw |= R_FOOT;
    }
    if (dirty_at && (int32_t)(now - dirty_at) > 3000)   /* settings saved once the knobs rest */
        rec_save();
    lights(now);
    speak();
    ann_tick(now);
    if (ui.redraw) draw();
}

/* --------------------------------------------------------- host simulator --- */
#ifdef PLAT_HOST
int app_expect(const char *what, int *got)
{
    if (!strcmp(what, "tonic")) *got = rec.tonic;
    else if (!strcmp(what, "mode")) *got = rec.mode;
    else if (!strcmp(what, "keymode")) *got = rec.keymode;
    else if (!strcmp(what, "sevenths")) *got = rec.sevenths;
    else if (!strcmp(what, "labels")) *got = rec.labels;
    else if (!strcmp(what, "mapping")) *got = rec.mapping;
    else if (!strcmp(what, "oct")) *got = rec.oct;
    else if (!strcmp(what, "voices")) *got = synth_active();
    else if (!strcmp(what, "lit_keys")) *got = (int)ui.lit;
    else if (!strcmp(what, "mid_keys")) *got = (int)ui.mid;
    else if (!strcmp(what, "glow_keys")) *got = (int)ui.glow;
    else if (!strcmp(what, "shown")) *got = ui.sh;
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
    char want[96], raw[96], got[96], small[96];
    (void)c, (void)d;
    if (strcmp(cmd, "expect_text")) return 0;
    /* expect_text key|chord|roman|nash|footer|rowN TEXT (_ a space): what the screen shows, signs as # b o /o */
    host_text(b, want, sizeof want);
    raw[0] = 0;
    if (!strcmp(a, "key")) key_text(raw);
    else if (!strcmp(a, "chord") && ui.sh == SH_CHORD) th_chord_name(ui.id.c, raw);
    else if (!strcmp(a, "roman") && ui.sh == SH_CHORD) th_roman_name(cur_key(), ui.id.c, raw);
    else if (!strcmp(a, "nash") && ui.sh == SH_CHORD) th_nashville_name(cur_key(), ui.id.c, raw);
    else if (!strcmp(a, "footer")) {
        footer_text(raw, small);
        if (small[0]) {
            tcat(raw, " ");
            tcat(raw, small);
        }
    } else if (!strncmp(a, "row", 3)) {
        th_chord_t ch;
        if (th_diatonic(cur_key(), atoi(a + 3), rec.sevenths, &ch)) th_chord_name(ch, raw);
    }
    gt_ascii(raw, got, sizeof got);
    if (strcmp(got, want)) {
        printf("  FAIL expect_text %s: \"%s\", got \"%s\"\n", a, want, got);
        host_fails++;
    } else
        printf("  ok text %s \"%s\"\n", a, want);
    return 1;
}

int app_host_fails(void) { return host_fails; }
#endif
