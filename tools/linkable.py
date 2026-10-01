#!/usr/bin/env python3
"""Run diff_unit.py on every C file and report which units are LINKABLE.

Run under WSL from the repo root (after `ninja` has built the asm objects):

    .venv/bin/python tools/linkable.py            # report
    .venv/bin/python tools/linkable.py --write    # also rewrite config/c_units.txt with the LINKABLE set

Overlay units are recognized from config/gx_*.yaml; everything else is `main`.
"""
import argparse
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def unit_targets():
    """{unit: target} for every text unit in every splat config."""
    out = {}
    for cfg in sorted((ROOT / "config").glob("*.yaml")):
        for line in cfg.read_text(encoding="utf-8").splitlines():
            m = re.match(r"\s+- \[0x[0-9A-F]+, (asmtu|c), (.+)\]", line)
            if m:
                out.setdefault(m.group(2), cfg.stem)
    return out


def check(unit, target):
    arg = unit if target == "main" else f"{target}:{unit}"
    r = subprocess.run([sys.executable, "tools/diff_unit.py", arg], cwd=ROOT, capture_output=True, text=True)
    summary = next((ln for ln in r.stdout.splitlines() if " functions match" in ln), r.stdout.strip()[-200:])
    problems = [ln.strip() for ln in r.stdout.splitlines() if ln.strip().startswith(("!!", "XX", "--", "??"))]
    return unit, target, r.returncode == 0, summary, problems


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--write", action="store_true", help="rewrite config/c_units.txt with the LINKABLE units")
    args = ap.parse_args()
    targets = unit_targets()
    units = []
    for c in sorted((ROOT / "src").rglob("*.c")):
        unit = c.relative_to(ROOT / "src").with_suffix("").as_posix()
        if unit not in targets:
            print(f"?? {unit}: not a unit in any config")
            continue
        units.append((unit, targets[unit]))
    with ThreadPoolExecutor(max_workers=6) as pool:
        results = list(pool.map(lambda ut: check(*ut), units))
    good = [(u, t) for u, t, ok, _, _ in results if ok]
    for u, t, ok, summary, problems in results:
        print(f"{'LINKABLE' if ok else 'not yet '}  {summary}")
        if not ok:
            for p in problems[:3]:
                print(f"            {p}")
    print(f"\n{len(good)}/{len(results)} units LINKABLE")
    if args.write:
        text = "".join(f"{t} {u}\n" for u, t in sorted(good, key=lambda x: (x[1] != "main", x[1], x[0])))
        (ROOT / "config" / "c_units.txt").write_text(text, encoding="utf-8", newline="\n")
        print("config/c_units.txt updated")
    return 0


if __name__ == "__main__":
    sys.exit(main())
