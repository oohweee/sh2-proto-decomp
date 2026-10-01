#!/usr/bin/env python3
"""Decompilation progress by code bytes and functions, per target, written to PROGRESS.md.

    python3 tools/progress.py          # print a summary and rewrite PROGRESS.md

Levels:
- linked:  functions in units built from C in the matching build (config/c_units.txt).
- matched: functions that match in any C file, including units not linked yet
           (from the latest tools/diff_unit.py run of each unit, build/diff/**/*.json).
           Functions linked from the original code (config/asm_functions.txt) are linked but
           not matched.

Every matched game function falls in exactly one of these categories, the first that applies:
- fake:    matched functions marked FAKEMATCH (STYLE.md): C that matches but that nobody would
           have written; and the dependents of stand-ins at a place where the original's line
           table shows that no code stood (tools/standins.py, docs/stand-ins.md): such a stand-in
           represents nothing of the original. A note `FAKEMATCH:` marks the function defined
           after it. A note `FAKEMATCH (for <function>[, <function>...] ...):` above a function
           whose own code comes out the same either way marks a fitted spelling there that sets
           a later function's float-constant order: the functions named in the parentheses are
           the fake ones (fakematch_targets()).
- fitted:  matched functions whose match relies on a stand-in for stripped code (a `__stripped_*`
           function: the constant-count stand-ins `__stripped_float_code*` and
           STRIPPED_DOUBLE_CODE(), docs/toolchain.md, and the guessed reconstructions of
           config/stripped_functions.txt): they stop matching when a stand-in alone is removed,
           or when all of their unit's stand-ins are removed at once (tools/standin_deps.py,
           config/standin_deps.txt). The bytes are right, the stand-in is not recovered code.
           Counted in two buckets by the stand-in's position (tools/standins.py): at an unknown
           position, then with room. A function needing several stand-ins takes the worst
           verdict (no room, then unknown, then room).
- order fit: matched functions whose float-constant order comes from a spelling of earlier code
           chosen for it among spellings that compile the same (config/order_fits.txt, STYLE.md):
           plausible source, but the exact spelling is inferred. Not fake; counted so the other
           figures can't hide them.
- assembly: matched functions written as assembly, not C: whole `asm` functions, and C functions
           whose code is mostly inline asm: at least half of the function's instructions (its
           size / 4) are instructions written in the `asm { }` blocks and `asm("...")` templates
           of its body (asm_functions(); labels, comments and directives other than
           `.word`/`.half`/`.byte` don't count). A function whose comment has a line starting
           `Original asm:` (like FAKEMATCH: `/* Original asm: <evidence> */`, or a
           ` * Original asm: <evidence>` line of the comment above it), giving the evidence that
           the original wrote it in assembly too, is counted as assembly in the original too;
           the others as assembly where the original may have had C. Either way it isn't clean C.
           C functions with inline asm for fewer than half of their instructions aren't a
           category: PROGRESS.md reports how many there are.
- clean:   the rest: matched from C with none of the above.

A unit is complete when it is built from C, no function in it is linked from the original code,
and no function in it is a fake match (fake_units()). `configure.py` marks exactly those units
complete in objdiff.json (what decomp.dev calls fully linked).

"Game" excludes the Sony libraries and the C library, newlib (both lib/*), crt0, the Metrowerks runtime and the
sound driver's EE side (sd0712/*, built outside the game's source tree), which are libraries.
"""
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import asm_fallback  # noqa: E402
import lint  # noqa: E402
from diff_unit import plain_name  # noqa: E402
THIRD_PARTY = ("lib/", "crt0", "mwcc_runtime/", "sd0712/")

COMMENT_START = r"^\s*(?:/\*+|\*|//)?\s*"
# `FAKEMATCH:` or `FAKEMATCH (for <function>'s float-constant order):` at the start of a comment line.
FAKEMATCH = re.compile(COMMENT_START + r"FAKEMATCH(?:\s*\(for ([^)]*)\))?:")
FAKEMATCH_LIKE = re.compile(r"FAKEMATCH\s*[:(]")  # anything that looks like a marker (lint)
ORIGINAL_ASM = re.compile(COMMENT_START + r"Original asm:")
FUNC_DEF = re.compile(r"^[A-Za-z_][^;()]*?\b(\w+)\s*\([^;]*\)\s*\{")
DEFINITION = re.compile(r"^(?:static\s+)?[A-Za-z_][\w \t*]*?\b(\w+)\s*\([^;{]*\)\s*\{", re.M)
VERDICTS = ("room", "unknown", "no room")  # from best to worst


def load_target(target):
    """[(unit, is_c, [(func name, size)])] for one target."""
    units = []
    for line in (ROOT / f"config/{target}.yaml").read_text(encoding="utf-8").splitlines():
        m = re.match(r"\s+- \[0x([0-9A-F]+), (asmtu|c), (.+)\]", line)
        if m:
            units.append((int(m.group(1), 16), m.group(3), m.group(2) == "c"))
    vram = 0x100000 if target == "main" else 0x1F01E00
    funcs = []
    for line in (ROOT / f"config/symbols_{target}.txt").read_text(encoding="utf-8").splitlines():
        m = re.match(r"(\S+) = 0x([0-9A-F]+); // type:func size:0x([0-9A-F]+)", line)
        if m:
            funcs.append((int(m.group(2), 16), m.group(1), int(m.group(3), 16)))
    funcs.sort()
    starts = sorted((a + vram, u, c) for a, u, c in units)
    out = []
    for i, (start, unit, is_c) in enumerate(starts):
        end = starts[i + 1][0] if i + 1 < len(starts) else 1 << 32
        out.append((unit, is_c, [(n, s) for a, n, s in funcs if start <= a < end]))
    return out


def c_files():
    """[(unit, path, text)] for every C file in src/."""
    return [(p.relative_to(ROOT / "src").with_suffix("").as_posix(), p, p.read_text(encoding="utf-8"))
            for p in sorted((ROOT / "src").rglob("*.c"))]


def markers(text, marker):
    """[(line number, marker match, function or None)] for each line of a C file's text that
    matches `marker`: the note is for the first function defined after it (None: there is none)."""
    out, pending = [], []
    for n, line in enumerate(text.splitlines(), 1):
        mk = marker.match(line)
        if mk:
            pending.append((n, mk))
        elif pending:
            m = FUNC_DEF.match(line)
            # inline definitions aren't emitted as functions: the marker is for the next one
            if m and "__stripped" not in line and not re.match(r"(static\s+)?inline\b", line):
                out += [(ln, k, m.group(1)) for ln, k in pending]
                pending = []
    return out + [(ln, k, None) for ln, k in pending]


def marked_functions(marker):
    """{(unit, function)} for the first function defined after each line that matches `marker`."""
    return {(unit, func) for unit, _, text in c_files() for _, _, func in markers(text, marker) if func}


def fakematch_targets(mk, host):
    """The functions a FAKEMATCH marker makes fake: the function it precedes (`host`), or for
    `FAKEMATCH (for a's ..., b):` the functions named in the parentheses."""
    if mk.group(1) is None:
        return [host] if host else []
    return [m.group(1) for part in re.split(r",|\band\b", mk.group(1))
            for m in [re.match(r"\s*([A-Za-z_]\w*)", part)] if m]


def fakematch_problems():
    """[(file, line, problem)]: FAKEMATCH markers that aren't parsed or aren't attributed to a
    function defined in their file (checked by tools/lint.py; needs only src/)."""
    out = []
    for _, path, text in c_files():
        rel = path.relative_to(ROOT).as_posix()
        defined = set(DEFINITION.findall(lint.strip_c(text)))
        parsed = {}
        for ln, mk, host in markers(text, FAKEMATCH):
            parsed[ln] = True
            targets = fakematch_targets(mk, host)
            if host is None:
                out.append((rel, ln, "FAKEMATCH note with no function defined after it"))
            elif not targets:
                out.append((rel, ln, "FAKEMATCH (for ...) names no function"))
            else:
                missing = [t for t in targets if t not in defined]
                if missing:
                    out.append((rel, ln, f"FAKEMATCH names {', '.join(missing)}: not defined in this file"))
        for n, line in enumerate(text.splitlines(), 1):
            if n not in parsed and FAKEMATCH_LIKE.search(line):
                out.append((rel, n, "FAKEMATCH note not parsed (tools/progress.py): write `FAKEMATCH:` or "
                                    "`FAKEMATCH (for <function>...):` at the start of a comment line"))
    return out


def fakematch_functions():
    """{(unit, function)} made fake by a FAKEMATCH note (STYLE.md; fakematch_targets())."""
    out = set()
    for unit, _, text in c_files():
        for _, mk, host in markers(text, FAKEMATCH):
            out |= {(unit, t) for t in fakematch_targets(mk, host)}
    return out


def standin_dependents():
    """{(unit, stand-in): [dependents]}: what each stand-in is needed by alone (config/standin_deps.txt,
    measured by tools/standin_deps.py). An entry starting with `!` is a layout problem or compile
    error that removing it causes where no function fails."""
    import standin_deps
    return standin_deps.dependents()


def standin_positions():
    """{(unit, stand-in): "room" | "no room" | "unknown"}: whether the original's line table has
    room for code where each stand-in stands (tools/standins.py). It needs the original executable
    (baserom/disc/): without it the fake matches can't be told apart, so this stops."""
    import standins
    try:
        rows = standins.classify()
    except FileNotFoundError as e:
        sys.exit(f"tools/progress.py: the stand-ins' positions need the original executable: {e}")
    return {(Path(r[0]).with_suffix("").as_posix(), r[1]): r[5] for r in rows}


def standin_verdicts(measured=None, positions=None):
    """{(unit, function): verdict} for every function a stand-in is needed by: the worst verdict
    among the stand-ins it needs (no room, then unknown, then room). A unit's `together`
    dependents (several stand-ins support them) take tools/standins.py's group_verdict()."""
    import standin_deps
    import standins
    measured = standin_dependents() if measured is None else measured
    positions = standin_positions() if positions is None else positions
    worst, byunit = {}, {}

    def add(unit, deps, verdict):
        for d in deps:
            if not d.startswith("!") and VERDICTS.index(verdict) >= VERDICTS.index(worst.get((unit, d), "room")):
                worst[(unit, d)] = verdict

    for (unit, name), deps in measured.items():
        verdict = positions.get((unit, name), "unknown")
        byunit.setdefault(unit, []).append(verdict)
        add(unit, deps, verdict)
    for unit, deps in standin_deps.together().items():
        add(unit, deps, standins.group_verdict(byunit.get(unit, ["unknown"])))
    return worst


def fake_functions(verdicts=None):
    """{(unit, function)}: the fake matches: FAKEMATCH notes, and the dependents of stand-ins
    where the original's line table shows that no code stood."""
    verdicts = standin_verdicts() if verdicts is None else verdicts
    return fakematch_functions() | {k for k, v in verdicts.items() if v == "no room"}


def fake_units():
    """Units containing a fake match (fake_functions()). They are never complete: configure.py
    marks them incomplete in objdiff.json, and PROGRESS.md doesn't count them."""
    return {unit for unit, _ in fake_functions()}


def order_fit_functions():
    """{(unit, function)} listed in config/order_fits.txt."""
    path = ROOT / "config" / "order_fits.txt"
    if not path.exists():
        return set()
    out = set()
    for line in path.read_text(encoding="utf-8").splitlines():
        p = line.split("#")[0].split()
        if len(p) == 2:
            out.add((p[0], p[1]))
    return out


ASM_DEFINITION = re.compile(r"^(?:static\s+)?(?:inline\s+)?(asm\s+)?[A-Za-z_][\w \t*]*?\b(\w+)\s*\([^;{]*\)\s*\{",
                            re.M)
ASM_STATEMENT = re.compile(r"\b(?:asm|__asm__|__asm)\b(?:\s+(?:volatile|__volatile__))?\s*([{(])")


def _close(code, i):
    """The offset just past the bracket that closes the one at code[i]."""
    op, cl, depth = code[i], {"{": "}", "(": ")"}[code[i]], 0
    while True:
        depth += (code[i] == op) - (code[i] == cl)
        i += 1
        if depth == 0:
            return i


def asm_instructions(src):
    """The number of instructions in asm source: lines (or `;`-separated parts) that aren't blank,
    labels or assembler directives (`.word`, `.half` and `.byte` emit code and count)."""
    n = 0
    for part in re.split(r"\n|;|\\n", src):
        s = re.sub(r"^[A-Za-z_.$]\w*:\s*", "", part.strip())  # a label
        if s and (not s.startswith(".") or s.startswith((".word", ".half", ".byte"))):
            n += 1
    return n


def asm_bodies(text):
    """{function: (whole asm function?, instructions written in asm)} for every function defined in
    a C file's text that is an `asm` function or has inline asm statements in its body."""
    code = lint.strip_c(text)  # comments, literals and preprocessor lines blanked; same offsets
    out = {}
    for m in ASM_DEFINITION.finditer(code):
        start, end = m.end() - 1, _close(code, m.end() - 1)
        if m.group(1):
            out[m.group(2)] = (True, asm_instructions(code[start + 1:end - 1]))
            continue
        n = 0
        for a in ASM_STATEMENT.finditer(code, start, end):
            i = a.end() - 1
            j = _close(code, i)
            if a.group(1) == "{":
                n += asm_instructions(code[i + 1:j - 1])
            else:
                # asm("template" : operands): the template's string literals, read from the source
                colon = code.find(":", i, j)
                seg = text[i + 1:j - 1 if colon < 0 else colon]
                n += asm_instructions("".join(re.findall(r'"((?:[^"\\]|\\.)*)"', seg, re.S)))
        if n:
            out[m.group(2)] = (False, n)
    return out


def asm_functions(sizes):
    """{(unit, function): "whole" | "inline" | "partly"}: the functions written as assembly (the
    module docstring), given {(unit, function): size in bytes} of the functions in the binary, and
    ("partly") the C functions with inline asm statements for fewer than half of their
    instructions, which aren't counted as assembly (they are only reported)."""
    out = {}
    for unit, _, text in c_files():
        for name, (whole, n) in asm_bodies(text).items():
            size = sizes.get((unit, name))
            if size is None:
                continue  # not in the binary (an inline helper)
            if whole:
                out[(unit, name)] = "whole"
            else:
                out[(unit, name)] = "inline" if 2 * n >= size // 4 else "partly"
    return out


CATEGORIES = [  # (category, PROGRESS.md description), in the order they apply
    ("fake", "fake: C marked `FAKEMATCH` (STYLE.md), or needed by a stand-in at a place where the "
             "original's line table shows no code stood (docs/stand-ins.md)"),
    ("fitted, unknown", "fitted: needed by a stand-in for stripped code whose position can't be checked "
                        "against the original's line table (unknown)"),
    ("fitted, room", "fitted: needed by a stand-in for stripped code where the original had room for "
                     "code (still not recovered code)"),
    ("order fit", "order fit: a spelling of earlier code chosen for the float-constant order among "
                  "spellings that compile the same (config/order_fits.txt): plausible, but inferred"),
    ("assembly, original", "written as assembly, with an `Original asm:` note giving the evidence that "
                           "the original was assembly too"),
    ("assembly", "written as assembly, with no evidence yet that the original was"),
    ("clean", "clean: matched from C with none of the above"),
]


def main():
    targets = sorted(p.stem for p in (ROOT / "config").glob("*.yaml"))
    rows = []
    totals = {k: [0, 0, 0, 0, 0, 0] for k in ("all", "game")}  # bytes, funcs, linked b/f, matched b/f
    import standin_deps
    measured = standin_dependents()
    positions = standin_positions()
    for k in positions:
        if k not in measured:
            print(f"warning: stand-in {k[0]} {k[1]} isn't in config/standin_deps.txt: run tools/standin_deps.py",
                  file=sys.stderr)
    together = standin_deps.together()
    dead = sorted(k for k, v in measured.items() if not v and k[0] not in together)
    for unit, name in dead:
        print(f"dead stand-in: {unit} {name} has no dependents (config/standin_deps.txt): remove it")
    verdicts = standin_verdicts(measured, positions)
    fake = fake_functions(verdicts)
    bad_units = {unit for unit, _ in fake}
    order_fits = order_fit_functions()
    original_asm = marked_functions(ORIGINAL_ASM)
    loaded = {t: load_target(t) for t in targets}
    sizes = {(unit, plain_name(n)): s for t in targets for unit, _, funcs in loaded[t] for n, s in funcs}
    asm = asm_functions(sizes)
    partly = {k for k, v in asm.items() if v == "partly"}
    asm = {k: v for k, v in asm.items() if v != "partly"}
    for unit, name in sorted(original_asm - set(asm)):
        print(f"warning: {unit} {name}: `Original asm:` note on a function not written as assembly",
              file=sys.stderr)
    for unit, name in sorted(fake - set(sizes)):
        print(f"warning: fake match {unit} {name} is not a function of that unit", file=sys.stderr)

    def category(key):
        if key in fake:
            return "fake"
        if key in verdicts:
            return "fitted, unknown" if verdicts[key] == "unknown" else "fitted, room"
        if key in order_fits:
            return "order fit"
        if key in asm:
            return "assembly, original" if key in original_asm else "assembly"
        return "clean"

    cats = {c: [0, 0] for c, _ in CATEGORIES}  # game: category -> [bytes, functions]
    asm_kinds = {"whole": 0, "inline": 0}
    n_partly = 0  # matched game functions with some inline asm, under the 50% rule
    seen = set()
    complete = [0, 0, 0]  # game: code bytes and number of complete units (fake_units()), C units
    for t in targets:
        tb = [0] * 6
        for unit, is_c, funcs in loaded[t]:
            result = ROOT / "build" / "diff" / t / f"{unit}.c.json"
            matched = set(json.loads(result.read_text())["matched"]) if result.exists() else set()
            original = set(asm_fallback.listed(t, unit))
            if is_c and not unit.startswith(THIRD_PARTY):
                complete[2] += 1
                if not original and unit not in bad_units:
                    complete[0] += sum(size for _, size in funcs)
                    complete[1] += 1
            for name, size in funcs:
                key = (unit, plain_name(name))
                ok = (is_c and plain_name(name) not in original) or name in matched
                if key in order_fits:
                    seen.add(key)
                if ok and not unit.startswith(THIRD_PARTY):
                    c = category(key)
                    cats[c] = [cats[c][0] + size, cats[c][1] + 1]
                    if c.startswith("assembly"):
                        asm_kinds[asm[key]] += 1
                    elif key in partly:
                        n_partly += 1
                vals = [size, 1, size if is_c else 0, 1 if is_c else 0, size if ok else 0, 1 if ok else 0]
                keys = ["all"] + ([] if unit.startswith(THIRD_PARTY) else ["game"])
                for k in keys:
                    totals[k] = [a + b for a, b in zip(totals[k], vals)]
                tb = [a + b for a, b in zip(tb, vals)]
        rows.append((t, tb))

    pct = lambda a, b: 100.0 * a / b if b else 0.0
    g = totals["game"]
    clean = cats["clean"]
    n_asm = cats["assembly"][1] + cats["assembly, original"][1]
    n_orig = sum(1 for line in asm_fallback.LIST.read_text(encoding="utf-8").splitlines()
                 if len(line.split("#")[0].split()) == 2) if asm_fallback.LIST.exists() else 0
    lines = ["# Progress", "", "Generated by `tools/progress.py`.", "",
             f"**Clean matches: {pct(clean[0], g[0]):.2f}% of game code ({clean[1]} of {g[1]} functions)**, "
             "matched from C with no stand-in, fake match, order fit or assembly (see below). "
             f"Counting those too: {pct(g[4], g[0]):.2f}% ({g[5]} functions).", "",
             "| scope | code | in C units | matched |",
             "|---|---|---|---|"]
    for label, v in [("game code", totals["game"]), ("all code", totals["all"])] + rows:
        lines.append(f"| {label} | {v[0]:,} B / {v[1]} funcs | {pct(v[2], v[0]):.2f}% ({v[3]} funcs) | "
                     f"{pct(v[4], v[0]):.2f}% ({v[5]} funcs) |")
    lines += ["", f"The {g[5]} matched game functions, each counted once, in the first of these categories "
              "that applies:", "", "| category | functions | game code |", "|---|---:|---:|"]
    lines += [f"| {text} | {cats[c][1]} | {pct(cats[c][0], g[0]):.2f}% |" for c, text in CATEGORIES]
    lines += ["",
              "A stand-in for stripped code is a `__stripped_*` function (docs/stand-ins.md): a fitted "
              "constant-count stand-in or a guessed reconstruction. The functions that need it stop matching "
              "when it alone is removed, or when all the stand-ins of their file are removed at once "
              "(config/standin_deps.txt). "
              + (f"{len(dead)} stand-ins are needed by nothing (dead code, to be removed). " if dead else "")
              + f"Written as assembly: {asm_kinds['whole']} whole `asm` functions and {asm_kinds['inline']} C "
              "functions at least half of whose instructions are written in inline asm statements (the rule "
              f"is in `tools/progress.py`). {n_partly} more matched functions have inline asm statements for "
              "fewer than half of their instructions; they count in the other categories. "
              + (f"{n_orig} functions are linked from the original code (config/asm_functions.txt); they "
                 "count as linked, not matched." if n_orig != 1 else
                 "1 function is linked from the original code (config/asm_functions.txt); it counts as "
                 "linked, not matched."),
              "",
              "\"In C units\" counts every function of a unit built from C, including one linked from the "
              "original code.",
              "",
              f"Complete units: {complete[1]} of {complete[2]} game units, {pct(complete[0], g[0]):.2f}% of "
              "game code. A unit is complete when it is built from C, has no function linked from the "
              "original code and has no fake match; it can still contain the fitted matches, order fits and "
              "assembly above. `configure.py` marks exactly these units complete in `objdiff.json`, with the "
              "same definition (`fake_units()` in `tools/progress.py`), so decomp.dev's \"fully linked\" "
              "counts the same units."]
    for unit, name in sorted(order_fits - seen):
        print(f"warning: config/order_fits.txt: no function {unit} {name}", file=sys.stderr)
    text = "\n".join(lines) + "\n"
    (ROOT / "PROGRESS.md").write_text(text, encoding="utf-8", newline="\n")
    n = lambda c: cats[c][1]
    print(f"game code: clean {pct(clean[0], g[0]):.2f}% ({clean[1]} funcs); {pct(g[2], g[0]):.2f}% in C units "
          f"({g[3]}/{g[1]} funcs), {pct(g[4], g[0]):.2f}% matched ({g[5]} funcs: {n('fake')} fake, "
          f"{n('fitted, unknown')} fitted at an unknown position, {n('fitted, room')} fitted with room, "
          f"{n('order fit')} order fits, {n_asm} assembly ({n('assembly, original')} assembly in the original "
          f"too; {asm_kinds['whole']} whole asm functions, {asm_kinds['inline']} mostly inline asm)); "
          f"{n_orig} funcs linked from original code; complete units {complete[1]}/{complete[2]} "
          f"({pct(complete[0], g[0]):.2f}% of game code)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
