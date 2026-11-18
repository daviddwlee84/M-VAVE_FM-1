/* SPDX-License-Identifier: GPL-3.0-or-later */
/* srs: flash cards in Leitner boxes, counted in rounds, for any list of items. The method is Morse's learn.c (same
 * author, github.com/daviddwlee84/fm1-morse) with the Morse taken out: each item is in a box 0..5, box n comes back
 * every 2^n rounds, a wrong answer sends it to box 0 and asks it again at the end of the round. The FM-1 has no
 * clock that lives through a power-off, so the schedule counts rounds, not days (docs/dev/theory.md#srs).
 *
 * What the app gives (sr_course_t): how many items, each item's segment and key, and the groups in which they open.
 * A round plays the new items first, shown together (a new thing is met in a block), then each segment in turn
 * (theory, then ear), its cards shuffled so that two cards of one key never come back to back (interleaving, once
 * the items are known), then the weak ones again. A focus mask keeps a round to some segments.
 *
 * No platform calls, no libc (tests/srs_test.c). */
#pragma once
#include <stdint.h>

#define SR_MAX 96u                       /* items at most */
#define SR_TOP 5u                        /* the highest box */
#define SR_NEW_MAX 3u                    /* new items in one round at most */
#define SR_CARDS_MAX 48u                 /* cards in a round, with the ones asked again */
#define SR_ROUND_MAX 24u                 /* cards asked in a round before repeats */
#define SR_NEVER 0xFFFFu
#define SR_NOKEY 0xFFu

typedef struct {
    uint8_t n;                           /* items */
    const uint8_t *seg;                  /* per item: its segment (rounds play segment 0, then 1, ...) */
    const uint8_t *key;                  /* per item: items of one key are not asked back to back (SR_NOKEY: none) */
    const uint8_t *group_end;            /* the course opens up to these, in turn */
    uint8_t ngroups;
    uint8_t unlock_box;                  /* every open item in this box or higher: the next group opens */
} sr_course_t;

typedef struct {
    uint16_t round, streak;              /* rounds played; rounds in a row at 80 % or better */
    uint8_t n, open;                     /* items in the course; items open (a prefix of it) */
    uint32_t answered, right;
    uint8_t box[SR_MAX];
    uint16_t last[SR_MAX];               /* the round it was last asked in, SR_NEVER never */
} sr_state_t;

enum { SR_ASK, SR_SHOW };                /* a card asks, or shows a new item (nothing asked) */

typedef struct {
    uint8_t item, kind, again;           /* again: asked once more this round; a right answer does not move it up */
} sr_card_t;

typedef struct {
    sr_card_t card[SR_CARDS_MAX];
    uint8_t n, at;                       /* cards, the one being asked */
    uint8_t ok, miss, slow, up, shown;
    uint8_t opened;                      /* sr_finish opened a group */
    uint32_t rng;
} sr_round_t;

enum { SR_WRONG, SR_RIGHT, SR_SLOW, SR_EASY };

uint32_t sr_rand(uint32_t *x);           /* xorshift32 */
void sr_init(sr_state_t *s, const sr_course_t *c);
int sr_valid(const sr_state_t *s, const sr_course_t *c);
int sr_due(const sr_state_t *s, int item);
int sr_due_count(const sr_state_t *s, const sr_course_t *c, uint32_t focus);
int sr_new_count(const sr_state_t *s, const sr_course_t *c, uint32_t focus);

/* a round of about n cards (clamped to 4 .. SR_ROUND_MAX), from the segments in focus (bit k: segment k) */
void sr_build(const sr_state_t *s, sr_round_t *r, const sr_course_t *c, int n, uint32_t focus, uint32_t seed);
const sr_card_t *sr_card(const sr_round_t *r);   /* the card now, 0 when the round is over */
void sr_answer(sr_state_t *s, sr_round_t *r, int grade);    /* SR_*; a SR_SHOW card takes any grade */
int sr_finish(sr_state_t *s, sr_round_t *r, const sr_course_t *c);   /* the round counted; 1 if a group opened */
int sr_waiting(const sr_state_t *s, const sr_course_t *c, uint8_t *items, int max);   /* open items holding the next group back */

/* backup: little-endian, 14 + 3n bytes */
#define SR_BLOB_N(n) (14 + 3 * (n))
int sr_blob(const sr_state_t *s, uint8_t *out);  /* bytes written */
int sr_unblob(sr_state_t *s, const sr_course_t *c, const uint8_t *in, int len);   /* 1 taken; 0 refused, s untouched */
