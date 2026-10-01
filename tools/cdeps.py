#!/usr/bin/env python3
"""Write a Make-style dependency file for a C file: every header it includes, recursively.

MWCC's own -MD writes Windows paths into the working directory (colliding between files with
the same name when ninja builds in parallel), so this scans `#include "..."` itself, the only
include form the project uses, with the same search path as the compiler (the file's directory,
then include/).

    cdeps.py src/foo.c build/src/foo.c.o     # writes build/src/foo.c.o.d
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
INCLUDE = re.compile(r'^\s*#\s*include\s+"([^"]+)"', re.M)


def scan(path, seen):
    for name in INCLUDE.findall(path.read_text(encoding="utf-8", errors="replace")):
        for base in (path.parent, ROOT / "include"):
            found = (base / name).resolve()
            if found.exists():
                if found not in seen:
                    seen.add(found)
                    scan(found, seen)
                break
    return seen


def main():
    src, out = Path(sys.argv[1]), sys.argv[2]
    deps = sorted(p.relative_to(ROOT).as_posix() for p in scan((ROOT / src).resolve(), set()))
    Path(out + ".d").write_text(f"{out}: {src.as_posix()} " + " ".join(deps) + "\n",
                                encoding="utf-8", newline="\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
