"""python -m voicelab build [ENGINE ...] | score | page | firmware   (just voice-lab, voice-score, voice-open, voice-fw)"""

from __future__ import annotations

import sys


def main(argv: list[str]) -> int:
    cmd, rest = (argv[0], argv[1:]) if argv else ("build", [])
    if cmd == "build":
        from . import lab, page
        lab.build(set(rest) or None)
        page.write()
    elif cmd == "score":
        from . import page, score
        score.score()
        page.write()
    elif cmd == "firmware":
        from . import fwdata
        fwdata.write()
    elif cmd == "page":
        from . import page
        page.write()
    else:
        print(__doc__)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
