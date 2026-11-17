"""CMUdict pronunciations (ARPAbet) as IPA for Moonshine's Klatt synth (which reads IPA with stress marks).

CMUdict is BSD-licensed (Carnegie Mellon University); `just voice-vendor` puts cmudict.dict in vendor/cmudict/.
"""

from __future__ import annotations

from pathlib import Path

# ARPAbet (CMUdict's 39 phonemes) -> IPA. AH and ER depend on stress: unstressed AH is schwa.
VOWELS = {
    "AA": "ɑ", "AE": "æ", "AO": "ɔ", "AW": "aʊ", "AY": "aɪ", "EH": "ɛ", "EY": "eɪ",
    "IH": "ɪ", "IY": "i", "OW": "oʊ", "OY": "ɔɪ", "UH": "ʊ", "UW": "u",
}
CONSONANTS = {
    "B": "b", "CH": "tʃ", "D": "d", "DH": "ð", "F": "f", "G": "g", "HH": "h", "JH": "dʒ", "K": "k", "L": "l",
    "M": "m", "N": "n", "NG": "ŋ", "P": "p", "R": "ɹ", "S": "s", "SH": "ʃ", "T": "t", "TH": "θ", "V": "v",
    "W": "w", "Y": "j", "Z": "z", "ZH": "ʒ",
}
STRESS = {"1": "ˈ", "2": "ˌ", "0": ""}


def phone_to_ipa(phone: str) -> str:
    """One ARPAbet phone ("AH0", "TH") as IPA, the stress mark (if any) in front of the vowel."""
    base, digit = (phone[:-1], phone[-1]) if phone[-1].isdigit() else (phone, "")
    if base in CONSONANTS:
        return CONSONANTS[base]
    if base == "AH":
        vowel = "ə" if digit == "0" else "ʌ"
    elif base == "ER":
        vowel = "ɚ" if digit == "0" else "ɝ"
    elif base in VOWELS:
        vowel = VOWELS[base]
    else:
        raise ValueError(f"not an ARPAbet phone: {phone}")
    return STRESS.get(digit, "") + vowel


def to_ipa(phones: list[str]) -> str:
    return "".join(phone_to_ipa(p) for p in phones)


def load(path: Path) -> dict[str, list[str]]:
    """cmudict.dict: "word PH1 PH2 ..." a line; the first pronunciation of each word ("word(2)" are variants)."""
    out: dict[str, list[str]] = {}
    for line in path.read_text(encoding="latin-1").splitlines():
        if not line or line.startswith(";;;"):
            continue
        word, *phones = line.split("#")[0].split()
        if "(" in word or word in out:
            continue
        out[word.lower()] = phones
    return out


def line_ipa(text: str, cmu: dict[str, list[str]]) -> tuple[str, list[str]]:
    """A line of words as IPA (words joined by spaces) and the words CMUdict does not have."""
    words, missing = [], []
    for raw in text.split():
        w = "".join(c for c in raw.lower() if c.isalpha() or c == "'")
        if not w:
            continue
        if w in cmu:
            words.append(to_ipa(cmu[w]))
        else:
            missing.append(w)
    return " ".join(words), missing


def phone_count(text: str, cmu: dict[str, list[str]]) -> int:
    """Phones in the line (what a packed one-byte-a-phone table would store), stress included in the vowel."""
    return sum(len(cmu.get("".join(c for c in w.lower() if c.isalpha() or c == "'"), [])) for w in text.split())
