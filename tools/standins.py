#!/usr/bin/env python3
"""Where the original had room for the code a stand-in for stripped code stands in for.

A function the linker dead-stripped leaves no code, but its source lines still separate the
functions around it in the original's line table. A stand-in is every `__stripped_*` function
in src/: the constant-count stand-ins (`static float __stripped_float_code_*`,
`STRIPPED_DOUBLE_CODE()`, fitted to the compiler's state) and the reconstructions listed in
config/stripped_functions.txt (some evidence says a function stood there, but the body is a
guess, and it leaves compiler state like the others). For each, this compares the gap the
original's line table leaves between the previous and the next function with the usual gap
between functions in that file:

- room:     the gap is at least 3 lines bigger than usual: code of about that many lines could
            have stood there (a function can be that short: in the original's line table, 781 of
            its 4684 functions span 1 to 3 lines);
- no room:  the gap is at most 2 lines bigger than usual (or smaller): nothing stood there, so
            the stand-in represents no code of the original; it only reproduces a compiler state
            (the float-constant order or the software-double register mode) that the original's
            source produced some other way.
- unknown:  the file has too few functions (fewer than three gaps, or no gap size occurring
            twice) to tell its usual spacing, the next function isn't in the original, or it is
            the file's first function (the lines before it hold the file's declarations).

`inline` definitions are skipped: MWCC emits no function for them, so a stand-in before one
affects the next function that is emitted.

The verdict is about the stand-in's own position. Which functions need it is measured separately
(tools/standin_deps.py, config/standin_deps.txt) and listed with it: they need not be the next
function, and there can be several. Functions that only fail when all of a file's stand-ins are
removed at once (supported by more than one) are listed after the table, with the verdict of
the group (group_verdict()).

    python tools/standins.py              # Markdown report (docs/stand-ins.md)
"""
import re
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import dwarf_lines  # noqa: E402

FUNC = re.compile(r"^(?:static\s+)?[A-Za-z_][\w \*]*?\b(\w+)\s*\([^;{]*\)\s*\{", re.M)
INLINE = re.compile(r"(?:static\s+)?inline\b")  # not emitted as a function: the stand-in precedes the next one
# A stand-in: the definition of a `__stripped_*` function (up to its `{`; not a declaration or a
# call), or STRIPPED_DOUBLE_CODE() (include/common.h, which defines `__stripped_double_code`).
STANDIN = re.compile(r"^(?:(?:static\s+)?[A-Za-z_][\w \t*]*?\b(__stripped_\w+)\s*\([^;{]*\)\s*\{"
                     r"|(STRIPPED_DOUBLE_CODE)\(\))", re.M)


def original_functions():
    """{unit file: [(first line, last line, function)]} from the original's line table."""
    byfile = {}
    for src, fn, _lo, ent in dwarf_lines.original():
        lines = [e[0] for e in ent if e[0]]
        if lines:
            key = src.split("/src/")[-1] if "/src/" in src else src
            byfile.setdefault(key, []).append((min(lines), max(lines), fn))
    return {k: sorted(v) for k, v in byfile.items()}


def position(text, offset, funcs):
    """(next function, gap, usual gap, verdict) for a stand-in at `offset` in a C file's text,
    given the file's original functions (original_functions())."""
    gaps = [b[0] - a[1] - 1 for a, b in zip(funcs, funcs[1:])]
    # The usual gap between functions needs a clear majority: with a couple of gaps (or all
    # different) there is no telling the usual spacing from room for a stripped function.
    common = Counter(gaps).most_common(1)
    usual = common[0][0] if len(gaps) >= 3 and common[0][1] >= 2 else None
    nxt = next((f.group(1) for f in FUNC.finditer(text, offset)
                if not f.group(1).startswith("__stripped") and not INLINE.match(f.group(0))), None)
    here = [r for r in funcs if r[2] == nxt]
    if not here or usual is None:
        return nxt, None, usual, "unknown"
    start = here[0][0]
    prev = [r for r in funcs if r[0] < start]
    if not prev:
        # Before the file's first function the gap holds its includes and declarations.
        return nxt, None, usual, "unknown"
    room = start - max(r[1] for r in prev) - 1
    return nxt, room, usual, "room" if room > usual + 2 else "no room"


def classify():
    """[(unit file, stand-in, next function, gap, usual gap, verdict)] for every stand-in."""
    byfile = original_functions()
    rows = []
    for path in sorted((ROOT / "src").rglob("*.c")):
        text = path.read_text(encoding="utf-8")
        if not STANDIN.search(text):
            continue
        rel = path.relative_to(ROOT / "src").as_posix()
        for m in STANDIN.finditer(text):
            rows.append((rel, m.group(1) or m.group(2), *position(text, m.end(), byfile.get(rel, []))))
    return rows


def group_verdict(verdicts):
    """The verdict for functions that several stand-ins support together (they fail only when all
    of a file's stand-ins are removed): which of them they need is not measured, so "no room" only
    if every one has no room, "room" only if every one has room, otherwise "unknown"."""
    verdicts = set(verdicts)
    return verdicts.pop() if len(verdicts) == 1 else "unknown"


def main():
    import standin_deps
    rows = classify()
    deps = standin_deps.dependents()
    together = standin_deps.together()
    counts = Counter(r[5] for r in rows)
    print("# Stand-ins for stripped code: where the original had room for code\n")
    print("Generated by `tools/standins.py`. A stand-in is a `__stripped_*` function: code the linker")
    print("dead-stripped, so none of it is in the binary. The constant-count stand-ins")
    print("(`static float __stripped_float_code_*`, `STRIPPED_DOUBLE_CODE()`) are fitted: they recreate a")
    print("compiler state that later functions need, their float-constant order or the software-double")
    print("register mode (see [toolchain.md](toolchain.md), \"Root cause\"). The other `__stripped_*`")
    print("functions (`config/stripped_functions.txt`) are reconstructions: some evidence says a")
    print("function stood there, but the body is a guess, and it leaves compiler state too. None is")
    print("recovered code. \"Needed by\" lists a stand-in's dependents: the functions that stop matching")
    print("when it alone is removed (`tools/standin_deps.py`, `config/standin_deps.txt`). They need not")
    print("be the function right after it.\n")
    print("The verdict checks the stand-in's own position against the original's line table. A")
    print("function the linker dead-stripped still takes up source lines, so where the gap before the")
    print("next function is at most 2 lines bigger than the file's usual gap between functions (no")
    print("room), **no code stood there**: the stand-in represents no code of the original at all. It")
    print("only reproduces a compiler state that the original's source produced in some other way,")
    print("not found yet. Its dependents are fake matches; `tools/progress.py` counts them separately.")
    print("Where the gap is at least 3 lines bigger (room), code could have stood there: the stand-in")
    print("is still not recovered code, and its dependents count as fitted, not fake.\n")
    print("Unknown: the file has too few functions to tell its usual gap between functions (fewer than")
    print("three gaps, or no gap size occurring twice), the next function isn't in the original's line")
    print("table, or the stand-in comes before the file's first function (the lines there hold its")
    print("declarations). `tools/progress.py` counts their dependents as fitted at an unknown position,")
    print("apart from those with room.\n")
    print(f"{counts['no room']} stand-ins with no room, {counts['room']} with room, "
          f"{counts['unknown']} unknown.\n")
    print("| file | stand-in | before | gap (lines) | usual gap | verdict | needed by |")
    print("|---|---|---|---:|---:|---|---|")
    byunit = {}
    for rel, name, nxt, room, usual, verdict in rows:
        unit = Path(rel).with_suffix("").as_posix()
        byunit.setdefault(unit, []).append(verdict)
        d = deps.get((unit, name))
        needed = "not measured" if d is None else ", ".join(f"`{x}`" for x in d) or "nothing alone"
        print(f"| `{rel}` | `{name}` | `{nxt}` | {'' if room is None else room} | "
              f"{'' if usual is None else usual} | {verdict} | {needed} |")
    print("\n## Needed by several stand-ins together\n")
    print("Each file with more than one stand-in is also compiled with all of them removed at once. The")
    print("functions below still match without any one of them, but not without all: more than one")
    print("stand-in supports them. Which ones is not measured, so the verdict is \"no room\" only if every")
    print("stand-in in the file has no room, \"room\" only if every one has room, and \"unknown\" otherwise.")
    print("A stand-in listed above as needed by \"nothing alone\" in such a file may be one of those.\n")
    groups = sorted((u, d) for u, d in together.items() if d)
    if not groups:
        print("None.")
        return
    print("| file | verdict | needed by |")
    print("|---|---|---|")
    for unit, d in groups:
        print(f"| `{unit}.c` | {group_verdict(byunit.get(unit, ['unknown']))} | "
              f"{', '.join(f'`{x}`' for x in d)} |")


if __name__ == "__main__":
    main()
