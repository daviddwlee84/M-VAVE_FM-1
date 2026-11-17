/* SPDX-License-Identifier: MIT */
/* lpc.c on the computer: frames packed by hand as python_wizard packs them, decoded and checked.
 *   cc -I.. -o /tmp/lpc_test lpc_test.c && /tmp/lpc_test      (just fw-common-test) */
#include <stdio.h>
#include <string.h>
#include "../lpc.c"

static int fails;
#define CHECK(c, msg) do { if (!(c)) { printf("FAIL %s\n", msg); fails++; } } while (0)

/* python_wizard's packing: fields highest bit first into one bit string, each byte holding its first bit lowest */
static uint8_t buf[256];
static uint32_t nbits;
static void put(uint32_t v, uint32_t k)
{
    while (k--) {
        uint32_t b = (v >> k) & 1u;
        if (b)
            buf[nbits >> 3] |= (uint8_t)(1u << (nbits & 7u));
        nbits++;
    }
}
static void voiced(uint32_t e, uint32_t pitch, const uint32_t k[10])
{
    uint32_t i;
    put(e, 4), put(0, 1), put(pitch, 6);
    for (i = 0; i < 10; i++)
        put(k[i], lpc_kbits[i]);
}
static void unvoiced(uint32_t e, const uint32_t k[4])
{
    uint32_t i;
    put(e, 4), put(0, 1), put(0, 6);
    for (i = 0; i < 4; i++)
        put(k[i], lpc_kbits[i]);
}

int main(void)
{
    static const uint32_t kv[10] = {20, 10, 9, 7, 8, 8, 7, 4, 3, 4}, ku[4] = {25, 12, 8, 8};
    int16_t pcm[4000];
    uint32_t n, i, peak = 0, zeros = 0;
    lpc_t s;

    memset(buf, 0, sizeof buf);
    nbits = 0;
    put(0, 4);                       /* silence */
    voiced(10, 30, kv);              /* voiced */
    put(10, 4), put(1, 1), put(30, 6);   /* repeat: same K's */
    unvoiced(9, ku);                 /* unvoiced */
    put(15, 4);                      /* stop */
    CHECK(nbits == 4 + 50 + 11 + 29 + 4, "bit count of the frames");
    CHECK(lpc_frames(buf, (nbits + 7) / 8) == 4, "four frames before the stop");

    lpc_begin(&s, buf, (nbits + 7) / 8, 25);
    n = lpc_render(&s, pcm, 4000);
    CHECK(n == 4 * 200, "25 ms frames: 200 samples each at 8 kHz");
    CHECK(lpc_done(&s), "the stop frame ends it");
    for (i = 0; i < 200; i++)
        zeros += pcm[i] == 0;
    CHECK(zeros == 200, "a silent frame is silent");
    for (i = 200; i < n; i++) {
        uint32_t a = (uint32_t)(pcm[i] < 0 ? -pcm[i] : pcm[i]);
        peak = a > peak ? a : peak;
    }
    CHECK(peak > 64 && peak <= 512 * 64, "voiced and unvoiced frames make sound, within 10 bits");
    CHECK(lpc_render(&s, pcm, 100) == 0, "nothing after the end");

    lpc_begin(&s, buf, (nbits + 7) / 8, 10);
    n = 0;
    while (!lpc_done(&s)) {
        uint32_t got = lpc_render(&s, pcm + n, 7);   /* small, odd chunks */
        n += got;
        if (!got)
            break;
    }
    CHECK(n == 4 * 80, "10 ms frames: 80 samples each, chunked");

    lpc_begin(&s, buf, 5, 25);       /* cut in the middle of the voiced frame */
    n = lpc_render(&s, pcm, 4000);
    CHECK(n == 200 && lpc_done(&s), "a cut stream ends at the last whole frame");

    printf(fails ? "lpc: %d failed\n" : "lpc: ok\n", fails);
    return fails != 0;
}
