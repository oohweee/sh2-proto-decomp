#!/usr/bin/env python3
"""Measure how far the C of each function in config/asm_functions.txt is from the original.

The matching build links those functions from the original code, so their own C is never
compared. This compiles each listed unit with the fallback switched off and prints, per
function, the number of differing words and the sizes (C vs original), as a Markdown table
for docs/nonmatching.md. Run it in the build environment after `ninja`:

    .venv/bin/python tools/nonmatching.py            # all listed functions
    .venv/bin/python tools/nonmatching.py Font/font  # only these units

Uses `diff_unit.py --src`, so the results of the regular diff run (build/diff/) are left alone.
"""
import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
XX = re.compile(r"^\s+XX (\S+): (\d+) differing words(?: \(size 0x([0-9a-f]+) vs 0x([0-9a-f]+)\))?")
OK = re.compile(r"^\s+OK (\S+)")


def measure(unit):
    """Returns {function: (differing words, C size or None, original size or None)} for a unit."""
    src = ROOT / "src" / (unit.split(":")[-1] + ".c")
    env = dict(os.environ, ASM_FALLBACK_LIST=os.devnull)
    out = subprocess.run([sys.executable, str(ROOT / "tools/diff_unit.py"), unit, "--src", str(src)],
                         cwd=ROOT, env=env, capture_output=True, text=True).stdout
    result = {}
    for line in out.splitlines():
        m = XX.match(line)
        if m:
            size = (int(m.group(3), 16), int(m.group(4), 16)) if m.group(3) else (None, None)
            result[m.group(1)] = (int(m.group(2)),) + size
        m = OK.match(line)
        if m:
            result[m.group(1)] = (0, None, None)
    return result


def listed():
    """(unit, function) pairs of config/asm_functions.txt, in file order."""
    out = []
    for line in (ROOT / "config/asm_functions.txt").read_text(encoding="utf-8").splitlines():
        parts = line.split("#")[0].split()
        if len(parts) == 2:
            out.append((parts[0], parts[1]))
    return out


def main():
    entries = listed()
    units = sorted({u for u, _ in entries if len(sys.argv) < 2 or u in sys.argv[1:]})
    with ThreadPoolExecutor(max_workers=os.cpu_count()) as pool:
        results = dict(zip(units, pool.map(measure, units)))
    print("| unit | function | differing words | size (C / original) |")
    print("|---|---|---:|---|")
    total = 0
    for unit, func in entries:
        if unit not in results:
            continue
        words, ours, orig = results[unit].get(func, (None, None, None))
        if words is None:
            cell = "not found"
        elif words == 0:
            cell = "**0 (matches: remove it from the list)**"
        else:
            cell = str(words)
            total += words
        size = "same" if ours is None else f"0x{ours:X} / 0x{orig:X}"
        print(f"| `{unit}` | `{func}` | {cell} | {size} |")
    print(f"\n{len([1 for u, _ in entries if u in results])} functions, {total} differing words in all")
    return 0


if __name__ == "__main__":
    sys.exit(main())
