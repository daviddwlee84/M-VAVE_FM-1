/* SPDX-License-Identifier: MIT */
/* Conductor: see app.h. Part of fm1-base's unity build, after the platform (gfx.c, announce.c in scope).
 * Protocol: host/conductor/PROTOCOL.md (the numbers below match host/conductor/protocol.py). */
#include "fastmath.h"
#include "hostlink.c"
#include "ledfx.c"
#include "chime.c"

/* ---------------------------------------------------------------- protocol --- */
#define CN_VER 1u
enum { T_HELLO_ACK = 0x01, T_CLEAR, T_SLOT, T_DETAIL, T_META, T_FOOTER, T_BEEP, T_DETAIL_BMP };
enum { T_HELLO = 0x41, T_HEARTBEAT, T_ACTION, T_SELECT };
enum { S_EMPTY, S_IDLE, S_WORKING, S_BLOCKED, S_DONE, S_ERROR, S_UNKNOWN, S_N };
enum { OP_APPROVE = 1, OP_ALWAYS, OP_DENY, OP_INTERRUPT, OP_ENTER, OP_CONTINUE, OP_FOCUS, OP_UP, OP_DOWN, OP_MODE,
       OP_VOICE_DOWN, OP_VOICE_UP };
#define F_QUESTION 0x01u
#define F_REMOTE 0x02u
#define FW_MAJOR 0u
#define FW_MINOR 3u
#define NAME_MAX 20u
#define DETAIL_LINES 8u
#define LINE_MAX 36u
/* a DETAIL line drawn by the host (any script: our fonts are ASCII): 240 x 16 dots, 7 per byte, pixel i at bit
 * 6 - i % 7 of byte i / 7 (host/conductor/render.py) */
#define BMP_W 240u
#define BMP_H 16u
#define BMP_BYTES ((BMP_W * BMP_H + 6u) / 7u)

typedef struct {
    uint8_t state, kind, flags, seen;    /* seen: a DONE looked at (selected) since it came */
    uint16_t tag, age;
    char name[NAME_MAX + 1];
} slot_t;

static slot_t slots[NKEYS];
static hostlink_t link;
static struct {
    uint16_t total, hidden;
    char backend[9];
} meta;
static struct {
    uint8_t slot;
    char line[DETAIL_LINES][LINE_MAX + 1];
    uint8_t is_bmp[DETAIL_LINES];
    uint8_t bmp[DETAIL_LINES][BMP_BYTES];
} detail;
static uint32_t chimes;                  /* figures played since boot (the host simulator's "chimes") */
static int ble_on = 1;                   /* Bluetooth starts at every power-on; OCT+ turns it off until the next */

static const char *const KIND2[] = {"??", "CL", "CX", "OC", "GM", "CU", "PI", "SH"};
static const char *const STATE_WORD[S_N] = {"", "IDLE", "WORK", "WAIT", "DONE", "ERR", "?"};

static uint16_t state_rgb(uint32_t s)
{
    switch (s) {
    case S_IDLE:
        return RGB(200, 200, 206);
    case S_WORKING:
        return RGB(70, 140, 255);
    case S_BLOCKED:
        return RGB(255, 176, 0);
    case S_DONE:
        return RGB(80, 220, 120);
    case S_ERROR:
        return RGB(240, 70, 60);
    default:
        return RGB(120, 120, 128);
    }
}

/* ---------------------------------------------------------------------- UI --- */
enum { PG_GRID, PG_DETAIL, PG_APPROVALS, PG_HELP, PG_N };
static struct {
    uint8_t page, sel, top;
    uint8_t scroll;                      /* DETAIL: KNOB 1 detents back (the host turns them into lines) */
    uint8_t back;                        /* the page HELP returns to */
    uint8_t redraw;
    uint32_t keys, btn;
    char msg[2][24];
    uint32_t msg_until;
    char footer[LINE_MAX + 1];
    uint32_t footer_until;
    uint8_t host_was, ble_was;
} ui;

static void copy_text(char *d, const uint8_t *s, uint32_t n, uint32_t max)
{
    uint32_t i;
    for (i = 0; i < n && i < max; i++)
        d[i] = (s[i] >= 0x20u && s[i] < 0x7Fu) ? (char)s[i] : '?';
    d[i] = 0;
}

static void text_cat(char *d, const char *s)
{
    while (*d)
        d++;
    while (*s)
        *d++ = *s++;
    *d = 0;
}

static void int_text(int32_t v, char *b)
{
    char t[12];
    int n = 0;
    uint32_t u = (uint32_t)(v < 0 ? -v : v);
    do
        t[n++] = (char)('0' + u % 10u);
    while ((u /= 10u) && n < 11);
    if (v < 0)
        *b++ = '-';
    while (n)
        *b++ = t[--n];
    *b = 0;
}

static void note_name(uint32_t key, char *b)   /* key 0 is F3 */
{
    static const char *const N[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    uint32_t note = 53u + key;
    b[0] = 0;
    text_cat(b, N[note % 12u]);
    int_text((int32_t)(note / 12u) - 1, b + (b[1] == '#' ? 2 : 1));
}

void app_say(const char *a, const char *b)
{
    uint32_t i;
    char t[48];
    for (i = 0; i < 23u && a && a[i]; i++)
        ui.msg[0][i] = a[i];
    ui.msg[0][i] = 0;
    for (i = 0; i < 23u && b && b[i]; i++)
        ui.msg[1][i] = b[i];
    ui.msg[1][i] = 0;
    ui.msg_until = plat_ms() + 1500u;
    ui.redraw = 1;
    if (ann_on()) {
        t[0] = 0;
        text_cat(t, ui.msg[0]);
        if (ui.msg[1][0]) {
            text_cat(t, " ");
            text_cat(t, ui.msg[1]);
        }
        ann_say(t, 0, ANN_NOW);
    }
}

static int on_page(uint32_t s)            /* slot s is listed on the current page */
{
    if (slots[s].state == S_EMPTY)
        return 0;
    return ui.page != PG_APPROVALS || slots[s].state == S_BLOCKED;
}

/* ---------------------------------------------------------------- host in --- */
static void slot_in(const uint8_t *d, uint32_t n)
{
    slot_t *s, old;
    if (n < 8u || d[0] >= NKEYS || d[1] >= S_N)
        return;
    s = &slots[d[0]];
    old = *s;
    s->state = d[1];
    s->kind = d[2] < sizeof KIND2 / sizeof KIND2[0] ? d[2] : 0u;
    s->flags = d[3];
    s->tag = (uint16_t)(d[4] | d[5] << 7);
    s->age = (uint16_t)(d[6] | d[7] << 7);
    copy_text(s->name, d + 8, n - 8u, NAME_MAX);
    if (s->tag != old.tag || s->state != old.state)
        s->seen = d[0] == ui.sel && ui.page != PG_APPROVALS;   /* a DONE on the key we look at is seen */
    /* a chime when an agent we already knew changes: not for the table filling up at a (re)connect */
    if (old.state != S_EMPTY && old.tag == s->tag && old.state != s->state) {
        int f = s->state == S_BLOCKED ? CHIME_UP : s->state == S_DONE ? CHIME_DONE : s->state == S_ERROR ? CHIME_ERROR : -1;
        if (f >= 0 && !chime_busy()) {
            chime_play(f);
            chimes++;
        }
    }
    ui.redraw = 1;
}

static int host_frame(const uint8_t *p, uint32_t n, int transport)
{
    uint8_t t;
    uint32_t len, i, now = plat_ms();
    const uint8_t *d = hl_match(&link, p, n, &t, &len);
    if (!d)
        return 0;
    hl_seen(&link, now);
    switch (t) {
    case T_HELLO_ACK:
        hl_acked(&link, now, transport);
        break;
    case T_CLEAR:
        for (i = 0; i < NKEYS; i++)
            slots[i].state = S_EMPTY, slots[i].tag = 0, slots[i].name[0] = 0;
        break;
    case T_SLOT:
        slot_in(d, len);
        break;
    case T_DETAIL:
        if (len >= 2u && d[1] < DETAIL_LINES) {
            detail.slot = d[0];
            copy_text(detail.line[d[1]], d + 2, len - 2u, LINE_MAX);
            detail.is_bmp[d[1]] = 0;
        }
        break;
    case T_DETAIL_BMP:
        if (len == 2u + BMP_BYTES && d[1] < DETAIL_LINES) {
            detail.slot = d[0];
            for (i = 0; i < BMP_BYTES; i++)
                detail.bmp[d[1]][i] = d[2 + i];
            detail.is_bmp[d[1]] = 1;
        }
        break;
    case T_META:
        if (len >= 4u) {
            meta.total = (uint16_t)(d[0] | d[1] << 7);
            meta.hidden = (uint16_t)(d[2] | d[3] << 7);
            copy_text(meta.backend, d + 4, len - 4u, 8);
        }
        break;
    case T_FOOTER:
        copy_text(ui.footer, d, len, LINE_MAX);
        ui.footer_until = now + 3000u;
        break;
    case T_BEEP:
        if (len)
            chime_play(d[0]);
        break;
    default:
        break;
    }
    ui.redraw = 1;
    return 1;
}

int app_sysex(const uint8_t *p, uint32_t n) { return host_frame(p, n, HL_USB); }

/* Bluetooth: fm1-base's own stack hands SysEx over as USB-MIDI packets on PORT_BLE (main loop) */
static hl_sx_t ble_rx;
static void ble_in(void)
{
    uint32_t pkt, n;
    int port;
    const uint8_t *f;
    while (plat_midi_in(&pkt, &port))
        if (port == PORT_BLE && hl_sx_packet(&ble_rx, pkt, &f, &n))
            host_frame(f, n, HL_BLE);
}

/* nothing of Bluetooth's is written to the flash: the own stack makes its address from the flash part's unique ID
 * (the same at every start), there is no pairing to keep (macOS and iOS connect without one), so the record lives
 * in RAM and its save is a no-op */
int app_ble_load(struct plat_ble_rec *r)
{
    (void)r;
    return 0;
}
int app_ble_save(const struct plat_ble_rec *r)
{
    (void)r;
    return 0;
}

/* --------------------------------------------------------------- host out --- */
static void send_select(void)
{
    uint8_t p[2] = {ui.sel, ui.scroll};
    if (hl_host(&link, plat_ms()))
        hl_send(&link, T_SELECT, p, ui.scroll ? 2u : 1u);
}

static void act(uint32_t op)
{
    slot_t *s = &slots[ui.sel];
    uint8_t p[4];
    if (!hl_host(&link, plat_ms())) {
        app_say("NO HOST", "");
        return;
    }
    if (s->state == S_EMPTY) {
        app_say("EMPTY", "");
        return;
    }
    if ((op == OP_APPROVE || op == OP_ALWAYS || op == OP_DENY) && s->state != S_BLOCKED) {
        app_say("NOT WAITING", "");   /* the host checks again; this saves the trip */
        return;
    }
    p[0] = (uint8_t)op;
    p[1] = ui.sel;
    p[2] = (uint8_t)(s->tag & 0x7Fu);
    p[3] = (uint8_t)(s->tag >> 7);
    hl_send(&link, T_ACTION, p, 4);
}

/* ARP held: the host holds the computer's dictation hotkey (Wispr Flow, Handy) down; not about an agent, so an
 * empty key is fine. The release always goes, so the host never keeps the keys down */
static void voice(uint32_t op)
{
    uint8_t p[4];
    if (!hl_host(&link, plat_ms())) {
        if (op == OP_VOICE_DOWN)
            app_say("NO HOST", "");
        return;
    }
    p[0] = (uint8_t)op;
    p[1] = ui.sel;
    p[2] = (uint8_t)(slots[ui.sel].tag & 0x7Fu);
    p[3] = (uint8_t)(slots[ui.sel].tag >> 7);
    hl_send(&link, T_ACTION, p, 4);
    if (op == OP_VOICE_DOWN)
        app_say("LISTENING", "");
}

static void select_slot(uint32_t k)
{
    if (ui.sel != k)
        ui.scroll = 0;
    ui.sel = (uint8_t)k;
    if (slots[k].state == S_DONE)
        slots[k].seen = 1;
    send_select();
    ui.redraw = 1;
}

static void step_sel(int32_t d)          /* SELECT: the next slot listed on this page */
{
    uint32_t i, k = ui.sel;
    for (i = 0; i < NKEYS; i++) {
        k = (k + NKEYS + (d > 0 ? 1u : NKEYS - 1u)) % NKEYS;
        if (on_page(k)) {
            select_slot(k);
            return;
        }
    }
}

static void next_attention(void)          /* K4: BLOCKED, then ERROR, then an unseen DONE */
{
    static const uint8_t ORDER[3] = {S_BLOCKED, S_ERROR, S_DONE};
    uint32_t o, i;
    for (o = 0; o < 3u; o++)
        for (i = 1; i <= NKEYS; i++) {
            uint32_t k = (ui.sel + i) % NKEYS;
            if (slots[k].state == ORDER[o] && !(ORDER[o] == S_DONE && slots[k].seen)) {
                select_slot(k);
                return;
            }
        }
    app_say("ALL CLEAR", "");
}

static void set_page(uint32_t p)
{
    ui.page = (uint8_t)(p % PG_N);
    ui.top = 0;
    if (ui.page == PG_DETAIL)
        send_select();
    ui.redraw = 1;
}

static void input(void)
{
    uint32_t keys = plat_keys(), btn = plat_buttons(), k;
    int32_t e;
    for (k = 0; k < NKEYS; k++)
        if ((keys >> k & 1u) && !(ui.keys >> k & 1u))
            select_slot(k);
    for (k = 0; k < NB; k++) {
        if (!(btn >> k & 1u) || (ui.btn >> k & 1u))
            continue;
        switch (k) {
        case B_REC: act(OP_APPROVE); break;
        case B_SAVE: act(OP_ALWAYS); break;
        case B_FX: act(OP_DENY); break;
        case B_ENV: act(OP_INTERRUPT); break;
        case B_SEL: act(OP_ENTER); break;
        case B_PLAY: act(OP_CONTINUE); break;
        case B_LFO: act(OP_MODE); break;
        case B_EDIT:
            set_page(PG_DETAIL);
            act(OP_FOCUS);
            break;
        case B_HOME: set_page(PG_GRID); break;
        case B_GLO: set_page(PG_APPROVALS); break;
        case B_OCTUP:                    /* Bluetooth on / off, for this power-on (nothing saved) */
            ble_on = !ble_on;
            plat_ble_enable(ble_on);
            app_say("BLUETOOTH", ble_on ? "ON" : "OFF");
            break;
        case B_SEQ:                      /* help, and back to where we were */
            if (ui.page == PG_HELP)
                set_page(ui.back);
            else {
                ui.back = ui.page;
                set_page(PG_HELP);
            }
            break;
        case B_ARP: voice(OP_VOICE_DOWN); break;
        default: break;
        }
    }
    if ((ui.btn >> B_ARP & 1u) && !(btn >> B_ARP & 1u))
        voice(OP_VOICE_UP);
    if ((e = plat_enc(EN_SELECT)) != 0)
        step_sel(e);
    if ((e = plat_enc(EN_ALGO)) != 0)
        set_page((uint32_t)((int32_t)ui.page + PG_N + (e > 0 ? 1 : -1)));
    if ((e = plat_enc(EN_K2)) != 0)
        act(e > 0 ? OP_DOWN : OP_UP);
    if ((e = plat_enc(EN_K4)) != 0)
        next_attention();
    if ((e = plat_enc(EN_K1)) != 0 && ui.page == PG_DETAIL) {   /* back through the agent's output */
        int32_t v = clamp((int32_t)ui.scroll - e, 0, 30);   /* counter-clockwise: further back */
        if (v != ui.scroll) {
            ui.scroll = (uint8_t)v;
            send_select();
            ui.redraw = 1;
        }
    }
    (void)plat_enc(EN_PRESET), (void)plat_enc(EN_K3);
    if (keys != ui.keys || btn != ui.btn)
        ui.redraw = 1;
    ui.keys = keys;
    ui.btn = btn;
}

void app_input_only(void) { input(); }

/* ----------------------------------------------------------------- lights --- */
static void lights(uint32_t now)
{
    static const uint8_t FX[S_N] = {FX_OFF, FX_GLOW, FX_PULSE, FX_BLINK, FX_LIT, FX_DBLINK, FX_MID};
    ledfx_t kb = {0, 0, 0}, bb = {0, 0, 0};
    uint32_t k, alarm = 0, host = (uint32_t)hl_host(&link, now);
    for (k = 0; k < NKEYS; k++) {
        uint32_t s = slots[k].state;
        int fx = FX[s];
        if (s == S_DONE && slots[k].seen)
            fx = FX_GLOW;
        if (!host && s != S_EMPTY)
            fx = FX_GLOW;                /* stale: we do not know any more */
        ledfx_set(&kb, k, fx, now);
        alarm |= s == S_BLOCKED || s == S_ERROR;
    }
    for (k = 0; k < NB; k++)
        if (k != B_PLAY)
            ledfx_set(&bb, k, FX_GLOW, now);   /* the panel readable in the dark */
    if (ui.btn >> B_ARP & 1u)
        ledfx_set(&bb, B_ARP, FX_LIT, now);   /* listening */
    if (slots[ui.sel].state == S_BLOCKED) {
        ledfx_set(&bb, B_REC, FX_BLINK, now);
        ledfx_set(&bb, B_FX, FX_LIT, now);
    }
    /* PLAY, the one LED with colour: red while anyone waits or broke, green while all is well, off with no host */
    if (host)
        bb.lit |= alarm ? 1u << B_PLAY : LED_PLAY_GREEN;
    ledfx_show(&bb, &kb);
}

/* ----------------------------------------------------------------- screen --- */
#define ROW_H 22
#define ROWS 8

static void draw_header(uint32_t now)
{
    char t[24];
    uint32_t k, waiting = 0, working = 0;
    int host = hl_host(&link, now);
    cv_begin(240, 32, C_BLACK);
    if (now < ui.msg_until) {
        int32_t x = cv_text(8, 7, &FONT_B, ui.msg[0], C_WHITE);
        cv_text(x + 8, 7, &FONT_B, ui.msg[1], state_rgb(S_BLOCKED));
    } else {
        static const char *const PAGE[PG_N] = {"AGENTS", "DETAIL", "WAITING", "HELP"};
        cv_text(8, 7, &FONT_B, PAGE[ui.page], C_WHITE);
        for (k = 0; k < NKEYS; k++) {
            waiting += slots[k].state == S_BLOCKED;
            working += slots[k].state == S_WORKING;
        }
        if (host) {
            int32_t x;
            t[0] = 0;
            int_text((int32_t)waiting, t);
            x = 232 - text_w(&FONT_S, t);
            cv_text(x, 8, &FONT_S, t, waiting ? state_rgb(S_BLOCKED) : C_DIM);
            int_text((int32_t)working, t);
            x -= 10 + text_w(&FONT_S, t);
            cv_text(x, 8, &FONT_S, t, state_rgb(S_WORKING));
            x -= 6 + text_w(&FONT_XS, meta.backend);
            cv_text(x, 10, &FONT_XS, meta.backend, C_GRAY);
            if (link.transport == HL_BLE)
                cv_text(x - 6 - text_w(&FONT_XS, "BLE"), 10, &FONT_XS, "BLE", state_rgb(S_WORKING));
        } else {
            int32_t x = 232 - text_w(&FONT_XS, "NO HOST");
            cv_text(x, 10, &FONT_XS, "NO HOST", state_rgb(S_ERROR));
            if (FM1B_BLE && ble_on)              /* grey while it advertises, blue once a computer is connected */
                cv_text(x - 6 - text_w(&FONT_XS, "BLE"), 10, &FONT_XS, "BLE",
                        plat_ble_connected() ? state_rgb(S_WORKING) : C_DIM);
        }
    }
    cv_rect(0, 31, 240, 1, C_LINE);
    cv_blit(0, 0);
}

static void draw_row(uint32_t row, int32_t k)   /* k: the slot, -1 an empty row */
{
    char t[16];
    cv_begin(240, ROW_H, C_BLACK);
    if (k >= 0) {
        const slot_t *s = &slots[k];
        uint16_t c = state_rgb(s->state);
        int sel = (uint32_t)k == ui.sel;
        if (sel)
            cv_rect(0, 0, 240, ROW_H, C_LINE);
        cv_rect(0, 2, 4, ROW_H - 4, c);
        note_name((uint32_t)k, t);
        cv_text(8, 4, &FONT_XS, t, C_GRAY);
        cv_text(36, 4, &FONT_XS, KIND2[s->kind], (s->flags & F_REMOTE) ? state_rgb(S_WORKING) : C_DIM);
        cv_text(56, 4, &FONT_XS, s->name, sel ? C_WHITE : C_AMB);
        t[0] = 0;
        if (s->state == S_BLOCKED || s->state == S_DONE || s->state == S_ERROR || s->age == 0)
            text_cat(t, s->state == S_BLOCKED && (s->flags & F_QUESTION) ? "ASK" : STATE_WORD[s->state]);
        else {
            int_text(s->age, t);
            text_cat(t, "m");
        }
        cv_text(234 - text_w(&FONT_XS, t), 4, &FONT_XS, t, c);
    }
    cv_blit(0, 32 + row * ROW_H);
}

static void draw_list(void)
{
    int32_t list[NKEYS];
    uint32_t n = 0, k, r, at = 0;
    for (k = 0; k < NKEYS; k++)
        if (on_page(k)) {
            if (k == ui.sel)
                at = n;
            list[n++] = (int32_t)k;
        }
    if (at < ui.top)
        ui.top = (uint8_t)at;
    if (at >= ui.top + ROWS)
        ui.top = (uint8_t)(at - ROWS + 1u);
    if (!n) {
        cv_begin(240, ROW_H * ROWS, C_BLACK);
        cv_text(8, 40, &FONT_B, ui.page == PG_APPROVALS ? "ALL CLEAR" : "NO AGENTS", C_GRAY);
        if (!hl_host(&link, plat_ms())) {
            cv_text(8, 70, &FONT_XS, "start the host daemon:", C_DIM);
            cv_text(8, 88, &FONT_XS, "just cond-run hc", C_AMB);
        }
        cv_blit(0, 32);
        return;
    }
    for (r = 0; r < ROWS; r++)
        draw_row(r, ui.top + r < n ? list[ui.top + r] : -1);
}

/* the main area, 240 x 176, is taller than fm1-base's canvas (240 x 144): drawn in two halves, each pass with the
 * rows above it taken off (oy), so a line across the seam is cut in two and shown whole */
#define MAIN_H (ROW_H * ROWS)
static void main_area(void (*fn)(int32_t oy))
{
    cv_begin(240, MAIN_H / 2, C_BLACK);
    fn(0);
    cv_blit(0, 32);
    cv_begin(240, MAIN_H / 2, C_BLACK);
    fn(MAIN_H / 2);
    cv_blit(0, 32 + MAIN_H / 2);
}

static void bmp_line(const uint8_t *b, int32_t y0, uint16_t c)
{
    uint32_t i;
    for (i = 0; i < BMP_W * BMP_H; i++)
        if (b[i / 7u] >> (6u - i % 7u) & 1u)
            cv_pset((int32_t)(i % BMP_W), y0 + (int32_t)(i / BMP_W), c);
}

static void detail_at(int32_t oy)
{
    const slot_t *s = &slots[ui.sel];
    char t[24];
    uint32_t i;
    note_name(ui.sel, t);
    cv_text(8, 2 - oy, &FONT_XS, t, C_GRAY);
    if (s->state == S_EMPTY) {
        cv_text(40, 2 - oy, &FONT_XS, "EMPTY KEY", C_DIM);
        return;
    }
    cv_rect(36, 4 - oy, 4, 12, state_rgb(s->state));
    cv_text(46, 2 - oy, &FONT_XS, KIND2[s->kind], C_DIM);
    cv_text(66, 2 - oy, &FONT_XS, s->name, C_WHITE);
    cv_text(234 - text_w(&FONT_XS, STATE_WORD[s->state]), 2 - oy, &FONT_XS, STATE_WORD[s->state], state_rgb(s->state));
    if (detail.slot == ui.sel)
        for (i = 0; i < DETAIL_LINES; i++) {
            if (detail.is_bmp[i])
                bmp_line(detail.bmp[i], 22 + (int32_t)i * 18 - oy, C_AMB);
            else
                cv_text(4, 24 + (int32_t)i * 18 - oy, &FONT_XS, detail.line[i], C_AMB);
        }
}

static void draw_detail(void) { main_area(detail_at); }

static void help_at(int32_t oy)
{
    static const char *const L[][2] = {
        {"REC", "approve"}, {"FX", "deny"},
        {"SAVE", "always"}, {"ENV", "stop"},
        {"SEL", "enter"}, {"PLAY", "continue"},
        {"LFO", "mode"}, {"ARP", "hold: talk"},
        {"EDIT", "output"}, {"K1", "scroll it"},
        {"HOME", "all"}, {"GLO", "waiting"},
        {"SELECT", "move"}, {"K4", "next"},
        {"K2", "up / down"}, {"SEQ", "this help"},
    };
    static const char *const LIGHT[] = {"blink waits", "pulse works", "lit done", "2x error", "glow idle"};
    static const uint8_t LS[] = {S_BLOCKED, S_WORKING, S_DONE, S_ERROR, S_IDLE};
    uint32_t i;
    int32_t x = 4;
    for (i = 0; i < sizeof L / sizeof L[0]; i++) {
        int32_t cx = (i & 1u) ? 122 : 4, y = 2 + (int32_t)(i / 2u) * 17 - oy;
        cv_text(cx, y, &FONT_XS, L[i][0], C_WHITE);
        cv_text(cx + 44, y, &FONT_XS, L[i][1], C_AMB);
    }
    for (i = 0; i < 5u; i++) {                 /* the keys' lights, in their state colours */
        if (i == 3u)
            x = 4;
        cv_text(x, (i < 3u ? 142 : 158) - oy, &FONT_XS, LIGHT[i], state_rgb(LS[i]));
        x += text_w(&FONT_XS, LIGHT[i]) + 12;
    }
}

static void draw_help(void) { main_area(help_at); }

static void draw_footer(uint32_t now)
{
    cv_begin(240, 32, C_BLACK);
    cv_rect(0, 0, 240, 1, C_LINE);
    if (now < ui.footer_until)
        cv_text(8, 9, &FONT_XS, ui.footer, C_WHITE);
    else if (ui.page == PG_DETAIL) {
        char t[24] = "K1 scroll";
        if (ui.scroll) {
            text_cat(t, "  -");
            int_text(ui.scroll * 4, t + 12);
        }
        cv_text(8, 9, &FONT_XS, t, C_DIM);
    } else if (ui.page == PG_HELP)
        cv_text(8, 9, &FONT_XS, "SEQ back  keys: one agent each", C_DIM);
    else if (meta.hidden) {
        char t[24] = "+";
        int_text(meta.hidden, t + 1);
        text_cat(t, " NOT ON KEYS");
        cv_text(8, 9, &FONT_XS, t, C_GRAY);
    } else
        cv_text(8, 9, &FONT_XS, "REC ok  FX no  K4 next  SEQ help", C_DIM);
    cv_blit(0, 208);
}

static void draw(uint32_t now)
{
    draw_header(now);
    if (ui.page == PG_DETAIL)
        draw_detail();
    else if (ui.page == PG_HELP)
        draw_help();
    else
        draw_list();
    draw_footer(now);
}

/* ------------------------------------------------------------------ audio --- */
static float gain_now;

void app_render(int32_t *lr, uint32_t n, uint32_t master_q12)
{
    uint32_t i;
    float gain = (float)master_q12 / 4096.0f * 8300000.0f;
    gain_now += (gain - gain_now) * 0.5f;
    for (i = 0; i < 2u * n; i++)
        lr[i] = 0;
    chime_render(lr, n, gain_now);
}

void app_panic(void) { chime_stop(); }

/* ------------------------------------------------------------ the contract --- */
void app_init(void)
{
    uint32_t k;
    hl_init(&link, 0x43, 0x4E, CN_VER);
    for (k = 0; k < NKEYS; k++)
        slots[k].state = S_EMPTY;
}

void app_ui_init(void)
{
    lcd_fill(0, 0, 240, 240, C_BLACK);
    ui.redraw = 1;
}

void app_frame(void)
{
    static const uint8_t HELLO[3] = {CN_VER, FW_MAJOR, FW_MINOR};
    static uint32_t last_msg, last_footer;
    uint32_t now = plat_ms();
    uint8_t host;
    input();
    ble_in();
    hl_tick(&link, now, T_HELLO, T_HEARTBEAT, HELLO, 3);
    host = (uint8_t)hl_host(&link, now);
    if (host != ui.host_was || (uint8_t)plat_ble_connected() != ui.ble_was) {
        ui.host_was = host;
        ui.ble_was = (uint8_t)plat_ble_connected();
        ui.redraw = 1;
    }
    /* a message or footer running out */
    if ((ui.msg_until && now >= ui.msg_until) != last_msg || (ui.footer_until && now >= ui.footer_until) != last_footer) {
        last_msg = ui.msg_until && now >= ui.msg_until;
        last_footer = ui.footer_until && now >= ui.footer_until;
        ui.redraw = 1;
    }
    lights(now);
    ann_tick(now);
    if (ui.redraw) {
        ui.redraw = 0;
        draw(now);
    }
}

#ifdef PLAT_HOST
int app_expect(const char *what, int *got)
{
    if (!strcmp(what, "host"))
        *got = hl_host(&link, plat_ms());
    else if (!strcmp(what, "page"))
        *got = ui.page;
    else if (!strcmp(what, "sel"))
        *got = ui.sel;
    else if (!strcmp(what, "total"))
        *got = meta.total;
    else if (!strcmp(what, "transport"))
        *got = link.transport;
    else if (!strcmp(what, "bmp0")) {           /* dots lit in DETAIL picture line 0 */
        uint32_t i, n = 0;
        for (i = 0; i < BMP_BYTES; i++)
            n += (uint32_t)__builtin_popcount(detail.bmp[0][i]);
        *got = detail.is_bmp[0] ? (int)n : -1;
    } else if (!strcmp(what, "chimes"))
        *got = (int)chimes;
    else if (!strncmp(what, "state", 5))
        *got = slots[atoi(what + 5) % NKEYS].state;
    else
        return 0;
    return 1;
}
#endif
