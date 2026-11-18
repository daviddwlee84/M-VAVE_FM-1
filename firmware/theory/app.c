/* SPDX-License-Identifier: GPL-3.0-or-later */
/* THEORY v0.2 (see app.h): pages MENU, TODAY (flash cards in rounds), WHEEL (the circle of fifths), CHORD (name what
 * is played), CHART (progress), SETTINGS. The theory is theory.c, the rounds srs.c and course.h, the voice synth.c,
 * the music signs gtext.c, the computer's link hostlink.c (../common). Part of fm1-base's unity build, after the
 * platform: gfx.c and announce.c are in scope. */
#include "fastmath.h"
#include "gtext.h"
#define TH_GLYPH_SHARP GT_SHARP
#define TH_GLYPH_FLAT GT_FLAT
#define TH_GLYPH_DIM GT_DIM
#define TH_GLYPH_HDIM GT_HDIM
#include "theory.c"
#include "srs.c"
#include "course.h"
#include "synth.c"
#include "gtext.c"
#include "ledfx.c"
#include "hostlink.c"

#define FW_MAJOR 0
#define FW_MINOR 2

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
enum { SP_NORMAL, SP_SLOW, SP_FAST, SP_N };
#define CARDS_DEFAULT 15u

/* 0.1's record, its spare bytes now used (0 meaning the default, so 0.1 reads it and 0.2 reads 0.1's) */
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
    uint8_t start;                       /* 0.2: the page at power-on + 1; 0: not chosen yet (asked once) */
    uint8_t cards;                       /* 0.2: cards a round, 8 .. 24 (0: CARDS_DEFAULT) */
    uint8_t speed;                       /* 0.2: SP_*: how fast cadences play */
    uint8_t rsv[2];
} rec_t;
static rec_t rec;

/* the flash cards' progress, its own record (OBJ_PROGRESS) */
#define PROG_MAGIC 0x31305054u           /* "TP01" */
#define PROG_VER 1u
typedef struct {
    uint32_t magic;
    uint16_t ver, size;
    sr_state_t st;
} prog_rec_t;
static prog_rec_t prec;

static uint32_t saves;                   /* records written since boot */
static uint32_t dirty_at;                /* a setting changed at (0: saved) */

static int cards_n(void) { return rec.cards ? rec.cards : (int)CARDS_DEFAULT; }

static void defaults(void)
{
    rec.tonic = 0;
    rec.mode = 0;
    rec.sevenths = 0;
    rec.labels = LB_ROMAN;
    rec.mapping = MAP_LETTER;
    rec.keymode = KM_NOTES;
    rec.oct = 0;
    rec.start = 0;
    rec.cards = 0;
    rec.speed = SP_NORMAL;
}

static int set_valid(const rec_t *r)
{
    return r->tonic >= -9 && r->tonic <= 12 && r->mode < NMODES && r->sevenths <= 1u && r->labels < LB_N
           && r->mapping < MAP_N && r->keymode <= 1u && r->oct >= -2 && r->oct <= 2 && r->start <= 5u
           && (r->cards == 0u || (r->cards >= 8u && r->cards <= SR_ROUND_MAX)) && r->speed < SP_N;
}

static int rec_valid(const rec_t *r)
{
    return r->magic == REC_MAGIC && r->ver == REC_VER && r->size == sizeof *r && set_valid(r);
}

static void rec_save(void)
{
    rec.magic = REC_MAGIC;
    rec.ver = REC_VER;
    rec.size = (uint16_t)sizeof rec;
    if (plat_store_save(OBJ_THEORY, &rec, sizeof rec) == 0) saves++;
    dirty_at = 0;
}

static void prog_save(void)
{
    prec.magic = PROG_MAGIC;
    prec.ver = PROG_VER;
    prec.size = (uint16_t)sizeof prec;
    if (plat_store_save(OBJ_PROGRESS, &prec, sizeof prec) == 0) saves++;
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
#define BRUSH_MS 50u                     /* a key let go sooner was brushed, not played */

enum { PG_MENU, PG_TODAY, PG_WHEEL, PG_CHORD, PG_CHART, PG_SET, PG_N };
static const char *const PAGE_NAME[PG_N] = { "MENU", "TODAY", "WHEEL", "CHORD", "CHART", "SETTINGS" };
static const char *const PAGE_WHAT[PG_N] = { "", "flash cards, a round a day", "the circle of fifths",
                                             "name what you play", "what you know", "" };
enum { SH_NONE, SH_NOTE, SH_INTERVAL, SH_CHORD, SH_UNKNOWN };
enum { ASK_NONE, ASK_LOAD };
#define HIST_N 4

static struct {
    uint32_t keys, btn;
    uint8_t redraw;
    uint8_t page, back, msel, srow;      /* page; the page HOME goes back to; menu and settings cursors */
    uint8_t first, fsel;                 /* the start page asked for (first power-on); its cursor */
    uint8_t ask;
    uint32_t down_at[NKEYS];
    uint8_t knotes[NKEYS][TH_TONES_MAX + 1];  /* the notes each held key started */
    uint8_t kn[NKEYS];
    uint8_t prev[TH_TONES_MAX], nprev;   /* CHORDS: the last voicing, for the next to move least from */
    uint8_t held_changed, best_n;        /* NOTES: the held keys changed; most notes held in this gesture */
    uint8_t sh;                          /* SH_*: what the footer / CHORD page shows */
    th_ident_t id;
    th_ident_t alt[4];
    uint8_t nalt;
    uint8_t notes[12], nnotes;           /* the notes behind it (MIDI) */
    th_chord_t hist[HIST_N];             /* CHORD: the chords played, newest last */
    uint8_t nhist, gesture_chord;
    char toast[2][24];
    uint32_t toast_until;
    uint32_t lit, mid, glow;             /* the keys' lights, last shown (the simulator's checks) */
    uint8_t ccur;                        /* CHART: the key under the cursor (course key index) */
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

static void sig_text(int tonic, int scale, char *t)   /* "2#", "3b", "0" */
{
    int8_t s[7];
    int c = th_keysig(tonic, scale, s);
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

static void key_sound(uint32_t k, int vel)   /* a key plays its own note */
{
    int note = KEY_MIDI(k) + 12 * rec.oct;
    ui.knotes[k][0] = (uint8_t)note;
    ui.kn[k] = 1;
    synth_note_on(note, vel);
    midi_out(1, note, vel);
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
    ui.nalt = 0;
    ui.nnotes = (uint8_t)(n > 12 ? 12 : n);
    for (i = 0; i < ui.nnotes; i++) ui.notes[i] = notes[i];
    ui.redraw |= R_MAIN | R_FOOT;
}

/* WHEEL, CHORDS: a white key plays the key's chord on its degree, voiced near the last one, with the root below */
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

/* NOTES / CHORD: name what is held. While a chord is let go, the fullest one held stays on the screen */
static void identify_held(void)
{
    th_key_t key = cur_key();
    th_ident_t all[5];
    uint8_t m[NKEYS];
    int n = 0, i, na;
    uint32_t k;
    for (k = 0; k < NKEYS; k++)
        if (ui.keys >> k & 1u) m[n++] = (uint8_t)KEY_MIDI(k);
    if (n == 0 || n < ui.best_n) return;
    ui.best_n = (uint8_t)n;
    ui.nnotes = (uint8_t)(n > 12 ? 12 : n);
    for (i = 0; i < ui.nnotes; i++) ui.notes[i] = m[i];
    ui.nalt = 0;
    if (n == 1) ui.sh = SH_NOTE;
    else if ((na = th_identify_all(m, n, key, all, 5)) > 0) {
        ui.sh = SH_CHORD;
        ui.id = all[0];
        for (i = 1; i < na; i++) ui.alt[ui.nalt++] = all[i];
        ui.gesture_chord = 1;
    } else ui.sh = n == 2 ? SH_INTERVAL : SH_UNKNOWN;
    ui.redraw |= R_MAIN | R_FOOT;
}

static void hist_push(th_chord_t c)      /* CHORD: a chord played, at the end of the gesture */
{
    int i;
    if (ui.nhist && ui.hist[ui.nhist - 1].root == c.root && ui.hist[ui.nhist - 1].q == c.q
        && ui.hist[ui.nhist - 1].bass == c.bass) return;
    if (ui.nhist == HIST_N) {
        for (i = 1; i < HIST_N; i++) ui.hist[i - 1] = ui.hist[i];
        ui.nhist--;
    }
    ui.hist[ui.nhist++] = c;
}

/* ----------------------------------------------------------------- TODAY --- */
enum { PS_START, PS_CARD, PS_RESULT, PS_DONE };
enum { FO_ALL, FO_THEORY, FO_EAR, FO_N };
static const char *const FOCUS_NAME[FO_N] = { "ALL", "THEORY", "EAR" };
static const uint8_t CHOICE_KEY[4] = { 11, 12, 14, 16 };   /* E4 F4 G4 A4 */
static const uint16_t EASY_MS[SK_N] = { 6000, 4000, 2500, 4000, 2500, 2000 };
static const uint16_t SLOW_MS[SK_N] = { 15000, 12000, 8000, 12000, 8000, 6000 };
static const char *const RES_NAME[4] = { "NO", "RIGHT", "SLOW", "RIGHT" };

static struct {
    sr_round_t r;
    uint8_t ps, focus;
    uint8_t item, kind, skill, again;    /* the card */
    int8_t tonic;
    uint8_t degree;
    th_chord_t chord;
    int8_t want[8];
    uint8_t nwant;
    uint16_t want_pcs, tray;             /* pitch classes: the answer; what has been played */
    uint8_t step, slips, fixed, retry, too_many;
    uint8_t added[NKEYS], toggle[NKEYS], slip_key[NKEYS];
    uint8_t opts[4], nopts, right_opt, pick;
    uint8_t answer_ok, replays, res;
    uint32_t ok_at, until;
    uint8_t opened_key;                  /* the key a round end opened, + 1 */
} pr;

static const char *qname(int q)          /* a chord quality, short */
{
    return q == TH_MIN ? "min" : q == TH_DIM ? "dim" : q == TH_AUG ? "aug" : "maj";
}

static int item_tonic(int item) { return CO_TONIC[co_key[item] < CO_KEYS ? co_key[item] : 0]; }

static void item_name(int item, char *t)   /* "D scale", "maj by ear" */
{
    static const char *const SK[SK_N] = { " scale", " key sig.", " rel. minor", " chords", " by ear", " by ear" };
    t[0] = 0;
    if (co_skill[item] == SK_EARQ) tcat(t, qname(co_param[item]));
    else th_name(item_tonic(item), t);
    tcat(t, SK[co_skill[item]]);
}

static uint16_t pcs_of(const int8_t *f, int n)
{
    uint16_t m = 0;
    int i;
    for (i = 0; i < n; i++) m |= (uint16_t)(1u << th_pc(f[i]));
    return m;
}

static int popcount12(uint16_t m)
{
    int n = 0;
    while (m) { n += m & 1u; m >>= 1; }
    return n;
}

static void answer_text(char *t)         /* the answer, spelled */
{
    int i;
    t[0] = 0;
    if (pr.skill == SK_EARQ) {
        tcat(t, qname(pr.chord.q));
        return;
    }
    for (i = 0; i < pr.nwant; i++) {
        char n[TH_NAME_MAX];
        th_name(pr.want[i], n);
        if (i) tcat(t, " ");
        tcat(t, n);
    }
}

static int spd(int ms) { return rec.speed == SP_SLOW ? ms * 3 / 2 : rec.speed == SP_FAST ? ms * 2 / 3 : ms; }

/* a cadence I-IV-V-I in the key, voiced near, with the root below; then the notes given, at its end */
static int cadence(int tonic, sy_note_t *s)
{
    th_key_t key;
    uint8_t prev[TH_TONES_MAX], v[TH_TONES_MAX];
    int np = 0, i, j, n = 0, t = 0, d = spd(520);
    static const uint8_t DEG[4] = { 0, 3, 4, 0 };
    key.tonic = (int8_t)tonic;
    key.scale = TH_MAJOR;
    for (i = 0; i < 4; i++) {
        th_chord_t c;
        int nv, root;
        th_diatonic(key, DEG[i], 0, &c);
        nv = th_voice_near(prev, np, c, 57, 74, v);
        root = 43 + th_mod(th_pc(c.root) - 43, 12);
        s[n].at = (uint16_t)t; s[n].dur = (uint16_t)(d - 40); s[n].note = (uint8_t)root; s[n].vel = 70; n++;
        for (j = 0; j < nv; j++) {
            s[n].at = (uint16_t)t; s[n].dur = (uint16_t)(d - 40); s[n].note = v[j]; s[n].vel = 64; n++;
            prev[j] = v[j];
        }
        np = nv;
        t += d;
    }
    return n;
}

static int note_in(int f, int lo)        /* a spelled note's pitch at lo or above, within an octave */
{
    return lo + th_mod(th_pc(f) - lo, 12);
}

static void play_prompt(void)            /* the card's sound */
{
    sy_note_t s[SY_SCORE_MAX];
    int n = 0, i, t;
    if (pr.skill == SK_EARDEG) {
        n = cadence(pr.tonic, s);
        t = 4 * spd(520) + spd(300);
        s[n].at = (uint16_t)t; s[n].dur = 1000; s[n].note = (uint8_t)note_in(pr.want[0], 60); s[n].vel = 90; n++;
    } else if (pr.skill == SK_EARQ) {
        int8_t f[TH_TONES_MAX];
        int nt = th_chord(pr.chord.root, pr.chord.q, f), lo = 52 + th_mod(th_pc(pr.chord.root) - 52, 12), last = lo;
        for (i = 0; i < nt; i++) {
            int m = i ? last + th_mod(th_pc(f[i]) - last, 12) : lo;
            s[n].at = 0; s[n].dur = 1300; s[n].note = (uint8_t)m; s[n].vel = 80; n++;
            last = m;
        }
    }
    if (n) synth_score(s, (uint32_t)n);
}

static void play_answer(void)            /* the answer's sound, after a card */
{
    sy_note_t s[SY_SCORE_MAX];
    int n = 0, i, last = 59;
    if (pr.skill == SK_EARQ || pr.skill == SK_EARDEG) {
        play_prompt();                   /* again, now that it is known */
        if (pr.skill == SK_EARDEG) {     /* only the note this time */
            s[0].at = 0; s[0].dur = 900; s[0].note = (uint8_t)note_in(pr.want[0], 60); s[0].vel = 90;
            synth_score(s, 1);
        }
        return;
    }
    for (i = 0; i < pr.nwant; i++) {
        int m = note_in(pr.want[i], i ? last + 1 : 60);
        int arp = pr.skill == SK_SCALE || pr.skill == SK_KEYSIG;
        s[n].at = (uint16_t)(arp ? i * 220 : 0);
        s[n].dur = (uint16_t)(arp ? 260 : 1000);
        s[n].note = (uint8_t)m;
        s[n].vel = 80;
        n++;
        last = m;
    }
    if (pr.skill == SK_SCALE) {          /* up to the tonic */
        s[n].at = (uint16_t)(n * 220); s[n].dur = 500; s[n].note = (uint8_t)(note_in(pr.want[0], 60) + 12); s[n].vel = 80; n++;
    }
    if (n) synth_score(s, (uint32_t)n);
}

static void card_setup(void)             /* the card's answer, from its item */
{
    th_key_t key;
    int8_t f[8];
    int n, i, box = prec.st.box[pr.item];
    pr.skill = co_skill[pr.item];
    pr.tonic = (int8_t)item_tonic(pr.item);
    key.tonic = pr.tonic;
    key.scale = TH_MAJOR;
    pr.nwant = 0;
    pr.nopts = 0;
    switch (pr.skill) {
    case SK_SCALE:
        pr.nwant = (uint8_t)th_scale(pr.tonic, TH_MAJOR, pr.want);
        break;
    case SK_KEYSIG:
        n = th_keysig(pr.tonic, TH_MAJOR, f);
        n = n < 0 ? -n : n;
        for (i = 0; i < n && i < 7; i++) pr.want[i] = f[i];
        pr.nwant = (uint8_t)(n > 7 ? 7 : n);
        break;
    case SK_RELMIN:
        pr.want[0] = (int8_t)(pr.tonic + 3);
        pr.nwant = 1;
        break;
    case SK_CHORDS:
        pr.degree = (uint8_t)(sr_rand(&pr.r.rng) % 7u);
        th_diatonic(key, pr.degree, 0, &pr.chord);
        pr.nwant = (uint8_t)th_chord(pr.chord.root, pr.chord.q, pr.want);
        break;
    case SK_EARDEG:
        pr.degree = (uint8_t)(box <= 1 ? (sr_rand(&pr.r.rng) % 3u) * 2u : sr_rand(&pr.r.rng) % 7u);   /* 1 3 5, then all */
        pr.want[0] = (int8_t)(pr.tonic + th_scales[TH_MAJOR].f[pr.degree]);
        pr.nwant = 1;
        break;
    case SK_EARQ: {
        static const uint8_t QS[4] = { TH_MAJ, TH_MIN, TH_DIM, TH_AUG };
        int j, root = (int)(sr_rand(&pr.r.rng) % 12u);
        pr.chord.root = pr.chord.bass = (int8_t)th_spell_pc(root, 0);
        pr.chord.q = co_param[pr.item];
        for (j = 0; j < 4; j++) {        /* the qualities open so far are the choices */
            for (i = 0; i < prec.st.open; i++)
                if (co_skill[i] == SK_EARQ && co_param[i] == QS[j]) break;
            if (i < prec.st.open) {
                if (QS[j] == pr.chord.q) pr.right_opt = pr.nopts;
                pr.opts[pr.nopts++] = QS[j];
            }
        }
        break;
    }
    default:
        break;
    }
    pr.want_pcs = pcs_of(pr.want, pr.nwant);
}

static void card_present(void)
{
    const sr_card_t *c = sr_card(&pr.r);
    synth_score_stop();
    zero(pr.added, sizeof pr.added);
    zero(pr.toggle, sizeof pr.toggle);
    zero(pr.slip_key, sizeof pr.slip_key);
    pr.tray = 0;
    pr.step = pr.slips = pr.fixed = pr.retry = pr.too_many = 0;
    pr.replays = 0;
    pr.pick = 0;
    pr.answer_ok = 0;
    if (!c) {
        pr.ps = PS_DONE;
        pr.opened_key = 0;
        if (sr_finish(&prec.st, &pr.r, &course)) pr.opened_key = (uint8_t)(co_key[prec.st.open - 1] + 1u);
        prog_save();
        if (pr.opened_key) app_say("NEW KEY", "");
        ui.redraw |= R_ALL;
        return;
    }
    pr.item = c->item;
    pr.kind = c->kind;
    pr.again = c->again;
    card_setup();
    pr.ps = PS_CARD;
    if (pr.kind == SR_SHOW) play_answer();   /* a new item: shown and heard */
    else play_prompt();
    ui.redraw |= R_ALL;
}

static void today_start(void)
{
    static const uint32_t FOCUS_MASK[FO_N] = { 3u, 1u << SEG_THEORY, 1u << SEG_EAR };
    release_all();
    sr_build(&prec.st, &pr.r, &course, cards_n(), FOCUS_MASK[pr.focus], plat_ms() * 2654435761u + prec.st.round + 1u);
    card_present();
}

static void card_result(int grade)       /* the card answered */
{
    const sr_card_t *c = sr_card(&pr.r);
    if (!c) return;
    sr_answer(&prec.st, &pr.r, grade);
    pr.res = (uint8_t)grade;
    pr.ps = PS_RESULT;
    play_answer();
    pr.until = plat_ms() + (grade == SR_WRONG ? 2400u : 1000u);
    ui.redraw |= R_ALL;
}

static void show_next(void)             /* a new item seen: on */
{
    sr_answer(&prec.st, &pr.r, SR_RIGHT);
    card_present();
}

static int grade_right(void)             /* right: how fast, and was it clean */
{
    uint32_t t = plat_ms() - pr.ok_at;
    if (pr.fixed || pr.slips || pr.retry || pr.replays >= 2u || t > SLOW_MS[pr.skill]) return SR_SLOW;
    return t < EASY_MS[pr.skill] ? SR_EASY : SR_RIGHT;
}

static void today_leave(void)            /* left halfway: what was answered still counts */
{
    if (pr.ps == PS_CARD || pr.ps == PS_RESULT) {
        if (pr.r.ok + pr.r.miss + pr.r.shown) {
            sr_finish(&prec.st, &pr.r, &course);
            prog_save();
        }
    }
    synth_score_stop();
    pr.ps = PS_START;
}

static void today_judge_set(void)        /* all keys up: is the tray the answer? */
{
    int n = popcount12(pr.tray);
    pr.too_many = n > pr.nwant;
    if (n != pr.nwant) { ui.redraw |= R_MAIN; return; }
    if (pr.tray == pr.want_pcs) card_result(grade_right());
    else if (pr.nwant == 1 && !pr.retry) {   /* one note, one wrong guess: once more */
        pr.retry = 1;
        pr.tray = 0;
        app_say("TRY AGAIN", "");
        ui.redraw |= R_MAIN;
    } else card_result(SR_WRONG);
}

static void today_key_down(uint32_t k, int vel)
{
    int pc = KEY_MIDI(k) % 12, i;
    key_sound(k, vel);
    if (pr.ps == PS_RESULT) { pr.until = 0; return; }   /* any key: on to the next card */
    if (pr.ps != PS_CARD) return;
    if (pr.kind == SR_SHOW) { show_next(); return; }
    if (!pr.answer_ok) return;
    if (pr.skill == SK_EARQ) {
        for (i = 0; i < 4; i++)
            if (CHOICE_KEY[i] == k && i < pr.nopts) card_result(i == pr.right_opt ? grade_right() : SR_WRONG);
        return;
    }
    if (pr.skill == SK_SCALE) {
        if (pc == th_pc(pr.want[pr.step])) {
            pr.step++;
            pr.tray |= (uint16_t)(1u << pc);
            if (pr.step >= pr.nwant) card_result(grade_right());
            else ui.redraw |= R_MAIN;
        } else if (!(pr.step && pc == th_pc(pr.want[pr.step - 1]))) pr.slip_key[k] = 1;   /* counted if held */
        return;
    }
    {
        uint32_t others = ui.keys & ~(1u << k), j, held_elsewhere = 0;
        for (j = 0; j < NKEYS; j++)
            if ((others >> j & 1u) && KEY_MIDI(j) % 12 == pc) held_elsewhere = 1;
        if (pr.tray >> pc & 1u) {
            if (!held_elsewhere) pr.toggle[k] = 1;   /* again: taken out, if it is held (not brushed) */
        } else {
            pr.tray |= (uint16_t)(1u << pc);
            pr.added[k] = 1;
        }
        ui.redraw |= R_MAIN;
    }
}

static void today_key_up(uint32_t k, uint32_t held_ms)
{
    int pc = KEY_MIDI(k) % 12;
    if (pr.ps != PS_CARD || pr.kind == SR_SHOW || !pr.answer_ok) return;
    if (pr.skill == SK_SCALE) {
        if (pr.slip_key[k] && held_ms >= BRUSH_MS) {
            pr.slips++;
            ui.redraw |= R_MAIN;
            if (pr.slips >= 2u) card_result(SR_WRONG);
        }
        pr.slip_key[k] = 0;
        return;
    }
    if (pr.skill == SK_EARQ) return;
    if (pr.added[k] && held_ms < BRUSH_MS) pr.tray &= (uint16_t)~(1u << pc);   /* brushed: not played */
    if (pr.toggle[k] && held_ms >= BRUSH_MS) {
        pr.tray &= (uint16_t)~(1u << pc);
        pr.fixed = 1;
    }
    pr.added[k] = pr.toggle[k] = 0;
    if (!(ui.keys & ~(1u << k))) today_judge_set();
    else ui.redraw |= R_MAIN;
}

static void today_step(uint32_t now)
{
    if (pr.ps == PS_CARD && !pr.answer_ok && pr.kind == SR_ASK && !synth_score_busy()) {
        pr.answer_ok = 1;
        pr.ok_at = now;
        ui.redraw |= R_MAIN;
    }
    if (pr.ps == PS_RESULT && now >= pr.until && !synth_score_busy()) card_present();
}

/* ------------------------------------------------------- progress backup --- */
enum { T_HELLO_ACK = 0x01, T_PROG_REQ = 0x04, T_PROG_LOAD = 0x05,
       T_HELLO = 0x41, T_HEARTBEAT = 0x42, T_PROG = 0x44, T_PROG_DONE = 0x45 };
enum { PD_LOADED, PD_DECLINED, PD_BAD };
#define TH_PROTO 1u
#define PROG_FORMAT 1u
#define SET_N 12u
#define PROG_N (1u + SET_N + SR_BLOB_N(CO_N) + 2u)
#define PROG_CHUNK 49u
#define PROG_FRAMES ((PROG_N + PROG_CHUNK - 1u) / PROG_CHUNK)

static hostlink_t link;
static struct {
    uint8_t tx_on, tx_i, rx_next;
    uint8_t buf[PROG_FRAMES * PROG_CHUNK];
    rec_t set;                           /* a backup waiting for PLAY */
    sr_state_t st;
} xfer;

static uint16_t crc16(const uint8_t *p, uint32_t n)   /* CRC-16/CCITT-FALSE */
{
    uint16_t c = 0xFFFFu;
    uint32_t k;
    while (n--) {
        c ^= (uint16_t)(*p++ << 8);
        for (k = 0; k < 8u; k++) c = (uint16_t)(c & 0x8000u ? (c << 1) ^ 0x1021u : c << 1);
    }
    return c;
}

static void set_blob(const rec_t *r, uint8_t *b)  /* SET_N bytes, field by field */
{
    b[0] = (uint8_t)r->tonic;
    b[1] = r->mode;
    b[2] = r->sevenths;
    b[3] = r->labels;
    b[4] = r->mapping;
    b[5] = r->keymode;
    b[6] = (uint8_t)r->oct;
    b[7] = r->start;
    b[8] = r->cards;
    b[9] = r->speed;
    b[10] = b[11] = 0;
}

static int set_unblob(rec_t *r, const uint8_t *b)
{
    zero(r, sizeof *r);
    r->tonic = (int8_t)b[0];
    r->mode = b[1];
    r->sevenths = b[2];
    r->labels = b[3];
    r->mapping = b[4];
    r->keymode = b[5];
    r->oct = (int8_t)b[6];
    r->start = b[7];
    r->cards = b[8];
    r->speed = b[9];
    return set_valid(r);
}

static void prog_fill(uint8_t *b)        /* PROG_N bytes: the format, the settings, the progress, the CRC */
{
    uint16_t c;
    b[0] = PROG_FORMAT;
    set_blob(&rec, b + 1);
    sr_blob(&prec.st, b + 1u + SET_N);
    c = crc16(b, PROG_N - 2u);
    b[PROG_N - 2u] = (uint8_t)c;
    b[PROG_N - 1u] = (uint8_t)(c >> 8);
}

static uint32_t pack7(const uint8_t *in, uint32_t n, uint8_t *out)   /* each 7 bytes: their high bits, then them */
{
    uint32_t i, j, k = 0;
    for (i = 0; i < n; i += 7u) {
        uint32_t m = n - i < 7u ? n - i : 7u;
        uint8_t hi = 0;
        for (j = 0; j < m; j++) hi |= (uint8_t)((in[i + j] >> 7) << j);
        out[k++] = hi;
        for (j = 0; j < m; j++) out[k++] = in[i + j] & 0x7Fu;
    }
    return k;
}

static uint32_t unpack7(const uint8_t *in, uint32_t n, uint8_t *out, uint32_t max)
{
    uint32_t i = 0, j, k = 0;
    while (i < n) {
        uint8_t hi = in[i++];
        for (j = 0; j < 7u && i < n && k < max; j++) out[k++] = (uint8_t)(in[i++] | ((hi >> j) & 1u) << 7);
    }
    return k;
}

static void prog_done(int answer)
{
    uint8_t a = (uint8_t)answer;
    hl_send(&link, T_PROG_DONE, &a, 1);
}

static void prog_pump(void)              /* the backup out, a frame each main-loop pass */
{
    uint8_t b[PROG_N], f[2u + PROG_CHUNK + PROG_CHUNK / 7u + 1u];
    uint32_t off, m, k;
    if (!xfer.tx_on) return;
    prog_fill(b);
    off = xfer.tx_i * PROG_CHUNK;
    m = PROG_N - off < PROG_CHUNK ? PROG_N - off : PROG_CHUNK;
    f[0] = xfer.tx_i;
    f[1] = (uint8_t)PROG_FRAMES;
    k = 2u + pack7(b + off, m, f + 2);
    if (hl_send(&link, T_PROG, f, k) == 0 && ++xfer.tx_i >= PROG_FRAMES) xfer.tx_on = 0;
}

static void prog_in(const uint8_t *d, uint32_t len)   /* a chunk of a backup to load */
{
    uint32_t off;
    uint16_t c;
    if (len < 3u || d[1] != PROG_FRAMES || d[0] >= PROG_FRAMES) { prog_done(PD_BAD); return; }
    if (d[0] == 0u) xfer.rx_next = 0;
    if (d[0] != xfer.rx_next) { xfer.rx_next = 0; prog_done(PD_BAD); return; }
    off = d[0] * PROG_CHUNK;
    (void)unpack7(d + 2, len - 2u, xfer.buf + off, PROG_N - off < PROG_CHUNK ? PROG_N - off : PROG_CHUNK);
    if (++xfer.rx_next < PROG_FRAMES) return;
    xfer.rx_next = 0;
    c = crc16(xfer.buf, PROG_N - 2u);
    if (ui.ask) { prog_done(PD_DECLINED); return; }
    if (xfer.buf[0] != PROG_FORMAT || xfer.buf[PROG_N - 2u] != (uint8_t)c || xfer.buf[PROG_N - 1u] != (uint8_t)(c >> 8)
        || !set_unblob(&xfer.set, xfer.buf + 1) || !sr_unblob(&xfer.st, &course, xfer.buf + 1u + SET_N, (int)SR_BLOB_N(CO_N))) {
        prog_done(PD_BAD);
        app_say("BACKUP REFUSED", "IT DOES NOT CHECK OUT");
        return;
    }
    ui.ask = ASK_LOAD;
    release_all();
    ui.redraw |= R_ALL;
}

static void ask_answer(int yes)
{
    if (ui.ask != ASK_LOAD) return;
    if (yes) {
        today_leave();
        xfer.set.magic = rec.magic;
        rec = xfer.set;
        prec.st = xfer.st;
        rec_save();
        prog_save();
        prog_done(PD_LOADED);
        app_say("BACKUP LOADED", "");
    } else {
        prog_done(PD_DECLINED);
        app_say("NOT LOADED", "");
    }
    ui.ask = ASK_NONE;
    ui.redraw |= R_ALL;
}

static int host_frame(const uint8_t *p, uint32_t n)
{
    uint8_t type;
    uint32_t len;
    const uint8_t *d = hl_match(&link, p, n, &type, &len);
    if (!d) return 0;
    hl_seen(&link, plat_ms());
    if (type == T_HELLO_ACK) hl_acked(&link, plat_ms(), HL_USB);
    else if (type == T_PROG_REQ) { xfer.tx_on = 1; xfer.tx_i = 0; }
    else if (type == T_PROG_LOAD) prog_in(d, len);
    return 1;
}

int app_sysex(const uint8_t *p, uint32_t n) { return host_frame(p, n); }

/* ----------------------------------------------------------------- pages --- */
static void set_page(int p)
{
    if (p == ui.page) return;
    if (ui.page == PG_TODAY) today_leave();
    release_all();
    if (ui.page != PG_MENU) ui.back = ui.page;
    ui.page = (uint8_t)p;
    ui.sh = SH_NONE;
    ui.best_n = 0;
    if (p == PG_MENU) ui.msel = (uint8_t)(ui.back > 0 ? ui.back - 1 : 0);
    if (p == PG_CHART) ui.ccur = 0;
    ui.redraw = R_ALL;
}

static void set_tonic(int f)
{
    f = th_key_for_pc(th_pc(f), MODES[rec.mode]);
    if (f == rec.tonic) return;
    rec.tonic = (int8_t)f;
    ui.nprev = 0;
    changed(R_ALL);
}

static const char *const SPEED_NAME[SP_N] = { "NORMAL", "SLOW", "FAST" };
enum { SR_START, SR_CARDS, SR_SPEED, SR_ROWS };

static void settings_change(int d)
{
    char t[16];
    t[0] = 0;
    if (ui.srow == SR_START) {
        int s = rec.start ? rec.start - 1 : 0;
        s = th_mod(s + d, 5);
        rec.start = (uint8_t)(s + 1);
        app_say("START PAGE", PAGE_NAME[s]);
    } else if (ui.srow == SR_CARDS) {
        int c = clamp(cards_n() + d, 8, (int)SR_ROUND_MAX);
        rec.cards = (uint8_t)c;
        tint(t, c);
        app_say("CARDS A ROUND", t);
    } else {
        rec.speed = (uint8_t)th_mod(rec.speed + d, SP_N);
        app_say("SPEED", SPEED_NAME[rec.speed]);
    }
    changed(R_MAIN);
}

static void first_choose(void)           /* the start page, chosen at the first power-on */
{
    rec.start = (uint8_t)(ui.fsel + 1u);
    ui.first = 0;
    rec_save();
    ui.page = PG_MENU;
    set_page(ui.fsel);
    ui.back = ui.fsel ? ui.fsel : PG_TODAY;
    app_say("START PAGE", PAGE_NAME[ui.fsel]);
    ui.redraw = R_ALL;
}

/* ----------------------------------------------------------------- input --- */
static void key_down(uint32_t k, int vel, uint32_t btn)
{
    char t[32];
    switch (ui.page) {
    case PG_WHEEL:
        if (btn >> B_SEL & 1u) {         /* SEL held: this note becomes the tonic */
            set_tonic(th_spell_pc(KEY_MIDI(k) % 12, 0));
            key_text(t);
            app_say("KEY", t);
            return;
        }
        if (!ui.keys) ui.best_n = 0;
        if (rec.keymode == KM_CHORDS) chord_key(k, vel);
        else {
            key_sound(k, vel);
            ui.held_changed = 1;
        }
        break;
    case PG_CHORD:
        if (!ui.keys) { ui.best_n = 0; ui.gesture_chord = 0; }
        key_sound(k, vel);
        ui.held_changed = 1;
        break;
    case PG_TODAY:
        today_key_down(k, vel);
        break;
    default:
        key_sound(k, vel);
        break;
    }
}

static void key_up(uint32_t k, uint32_t held)
{
    key_release(k);
    if (ui.page == PG_WHEEL && rec.keymode == KM_NOTES) ui.held_changed = 1;
    if (ui.page == PG_CHORD) {
        ui.held_changed = 1;
        if (!(ui.keys & ~(1u << k)) && ui.gesture_chord && ui.sh == SH_CHORD) {
            hist_push(ui.id.c);
            ui.gesture_chord = 0;
            ui.redraw |= R_MAIN | R_FOOT;
        }
    }
    if (ui.page == PG_TODAY) today_key_up(k, held);
}

static void button(uint32_t b)
{
    char t[16];
    if (ui.first) {
        if (b == B_PLAY || b == B_SEL) first_choose();
        return;
    }
    if (ui.ask) {                        /* a question: PLAY yes, HOME no, nothing else */
        if (b == B_PLAY) ask_answer(1);
        if (b == B_HOME) ask_answer(0);
        return;
    }
    if (b == B_HOME) { set_page(ui.page == PG_MENU ? (ui.back ? ui.back : PG_TODAY) : PG_MENU); return; }
    switch (ui.page) {
    case PG_MENU:
        if (b == B_PLAY || b == B_SEL) set_page(ui.msel + 1);
        break;
    case PG_TODAY:
        if (b == B_PLAY) {
            if (pr.ps == PS_START || pr.ps == PS_DONE) today_start();
            else if (pr.ps == PS_CARD && pr.kind == SR_ASK && !synth_score_busy()) {
                if (pr.skill >= SK_EARDEG) pr.replays++;
                pr.answer_ok = 0;        /* listen first, again */
                play_prompt();
            }
        } else if (b == B_SEL) {
            if (pr.ps == PS_START || pr.ps == PS_DONE) today_start();
            else if (pr.ps == PS_CARD && pr.kind == SR_SHOW) show_next();
            else if (pr.ps == PS_CARD && pr.skill == SK_EARQ && pr.answer_ok)
                card_result(pr.pick == pr.right_opt ? grade_right() : SR_WRONG);
            else if (pr.ps == PS_CARD) card_result(SR_WRONG);   /* don't know */
            else if (pr.ps == PS_RESULT) pr.until = 0;
        } else if (b == B_SEQ && pr.ps == PS_CARD && pr.kind == SR_ASK) {
            card_result(SR_WRONG);       /* show me: the answer, counted wrong */
        }
        break;
    case PG_WHEEL:
        if (b == B_PLAY) {
            release_all();
            rec.keymode = (uint8_t)!rec.keymode;
            app_say("KEYS", rec.keymode ? "CHORDS" : "NOTES");
            changed(R_ALL);
        }
        break;
    case PG_CHORD:
        if (b == B_PLAY) {
            ui.nhist = 0;
            app_say("HISTORY", "CLEARED");
            ui.redraw |= R_MAIN | R_FOOT;
        }
        break;
    case PG_SET:
        if (b == B_SEL || b == B_PLAY) settings_change(1);
        break;
    default:
        break;
    }
    if (b == B_OCTDN || b == B_OCTUP) {
        int o = clamp(rec.oct + (b == B_OCTUP ? 1 : -1), -2, 2);
        if (o != rec.oct && ui.page != PG_TODAY) {
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

static void encoders(void)
{
    int32_t e;
    char t[32];
    if (ui.first) {
        if ((e = plat_enc(EN_SELECT)) != 0) { ui.fsel = (uint8_t)th_mod(ui.fsel + e, 5); ui.redraw |= R_MAIN; }
        (void)plat_enc(EN_ALGO);
        goto drain;
    }
    if (ui.ask) {
        (void)plat_enc(EN_SELECT);
        (void)plat_enc(EN_ALGO);
        goto drain;
    }
    if ((e = plat_enc(EN_ALGO)) != 0) set_page(th_mod(ui.page + e, PG_N));
    e = plat_enc(EN_SELECT);
    if (e) switch (ui.page) {
        case PG_MENU: ui.msel = (uint8_t)th_mod(ui.msel + e, PG_N - 1); ui.redraw |= R_MAIN; break;
        case PG_SET: ui.srow = (uint8_t)th_mod(ui.srow + e, SR_ROWS); ui.redraw |= R_MAIN; break;
        case PG_CHART: ui.ccur = (uint8_t)th_mod(ui.ccur + e, CO_KEYS); ui.redraw |= R_MAIN; break;
        case PG_TODAY:
            if (pr.ps == PS_START || pr.ps == PS_DONE) {
                pr.focus = (uint8_t)th_mod(pr.focus + e, FO_N);
                app_say("FOCUS", FOCUS_NAME[pr.focus]);
                ui.redraw |= R_MAIN;
            } else if (pr.ps == PS_CARD && pr.skill == SK_EARQ && pr.nopts) {
                pr.pick = (uint8_t)th_mod(pr.pick + e, pr.nopts);
                ui.redraw |= R_MAIN;
            }
            break;
        case PG_WHEEL:
        case PG_CHORD:
            set_tonic(rec.tonic + e);
            key_text(t);
            app_say("KEY", t);
            break;
        default: break;
        }
    if (ui.page == PG_SET && (e = plat_enc(EN_K1)) != 0) settings_change(e > 0 ? 1 : -1);
    if (ui.page == PG_WHEEL || ui.page == PG_CHORD) {
        if ((e = plat_enc(EN_K1)) != 0) {    /* the mode, the tonic kept (respelled if it must be) */
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
            changed(R_ALL);
        }
        if ((e = plat_enc(EN_K4)) != 0) {
            rec.mapping = (uint8_t)th_mod(rec.mapping + e, MAP_N);
            app_say("CHORD KEYS", rec.mapping == MAP_C ? "C IS I" : "LETTERS");
            changed(R_FOOT);
        }
    }
drain:
    (void)plat_enc(EN_PRESET);           /* the rest: their turns thrown away */
    (void)plat_enc(EN_K1);
    (void)plat_enc(EN_K2);
    (void)plat_enc(EN_K3);
    (void)plat_enc(EN_K4);
}

static void input(void)
{
    uint32_t keys, btn = plat_buttons(), k, now = plat_ms();
    touch_poll();
    keys = touch_keys();
    for (k = 0; k < NKEYS; k++) {
        uint32_t bit = 1u << k;
        if ((keys & bit) && !(ui.keys & bit)) {
            ui.down_at[k] = now;
            if (!ui.first && !ui.ask) key_down(k, touch_vel(k), btn);
            ui.keys |= bit;
        } else if (!(keys & bit) && (ui.keys & bit)) {
            ui.keys &= ~bit;
            if (!ui.first && !ui.ask) key_up(k, now - ui.down_at[k]);
            else key_release(k);
        }
    }
    for (k = 0; k < NB; k++)
        if ((btn >> k & 1u) && !(ui.btn >> k & 1u)) button(k);
    encoders();
    ui.keys = keys;
    ui.btn = btn;
}

void app_input_only(void) { input(); }

/* ---------------------------------------------------------------- lights --- */
static void lights(uint32_t now)
{
    ledfx_t kb = { 0, 0, 0 }, bb = { 0, 0, 0 };
    th_key_t key = cur_key();
    int mask = th_key_mask(key), tpc = th_pc(rec.tonic), k, i;
    for (k = 0; k < NB; k++)
        if (k != B_PLAY) ledfx_set(&bb, (uint32_t)k, FX_GLOW, now);
    if (rec.oct < 0) ledfx_set(&bb, B_OCTDN, FX_LIT, now);
    if (rec.oct > 0) ledfx_set(&bb, B_OCTUP, FX_LIT, now);
    for (k = 0; k < NKEYS; k++) {
        int pc = KEY_MIDI(k) % 12, fx = FX_OFF;
        if (ui.page == PG_WHEEL && rec.keymode == KM_CHORDS) {
            int letter = white_letter(pc);
            if (letter >= 0) {
                int degree = rec.mapping == MAP_C ? letter : th_mod(letter - th_letter(rec.tonic), 7);
                fx = degree == 0 ? FX_MID : FX_GLOW;
            }
        } else if (ui.page == PG_WHEEL && (mask >> pc & 1)) fx = pc == tpc ? FX_MID : FX_GLOW;
        else if (ui.page == PG_TODAY && pr.ps == PS_RESULT && (pr.want_pcs >> pc & 1u)) fx = FX_MID;   /* the answer, after */
        else if (ui.page == PG_TODAY && pr.ps == PS_CARD && pr.skill != SK_EARQ && (pr.tray >> pc & 1u)) fx = FX_GLOW;
        if (ui.keys >> k & 1u) fx = FX_LIT;
        ledfx_set(&kb, (uint32_t)k, fx, now);
    }
    if (ui.page == PG_TODAY && pr.ps == PS_CARD && pr.skill == SK_EARQ && pr.answer_ok)
        for (i = 0; i < pr.nopts; i++) ledfx_set(&kb, CHOICE_KEY[i], i == pr.pick ? FX_BLINK : FX_LIT, now);
    if (ui.page == PG_WHEEL && rec.keymode == KM_CHORDS) bb.lit |= LED_PLAY_GREEN;
    if (ui.ask) { ledfx_set(&bb, B_PLAY, FX_BLINK, now); ledfx_set(&bb, B_HOME, FX_BLINK, now); }
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
#define C_OK RGB(90, 210, 120)
#define C_MEH RGB(230, 180, 60)
#define C_BAD RGB(235, 90, 80)

static uint16_t level_rgb(int box)       /* Morse's colours for boxes 0..5 */
{
    static const uint16_t L[6] = { RGB(70, 40, 40), RGB(140, 60, 40), RGB(190, 120, 30), RGB(170, 170, 40),
                                   RGB(90, 180, 70), RGB(60, 210, 120) };
    return L[box < 0 ? 0 : box > 5 ? 5 : box];
}

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
    if (q == TH_MAJ || q == TH_7 || q == TH_MAJ7 || q == TH_6 || q == TH_ADD9 || q == TH_9 || q == TH_MAJ9 || q == TH_69
        || q == TH_7B9 || q == TH_7S9 || q == TH_13) return 1;
    if (q == TH_MIN || q == TH_M7 || q == TH_M6 || q == TH_M9 || q == TH_MADD9 || q == TH_MMAJ7) return 2;
    return 0;
}

static void label_bg(int32_t x, int32_t y, int32_t oy, const felucca_font_t *f, const char *s, uint16_t fg, uint16_t bg)
{
    int32_t w = gtext_w(f, s), tx = x - w / 2, ty = y - oy - (int32_t)f->h / 2;
    if (bg) cv_rect(tx - 3, ty, w + 6, (int32_t)f->h, bg);
    gtext(tx, ty, f, s, fg);
}

static void label(int32_t x, int32_t y, int32_t oy, const felucca_font_t *f, const char *s, int mark, int inner)
{
    uint16_t c = mark == 2 ? C_BLACK : mark == 1 ? C_HI : inner ? C_DIM : C_GRAY;
    label_bg(x, y, oy, f, s, c, mark == 2 ? C_HI : 0);
}

static void circle_names(int i, char *outer, char *inner)
{
    th_name(th_circle_tonic(i, 0), outer);
    th_name(th_circle_tonic(i, 1), inner);
    inner[0] = (char)(inner[0] + 32);    /* minor keys in lower case */
}

static void draw_wheel_at(int32_t oy)
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
        circle_names(i, t, u);
        label(ui.ox[i], ui.oy[i], oy, &FONT_B, t, out[i], 0);
        label(ui.ix[i], ui.iy[i], oy, &FONT_XS, u, in[i], 1);
    }
    if (ui.sh == SH_CHORD) {             /* the chord just played, where it sits */
        int fam = family(ui.id.c.q);
        if (fam == 1) ring(ui.ox[th_circle_pos(ui.id.c.root)], ui.oy[th_circle_pos(ui.id.c.root)], 14, oy, C_WHITE);
        if (fam == 2) ring(ui.ix[th_circle_pos(ui.id.c.root - 3)], ui.iy[th_circle_pos(ui.id.c.root - 3)], 12, oy, C_WHITE);
    }
    /* the centre: the key signature, its count and its letters */
    c = th_keysig(rec.tonic, MODES[rec.mode], sig);
    sig_text(rec.tonic, MODES[rec.mode], t);
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

static void draw_menu_at(int32_t oy)
{
    int i;
    for (i = 1; i < PG_N; i++) {
        int32_t y = 8 + (i - 1) * 32 - oy;
        int on = ui.msel == i - 1;
        if (on) cv_rect(4, y - 2, 232, 30, C_LINE);
        cv_text(14, y + 1, &FONT_B, PAGE_NAME[i], on ? C_WHITE : C_GRAY);
        cv_text(110, y + 4, &FONT_XS, PAGE_WHAT[i], on ? C_HI : C_DIM);
    }
}

static void draw_first_at(int32_t oy)    /* the first power-on: which page to start on */
{
    int i;
    cv_text(14, 6 - oy, &FONT_B, "START PAGE?", C_WHITE);
    cv_text(14, 26 - oy, &FONT_XS, "where THEORY opens from now on", C_GRAY);
    for (i = 0; i < 5; i++) {
        int32_t y = 50 + i * 24 - oy;
        int on = ui.fsel == i;
        if (on) cv_rect(10, y - 2, 220, 22, C_LINE);
        cv_text(20, y, &FONT_B, PAGE_NAME[i], on ? C_WHITE : C_GRAY);
        cv_text(110, y + 2, &FONT_XS, i ? PAGE_WHAT[i] : "choose each time", on ? C_HI : C_DIM);
    }
}

static void draw_set_at(int32_t oy)
{
    static const char *const ROW[SR_ROWS] = { "START PAGE", "CARDS A ROUND", "CADENCE" };
    char t[16];
    int i;
    for (i = 0; i < SR_ROWS; i++) {
        int32_t y = 10 + i * 30 - oy;
        int on = ui.srow == i;
        if (on) cv_rect(4, y - 3, 232, 26, C_LINE);
        cv_text(12, y, &FONT_B, ROW[i], on ? C_WHITE : C_GRAY);
        t[0] = 0;
        if (i == SR_START) tcat(t, rec.start ? PAGE_NAME[rec.start - 1] : "MENU");
        else if (i == SR_CARDS) tint(t, cards_n());
        else tcat(t, SPEED_NAME[rec.speed]);
        cv_text(228 - text_w(&FONT_B, t), y, &FONT_B, t, C_HI);
    }
    t[0] = 0;
    tcat(t, "THEORY ");
    tint(t, FW_MAJOR);
    tcat(t, ".");
    tint(t, FW_MINOR);
    cv_text(12, 140 - oy, &FONT_XS, t, C_DIM);
}

static void draw_ask_at(int32_t oy)      /* a backup to load */
{
    char t[40];
    cv_rect(8, 10 - oy, 224, 150, C_LINE);
    cv_text(20, 22 - oy, &FONT_B, "LOAD THIS BACKUP?", C_WHITE);
    t[0] = 0;
    tcat(t, "ROUND ");
    tint(t, xfer.st.round);
    tcat(t, ", ");
    tint(t, xfer.st.open);
    tcat(t, " OPEN");
    cv_text(20, 52 - oy, &FONT_XS, t, C_HI);
    cv_text(20, 72 - oy, &FONT_XS, "it replaces the progress and settings here", C_GRAY);
    cv_text(20, 120 - oy, &FONT_B, "PLAY: YES   HOME: NO", C_HI);
}

static void draw_chart_at(int32_t oy)
{
    char t[TH_CHORD_MAX];
    int i, g, s;
    for (i = 0; i < 96; i++) cv_pset(ui.rx[i], ui.ry[i] - oy, C_LINE);
    for (g = 0; g < CO_KEYS; g++) {      /* each key: its items' average box; dark until it opens */
        int pos = th_circle_pos(CO_TONIC[g]), sum = 0, n = 0, open = 0;
        uint16_t bg;
        for (i = 0; i < CO_N; i++)
            if (co_key[i] == g) {
                n++;
                if (i < prec.st.open) { open = 1; sum += prec.st.box[i]; }
            }
        th_name(CO_TONIC[g], t);
        bg = open ? level_rgb((sum + n / 2) / (n ? n : 1)) : RGB(20, 20, 24);
        label_bg(ui.ox[pos], ui.oy[pos], oy, &FONT_B, t, open ? C_WHITE : C_DIM, bg);
        if (g == ui.ccur) ring(ui.ox[pos], ui.oy[pos], 16, oy, C_WHITE);
    }
    /* the key under the cursor: each kind of card */
    th_name(CO_TONIC[ui.ccur], t);
    tcat(t, " major");
    gtext(WX - gtext_w(&FONT_XS, t) / 2, WY - 16 - oy, &FONT_XS, t, C_WHITE);
    for (s = 0; s < SK_EARQ; s++) {
        static const char *const SKN[SK_EARQ] = { "SCALE", "KEY SIG", "REL MIN", "CHORDS", "BY EAR" };
        int32_t y = 4 + s * 24 - oy;
        for (i = 0; i < CO_N; i++)
            if (co_key[i] == ui.ccur && co_skill[i] == s) break;
        cv_text(178, y, &FONT_T, SKN[s], C_GRAY);
        if (i < CO_N) {
            int box = prec.st.box[i], b;
            for (b = 0; b < 5; b++) cv_rect(178 + b * 12, y + 12, 10, 6, i < prec.st.open && b < box ? level_rgb(box) : C_LINE);
        } else cv_text(178, y + 10, &FONT_T, "-", C_DIM);
    }
    for (i = 0, s = 0; i < CO_N; i++)    /* chord qualities by ear, along the bottom right */
        if (co_skill[i] == SK_EARQ) {
            int32_t x = 178 + (s % 2) * 31, y = 128 + (s / 2) * 22 - oy;
            cv_rect(x, y, 29, 18, i < prec.st.open ? level_rgb(prec.st.box[i]) : RGB(20, 20, 24));
            cv_text(x + 3, y + 2, &FONT_T, qname(co_param[i]), i < prec.st.open ? C_WHITE : C_DIM);
            s++;
        }
}

static void draw_keyboard(int32_t y, int32_t oy, uint32_t held, uint16_t mark_pcs)   /* the 27 keys, small */
{
    static const uint8_t WK[16] = PLAT_WHITE_KEYS, BK[11] = PLAT_BLACK_KEYS;
    static const uint8_t AT[11] = { 0, 1, 2, 4, 5, 7, 8, 9, 11, 12, 14 };
    int i;
    for (i = 0; i < 16; i++) {
        uint32_t k = WK[i];
        uint16_t c = held >> k & 1u ? C_HI : (mark_pcs >> (KEY_MIDI(k) % 12) & 1u) ? C_GRAY : C_DIM;
        cv_rect(2 + i * 15, y - oy, 14, 26, c);
    }
    for (i = 0; i < 11; i++) {
        uint32_t k = BK[i];
        uint16_t c = held >> k & 1u ? C_HI : (mark_pcs >> (KEY_MIDI(k) % 12) & 1u) ? C_AMB : C_BLACK;
        cv_rect(2 + AT[i] * 15 + 10, y - oy, 9, 16, C_BLACK);
        cv_rect(2 + AT[i] * 15 + 11, y - oy, 7, 15, c == C_BLACK ? C_LINE : c);
    }
}

static void draw_chord_at(int32_t oy)    /* CHORD: what is played, at length */
{
    th_key_t key = cur_key();
    char t[TH_CHORD_MAX + 8], u[TH_ROMAN_MAX];
    int i;
    uint16_t pcs = 0;
    if (ui.sh == SH_CHORD) {
        int8_t f[TH_TONES_MAX];
        int n = th_chord(ui.id.c.root, ui.id.c.q, f);
        th_chord_name(ui.id.c, t);
        gtext(8, 2 - oy, &FONT_L, t, ui.id.inkey ? C_WHITE : C_MEH);
        numeral_text(ui.id.c, u);
        gtext(232 - gtext_w(&FONT_B, u), 8 - oy, &FONT_B, u, C_HI);
        if (ui.id.inv || ui.id.no5) {
            t[0] = 0;
            if (ui.id.inv) { tcat(t, "INV "); tint(t, ui.id.inv); }
            if (ui.id.no5) tcat(t, ui.id.inv ? "  NO 5" : "NO 5");
            cv_text(232 - text_w(&FONT_T, t), 28 - oy, &FONT_T, t, C_GRAY);
        }
        for (i = 0; i < n; i++) {        /* each tone: its name, what it is in the chord */
            int32_t x = 8 + i * 38;
            th_name(f[i], t);
            th_tone_label(ui.id.c.q, f[i] - ui.id.c.root, u);
            gtext(x, 42 - oy, &FONT_B, t, C_WHITE);
            cv_text(x, 60 - oy, &FONT_T, u, i ? C_AMB : C_HI);
            pcs |= (uint16_t)(1u << th_pc(f[i]));
        }
        if (ui.nalt) {
            t[0] = 0;
            tcat(t, "also ");
            for (i = 0; i < ui.nalt && i < 3; i++) {
                if (i) tcat(t, ", ");
                th_chord_name(ui.alt[i].c, u);
                tcat(t, u);
            }
            gtext(8, 78 - oy, &FONT_XS, t, C_GRAY);
        }
    } else if (ui.sh == SH_NOTE || ui.sh == SH_INTERVAL || ui.sh == SH_UNKNOWN) {
        for (i = 0; i < ui.nnotes && i < 6; i++) {
            th_name(th_spell_pc(ui.notes[i] % 12, rec.tonic), t);
            gtext(8 + i * 38, 8 - oy, &FONT_M, t, C_WHITE);
        }
        cv_text(8, 42 - oy, &FONT_XS, ui.sh == SH_NOTE ? "one note" : ui.sh == SH_INTERVAL ? "an interval" : "no chord I know",
                C_GRAY);
    } else {
        cv_text(8, 20 - oy, &FONT_B, "PLAY ANY NOTES", C_GRAY);
        cv_text(8, 42 - oy, &FONT_XS, "they are named in the key, with what each one is", C_DIM);
    }
    draw_keyboard(98, oy, ui.keys, pcs);
    /* the chords played, as numbers in the key */
    for (i = 0; i < ui.nhist; i++) {
        int32_t x = 8 + i * 58;
        th_chord_name(ui.hist[i], t);
        numeral_text(ui.hist[i], u);
        gtext(x, 132 - oy, &FONT_XS, t, C_GRAY);
        gtext(x, 148 - oy, &FONT_B, u, i == ui.nhist - 1 ? C_HI : C_WHITE);
    }
    (void)key;
}

static void draw_today_at(int32_t oy)
{
    char t[64], u[48];
    int i;
    if (pr.ps == PS_START || pr.ps == PS_DONE) {
        uint8_t w[8];
        int nw;
        static const uint32_t FOCUS_MASK[FO_N] = { 3u, 1u << SEG_THEORY, 1u << SEG_EAR };
        t[0] = 0;
        tcat(t, "ROUND ");
        tint(t, pr.ps == PS_DONE ? prec.st.round : prec.st.round + 1);
        if (pr.ps == PS_DONE) tcat(t, " DONE");
        cv_text(10, 6 - oy, &FONT_M, t, C_WHITE);
        t[0] = 0;
        if (pr.ps == PS_DONE) {
            tint(t, pr.r.ok); tcat(t, " RIGHT  ");
            tint(t, pr.r.miss); tcat(t, " MISSED  +");
            tint(t, pr.r.up);
        } else {
            tint(t, sr_due_count(&prec.st, &course, FOCUS_MASK[pr.focus])); tcat(t, " DUE  ");
            tint(t, sr_new_count(&prec.st, &course, FOCUS_MASK[pr.focus])); tcat(t, " NEW  ");
            tint(t, prec.st.open); tcat(t, " OPEN");
        }
        cv_text(10, 40 - oy, &FONT_XS, t, C_HI);
        t[0] = 0;
        tcat(t, "STREAK ");
        tint(t, prec.st.streak);
        tcat(t, "   FOCUS ");
        tcat(t, FOCUS_NAME[pr.focus]);
        cv_text(10, 60 - oy, &FONT_XS, t, C_GRAY);
        if (pr.ps == PS_DONE && pr.opened_key) {
            t[0] = 0;
            tcat(t, "NEW KEY: ");
            th_name(CO_TONIC[pr.opened_key - 1], u);
            tcat(t, u);
            tcat(t, " major");
            gtext(10, 84 - oy, &FONT_B, t, C_OK);
        } else if ((nw = sr_waiting(&prec.st, &course, w, 3)) > 0 && prec.st.open < CO_N) {
            cv_text(10, 84 - oy, &FONT_T, "THE NEXT KEY WAITS ON", C_DIM);
            for (i = 0; i < nw; i++) {
                item_name(w[i], u);
                gtext(10, 98 + i * 13 - oy, &FONT_T, u, C_GRAY);
            }
        }
        cv_text(10, 150 - oy, &FONT_B, "PLAY: START", C_HI);
        return;
    }
    t[0] = 0;                            /* the card */
    tint(t, pr.r.at + 1);
    tcat(t, " / ");
    tint(t, pr.r.n);
    cv_text(8, 2 - oy, &FONT_T, t, C_DIM);
    if (pr.kind == SR_SHOW) cv_text(60, 2 - oy, &FONT_T, "NEW", C_OK);
    t[0] = u[0] = 0;
    switch (pr.skill) {
    case SK_SCALE: th_name(pr.tonic, t); tcat(t, " major"); tcat(u, "its scale, up"); break;
    case SK_KEYSIG: th_name(pr.tonic, t); tcat(t, " major"); tcat(u, "its key signature"); break;
    case SK_RELMIN: th_name(pr.tonic, t); tcat(t, " major"); tcat(u, "its relative minor"); break;
    case SK_CHORDS: {
        th_key_t k;
        char r[TH_ROMAN_MAX];
        k.tonic = pr.tonic;
        k.scale = TH_MAJOR;
        th_roman_name(k, pr.chord, r);
        tcat(t, r);
        tcat(t, " in ");
        th_name(pr.tonic, r);
        tcat(t, r);
        tcat(u, "play the chord");
        break;
    }
    case SK_EARDEG: th_name(pr.tonic, t); tcat(t, " major"); tcat(u, pr.answer_ok ? "which note was it?" : "listen"); break;
    case SK_EARQ: tcat(t, "which chord?"); tcat(u, pr.answer_ok ? "E F G A, or SELECT + SEL" : "listen"); break;
    default: break;
    }
    gtext(8, 16 - oy, &FONT_M, t, C_WHITE);
    cv_text(8, 44 - oy, &FONT_XS, u, C_GRAY);
    if (pr.ps == PS_RESULT || pr.kind == SR_SHOW) {   /* the answer */
        if (pr.ps == PS_RESULT && pr.kind == SR_ASK)
            cv_text(8, 66 - oy, &FONT_L, RES_NAME[pr.res], pr.res == SR_WRONG ? C_BAD : pr.res == SR_SLOW ? C_MEH : C_OK);
        answer_text(t);
        gtext(8, 106 - oy, &FONT_B, t, C_HI);
        if (pr.kind == SR_SHOW) cv_text(8, 150 - oy, &FONT_T, "ANY KEY: NEXT", C_DIM);
        return;
    }
    if (pr.skill == SK_EARQ) {           /* the choices, on E4 F4 G4 A4 */
        static const char *const KN[4] = { "E", "F", "G", "A" };
        for (i = 0; i < pr.nopts; i++) {
            int32_t x = 4 + i * 59;
            int on = pr.pick == i;
            cv_rect(x, 74 - oy, 55, 44, on ? C_HI : C_LINE);
            label_bg(x + 27, 96, oy, &FONT_B, qname(pr.opts[i]), on ? C_BLACK : C_WHITE, 0);
            cv_text(x + 24, 122 - oy, &FONT_T, KN[i], C_DIM);
        }
        return;
    }
    /* what has been played so far */
    t[0] = 0;
    for (i = 0; i < 12; i++)
        if (pr.tray >> i & 1u) {
            int j, f = th_spell_pc(i, pr.tonic);
            for (j = 0; j < pr.nwant; j++)
                if (th_pc(pr.want[j]) == i) f = pr.want[j];
            th_name(f, u);
            if (t[0]) tcat(t, " ");
            tcat(t, u);
        }
    if (pr.skill == SK_SCALE) {
        t[0] = 0;
        for (i = 0; i < pr.step; i++) {
            th_name(pr.want[i], u);
            if (i) tcat(t, " ");
            tcat(t, u);
        }
    }
    gtext(8, 72 - oy, &FONT_B, t[0] ? t : "...", t[0] ? C_HI : C_DIM);
    if (pr.too_many) cv_text(8, 96 - oy, &FONT_XS, "too many: play one again to take it out", C_MEH);
    if (pr.slips) cv_text(8, 96 - oy, &FONT_XS, "one slip: go on from the right note", C_MEH);
    if (pr.retry) cv_text(8, 112 - oy, &FONT_XS, "once more", C_MEH);
}

static void draw_main(void)
{
    int h;
    for (h = 0; h < 2; h++) {
        int32_t oy = h * (MAIN_H / 2);
        cv_begin(240, MAIN_H / 2, C_BLACK);
        if (ui.first) draw_first_at(oy);
        else if (ui.ask) draw_ask_at(oy);
        else switch (ui.page) {
            case PG_MENU: draw_menu_at(oy); break;
            case PG_TODAY: draw_today_at(oy); break;
            case PG_WHEEL: draw_wheel_at(oy); break;
            case PG_CHORD: draw_chord_at(oy); break;
            case PG_CHART: draw_chart_at(oy); break;
            default: draw_set_at(oy); break;
            }
        cv_blit(0, (uint32_t)(32 + oy));
    }
}

static void draw_header(void)
{
    char t[40];
    int32_t x = 232;
    cv_begin(240, 32, C_BLACK);
    if (ui.page == PG_WHEEL && !ui.first && !ui.ask) {
        key_text(t);
        gtext(8, 7, &FONT_B, t, C_WHITE);
        t[0] = 0;
        tcat(t, rec.keymode == KM_CHORDS ? "CHORDS" : "NOTES");
        x -= text_w(&FONT_T, t);
        cv_text(x, 10, &FONT_T, t, rec.keymode == KM_CHORDS ? C_HI : C_GRAY);
    } else {
        cv_text(8, 7, &FONT_B, ui.first ? "THEORY" : PAGE_NAME[ui.page], C_WHITE);
        t[0] = 0;
        if (ui.page == PG_CHORD) key_text(t);
        if (ui.page == PG_TODAY || ui.page == PG_CHART) {
            tcat(t, "ROUND ");
            tint(t, prec.st.round);
        }
        if (t[0]) {
            x -= gtext_w(&FONT_XS, t);
            gtext(x, 9, &FONT_XS, t, C_GRAY);
        }
    }
    if (rec.oct && (ui.page == PG_WHEEL || ui.page == PG_CHORD)) {
        t[0] = 0;
        tcat(t, rec.oct < 0 ? "OCT-" : "OCT+");
        tint(t, rec.oct < 0 ? -rec.oct : rec.oct);
        x -= 8 + text_w(&FONT_T, t);
        cv_text(x, 10, &FONT_T, t, C_AMB);
    }
    if (hl_host(&link, plat_ms())) {
        x -= 8 + text_w(&FONT_T, "PC");
        cv_text(x, 10, &FONT_T, "PC", C_HI);
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
    const char *hint = 0;
    int32_t i;
    cv_begin(240, 32, C_BLACK);
    cv_rect(0, 0, 240, 1, C_LINE);
    if (plat_ms() < ui.toast_until) {
        int32_t x = gtext(8, 9, &FONT_T, ui.toast[0], C_GRAY);
        gtext(x + 8, 7, &FONT_B, ui.toast[1], C_HI);
    } else if (ui.first) hint = "SELECT: CHOOSE   PLAY: THAT ONE";
    else if (ui.ask) hint = "PLAY YES   HOME NO";
    else if (ui.page == PG_WHEEL) {
        footer_text(big, small);
        if (big[0]) {
            gtext(8, 3, &FONT_M, big, ui.id.inkey || ui.sh != SH_CHORD ? C_WHITE : C_AMB);
            gtext(232 - gtext_w(&FONT_B, small), 8, &FONT_B, small, C_HI);
        } else hint = rec.keymode == KM_CHORDS ? "WHITE KEYS PLAY THE KEY'S CHORDS" : "PLAY A CHORD";
    } else if (ui.page == PG_CHORD) {
        if (ui.nhist) {
            small[0] = 0;
            for (i = 0; i < ui.nhist; i++) {
                numeral_text(ui.hist[i], big);
                if (i) tcat(small, "  ");
                tcat(small, big);
            }
            gtext(8, 7, &FONT_B, small, C_WHITE);
            cv_text(232 - text_w(&FONT_T, "PLAY: CLEAR"), 10, &FONT_T, "PLAY: CLEAR", C_DIM);
        } else hint = "THE CHORDS YOU PLAY, AS NUMBERS";
    } else if (ui.page == PG_TODAY) {
        hint = pr.ps == PS_CARD ? (pr.skill >= SK_EARDEG ? "PLAY AGAIN   SEL DON'T KNOW   SEQ SHOW"
                                                         : "SEL DON'T KNOW   SEQ SHOW ME")
             : pr.ps == PS_RESULT ? "SEL: NEXT" : "SELECT: FOCUS";
    } else if (ui.page == PG_MENU) hint = "SELECT, PLAY   ALGO: PAGES";
    else if (ui.page == PG_CHART) hint = "SELECT: A KEY";
    else if (ui.page == PG_SET) hint = "SELECT: ROW   K1 / SEL: CHANGE";
    if (hint) cv_text(8, 10, &FONT_T, hint, C_DIM);
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
        tcat(t, PAGE_NAME[ui.page]);
        tcat(t, ". ");
        key_text(k);
        gt_ascii(k, a, sizeof a);
        tcat(t, a);
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
    zero(&pr, sizeof pr);
    zero(&xfer, sizeof xfer);
    saves = 0;
    dirty_at = 0;
    course_init();
    if (plat_store_load(OBJ_THEORY, &rec, sizeof rec) != (int)sizeof rec || !rec_valid(&rec)) {
        zero(&rec, sizeof rec);
        defaults();
    }
    if (plat_store_load(OBJ_PROGRESS, &prec, sizeof prec) != (int)sizeof prec || prec.magic != PROG_MAGIC
        || prec.ver != PROG_VER || prec.size != sizeof prec || !sr_valid(&prec.st, &course)) {
        zero(&prec, sizeof prec);
        sr_init(&prec.st, &course);
    }
    synth_init();
    synth_stop();
    layout();
    hl_init(&link, 0x54, 0x48, TH_PROTO);   /* "TH" */
    ui.first = rec.start == 0u;
    ui.fsel = PG_TODAY;
    ui.page = ui.first ? PG_MENU : (uint8_t)(rec.start - 1u);
    ui.back = ui.page == PG_MENU ? PG_TODAY : ui.page;
    pr.ps = PS_START;
}

void app_ui_init(void)
{
    touch_on(1);                         /* velocity from the keys */
    lcd_fill(0, 0, 240, 240, C_BLACK);
    ui.redraw = R_ALL;
}

void app_frame(void)
{
    static const uint8_t HELLO[3] = { TH_PROTO, FW_MAJOR, FW_MINOR };
    static uint32_t last_toast;
    static uint8_t host_was;
    uint32_t now = plat_ms();
    uint8_t host;
    input();
    if (ui.held_changed) {
        ui.held_changed = 0;
        identify_held();
    }
    if (ui.page == PG_TODAY && !ui.ask) today_step(now);
    hl_tick(&link, now, T_HELLO, T_HEARTBEAT, HELLO, 3);
    prog_pump();
    host = (uint8_t)hl_host(&link, now);
    if (host != host_was) {
        host_was = host;
        ui.redraw |= R_HEAD;
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
    else if (!strcmp(what, "page")) *got = ui.page;
    else if (!strcmp(what, "first")) *got = ui.first;
    else if (!strcmp(what, "start")) *got = rec.start;
    else if (!strcmp(what, "cards")) *got = cards_n();
    else if (!strcmp(what, "speed")) *got = rec.speed;
    else if (!strcmp(what, "ps")) *got = pr.ps;
    else if (!strcmp(what, "item")) *got = pr.item;
    else if (!strcmp(what, "kind")) *got = pr.kind;
    else if (!strcmp(what, "skill")) *got = pr.skill;
    else if (!strcmp(what, "res")) *got = pr.res;
    else if (!strcmp(what, "answer_ok")) *got = pr.answer_ok;
    else if (!strcmp(what, "busy")) *got = synth_score_busy();
    else if (!strcmp(what, "round")) *got = prec.st.round;
    else if (!strcmp(what, "open")) *got = prec.st.open;
    else if (!strcmp(what, "streak")) *got = prec.st.streak;
    else if (!strcmp(what, "answered")) *got = (int)prec.st.answered;
    else if (!strcmp(what, "right")) *got = (int)prec.st.right;
    else if (!strncmp(what, "box", 3)) *got = prec.st.box[atoi(what + 3) % CO_N];
    else if (!strcmp(what, "focus")) *got = pr.focus;
    else if (!strcmp(what, "ask")) *got = ui.ask;
    else if (!strcmp(what, "hist")) *got = ui.nhist;
    else if (!strcmp(what, "nalt")) *got = ui.nalt;
    else if (!strcmp(what, "tray")) *got = pr.tray;
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
    if (!strcmp(cmd, "answer")) {        /* answer right|wrong|slow|easy: the card now, graded so */
        int g = !strcmp(a, "wrong") ? SR_WRONG : !strcmp(a, "slow") ? SR_SLOW : !strcmp(a, "easy") ? SR_EASY : SR_RIGHT;
        if (pr.ps != PS_CARD) { printf("  FAIL answer: no card\n"); host_fails++; return 1; }
        if (pr.kind == SR_SHOW) show_next();
        else {
            card_result(g);
            pr.until = 0;
        }
        return 1;
    }
    if (!strcmp(cmd, "answer_all")) {    /* answer_all right|wrong: the rest of the round */
        int g = !strcmp(a, "wrong") ? SR_WRONG : SR_RIGHT;
        while (pr.ps == PS_CARD || pr.ps == PS_RESULT) {
            if (pr.ps == PS_CARD && pr.kind == SR_SHOW) { show_next(); continue; }
            if (pr.ps == PS_CARD) card_result(g);
            card_present();
        }
        return 1;
    }
    if (!strcmp(cmd, "setcard")) {       /* setcard ITEM: the card now asks that item */
        int it = atoi(a);
        if (pr.ps != PS_CARD || it < 0 || it >= CO_N) { printf("  FAIL setcard\n"); host_fails++; return 1; }
        pr.r.card[pr.r.at].item = (uint8_t)it;
        pr.r.card[pr.r.at].kind = SR_ASK;
        card_present();
        return 1;
    }
    if (strcmp(cmd, "expect_text")) return 0;
    /* expect_text key|chord|roman|nash|footer|rowN|answer|prompt|hist|also TEXT (_ a space; signs as # b o /o) */
    host_text(b, want, sizeof want);
    raw[0] = 0;
    if (!strcmp(a, "key")) key_text(raw);
    else if (!strcmp(a, "chord") && ui.sh == SH_CHORD) th_chord_name(ui.id.c, raw);
    else if (!strcmp(a, "roman") && ui.sh == SH_CHORD) th_roman_name(cur_key(), ui.id.c, raw);
    else if (!strcmp(a, "nash") && ui.sh == SH_CHORD) th_nashville_name(cur_key(), ui.id.c, raw);
    else if (!strcmp(a, "footer")) {
        footer_text(raw, small);
        if (small[0]) { tcat(raw, " "); tcat(raw, small); }
    } else if (!strncmp(a, "row", 3)) {
        th_chord_t ch;
        if (th_diatonic(cur_key(), atoi(a + 3), rec.sevenths, &ch)) th_chord_name(ch, raw);
    } else if (!strcmp(a, "answer")) answer_text(raw);
    else if (!strcmp(a, "item")) item_name(pr.item, raw);
    else if (!strcmp(a, "hist")) {
        int i;
        for (i = 0; i < ui.nhist; i++) {
            if (i) tcat(raw, " ");
            numeral_text(ui.hist[i], small);
            tcat(raw, small);
        }
    } else if (!strcmp(a, "also")) {
        int i;
        for (i = 0; i < ui.nalt; i++) {
            if (i) tcat(raw, " ");
            th_chord_name(ui.alt[i].c, small);
            tcat(raw, small);
        }
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
