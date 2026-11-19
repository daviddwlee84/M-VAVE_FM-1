/* SPDX-License-Identifier: GPL-3.0-or-later */
/* kit.c on this computer (just drums-unit): every lane of every kit sounds, stays finite and in range, and ends. */
#include <stdio.h>
#include "../kit.c"

int main(void)
{
    static float buf[256];
    int kit, lane, fails = 0, checks = 0;
    kit_init(44100.0f);
    for (kit = 0; kit < KIT_N; kit++)
        for (lane = 0; lane < GV_LANES; lane++) {
            float peak = 0.0f;
            uint32_t blocks = 0, i;
            int finite = 1;
            kit_silence();
            kit_trigger(lane, 9, kit);
            while (kit_active() && blocks < 44100u * 6u / 256u) {
                kit_render(buf, 256);
                for (i = 0; i < 256; i++) {
                    float a = buf[i] < 0.0f ? -buf[i] : buf[i];
                    if (!(a == a) || a > 100.0f) finite = 0;
                    if (a > peak) peak = a;
                }
                blocks++;
            }
            checks += 4;
            if (!finite) fails++, printf("FAIL %s lane %d: not finite\n", kit_name(kit), lane);
            if (peak < 0.02f) fails++, printf("FAIL %s lane %d: too quiet (%.4f)\n", kit_name(kit), lane, (double)peak);
            if (peak > 2.5f) fails++, printf("FAIL %s lane %d: too loud (%.2f)\n", kit_name(kit), lane, (double)peak);
            if (kit_active()) fails++, printf("FAIL %s lane %d: still sounding after 6 s\n", kit_name(kit), lane);
            printf("  %-6s %-8s peak %.2f, %4u ms\n", kit_name(kit), gv_lane_name(lane), (double)peak,
                   blocks * 256u * 1000u / 44100u);
        }
    printf("kit: %d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}

const char *gv_lane_name(int lane)       /* (groove.c's, without the rest of it) */
{
    static const char *const n[GV_LANES] = { "KICK", "SNARE", "RIM", "CLAP", "HAT", "OPENHAT", "RIDE",
                                             "CRASH", "TOMHI", "TOMMID", "TOMLO", "COWBELL", "SHAKER" };
    return lane >= 0 && lane < GV_LANES ? n[lane] : "?";
}
