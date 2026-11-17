// SPDX-License-Identifier: MIT
// Moonshine's Klatt formant TTS (vendor/moonshine/micro/klatt-tts, MIT) on the computer: one utterance to a WAV.
//   klatt_render ipa|text INPUT OUT.wav [RATE]
// ipa: an IPA string (stress marks welcome), the G2P bypassed; text: plain English through Moonshine's own G2P.
// Prints the samples written and the arena the utterance needed.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "tts/tts.h"

static void put32(FILE *f, uint32_t v) { fwrite(&v, 4, 1, f); }
static void put16(FILE *f, uint16_t v) { fwrite(&v, 2, 1, f); }

int main(int argc, char **argv) {
  if (argc < 4) {
    std::fprintf(stderr, "usage: klatt_render ipa|text INPUT OUT.wav [RATE]\n");
    return 2;
  }
  const bool ipa = std::strcmp(argv[1], "ipa") == 0;
  const float rate = argc > 4 ? static_cast<float>(std::atof(argv[4])) : 22050.0f;
  static uint8_t arena[256 * 1024];
  tts::VoiceParams voice = tts::DefaultVoiceParams();
  tts::StreamSynth synth(voice, arena, sizeof arena);
  tts::StreamOptions opts;
  opts.sample_rate = rate;
  const int rc = ipa ? synth.BeginIpa(argv[2], opts) : synth.BeginText(argv[2], opts);
  if (rc != tts::kStreamOk) {
    std::fprintf(stderr, "klatt_render: begin failed (%d) for %s\n", rc, argv[2]);
    return 1;
  }
  std::vector<int16_t> pcm;
  float buf[256];
  for (int n; (n = synth.Read(buf, 256)) > 0;)
    for (int i = 0; i < n; i++) {
      float v = buf[i] * 32767.0f;
      v = v > 32767.0f ? 32767.0f : v < -32768.0f ? -32768.0f : v;
      pcm.push_back(static_cast<int16_t>(v));
    }
  FILE *f = std::fopen(argv[3], "wb");
  if (!f) {
    std::perror("klatt_render");
    return 1;
  }
  const uint32_t sr = static_cast<uint32_t>(synth.sample_rate()), bytes = static_cast<uint32_t>(pcm.size() * 2);
  std::fwrite("RIFF", 1, 4, f), put32(f, 36 + bytes), std::fwrite("WAVEfmt ", 1, 8, f);
  put32(f, 16), put16(f, 1), put16(f, 1), put32(f, sr), put32(f, sr * 2), put16(f, 2), put16(f, 16);
  std::fwrite("data", 1, 4, f), put32(f, bytes), std::fwrite(pcm.data(), 2, pcm.size(), f);
  std::fclose(f);
  std::printf("%zu samples at %u Hz\n", pcm.size(), sr);
  return 0;
}
