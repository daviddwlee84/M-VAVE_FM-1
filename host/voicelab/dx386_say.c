/* SPDX-License-Identifier: MIT */
/* The 386DX engine (vendor/386dx-engine: eSpeak NG's English front end + Klatt, GPL-3.0) speaking a line at one
 * pitch, for the listening lab. This file is ours and only calls the engine's public API (speech.h); linked with the
 * engine, the binary is GPL-3.0 and stays in build/, never committed or shared.
 *   dx386_say TEXT OUT.wav [SEMITONES]      (11025 Hz mono 16-bit; SEMITONES -12..12, 0: eSpeak's own intonation) */
#include "speech.h"
#include <stdio.h>
#include <stdlib.h>

void dx_speech_idle(void) {}

static void u16(FILE *f, unsigned n) { fputc(n & 255, f); fputc((n >> 8) & 255, f); }
static void u32(FILE *f, unsigned n) { u16(f, n); u16(f, n >> 16); }

int main(int argc, char **argv)
{
    static int16_t out[DX_SPEECH_LONG_SAMPLES];
    static const uint8_t sound[4] = {50, 20, 20, 20};   /* body, breath, edge, glide: the example's */
    int pitch = argc > 3 ? atoi(argv[3]) : 0, rc;
    if (argc < 3) {
        fprintf(stderr, "usage: dx386_say TEXT OUT.wav [SEMITONES]\n");
        return 2;
    }
    if (dx_speech_init() != 0) {
        fprintf(stderr, "dx386_say: the engine did not start\n");
        return 1;
    }
    dx_speech_sound(sound);
    rc = dx_speech_make_into(0, argv[1], pitch, out, DX_SPEECH_LONG_SAMPLES);
    if (rc < 0) {
        fprintf(stderr, "dx386_say: not spoken (%d)\n", rc);
        return 1;
    }
    unsigned n = dx_speech_length[0];
    FILE *f = fopen(argv[2], "wb");
    if (!f)
        return 1;
    fwrite("RIFF", 1, 4, f); u32(f, 36 + n * 2); fwrite("WAVEfmt ", 1, 8, f);
    u32(f, 16); u16(f, 1); u16(f, 1); u32(f, 11025); u32(f, 22050); u16(f, 2); u16(f, 16);
    fwrite("data", 1, 4, f); u32(f, n * 2);
    for (unsigned i = 0; i < n; i++)
        u16(f, (uint16_t)out[i]);
    fclose(f);
    printf("%u samples at 11025 Hz\n", n);
    return 0;
}
