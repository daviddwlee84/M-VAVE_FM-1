/* SPDX-License-Identifier: GPL-3.0-or-later */
/* DRUMS' built-in styles: written by hand for this firmware (no outside source).
 *
 * A line: PART LANE STEPS. PART: A, B (MAIN A, MAIN B), a, b (their fills), E (the ending). LANE: K kick, S snare,
 * R rim, C clap, H hat, O open hat, D ride, X crash, T M L toms (high, mid, low), B cowbell, Z shaker. STEPS: one a
 * step (16ths with 4 steps a beat, triplet 8ths with 3), '.' a rest, 1..9 the velocity; spaces are for the eye.
 * A style without its own fills gets the generic ones of its metre (groove.c), without an ending a kick and a crash. */
#pragma once
#include "groove.h"

static const gv_style_src_t DR_STYLES[] = {
    { "ROCK 8", 4, 4, 110,
      "A K 9... .... 9.7. ....\n"
      "A S .... 9... .... 9...\n"
      "A H 7.5. 7.5. 7.5. 7.5.\n"
      "B K 9... ..7. 9.7. ....\n"
      "B S .... 9... .... 9..5\n"
      "B D 7.5. 7.5. 7.5. 7.5.\n"
      "B X 6... .... .... ....\n" },
    { "POP 16", 4, 4, 100,
      "A K 9... ..7. 9... ....\n"
      "A S .... 9... .... 9...\n"
      "A H 7545 7545 7545 7545\n"
      "B K 9..7 ..7. 9.7. ..7.\n"
      "B S .... 9... .... 9...\n"
      "B C .... 7... .... 7...\n"
      "B H 7.4. 7.4. 7.4. 7...\n"
      "B O .... .... .... ..6.\n" },
    { "FUNK", 4, 4, 98,
      "A K 9..7 ..9. ..7. .9..\n"
      "A S .... 9..4 .4.. 9..4\n"
      "A H 7464 7464 7464 7464\n"
      "B K 9.7. ..9. .97. .9..\n"
      "B S .... 9..4 .4.9 ..4.\n"
      "B H 7464 7464 7464 74..\n"
      "B O .... .... .... ..7.\n"
      "a K 9... ..9. .... ....\n"
      "a S .... 9..5 .5.9 9999\n"
      "a H 7464 7464 .... ....\n" },
    { "SHUFFLE", 4, 3, 92,
      "A K 9.. ..6 9.. ...\n"
      "A S ... 9.. ... 9..\n"
      "A H 7.5 7.5 7.5 7.5\n"
      "B K 9.. ..7 9.. ..7\n"
      "B S ... 9.4 ... 9.4\n"
      "B D 8.6 8.6 8.6 8.6\n" },
    { "DISCO", 4, 4, 120,
      "A K 9... 9... 9... 9...\n"
      "A S .... 9... .... 9...\n"
      "A H 5... 5... 5... 5...\n"
      "A O ..8. ..8. ..8. ..8.\n"
      "B K 9... 9... 9... 9...\n"
      "B S .... 9... .... 9...\n"
      "B C .... 8... .... 8...\n"
      "B Z 5353 5353 5353 5353\n"
      "B O ..8. ..8. ..8. ..8.\n" },
    { "HIP-HOP", 4, 4, 90,
      "A K 9... ..7. .9.. ....\n"
      "A S .... 9... .... 9...\n"
      "A H 7.5. 7.5. 7.5. 7.5.\n"
      "B K 9..7 ..7. .9.. ..7.\n"
      "B S .... 9... .... 9...\n"
      "B R .... .... ..6. ....\n"
      "B H 7565 7.5. 7565 7.5.\n" },
    { "BOSSA", 4, 4, 130,
      "A K 9..7 9..7 9..7 9..7\n"
      "A R 8..8 ..8. ..8. .8..\n"
      "A Z 5353 5353 5353 5353\n"
      "B K 9..7 9..7 9..7 9..7\n"
      "B R 8..8 ..8. ..8. .8..\n"
      "B D 7.5. 7.5. 7.5. 7.5.\n" },
    { "REGGAE", 4, 4, 76,
      "A K .... .... 9... ....\n"
      "A R .... .... 9... ....\n"
      "A H 7.5. 7.5. 7.5. 7.5.\n"
      "B K 9... 9... 9... 9...\n"
      "B R .... .... 9... ....\n"
      "B H 7.57 7.57 7.57 7.57\n" },
    { "WALTZ 3/4", 3, 4, 120,
      "A K 9... .... ....\n"
      "A S .... 6... 6...\n"
      "A H 7.5. 7.5. 7.5.\n"
      "B K 9... .... ..6.\n"
      "B S .... 7... 7...\n"
      "B D 7.5. 7.5. 7.5.\n" },
    { "BALLAD 6/8", 2, 3, 56,
      "A K 9.. ...\n"
      "A S ... 9..\n"
      "A H 765 765\n"
      "B K 9.7 ...\n"
      "B S ... 9..\n"
      "B D 765 765\n" },
};
#define DR_NSTYLES ((int)(sizeof DR_STYLES / sizeof DR_STYLES[0]))
