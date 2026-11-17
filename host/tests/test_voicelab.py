# SPDX-License-Identifier: MIT
"""voicelab's pure parts: ARPAbet to IPA, IMA ADPCM, the word list, and how a recogniser's answer is judged."""

import math

from voicelab import adpcm, arpabet, lab, score


def test_arpabet_stress_and_reduced_vowels():
    assert arpabet.to_ipa(["TH", "IH1", "NG", "K"]) == "θˈɪŋk"
    # vocabulary: V OW0 K AE1 B Y AH0 L EH2 R IY0
    assert arpabet.to_ipa("V OW0 K AE1 B Y AH0 L EH2 R IY0".split()) == "voʊkˈæbjəlˌɛɹi"
    assert arpabet.phone_to_ipa("AH1") == "ˈʌ" and arpabet.phone_to_ipa("AH0") == "ə"
    assert arpabet.phone_to_ipa("ER0") == "ɚ" and arpabet.phone_to_ipa("ER1") == "ˈɝ"
    assert arpabet.phone_to_ipa("JH") == "dʒ" and arpabet.phone_to_ipa("CH") == "tʃ"


def test_arpabet_line_and_missing_words():
    cmu = {"practice": "P R AE1 K T AH0 S".split(), "makes": "M EY1 K S".split()}
    ipa, missing = arpabet.line_ipa("Practice makes perfect.", cmu)
    assert ipa == "pɹˈæktəs mˈeɪks" and missing == ["perfect"]
    assert arpabet.phone_count("practice makes", cmu) == 11


def test_adpcm_round_trip_is_close_and_half_a_byte_a_sample():
    pcm = [int(12000 * math.sin(i * 0.05)) for i in range(2000)]
    enc = adpcm.encode(pcm)
    blocks = -(-len(pcm) // adpcm.BLOCK)
    assert len(enc) <= blocks * 256
    dec = adpcm.decode(enc, len(pcm))
    assert len(dec) == len(pcm)
    err = math.sqrt(sum((a - b) ** 2 for a, b in zip(pcm, dec)) / len(pcm))
    sig = math.sqrt(sum(a * a for a in pcm) / len(pcm))
    assert 20 * math.log10(sig / err) > 20   # better than 20 dB SNR on a plain tone


def test_words_and_slugs():
    words = lab.load_words()
    assert "think" in words and "sink" in words and len(words) >= 20
    assert lab.slug("Practice makes perfect.") == "practice-makes-perfect"
    assert len({lab.slug(w) for w in words}) == len(words)


def test_heard_ok():
    assert score.heard_ok("three", "3.")
    assert score.heard_ok("world", "the world.")
    assert score.heard_ok("right", "All right.")
    assert not score.heard_ok("right", "Alright.")
    assert not score.heard_ok("tree", "3.")
    assert score.heard_ok("Practice makes perfect.", "Practice makes perfect!")
