/* SPDX-License-Identifier: MIT */
/* firmware/common/lpc.c on the computer: an LPC stream (python_wizard's bytes) to 8 kHz mono s16le.
 *   lpc_decode IN.lpc FRAME_MS OUT.raw */
#include <stdio.h>
#include <stdlib.h>
#include "lpc.c"

int main(int argc, char **argv)
{
    static uint8_t data[1 << 20];
    int16_t pcm[256];
    size_t n;
    uint32_t got;
    lpc_t s;
    FILE *in, *out;
    if (argc != 4) {
        fprintf(stderr, "usage: lpc_decode IN.lpc FRAME_MS OUT.raw\n");
        return 2;
    }
    in = fopen(argv[1], "rb");
    out = fopen(argv[3], "wb");
    if (!in || !out) {
        perror("lpc_decode");
        return 1;
    }
    n = fread(data, 1, sizeof data, in);
    lpc_begin(&s, data, (uint32_t)n, (uint32_t)atoi(argv[2]));
    while ((got = lpc_render(&s, pcm, 256)) > 0)
        fwrite(pcm, 2, got, out);
    fclose(in);
    fclose(out);
    return 0;
}
