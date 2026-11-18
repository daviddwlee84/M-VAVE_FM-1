/* SPDX-License-Identifier: GPL-3.0-or-later */
/* GUITAR: the NECK, SHAPE and LEARN pages (see app.h). The neck is fret.c; theory.c, synth.c and gtext.c are THEORY's.
 * Part of fm1-base's unity build, after the platform: gfx.c and announce.c are in scope. */
#include "gtext.h"
#define TH_GLYPH_SHARP GT_SHARP
#define TH_GLYPH_FLAT GT_FLAT
#define TH_GLYPH_DIM GT_DIM
#define TH_GLYPH_HDIM GT_HDIM
#include "theory.c"
#include "synth.c"
#include "gtext.c"
#include "ledfx.c"
#include "fret.c"
#include "lesson.c"

#define FW_MAJOR 0
#define FW_MINOR 2

/* ---------------------------------------------------------------- settings --- */
#define REC_MAGIC 0x31305447u            /* "GT01" */
#define REC_VER 2u                       /* 2: the LEARN progress appended (0.2) */
#define REC_V1_SIZE 24u

enum { PG_NECK, PG_SHAPE, PG_LEARN, PG_N };
enum { LB_DOTS, LB_NAMES, LB_DEGREES, LB_N };
static const char *const PAGE_NAME[PG_N] = { "NECK", "SHAPE", "LEARN" };
static const char *const LABEL_NAME[LB_N] = { "DOTS", "NAMES", "DEGREES" };
static const char *const FILTER_NAME[FR_NFILTERS] = { "EASY", "OPEN", "BARRE", "MOVABLE" };

/* the chord qualities SHAPE steps through with K1, the common ones first */
static const uint8_t QUALS[] = { TH_MAJ, TH_MIN, TH_7, TH_M7, TH_MAJ7, TH_SUS4, TH_SUS2, TH_5, TH_DIM, TH_AUG,
                                 TH_M7B5, TH_DIM7, TH_6, TH_M6, TH_7SUS4, TH_ADD9, TH_MADD9, TH_9, TH_MAJ9, TH_M9,
                                 TH_MMAJ7, TH_AUG7, TH_AUGMAJ7 };
#define NQUALS ((int)(sizeof QUALS / sizeof QUALS[0]))

typedef struct {
    uint32_t magic;
    uint16_t ver, size;
    int8_t tonic;                        /* NECK: the key's tonic on the line of fifths (theory.h) */
    uint8_t scale;                       /* TH_* scale */
    uint8_t labels;                      /* LB_* */
    uint8_t pos;                         /* 0 the whole neck, 1-5 CAGED C A G E D, 6.. NPS 1.. */
    uint8_t tuning;                      /* FR_* */
    uint8_t page;                        /* PG_* */
    int8_t oct;                          /* -2 .. +2: where the keys sound */
    int8_t root;                         /* SHAPE: the chord's root on the line of fifths */
    uint8_t qual;                        /* QUALS[] */
    uint8_t filter;                      /* SHAPE: FR_F_* (0.1 left it 0: EASY) */
    uint8_t rsv[4];
    ls_prog_t learn;                     /* LEARN: the boxes and the stage open (0.2) */
} rec_t;
static rec_t rec;
static uint32_t saves;
static uint32_t dirty_at;
static uint32_t ln_unsaved_at;           /* LEARN: an answer not on flash yet, saved after LN_SAVE_MS without one */

static void defaults(void)
{
    rec.tonic = 1;                       /* G major, the whole neck */
    rec.scale = TH_MAJOR;
    rec.labels = LB_DOTS;
    rec.pos = 0;
    rec.tuning = FR_STANDARD;
    rec.page = PG_NECK;
    rec.oct = 0;
    rec.root = 0;                        /* C */
    rec.qual = 0;
    ls_reset(&rec.learn);
}

static int rec_valid(const rec_t *r)
{
    return r->magic == REC_MAGIC && r->ver == REC_VER && r->size == sizeof *r && r->tonic >= -9 && r->tonic <= 12
           && r->scale < TH_NSCALES && r->labels < LB_N && r->pos < 16 && r->tuning < FR_NTUNINGS && r->page < PG_N
           && r->oct >= -2 && r->oct <= 2 && r->root >= -9 && r->root <= 12 && r->qual < NQUALS && r->filter < FR_NFILTERS
           && ls_valid(&r->learn);
}

static void rec_save(void)
{
    rec.magic = REC_MAGIC;
    rec.ver = REC_VER;
    rec.size = (uint16_t)sizeof rec;
    ln_unsaved_at = 0;
    if (plat_store_save(OBJ_GUITAR, &rec, sizeof rec) == 0)
        saves++;
    dirty_at = 0;
}

static th_key_t cur_key(void)
{
    th_key_t k;
    k.tonic = rec.tonic;
    k.scale = rec.scale;
    return k;
}

static const fr_tuning_t *tun(void) { return &fr_tunings[rec.tuning]; }

static th_chord_t cur_chord(void)
{
    th_chord_t c;
    c.root = rec.root;
    c.q = QUALS[rec.qual];
    c.bass = rec.root;
    return c;
}

/* positions: 0 the whole neck, then CAGED's five, then the NPS ones the scale has */
static int npos(void) { return 1 + 5 + fr_positions(cur_key(), FR_NPS); }

static int pos_system(int p, int *i)
{
    if (p == 0) { *i = 0; return FR_ALL; }
    if (p <= 5) { *i = p - 1; return FR_CAGED; }
    *i = p - 6;
    return FR_NPS;
}

/* --------------------------------------------------------------------- UI --- */
#define R_HEAD 1u
#define R_MAIN 2u
#define R_FOOT 4u
#define R_ALL 7u
#define MAIN_H 176
#define KEY_MIDI(k) (53 + (int)(k))      /* key 0 is F3 */

enum { SH_NONE, SH_NOTE, SH_CHORD, SH_UNKNOWN };

static struct {
    uint32_t keys, btn;
    uint8_t redraw;
    uint8_t knote[NKEYS];                /* the note each held key started (0: none) */
    uint8_t held_changed, best_n;
    uint8_t sh;                          /* SH_*: what was played (NECK) */
    th_ident_t id;
    int held_pcs;                        /* pitch classes of the keys held now: rings on the neck */
    uint8_t notes[12], nnotes;
    uint32_t mask[FR_STR_MAX];           /* NECK: the position's frets */
    int view;                            /* NECK: the first fret drawn */
    fr_shape_t shapes[FR_SHAPES_MAX];    /* SHAPE: the chord's shapes, easiest first */
    int nshapes, shape;
    /* PLAY: a strum or a scale run */
    uint8_t seq[40], nseq, iseq, seq_run, seq_on;
    uint32_t seq_at;
    int seq_s, seq_f;                    /* the run's note now, to mark on the neck */
    char toast[2][24];
    uint32_t toast_until;
    uint32_t lit, mid, glow;
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

static void key_text(char *t)            /* "G major" */
{
    th_name(rec.tonic, t);
    tcat(t, " ");
    tcat(t, th_scales[rec.scale].name);
}

static void pos_text(char *t)            /* "WHOLE NECK", "CAGED E / DO", "3NPS 1" */
{
    int i, sys = pos_system(rec.pos, &i);
    char one[2];
    t[0] = 0;
    if (sys == FR_ALL) tcat(t, "WHOLE NECK");
    else if (sys == FR_CAGED) {
        tcat(t, "CAGED ");
        one[0] = fr_caged_name[i];
        one[1] = 0;
        tcat(t, one);
        tcat(t, " / ");                  /* and its solfege name: CAGED E / DO */
        tcat(t, fr_solfege_name[i]);
    } else {
        tint(t, fr_nps(cur_key()));
        tcat(t, "NPS ");
        tint(t, i + 1);
    }
}

static void shape_text(char *t)          /* "x32010" */
{
    int s, n = tun()->n;
    t[0] = 0;
    if (ui.shape >= ui.nshapes) return;
    for (s = 0; s < n; s++) {
        int f = ui.shapes[ui.shape].fret[s];
        if (f < 0) tcat(t, "x");
        else if (f < 10) {
            char one[2];
            one[0] = (char)('0' + f);
            one[1] = 0;
            tcat(t, one);
        } else {
            tcat(t, "(");
            tint(t, f);
            tcat(t, ")");
        }
    }
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
    if (ann_on()) {
        gt_ascii(ui.toast[0], t, sizeof t);
        if (ui.toast[1][0]) {
            tcat(t, " ");
            gt_ascii(ui.toast[1], s, sizeof s);
            tcat(t, s);
        }
        ann_say(t, 0, ANN_NOW);
    }
}

static void changed(uint8_t r)
{
    ui.redraw |= r;
    dirty_at = plat_ms();
    if (!dirty_at) dirty_at = 1u;
}

/* the neck's position and the chord's shapes, worked out again when what they depend on changes */
static void neck_update(void)
{
    int i, sys, s, f, hi = 0;
    if (rec.pos >= npos()) rec.pos = 0;
    sys = pos_system(rec.pos, &i);
    if (!fr_position(tun(), cur_key(), sys, i, ui.mask)) fr_position(tun(), cur_key(), FR_ALL, 0, ui.mask);
    ui.view = 0;
    if (sys != FR_ALL) {
        int lo = 99;
        for (s = 0; s < tun()->n; s++)
            for (f = 0; f <= FR_FRETS; f++)
                if (ui.mask[s] >> f & 1u) {
                    if (f > hi) hi = f;
                    if (f < lo) lo = f;
                }
        if (hi > 12) ui.view = lo - 1 > hi - 12 ? hi - 12 : lo - 1;
        if (ui.view < 0) ui.view = 0;
    }
}

static void shapes_update(void)
{
    ui.nshapes = fr_shapes_filter(tun(), cur_chord(), rec.filter, ui.shapes, FR_SHAPES_MAX);
    ui.shape = 0;
}

/* -------------------------------------------------------------- the notes --- */
static void midi_out(int on, int note, int vel)
{
    if (note < 0 || note > 127) return;
    if (on) plat_midi_send(0x09u | 0x90u << 8 | (uint32_t)note << 16 | (uint32_t)vel << 24, TO_USB);
    else plat_midi_send(0x08u | 0x80u << 8 | (uint32_t)note << 16, TO_USB);
}

static void note_on(int note, int vel)
{
    synth_note_on(note, vel);
    midi_out(1, note, vel);
}

static void note_off(int note)
{
    synth_note_off(note);
    midi_out(0, note, 0);
}

static void seq_stop(void)
{
    int i;
    for (i = 0; i < ui.iseq && i < ui.nseq; i++) note_off(ui.seq[i]);
    ui.nseq = ui.iseq = 0;
    ui.seq_on = 0;
    ui.seq_s = -1;
    ui.redraw |= R_MAIN;
}

/* SHAPE and LEARN: strum a shape, low string first */
static void strum_shape(const fr_tuning_t *t, const fr_shape_t *sh)
{
    uint8_t m[FR_STR_MAX];
    int n, i;
    seq_stop();
    n = fr_shape_midi(t, sh, m);
    for (i = 0; i < n; i++) ui.seq[i] = m[i];
    ui.nseq = (uint8_t)n;
    ui.seq_run = 0;
    ui.seq_on = 1;
    ui.seq_at = plat_ms();
}

static void strum(void)
{
    if (ui.shape < ui.nshapes) strum_shape(tun(), &ui.shapes[ui.shape]);
    else seq_stop();
}

/* NECK: the position's notes, low to high */
static void scale_run(void)
{
    int s, f, n = 0;
    seq_stop();
    for (s = 0; s < tun()->n; s++)
        for (f = 0; f <= FR_FRETS && n < (int)sizeof ui.seq; f++)
            if (ui.mask[s] >> f & 1u) {
                int p = fr_midi(tun(), s, f), j, k;
                if (rec.pos == 0 && f > 12) continue;  /* the whole neck: what is on the screen */
                for (j = 0; j < n && ui.seq[j] != p; j++) {}
                if (j < n) continue;
                for (k = n; k > 0 && ui.seq[k - 1] > p; k--) ui.seq[k] = ui.seq[k - 1];
                ui.seq[k] = (uint8_t)p;
                n++;
            }
    ui.nseq = (uint8_t)n;
    ui.seq_run = 1;
    ui.seq_on = 1;
    ui.seq_at = plat_ms();
}

/* where the run's note n is on the neck: the lowest string of the position that has it */
static void run_mark(int p)
{
    int s, f;
    ui.seq_s = -1;
    for (s = 0; s < tun()->n; s++)
        for (f = 0; f <= FR_FRETS; f++)
            if ((ui.mask[s] >> f & 1u) && fr_midi(tun(), s, f) == p) {
                ui.seq_s = s;
                ui.seq_f = f;
                return;
            }
}

static void seq_tick(uint32_t now)
{
    if (!ui.seq_on || (int32_t)(now - ui.seq_at) < 0) return;
    if (ui.iseq < ui.nseq) {
        if (ui.seq_run && ui.iseq) note_off(ui.seq[ui.iseq - 1]);
        note_on(ui.seq[ui.iseq], 90);
        if (ui.seq_run) {
            run_mark(ui.seq[ui.iseq]);
            ui.redraw |= R_MAIN;
        }
        ui.iseq++;
        ui.seq_at = now + (ui.seq_run ? 180u : 28u);
        if (ui.iseq == ui.nseq) ui.seq_at = now + (ui.seq_run ? 400u : 1500u);
    } else
        seq_stop();
}

static void identify_held(void)
{
    uint8_t m[NKEYS];
    int n = 0, i;
    uint32_t k;
    for (k = 0; k < NKEYS; k++)
        if (ui.keys >> k & 1u) m[n++] = ui.knote[k];
    if (n == 0 || n < ui.best_n) return;
    ui.best_n = (uint8_t)n;
    ui.nnotes = (uint8_t)(n > 12 ? 12 : n);
    for (i = 0; i < ui.nnotes; i++) ui.notes[i] = m[i];
    if (n == 1) ui.sh = SH_NOTE;
    else if (th_identify(m, n, cur_key(), &ui.id)) ui.sh = SH_CHORD;
    else ui.sh = SH_UNKNOWN;
    if (rec.page == PG_SHAPE && ui.sh == SH_CHORD) {     /* that chord's shapes */
        for (i = 0; i < NQUALS && QUALS[i] != ui.id.c.q; i++) {}
        if (i < NQUALS && (ui.id.c.root != rec.root || i != rec.qual)) {
            rec.root = ui.id.c.root;
            rec.qual = (uint8_t)i;
            shapes_update();
            changed(R_ALL);
        }
    }
    ui.redraw |= R_MAIN | R_FOOT;
}

/* ----------------------------------------------------------------- LEARN --- */
/* A round of LS_ROUND cards from lesson.c. NAME: a chord's name, four shapes, SELECT picks one, PLAY answers.
 * NOTES: a shape without its name, play its notes on the keys (any octave, together or one by one while one key is
 * held), let go to answer. Then the answer is shown and strummed; PLAY goes on. Standard tuning only. */
enum { LN_ASK, LN_SHOWN, LN_END };
static const fr_tuning_t *const LSTD = &fr_tunings[FR_STANDARD];

static struct {
    uint8_t cards[LS_ROUND];
    int n, i;                            /* the round's cards, the one now */
    int item, kind, state;
    uint8_t choice[4];                   /* NAME: the items shown, in screen order */
    fr_shape_t sh[4];                    /* their shapes; NOTES: sh[0] is the item's */
    int pick, answer, right, nright, opened, played;
    uint32_t rng;
} ln;
#define LN_SAVE_MS 20000u                /* longer than the settings' 3 s: a round is 8 answers, not 8 saves */

static void ln_setup(int kind)
{
    int i;
    ln.item = ln.cards[ln.i];
    ln.kind = kind;
    ln.state = LN_ASK;
    ln.pick = 0;
    ln.played = 0;
    if (ln.kind == LS_NAME_TO_SHAPE) {
        uint8_t d[3];
        int nd = ls_distractors(&rec.learn, ln.item, &ln.rng, d);
        ln.answer = (int)(ls_rand(&ln.rng) % 4u);
        for (i = 0; i < 4; i++) ln.choice[i] = (uint8_t)ln.item;
        for (i = 0; i < nd; i++) ln.choice[i < ln.answer ? i : i + 1] = d[i];
        for (i = 0; i < 4; i++) ls_shape(ln.choice[i], &ln.sh[i]);
    } else {
        ln.answer = 0;
        ls_shape(ln.item, &ln.sh[0]);
    }
    ui.redraw |= R_ALL;
}

static void ln_card(void) { ln_setup(ls_card_kind(&rec.learn, ln.cards[ln.i], &ln.rng)); }

static void ln_round(void)
{
    ln.rng ^= plat_ms() * 2654435761u + rec.learn.rounds;
    ln.n = ls_round(&rec.learn, &ln.rng, ln.cards, LS_ROUND);
    ln.i = 0;
    ln.nright = 0;
    ln.opened = 0;
    if (ln.n) ln_card();
}

static void ln_answer(int right)
{
    fr_shape_t sh;
    char t[TH_CHORD_MAX];
    ln.right = right;
    ln.nright += right;
    if (ls_answer(&rec.learn, ln.item, right)) ln.opened = 1;
    ln_unsaved_at = plat_ms() | 1u;
    ln.state = LN_SHOWN;
    th_chord_name(ls_chord(ln.item), t);
    app_say(right ? "RIGHT" : "IT IS", t);
    if (ls_shape(ln.item, &sh)) strum_shape(LSTD, &sh);
    ui.redraw |= R_ALL;
}

static void ln_next(void)
{
    seq_stop();
    if (ln.state == LN_END || !ln.n) {
        ln_round();
        return;
    }
    if (++ln.i < ln.n) {
        ln_card();
        return;
    }
    ln.state = LN_END;
    rec.learn.rounds++;
    rec_save();
    if (ln.opened) app_say("NEW CHORDS", ls_stages[rec.learn.stage].what);
    ui.redraw |= R_ALL;
}

/* NOTES: every key let go, the notes played since the first key went down are the answer */
static void ln_notes_done(int pcs)
{
    if (rec.page != PG_LEARN || ln.state != LN_ASK || ln.kind != LS_SHAPE_TO_NOTES || !pcs) return;
    ln_answer(pcs == ls_pcs(ln.item));
}

/* ----------------------------------------------------------------- input --- */
static void set_tonic(int f)
{
    f = th_key_for_pc(th_pc(f), rec.scale);
    if (f == rec.tonic) return;
    rec.tonic = (int8_t)f;
    neck_update();
    changed(R_ALL);
}

static void set_root(int f)
{
    f = th_key_for_pc(th_pc(f), TH_MAJOR);   /* spelled as the major key would be: Bb, F#, Db */
    if (f == rec.root) return;
    rec.root = (int8_t)f;
    shapes_update();
    changed(R_ALL);
}

static void chord_say(void)
{
    char t[TH_CHORD_MAX];
    th_chord_name(cur_chord(), t);
    app_say("CHORD", t);
}

static void set_page(int p)
{
    seq_stop();
    rec.page = (uint8_t)th_mod(p, PG_N);
    ui.sh = SH_NONE;
    if (rec.page == PG_LEARN && !ln.n) ln_round();
    app_say("PAGE", PAGE_NAME[rec.page]);
    changed(R_ALL);
}

static void input(void)
{
    uint32_t keys, btn = plat_buttons(), k;
    int32_t e;
    char t[40];
    touch_poll();
    keys = touch_keys();
    for (k = 0; k < NKEYS; k++) {
        uint32_t bit = 1u << k;
        if ((keys & bit) && !(ui.keys & bit)) {
            if ((btn >> B_SEL & 1u) && rec.page != PG_LEARN) {     /* SEL held: this note becomes the key's tonic, or the chord's root */
                if (rec.page == PG_NECK) {
                    set_tonic(th_spell_pc(KEY_MIDI(k) % 12, 0));
                    key_text(t);
                    app_say("KEY", t);
                } else {
                    set_root(th_spell_pc(KEY_MIDI(k) % 12, 0));
                    chord_say();
                }
                continue;
            }
            if (!ui.keys) ui.best_n = 0;
            ui.knote[k] = (uint8_t)(KEY_MIDI(k) + 12 * rec.oct);
            note_on(ui.knote[k], touch_vel(k));
            ui.held_changed = 1;
            if (rec.page == PG_LEARN) ln.played |= 1 << (ui.knote[k] % 12);
        } else if (!(keys & bit) && (ui.keys & bit)) {
            if (ui.knote[k]) note_off(ui.knote[k]);
            ui.knote[k] = 0;
        }
    }
    if (keys != ui.keys) {
        int h = 0;
        for (k = 0; k < NKEYS; k++)
            if (ui.knote[k]) h |= 1 << (ui.knote[k] % 12);
        if (h != ui.held_pcs) ui.redraw |= R_MAIN;
        ui.held_pcs = h;
        if (!keys && rec.page == PG_LEARN) {     /* every key let go: a NOTES card is answered */
            h = ln.played;
            ln.played = 0;
            ln_notes_done(h);
        }
    }
    ui.keys = keys;
    for (k = 0; k < NB; k++) {
        uint32_t bit = 1u << k;
        if (!(btn & bit) || (ui.btn & bit)) continue;
        if (k == B_HOME) {               /* the next page (ALGO turns either way, as in THEORY and Morse) */
            set_page(rec.page + 1);
        } else if (k == B_PLAY) {
            if (rec.page == PG_LEARN) {
                if (ln.state != LN_ASK) ln_next();
                else if (ln.kind == LS_NAME_TO_SHAPE) ln_answer(ln.pick == ln.answer);
                else strum_shape(LSTD, &ln.sh[0]);       /* NOTES: hear it, as often as wanted */
            } else if (ui.seq_on) seq_stop();
            else if (rec.page == PG_SHAPE) strum();
            else scale_run();
        } else if (k == B_OCTDN || k == B_OCTUP) {
            int o = clamp(rec.oct + (k == B_OCTUP ? 1 : -1), -2, 2);
            if (o != rec.oct) {
                rec.oct = (int8_t)o;
                t[0] = 0;
                tcat(t, o < 0 ? "-" : "+");
                tint(t, o < 0 ? -o : o);
                app_say("OCTAVE", t);
                changed(R_HEAD);
            }
        }
    }
    if (rec.page == PG_LEARN) {          /* LEARN: SELECT picks a shape; the knobs rest */
        if ((e = plat_enc(EN_SELECT)) != 0 && ln.state == LN_ASK && ln.kind == LS_NAME_TO_SHAPE) {
            ln.pick = (int)th_mod(ln.pick + e, 4);
            ui.redraw |= R_MAIN;
        }
        (void)plat_enc(EN_K1);
        (void)plat_enc(EN_K2);
        (void)plat_enc(EN_K3);
        (void)plat_enc(EN_K4);
    } else if ((e = plat_enc(EN_SELECT)) != 0) {
        if (rec.page == PG_NECK) {       /* around the circle of fifths */
            set_tonic(rec.tonic + e);
            key_text(t);
            app_say("KEY", t);
        } else if (ui.nshapes) {         /* the next shape */
            ui.shape = (int)th_mod(ui.shape + e, ui.nshapes);
            seq_stop();
            t[0] = 0;
            tint(t, ui.shape + 1);
            tcat(t, " OF ");
            tint(t, ui.nshapes);
            app_say("SHAPE", t);
            ui.redraw |= R_MAIN | R_FOOT;
        }
    }
    if ((e = plat_enc(EN_K1)) != 0) {
        if (rec.page == PG_NECK) {       /* the scale, the tonic kept (respelled if it must be) */
            rec.scale = (uint8_t)th_mod(rec.scale + e, TH_NSCALES);
            rec.tonic = (int8_t)th_key_for_pc(th_pc(rec.tonic), rec.scale);
            neck_update();
            app_say("SCALE", th_scales[rec.scale].name);
        } else {                         /* the chord's quality */
            rec.qual = (uint8_t)th_mod(rec.qual + e, NQUALS);
            shapes_update();
            chord_say();
        }
        changed(R_ALL);
    }
    if ((e = plat_enc(EN_K2)) != 0) {
        if (rec.page == PG_NECK) {       /* dots, note names, degrees */
            rec.labels = (uint8_t)th_mod(rec.labels + e, LB_N);
            app_say("LABELS", LABEL_NAME[rec.labels]);
        } else {                         /* the chord's root, a semitone a detent */
            set_root(th_spell_pc(th_pc(rec.root) + e, 0));
            chord_say();
        }
        changed(R_MAIN | R_FOOT);
    }
    if ((e = plat_enc(EN_K3)) != 0 && rec.page == PG_SHAPE) {    /* which shapes: easy, open, barre, movable */
        rec.filter = (uint8_t)th_mod(rec.filter + e, FR_NFILTERS);
        shapes_update();
        seq_stop();
        app_say("SHAPES", FILTER_NAME[rec.filter]);
        changed(R_MAIN | R_FOOT);
    } else if (e != 0) {                 /* NECK: the position */
        rec.pos = (uint8_t)th_mod(rec.pos + e, npos());
        neck_update();
        seq_stop();
        pos_text(t);
        app_say("POSITION", t);
        changed(R_MAIN | R_FOOT);
    }
    if ((e = plat_enc(EN_K4)) != 0) {    /* the instrument and tuning */
        rec.tuning = (uint8_t)th_mod(rec.tuning + e, FR_NTUNINGS);
        seq_stop();
        neck_update();
        shapes_update();
        app_say("TUNING", tun()->name);
        changed(R_ALL);
    }
    if ((e = plat_enc(EN_ALGO)) != 0) set_page(rec.page + e);   /* pages: ALGO, as THEORY and Morse */
    (void)plat_enc(EN_PRESET);
    ui.btn = btn;
}

void app_input_only(void) { input(); }

/* ---------------------------------------------------------------- lights --- */
static void lights(uint32_t now)
{
    ledfx_t kb = { 0, 0, 0 }, bb = { 0, 0, 0 };
    int mask, tpc, k;
    if (rec.page == PG_NECK) {
        mask = fr_scale_pcs(cur_key());
        tpc = th_pc(rec.tonic);
    } else if (rec.page == PG_LEARN) {   /* the answer's notes once it is shown, nothing before */
        mask = ln.n && ln.state == LN_SHOWN ? ls_pcs(ln.item) : 0;
        tpc = ln.n ? th_pc(ls_items[ln.item].root) : -1;
    } else {
        int8_t f[TH_TONES_MAX];
        int n = th_chord(cur_chord().root, cur_chord().q, f), i;
        mask = 0;
        for (i = 0; i < n; i++) mask |= 1 << th_pc(f[i]);
        tpc = th_pc(rec.root);
    }
    ledfx_set(&bb, B_HOME, FX_GLOW, now);
    ledfx_set(&bb, B_SEL, FX_GLOW, now);
    if (rec.oct < 0) ledfx_set(&bb, B_OCTDN, FX_LIT, now);
    if (rec.oct > 0) ledfx_set(&bb, B_OCTUP, FX_LIT, now);
    for (k = 0; k < NKEYS; k++) {
        int pc = KEY_MIDI(k) % 12, fx = FX_OFF;
        if (mask >> pc & 1) fx = pc == tpc ? FX_MID : FX_GLOW;
        if (ui.keys >> k & 1u) fx = FX_LIT;
        ledfx_set(&kb, (uint32_t)k, fx, now);
    }
    if (ui.seq_on) bb.lit |= LED_PLAY_GREEN;
    ui.lit = kb.lit;
    ui.mid = kb.mid;
    ui.glow = kb.dim;
    ledfx_show(&bb, &kb);
}

/* ---------------------------------------------------------------- screen --- */
static void disc(int32_t x, int32_t y, int32_t r, uint16_t c)
{
    int32_t dy;
    for (dy = -r; dy <= r; dy++) {
        int32_t w = 0;
        while ((w + 1) * (w + 1) + dy * dy <= r * r + r / 2) w++;
        cv_rect(x - w, y + dy, 2 * w + 1, 1, c);
    }
}

static void circle(int32_t x, int32_t y, int32_t r, uint16_t c)
{
    int32_t dy;
    for (dy = -r; dy <= r; dy++) {
        int32_t w = 0;
        while ((w + 1) * (w + 1) + dy * dy <= r * r + r / 2) w++;
        cv_pset(x - w, y + dy, c);
        cv_pset(x + w, y + dy, c);
    }
    for (dy = -r; dy <= r; dy++) {
        int32_t w = 0;
        while ((w + 1) * (w + 1) + dy * dy <= r * r + r / 2) w++;
        cv_pset(x + dy, y - w, c);
        cv_pset(x + dy, y + w, c);
    }
}

static void ctext(int32_t x, int32_t y, const felucca_font_t *f, const char *s, uint16_t c)  /* centred on x, y */
{
    gtext(x - gtext_w(f, s) / 2, y - (int32_t)f->h / 2, f, s, c);
}

/* NECK: the high string on top, as tab is written. 13 columns: the nut (or the view's first fret) and 12 frets */
#define NK_X0 28                         /* the nut */
#define NK_W 17                          /* a fret */
#define NK_Y0 12

static int32_t nk_x(int slot) { return slot == 0 ? NK_X0 - 8 : NK_X0 + NK_W * slot - NK_W / 2; }

static int32_t nk_gap(void) { return tun()->n > 1 ? (146 - NK_Y0) / (tun()->n - 1) : 0; }

static int32_t nk_y(int s) { return NK_Y0 + nk_gap() * (tun()->n - 1 - s); }

static void draw_neck_at(int32_t oy)
{
    const fr_tuning_t *t = tun();
    th_key_t key = cur_key();
    int pcs = fr_scale_pcs(key), tpc = th_pc(rec.tonic), s, slot, i;
    char lab[TH_NAME_MAX];
    /* the strings, the frets, their numbers and inlays */
    for (s = 0; s < t->n; s++) {
        int32_t y = nk_y(s) - oy;
        cv_rect(NK_X0, y, NK_W * 12, 1, C_DIM);
        th_name(th_spell_pc(t->open[s] % 12, rec.tonic), lab);
        gtext(2, y - 6, &FONT_XS, lab, C_GRAY);
    }
    for (slot = 0; slot <= 12; slot++) {
        int32_t x = NK_X0 + NK_W * slot, f = ui.view + slot;
        if (slot == 0) cv_rect(x - 1, nk_y(t->n - 1) - oy, ui.view ? 1 : 3, nk_y(0) - nk_y(t->n - 1) + 1,
                               ui.view ? C_DIM : C_GRAY);
        else cv_rect(x, nk_y(t->n - 1) - oy, 1, nk_y(0) - nk_y(t->n - 1) + 1, C_LINE);
        if (slot && (f % 12 == 3 || f % 12 == 5 || f % 12 == 7 || f % 12 == 9 || f % 12 == 0)) {
            lab[0] = 0;
            tint(lab, f);
            ctext(nk_x(slot), 160 - oy, &FONT_XS, lab, f % 12 == 0 ? C_GRAY : C_DIM);
        } else if (!slot && ui.view) {
            lab[0] = 0;
            tint(lab, f);
            ctext(nk_x(slot), 160 - oy, &FONT_XS, lab, C_GRAY);
        }
    }
    /* the notes */
    for (s = 0; s < t->n; s++)
        for (slot = 0; slot <= 12; slot++) {
            int f = ui.view + slot, p = fr_midi(t, s, f), pc = p % 12, in = (int)(ui.mask[s] >> f & 1u);
            int32_t x = nk_x(slot), y = nk_y(s) - oy;
            uint16_t c = pc == tpc ? C_HI : C_AMB;
            if (!(pcs >> pc & 1)) {
                if (ui.held_pcs >> pc & 1) circle(x, y, 6, C_WHITE);    /* played, outside the key */
                continue;
            }
            if (!in) {
                cv_rect(x - 1, y - 1, 3, 3, C_DIM);
            } else if (rec.labels == LB_DOTS) {
                disc(x, y, pc == tpc ? 6 : 5, c);
            } else {
                if (rec.labels == LB_NAMES) th_name(th_spell_pc(pc, rec.tonic), lab);
                else th_degree_name(key, th_spell_pc(pc, rec.tonic), lab);
                int32_t w = gtext_w(&FONT_XS, lab);
                cv_rect(x - w / 2 - 1, y - 6, w + 2, 13, C_BLACK);
                ctext(x, y, &FONT_XS, lab, c);
            }
            if (ui.held_pcs >> pc & 1) circle(x, y, 8, C_WHITE);
            if (ui.seq_on && ui.seq_run && ui.seq_s == s && ui.seq_f == f) circle(x, y, 9, C_WHITE);
        }
    (void)i;
}

/* a closed shape's form: E FORM (the root on the 6th string, barred), A FORM (5th), D FORM (4th); a power chord's
 * root string. Standard-tuned six strings only (empty otherwise) */
static void form_text(const fr_shape_t *sh, char *t)
{
    int rs = fr_shape_root_string(tun(), sh);
    t[0] = 0;
    if (rec.tuning != FR_STANDARD && rec.tuning != FR_DROPD) return;
    if (fr_shape_open(sh, tun()->n) || rs < 0 || rs > 2) return;
    if (QUALS[rec.qual] == TH_5) {
        tcat(t, "ROOT ON ");
        tint(t, 6 - rs);
        return;
    }
    if (!sh->barre) return;
    tcat(t, rs == 0 ? "E FORM" : rs == 1 ? "A FORM" : "D FORM");
}

/* SHAPE: a chord diagram as chord books draw it, the low string on the left, the nut on top */
#define SH_X0 22
#define SH_Y0 30
#define SH_ROW 25                        /* a fret */
#define SH_ROWS 5

static void draw_shape_at(int32_t oy)
{
    const fr_tuning_t *t = tun();
    const fr_shape_t *sh = ui.shape < ui.nshapes ? &ui.shapes[ui.shape] : 0;
    th_chord_t c = cur_chord();
    int32_t gap = (110 / (t->n - 1)), x, y;
    int s, r, start = 1, rpc = th_pc(c.root);
    char lab[TH_CHORD_MAX];
    int8_t tones[TH_TONES_MAX];
    int nt = th_chord(c.root, c.q, tones), i;
    if (sh && (fr_shape_hi(sh, t->n) > SH_ROWS || (!fr_shape_open(sh, t->n) && fr_shape_lo(sh, t->n) > 1)))
        start = fr_shape_lo(sh, t->n);   /* a movable shape is drawn from its own fret, as chord books do */
    /* the grid */
    for (s = 0; s < t->n; s++) cv_rect(SH_X0 + gap * s, SH_Y0 - oy, 1, SH_ROW * SH_ROWS + 1, C_GRAY);
    for (r = 0; r <= SH_ROWS; r++)
        cv_rect(SH_X0, SH_Y0 + SH_ROW * r - oy - (r == 0 && start == 1 ? 3 : 0), gap * (t->n - 1) + 1,
                r == 0 && start == 1 ? 4 : 1, r == 0 && start == 1 ? C_WHITE : C_GRAY);
    if (start > 1) {
        lab[0] = 0;
        tint(lab, start);
        cv_text(2, SH_Y0 + SH_ROW / 2 - 6 - oy, &FONT_T, lab, C_HI);
    }
    if (!sh) {
        ctext(SH_X0 + gap * (t->n - 1) / 2, SH_Y0 + 60 - oy, &FONT_B, "NO SHAPE", C_AMB);
    } else {
        if (sh->barre) {                 /* the barre: a bar under one finger */
            y = SH_Y0 + SH_ROW * (sh->barre - start) + SH_ROW / 2 - oy;
            cv_rect(SH_X0 + gap * sh->bfrom - 4, y - 4, gap * (sh->bto - sh->bfrom) + 9, 9, C_AMB);
        }
        for (s = 0; s < t->n; s++) {
            int f = sh->fret[s], p, pc;
            x = SH_X0 + gap * s;
            if (f < 0) {                 /* x above a muted string */
                y = SH_Y0 - 12 - oy;
                cv_line(x - 4, y - 4, x + 4, y + 4, C_GRAY);
                cv_line(x - 4, y + 4, x + 4, y - 4, C_GRAY);
                continue;
            }
            p = fr_midi(t, s, f);
            pc = p % 12;
            if (f == 0) circle(x, SH_Y0 - 12 - oy, 4, pc == rpc ? C_HI : C_GRAY);
            else {
                y = SH_Y0 + SH_ROW * (f - start) + SH_ROW / 2 - oy;
                disc(x, y, 9, pc == rpc ? C_HI : C_AMB);
                lab[0] = (char)('0' + sh->finger[s]);
                lab[1] = 0;
                ctext(x + 1, y, &FONT_T, lab, C_BLACK);
            }
            /* the note's name below the grid */
            th_name(th_spell_pc(pc, c.root), lab);
            ctext(x, SH_Y0 + SH_ROW * SH_ROWS + 12 - oy, &FONT_XS, lab, pc == rpc ? C_HI : C_GRAY);
        }
    }
    /* the right side: the name, the tones, which shape */
    x = 152;
    th_chord_name(c, lab);
    gtext(x, 4 - oy, &FONT_M, lab, C_WHITE);
    for (i = 0; i < nt; i++) {
        char nm[TH_NAME_MAX];
        th_name(tones[i], nm);
        gtext(x + 22 * (i % 4), 40 + 16 * (i / 4) - oy, &FONT_B, nm, th_pc(tones[i]) == rpc ? C_HI : C_AMB);
    }
    if (sh) {
        lab[0] = 0;
        tint(lab, ui.shape + 1);
        tcat(lab, " / ");
        tint(lab, ui.nshapes);
        cv_text(x, 80 - oy, &FONT_T, lab, C_GRAY);
        lab[0] = 0;
        if (sh->barre) {
            tcat(lab, "BARRE ");
            tint(lab, sh->barre);
        } else if (fr_shape_open(sh, t->n)) tcat(lab, "OPEN");
        else {
            tcat(lab, "FRET ");
            tint(lab, fr_shape_lo(sh, t->n));
        }
        cv_text(x, 96 - oy, &FONT_T, lab, C_GRAY);
        form_text(sh, lab);
        if (lab[0]) cv_text(x, 124 - oy, &FONT_T, lab, C_AMB);
        if (sh->no5) cv_text(x, 110 - oy, &FONT_T, "NO 5TH", C_DIM);
        shape_text(lab);
        cv_text(x, 140 - oy, &FONT_T, lab, C_DIM);
    }
    cv_text(x, 156 - oy, &FONT_T, FILTER_NAME[rec.filter], rec.filter ? C_HI : C_DIM);
}

/* LEARN: a chord diagram anywhere, any size (standard tuning). fingers: the finger numbers in the dots */
static void diagram(int32_t x0, int32_t y0, int32_t oy, int32_t gap, int32_t row, int rows, int32_t dot,
                    const fr_shape_t *sh, int rpc, int fingers)
{
    int s, r, start = 1, n = LSTD->n;
    int32_t x, y, mk = dot / 2 + 1;
    char lab[4];
    if (fr_shape_hi(sh, n) > rows || (!fr_shape_open(sh, n) && fr_shape_lo(sh, n) > 1)) start = fr_shape_lo(sh, n);
    for (s = 0; s < n; s++) cv_rect(x0 + gap * s, y0 - oy, 1, row * rows + 1, C_GRAY);
    for (r = 0; r <= rows; r++)
        cv_rect(x0, y0 + row * r - oy - (r == 0 && start == 1 ? 2 : 0), gap * (n - 1) + 1, r == 0 && start == 1 ? 3 : 1,
                r == 0 && start == 1 ? C_WHITE : C_GRAY);
    if (start > 1) {
        lab[0] = 0;
        tint(lab, start);
        cv_text(x0 - 6 - text_w(fingers ? &FONT_T : &FONT_XS, lab), y0 + row / 2 - 6 - oy, fingers ? &FONT_T : &FONT_XS,
                lab, C_HI);
    }
    if (sh->barre) {
        y = y0 + row * (sh->barre - start) + row / 2 - oy;
        cv_rect(x0 + gap * sh->bfrom - dot / 2, y - dot / 2, gap * (sh->bto - sh->bfrom) + dot + 1, dot, C_AMB);
    }
    for (s = 0; s < n; s++) {
        int f = sh->fret[s], pc;
        x = x0 + gap * s;
        y = y0 - mk - 4 - oy;
        if (f < 0) {
            cv_line(x - mk, y - mk, x + mk, y + mk, C_GRAY);
            cv_line(x - mk, y + mk, x + mk, y - mk, C_GRAY);
            continue;
        }
        pc = fr_midi(LSTD, s, f) % 12;
        if (f == 0) circle(x, y, mk, pc == rpc ? C_HI : C_GRAY);
        else {
            y = y0 + row * (f - start) + row / 2 - oy;
            disc(x, y, dot, pc == rpc ? C_HI : C_AMB);
            if (fingers) {
                lab[0] = (char)('0' + sh->finger[s]);
                lab[1] = 0;
                ctext(x + 1, y, &FONT_T, lab, C_BLACK);
            }
        }
    }
}

static void frame(int32_t x, int32_t y, int32_t w, int32_t h, int32_t t, uint16_t c)
{
    cv_rect(x, y, w, t, c);
    cv_rect(x, y + h - t, w, t, c);
    cv_rect(x, y, t, h, c);
    cv_rect(x + w - t, y, t, h, c);
}

static void draw_learn_at(int32_t oy)
{
    int i, rpc;
    char lab[TH_CHORD_MAX];
    if (!ln.n) return;
    rpc = th_pc(ls_items[ln.item].root);
    if (ln.state == LN_END) {            /* the round's score, then every chord open: a bar per box */
        lab[0] = 0;
        tint(lab, ln.nright);
        tcat(lab, " / ");
        tint(lab, ln.n);
        ctext(120, 30 - oy, &FONT_M, lab, ln.nright == ln.n ? C_HI : C_WHITE);
        ctext(120, 60 - oy, &FONT_T, ln.opened ? "NEW CHORDS" : "RIGHT", ln.opened ? C_HI : C_GRAY);
        if (ln.opened) ctext(120, 78 - oy, &FONT_T, ls_stages[rec.learn.stage].what, C_AMB);
        for (i = 0; i < ls_nitems; i++) {
            int32_t x = 15 + (i % 21) * 10, b = rec.learn.box[i];
            if (!ls_open(&rec.learn, i)) {
                cv_rect(x, 140 - oy, 7, 1, C_LINE);
                continue;
            }
            cv_rect(x, 140 - 6 * b - oy, 7, 6 * b + 1, b >= LS_PASS ? C_HI : C_AMB);
        }
        ctext(120, 156 - oy, &FONT_XS, "BOX PER CHORD", C_DIM);
        return;
    }
    if (ln.kind == LS_NAME_TO_SHAPE) {   /* four shapes, two by two */
        for (i = 0; i < 4; i++) {
            int32_t cx = (i % 2) * 120, cy = (i / 2) * 88;
            diagram(cx + 30, cy + 16, oy, 12, 15, 4, 4, &ln.sh[i], ln.state == LN_SHOWN ? th_pc(ls_items[ln.choice[i]].root)
                                                                                        : -1, 0);
            if (ln.state == LN_ASK && i == ln.pick) frame(cx + 2, cy + 1 - oy, 116, 86, 2, C_HI);
            if (ln.state == LN_SHOWN) {
                if (i == ln.answer) frame(cx + 2, cy + 1 - oy, 116, 86, 3, C_WHITE);
                else if (i == ln.pick && !ln.right) frame(cx + 2, cy + 1 - oy, 116, 86, 1, C_GRAY);
                th_chord_name(ls_chord(ln.choice[i]), lab);
                gtext(cx + 96, cy + 36 - oy, &FONT_T, lab, i == ln.answer ? C_WHITE : C_GRAY);
            }
        }
        return;
    }
    /* NOTES: one big shape; the notes played so far, then the chord's */
    diagram(SH_X0, SH_Y0, oy, 22, SH_ROW, SH_ROWS, 9, &ln.sh[0], ln.state == LN_SHOWN ? rpc : -1, 1);
    if (ln.state == LN_ASK) {
        cv_text(152, 30 - oy, &FONT_T, "PLAY ITS", C_GRAY);
        cv_text(152, 46 - oy, &FONT_T, "NOTES", C_GRAY);
        int k = 0;
        for (i = 0; i < 12; i++)
            if (ln.played >> i & 1) {
                th_name(th_spell_pc(i, 0), lab);
                gtext(152 + 22 * (k % 4), 80 + 16 * (k / 4) - oy, &FONT_B, lab, C_WHITE);
                k++;
            }
    } else {
        int8_t tones[TH_TONES_MAX];
        th_chord_t c = ls_chord(ln.item);
        int nt = th_chord(c.root, c.q, tones);
        th_chord_name(c, lab);
        gtext(152, 4 - oy, &FONT_M, lab, C_WHITE);
        for (i = 0; i < nt; i++) {
            char nm[TH_NAME_MAX];
            th_name(tones[i], nm);
            gtext(152 + 22 * (i % 4), 40 + 16 * (i / 4) - oy, &FONT_B, nm, th_pc(tones[i]) == rpc ? C_HI : C_AMB);
        }
    }
}

static void draw_main(void)
{
    int h;
    for (h = 0; h < 2; h++) {
        cv_begin(240, MAIN_H / 2, C_BLACK);
        if (rec.page == PG_NECK) draw_neck_at(h * MAIN_H / 2);
        else if (rec.page == PG_LEARN) draw_learn_at(h * MAIN_H / 2);
        else draw_shape_at(h * MAIN_H / 2);
        cv_blit(0, 32 + (uint32_t)(h * MAIN_H / 2));
    }
}

/* LEARN's question, or the answer */
static void learn_title(char *t)
{
    char c[TH_CHORD_MAX];
    t[0] = 0;
    if (!ln.n) return;
    th_chord_name(ls_chord(ln.item), c);
    if (ln.state == LN_END) tcat(t, "ROUND DONE");
    else if (ln.state == LN_SHOWN) {
        tcat(t, ln.right ? "RIGHT: " : "NO: ");
        tcat(t, c);
    } else if (ln.kind == LS_NAME_TO_SHAPE) {
        tcat(t, "WHICH IS ");
        tcat(t, c);
        tcat(t, "?");
    } else
        tcat(t, "WHICH NOTES?");
}

static void learn_foot(char *t)          /* "STAGE 1 KEY C  3/8" */
{
    t[0] = 0;
    tcat(t, "STAGE ");
    tint(t, rec.learn.stage + 1);
    tcat(t, " KEY ");
    tcat(t, ls_stages[rec.learn.stage].key);
    if (ln.n && ln.state != LN_END) {
        tcat(t, "  ");
        tint(t, ln.i + 1);
        tcat(t, "/");
        tint(t, ln.n);
    }
}

static void draw_header(void)
{
    char t[48];
    int32_t x = 232;
    cv_begin(240, 32, C_BLACK);
    if (rec.page == PG_NECK) key_text(t);
    else if (rec.page == PG_LEARN) learn_title(t);
    else {
        t[0] = 0;
        tcat(t, "CHORD SHAPES");
    }
    gtext(8, 7, &FONT_B, t, C_WHITE);
    x -= text_w(&FONT_T, PAGE_NAME[rec.page]);
    cv_text(x, 10, &FONT_T, PAGE_NAME[rec.page], C_HI);
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

static void footer_text(char *big, char *small)
{
    th_key_t key = cur_key();
    int i;
    big[0] = small[0] = 0;
    switch (rec.page == PG_NECK ? ui.sh : SH_NONE) {
    case SH_NOTE: {
        int f = th_spell_pc(ui.notes[0] % 12, rec.tonic);
        th_note_name(ui.notes[0], rec.tonic, big);
        th_degree_name(key, f, small);
        break;
    }
    case SH_CHORD:
        th_chord_name(ui.id.c, big);
        th_roman_name(key, ui.id.c, small);
        break;
    case SH_UNKNOWN:
        tcat(big, "?");
        for (i = 0; i < ui.nnotes && i < 6; i++) {
            char t[TH_NAME_MAX];
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
            gtext(8, 3, &FONT_M, big, C_WHITE);
            gtext(232 - gtext_w(&FONT_B, small), 8, &FONT_B, small, C_HI);
        } else {
            char t[40];
            if (rec.page == PG_NECK) pos_text(t);
            else if (rec.page == PG_LEARN) learn_foot(t);
            else {
                t[0] = 0;
                tcat(t, "PLAY STRUMS");
            }
            cv_text(8, 10, &FONT_T, t, C_GRAY);
            cv_text(232 - text_w(&FONT_T, tun()->name), 10, &FONT_T, tun()->name, C_DIM);
        }
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

static void speak(void)
{
    char t[96], k[48], a[48];
    if (ann_on() && ann_where()) {
        t[0] = 0;
        tcat(t, APP_NAME ". ");
        tcat(t, PAGE_NAME[rec.page]);
        tcat(t, ". ");
        if (rec.page == PG_NECK) key_text(k);
        else if (rec.page == PG_LEARN) learn_title(k);
        else th_chord_name(cur_chord(), k);
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
    int n;
    zero(&ui, sizeof ui);
    zero(&ln, sizeof ln);
    saves = 0;
    dirty_at = 0;
    ln_unsaved_at = 0;
    n = plat_store_load(OBJ_GUITAR, &rec, sizeof rec);
    if (n == (int)REC_V1_SIZE && rec.ver == 1u && rec.size == REC_V1_SIZE) {    /* 0.1: no LEARN progress yet */
        ls_reset(&rec.learn);
        rec.ver = REC_VER;
        rec.size = (uint16_t)sizeof rec;
        n = (int)sizeof rec;
    }
    if (n != (int)sizeof rec || !rec_valid(&rec)) {
        zero(&rec, sizeof rec);
        defaults();
    }
    ui.seq_s = -1;
    synth_init();
    synth_stop();
    neck_update();
    shapes_update();
}

void app_ui_init(void)
{
    touch_on(1);
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
    seq_tick(now);
    if ((ui.toast_until && now >= ui.toast_until) != last_toast) {
        last_toast = ui.toast_until && now >= ui.toast_until;
        ui.redraw |= R_FOOT;
    }
    if ((dirty_at && (int32_t)(now - dirty_at) > 3000) || (ln_unsaved_at && now - ln_unsaved_at > LN_SAVE_MS))
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
    else if (!strcmp(what, "scale")) *got = rec.scale;
    else if (!strcmp(what, "labels")) *got = rec.labels;
    else if (!strcmp(what, "pos")) *got = rec.pos;
    else if (!strcmp(what, "tuning")) *got = rec.tuning;
    else if (!strcmp(what, "page")) *got = rec.page;
    else if (!strcmp(what, "oct")) *got = rec.oct;
    else if (!strcmp(what, "root")) *got = rec.root;
    else if (!strcmp(what, "qual")) *got = rec.qual;
    else if (!strcmp(what, "filter")) *got = rec.filter;
    else if (!strcmp(what, "shape")) *got = ui.shape;
    else if (!strcmp(what, "nshapes")) *got = ui.nshapes;
    else if (!strcmp(what, "view")) *got = ui.view;
    else if (!strcmp(what, "playing")) *got = ui.seq_on;
    else if (!strcmp(what, "voices")) *got = synth_active();
    else if (!strcmp(what, "lit_keys")) *got = (int)ui.lit;
    else if (!strcmp(what, "mid_keys")) *got = (int)ui.mid;
    else if (!strcmp(what, "glow_keys")) *got = (int)ui.glow;
    else if (!strcmp(what, "shown")) *got = ui.sh;
    else if (!strcmp(what, "saves")) *got = (int)saves;
    else if (!strcmp(what, "lstage")) *got = rec.learn.stage;
    else if (!strcmp(what, "lrounds")) *got = rec.learn.rounds;
    else if (!strcmp(what, "lstate")) *got = ln.state;
    else if (!strcmp(what, "lkind")) *got = ln.kind;
    else if (!strcmp(what, "litem")) *got = ln.item;
    else if (!strcmp(what, "lcard")) *got = ln.i;
    else if (!strcmp(what, "lpick")) *got = ln.pick;
    else if (!strcmp(what, "lright")) *got = ln.nright;
    else if (!strcmp(what, "lunsaved")) *got = ln_unsaved_at != 0;
    else if (!strcmp(what, "rec_size")) *got = (int)sizeof rec;
    else if (!strncmp(what, "lbox", 4)) *got = rec.learn.box[atoi(what + 4)];
    else if (!strncmp(what, "mask", 4)) *got = (int)ui.mask[atoi(what + 4)];
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
    if (!strcmp(cmd, "learn_pick")) {   /* learn_pick right|wrong: SELECT onto that choice (NAME cards) */
        ln.pick = !strcmp(a, "right") ? ln.answer : (ln.answer + 1) % 4;
        ui.redraw |= R_MAIN;
        return 1;
    }
    if (!strcmp(cmd, "learn_card")) {   /* learn_card ITEM|- KIND: this card now */
        if (strcmp(a, "-")) ln.cards[ln.i] = (uint8_t)atoi(a);     /* - keeps the card's chord */
        ln_setup(atoi(b));
        return 1;
    }
    if (strcmp(cmd, "expect_text")) return 0;
    /* expect_text key|pos|chord|frets|fingers|footer|tuning TEXT (_ a space), signs as # b o /o */
    host_text(b, want, sizeof want);
    raw[0] = 0;
    if (!strcmp(a, "key")) key_text(raw);
    else if (!strcmp(a, "pos")) pos_text(raw);
    else if (!strcmp(a, "chord")) th_chord_name(cur_chord(), raw);
    else if (!strcmp(a, "learn")) learn_title(raw);
    else if (!strcmp(a, "learn_foot")) learn_foot(raw);
    else if (!strcmp(a, "frets")) shape_text(raw);
    else if (!strcmp(a, "form") && ui.shape < ui.nshapes) form_text(&ui.shapes[ui.shape], raw);
    else if (!strcmp(a, "fingers") && ui.shape < ui.nshapes) {
        int s;
        for (s = 0; s < tun()->n; s++) {
            char one[2];
            one[0] = (char)('0' + ui.shapes[ui.shape].finger[s]);
            one[1] = 0;
            tcat(raw, one);
        }
    } else if (!strcmp(a, "tuning")) tcat(raw, tun()->name);
    else if (!strcmp(a, "footer")) {
        footer_text(raw, small);
        if (small[0]) {
            tcat(raw, " ");
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
