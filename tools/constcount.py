#!/usr/bin/env python3
"""Search for a fitted stand-in that fixes a function's float-constant load order.

Which float-constant argument of a call MWCC materializes first depends on flag bytes left in
the compiler's memory by earlier code in the file, mostly the previous function's syntax tree
(docs/toolchain.md, "Root cause"). Code placed before a function changes those leftovers. This
tool inserts stand-ins of a simple shape, k float constants:

    static float __stripped_float_code_<n>(float x) { return x + 3.0f + 5.0f + ...; }

For each function that doesn't match, in file order, it tries k = 1..--max constants right before
the function and keeps the smallest k that makes it match without breaking any function that
matched before. A stand-in is fitted, not recovered: it represents no known code of the original,
and at many positions the original had no code at all (docs/stand-ins.md, tools/standins.py).
Stand-ins take the place of the blank line before the function when there is one (so __LINE__
doesn't move), else they come with a `#line` giving the logical line number; where no use of
__LINE__ follows it, remove that `#line` (it changes no bytes; tools/lint.py flags it).
`config/stripped_functions.txt` lists `__stripped_float_code*`, so the build removes them.

    .venv/bin/python tools/constcount.py Enemy/en_ike            # report
    .venv/bin/python tools/constcount.py Enemy/en_ike --apply    # also rewrite src/<unit>.c
"""
import argparse
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def run(arg, src):
    out = subprocess.run([sys.executable, "tools/diff_unit.py", arg, "--src", str(src)], cwd=ROOT,
                         capture_output=True, text=True).stdout
    return re.findall(r"  (OK|XX) (\w+)", out), "LINKABLE" in out


def standin(n, k):
    return f"static float __stripped_float_code_{n}(float x) {{ return x" + \
        "".join(f" + {3 + 2 * i}.0f" for i in range(k)) + "; }"


def logical_line(lines, i):
    """The line number the compiler sees for physical line index i (replaying `#line N`)."""
    for j in range(i - 1, -1, -1):
        m = re.match(r"\s*#line\s+(\d+)", lines[j])
        if m:
            return int(m.group(1)) + (i - j - 1)
    return i + 1


def build(lines, starts, inserts):
    """Source text with stand-ins inserted before the given functions ({func: k})."""
    out = list(lines)
    for n, (func, k) in enumerate(sorted(inserts.items(), key=lambda kv: -starts[kv[0]])):
        i = starts[func]  # 0-based line index of the definition
        text = standin(len(inserts) - n, k)
        if i > 0 and out[i - 1].strip() == "":
            out[i - 1] = text
        else:
            out[i:i] = [text, f"#line {logical_line(lines, i)}"]
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("unit", help="unit, optionally <target>:<unit>")
    ap.add_argument("--max", type=int, default=24, help="most constants to try per stand-in")
    ap.add_argument("--apply", action="store_true", help="rewrite src/<unit>.c with the result")
    args = ap.parse_args()
    unit = args.unit.split(":", 1)[-1]
    path = ROOT / "src" / f"{unit}.c"
    raw = path.read_bytes().decode("utf-8")
    nl = "\r\n" if "\r\n" in raw else "\n"
    lines = raw.split(nl)
    work = ROOT / "build" / "constcount" / unit.replace("/", "_")
    work.mkdir(parents=True, exist_ok=True)
    trial = work / Path(unit).with_suffix(".c").name

    def test(inserts):
        trial.write_text("\n".join(build(lines, starts, inserts)), encoding="utf-8", newline="\n")
        res, linkable = run(args.unit, trial)
        return {f for s, f in res if s == "OK"}, [f for s, f in res], linkable

    starts = {}
    trial.write_text("\n".join(lines), encoding="utf-8", newline="\n")
    res, _ = run(args.unit, trial)
    order = [f for _, f in res]
    for f in order:
        pat = re.compile(rf"^[A-Za-z_].*\b{re.escape(f)}\s*\((?!.*;\s*$)")
        hit = next((i for i, l in enumerate(lines) if pat.match(l)), None)
        if hit is not None:
            starts[f] = hit
    ok, _, linkable = test({})
    print(f"baseline: {len(ok)}/{len(order)}")
    inserts = {}
    for f in order:
        if f in ok or f not in starts:
            continue
        for k in range(1, args.max + 1):
            cand = dict(inserts, **{f: k})
            ok2, _, linkable2 = test(cand)
            if f in ok2 and ok <= ok2:
                inserts, ok, linkable = cand, ok2, linkable2
                print(f"  {f}: fixed with {k} constants before it ({len(ok)}/{len(order)})")
                break
        else:
            print(f"  {f}: no count up to {args.max} fixes it")
    print(f"result: {len(ok)}/{len(order)}{'  LINKABLE' if linkable else ''}; stand-ins: {inserts}")
    if args.apply and inserts:
        path.write_bytes(nl.join(build(lines, starts, inserts)).encode("utf-8"))
        print(f"wrote {path.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
