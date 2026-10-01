#!/usr/bin/env python3
"""Compare the statement structure of matched functions with the original's, through the line
tables: the statement-level counterpart of tools/dwarf_compare.py.

Every C unit is compiled with `-g` (same code, see docs/toolchain.md) and, per matched function
(not in config/asm_functions.txt), our .line table is compared with the original's
(tools/dwarf_lines.py). MWCC writes an entry where a statement's code starts on another line than
the previous entry's, so an entry is a statement start that is also a line change. Matched
functions have the original's code, so entries are keyed by code offset and line up exactly.
Where only one side has an entry, the difference is one of three kinds:

- STRUCTURE: the syntax trees differ (statements merged or split, `&&` for nested `if`s, an inline
  function for written-out statements). Besides source fidelity this matters to matching: the
  tree decides what MWCC leaves in its arena (the order of float constants in a file).
- MACRO: a call of our function-like macro where the original wrote the statements out (or its
  call over several lines). The same tokens, so the same tree: source fidelity only.
- LAYOUT: only the line breaks differ.

Which one, by the side that has the entry:

- orig-only: the original starts a statement on a new line where we don't. LAYOUT if our line
  there holds several statements (`if (x) { y = 1; }` where the original broke the line; a
  `} else` line holds the jump over the else as well). At the test of our `} else if`, the
  original's `if` is on a later line than the `}` (MWCC gives the jump the `}`'s line): LAYOUT
  for `}` / `else if` or a one-line branch before `else if`, STRUCTURE for `} else {` + a nested
  `if`, decided by the line table (see else_if: room for the nested block's `}`, and where the
  function's or file's plain `else`s put their first statement); where it can't decide, the
  reason says so (tag `else-if?` in the report). Otherwise MACRO for a call of our macro, else
  STRUCTURE: our statement merges what the original wrote as several. The report guesses what
  from our line: an inline function of ours (the original wrote its statements out), `&&`
  (nested `if`s), a loop written differently, a chained assignment, `?:`, a nested call (a
  temporary).
- ours-only: we start a statement where the original's line doesn't change, so the original had
  it on the previous statement's line, or started no statement there. LAYOUT when the former is
  likely: the first statement of a body after its header (`if (x) y = 1;`, `for (...) for (...)`),
  a line of ours that starts with `}` (a jump or loop test lands on a brace the original's
  unbraced body didn't have), more statement starts than the original has lines there
  (`a = 0; b = 0;`), and by default. STRUCTURE when the latter is: an `if` directly inside an
  `if` (the original's `&&`, which gets no entry even over several lines), an `if` directly inside
  our `} else {` (the original's `else if`), inline-assembly instructions the original's source
  didn't have (asm gets an entry per instruction), a test where the original has lines to spare
  (folded into the previous statement), and assignments of the previous one's value where the
  original spends a line on each but starts no statement (one chained assignment).

The line table can't settle everything (`if (a) if (b) y;` on one line looks like `if (a && b)`,
two statements on one line like one statement over two lines, `}` / `else` / `if` like `}` /
`else {` / `if`); the rules pick the likelier reading. The original has no column information;
dead statements leave no entry on either side; the line distance between statements both sides
start (blank lines, comments) isn't compared. A `#line` that goes back before the function's first
line makes MWCC stop writing entries: the rest of such a function is 'untracked'.

    python tools/layout_compare.py              # writes docs/layout-fidelity.md, prints a summary
    python tools/layout_compare.py Font/font    # one unit: every difference, with our source line
    python tools/layout_compare.py -j4 ...      # at most 4 compiles at a time (default 8)
    UNIT_SRC=v.c python tools/layout_compare.py Font/font   # the same for a variant of the unit

Differences don't change the bytes; structural and macro ones are where the C still differs from
the original's source, with the evidence to fix it (STYLE.md, "Evidence decides").
`tools/dwarf_lines.py layout <unit> <function>` shows a function's two tables side by side.
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile
from collections import Counter
from multiprocessing import Pool
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT))
import dwarf_lines  # noqa: E402

CONTROL = re.compile(r"(?:\}\s*)?(?:else\s+)?(?:if|for|while|switch)\b")
HEADER = re.compile(r"^(?:\}\s*)?(?:(?:else\s+)?(?:if|for|while|switch)\b.*|else|do)\s*\{$")
ELSE_LINE = re.compile(r"\}\s*else\b")
ELSE_IF_LINE = re.compile(r"\}\s*else\s+if\b")


def strip_code(line):
    """A source line without comments, string/char literals and parenthesised text."""
    line = re.sub(r"/\*.*?\*/|//.*$", " ", line)
    line = re.sub(r"\"(?:\\.|[^\"\\])*\"|'(?:\\.|[^'\\])*'", "\"\"", line)
    line = line.split("/*")[0]
    prev = None
    while prev != line:
        prev, line = line, re.sub(r"\([^()]*\)", "\x01", line)
    return line.replace("\x01", "()").strip()


def statements_on(line):
    """Roughly how many statements start on a source line: `;`s outside parentheses, plus control
    statements with their body on the same line (`if (x) { y = 1; }` counts two), plus the jump
    over an `else` that a line starting `} else` holds (MWCC gives it the line of the `}`)."""
    s = strip_code(line)
    n = s.count(";") + len(re.findall(r"\b(?:if|for|switch)\b", s))
    n += len(re.findall(r"\bwhile\b(?!\s*\(\)\s*;)", s))
    return n + bool(ELSE_LINE.match(s))


def is_header(line):
    """Whether a line opens a control statement's braced body (`if (x) {`, `} else {`, `do {`)."""
    return bool(HEADER.match(strip_code(line)))


def is_control(line):
    return bool(CONTROL.match(strip_code(line)))


def is_asm(line):
    """Whether a line (of a function with an `asm` block) is an instruction rather than C."""
    s = re.sub(r"/\*.*?\*/|//.*$", " ", line).strip()
    return bool(re.match(r"[a-z][a-z0-9.]*(?:\s+[\w$(-].*)?$", s)) and not s.endswith((";", "{", "}")) \
        and not re.match(r"(?:return|else|case|goto|do)\b", s)


def rhs(line):
    """The right-hand side of a simple assignment statement, or None."""
    m = re.match(r"[^=;]*[^=!<>+\-*/%&|^]=([^=][^;]*);$", strip_code(line))
    return m.group(1).strip() if m else None


def merged_hint(line, helpers=None, first=True):
    """(kind, reason): what our statement probably merged that the original had separate (for
    orig-only rows). kind is 'macro' for a call of a function-like macro of ours (the original
    wrote the statements out, or its call over several lines: the same tokens, so the same syntax
    tree), else 'structure'. helpers: {name: 'inline' or 'macro'} of our inline functions and
    function-like macros; first: the first orig-only row in our statement (later ones in a
    `} else if` aren't at its test)."""
    helpers = helpers or {}
    s = strip_code(line)
    called = re.match(r"(?:[\w.\->\[\]*]+\s*=\s*)?(\w+)\s*\(", s)
    name = called.group(1) if called else None
    if name in helpers and helpers[name] == "macro":
        return "macro", f"macro `{name}`: the original wrote its statements out (the same tokens)"
    if name in helpers:
        return "structure", f"our helper `{name}` (inline function): the original wrote its statements out"
    if name and re.fullmatch(r"[A-Z][A-Z0-9_]+", name):
        return "macro", f"macro `{name}`?: the original wrote its statements out (the same tokens)"
    return "structure", tree_hint(line, first)


def tree_hint(line, first=True):
    """The reason for an orig-only row at a statement of ours that isn't a helper or macro call."""
    s = strip_code(line)
    full = re.sub(r"/\*.*?\*/|//.*$", " ", line).strip()
    if first and ELSE_IF_LINE.match(s):
        return "`else if`: the original's `if` is on a later line than its `else`"
    if re.match(r"(?:\}\s*)?(?:else\s+)?(?:if|while)\b", s) and "&&" in full:
        return "`&&`: nested `if`s in the original?"
    if re.match(r"(?:\}\s*)?(?:else\s+)?(?:if|while)\b", s) and "||" in full:
        return "`||`: separate tests in the original?"
    if re.match(r"for\b", s) and re.search(r"\bfor\s*\([^;]*,|;[^;]*;[^)]*,", full):
        return "comma in a `for` header: separate statements in the original?"
    if re.match(r"(?:for|while)\b", s):
        return "loop header: the original's loop was written differently"
    if s == "}" or re.match(r"\}\s*while\b", s):
        return "loop end: the original's loop was written differently"
    if "?" in full and ":" in full:
        return "`?:`: `if`/`else` in the original?"
    if re.match(r"[^=]*[^=!<>]=[^=][^=]*[^=!<>]=[^=]", s):
        return "chained assignment: separate assignments in the original?"
    if re.search(r"=\s*\{", full):
        return "initializer: separate assignments in the original?"
    if re.search(r"\b\w+\s*\((?:[^()]|\([^()]*\))*\b\w+\s*\(", full):
        return "nested call: a temporary in the original?"
    return "statement of ours merges the original's"


def body(entries):
    """[(offset, line)] of a line table, closing brace (the last entry) and line 0 dropped."""
    rows = [(off, ln) for ln, _, off in entries if ln]
    return rows[:-1] if len(rows) > 1 else rows


def classify(ours, orig, text, truncated=False, has_asm=False, helpers=None, unit_style=None):
    """Differences between our and the original's line tables of one function.

    ours, orig: dwarf_lines entries; text: {logical line: source text} of our file (every line of
    the function); truncated: our table stops early (see function_text), so it has no
    closing-brace entry and nothing after its last entry is compared; has_asm: the function has an
    `asm` block; helpers: {name: 'inline' or 'macro'} of our inline functions and function-like
    macros; unit_style: else_evidence summed over the file's functions.
    Returns [(offset, side, kind, orig_line, our_line, source, reason)], side 'orig' or 'ours'
    (which side alone starts a statement there), kind 'structure' (the syntax trees differ),
    'macro' (a macro call of ours where the original wrote the same tokens out), 'layout' or
    'untracked' (after a truncated table's end); our_line is the line of our statement containing
    the offset (for orig-only, the one to split).
    """
    unit_style = unit_style or Counter()
    a, b = body(ours), body(orig)
    if truncated:
        a = [(off, ln) for ln, _, off in ours if ln]
    if not a or not b:
        return []
    at_ours, at_orig = dict(a), dict(b)
    orig_full = sorted((off, ln) for ln, _, off in orig if ln)
    src = lambda ln: text.get(ln, "")  # noqa: E731
    out = []
    for i, (off, ln) in enumerate(a):
        if off in at_orig:
            continue
        # ours-only: the original's line doesn't change here
        prev_ln = a[i - 1][1] if i else None
        before = [(o, l) for o, l in orig_full if o < off]
        after = [(o, l) for o, l in orig_full if o > off]
        lp = before[-1] if before else None
        ln_next = after[0] if after else None
        crowd = sum(1 for o, _ in a if lp and ln_next and lp[0] < o < ln_next[0] and o not in at_orig)
        gap = ln_next[1] - lp[1] if lp and ln_next else None
        line, prev = src(ln), src(prev_ln) if prev_ln else ""
        roomy = gap is not None and gap > crowd
        if has_asm and is_asm(line):
            kind, why = "structure", "inline assembly: an instruction the original's source doesn't have"
        elif re.match(r"if\b", strip_code(line)) and prev_ln is not None and is_header(prev) \
                and re.search(r"\bif\b", strip_code(prev)):
            kind, why = "structure", "nested test (the original's `&&`?)"
        elif prev_ln is not None and is_header(prev) and not is_control(line) and ln > prev_ln:
            kind, why = "layout", "body on the header's line"
        elif prev_ln is not None and is_header(prev) and ln > prev_ln and re.match(r"if\b", strip_code(line)) \
                and re.fullmatch(r"(?:\}\s*)?else\s*\{", strip_code(prev)):
            kind, why = "structure", "`else { if` of ours: the original's `if` is on its `else`'s line (`else if`)"
        elif prev_ln is not None and is_header(prev) and ln > prev_ln \
                and re.match(r"(?:if|for|while|switch)\b", strip_code(line)):
            kind, why = "layout", "a control statement on its parent's header line (`for (...) for (...)`)"
        elif strip_code(line).startswith("}"):
            kind, why = "layout", "our brace (a loop test or jump on its `}`): the original's body had none"
        elif not roomy and gap is not None and gap > 0:
            kind, why = "layout", f"{crowd + 1} statements on {gap} original line(s)"
        elif is_control(line) and roomy:
            kind, why = "structure", "a test the original starts no statement for (part of the previous one?)"
        elif roomy and rhs(line) is not None and rhs(line) == rhs(prev):
            kind, why = "structure", f"same value as the previous assignment, no statement start over {gap} " \
                "original lines (a chained assignment?)"
        else:
            kind, why = "layout", "on the previous statement's line in the original (or one multi-line statement)"
        out.append((off, "ours", kind, lp[1] if lp else None, ln, line.strip(), why))
    ours_seen = Counter()
    local = else_evidence(ours, orig, text)
    for off, oln in b:
        if off in at_ours:
            continue
        # orig-only: our statement containing this offset
        cover = [(o, l) for o, l in a if o < off]
        mln = cover[-1][1] if cover else None
        line = src(mln) if mln else ""
        ours_seen[mln] += 1
        if truncated and off > a[-1][0]:
            kind, why = "untracked", "after the end of our (truncated) table"
        elif ours_seen[mln] == 1 and ELSE_IF_LINE.match(strip_code(line)):
            kind, why = else_if(off, cover[-1][0], mln, ours, orig, text, local, unit_style)
        elif mln is not None and statements_on(line) > ours_seen[mln]:
            kind, why = "layout", "our line holds several statements"
        else:
            kind, why = merged_hint(line, helpers, ours_seen[mln] == 1)
        out.append((off, "orig", kind, oln, mln, line.strip(), why))
    return sorted(out)


# --- `} else if` ---------------------------------------------------------------------------------
#
# MWCC gives the jump over an `else` the line of the `}` (or of the last statement) that ends the
# branch before it, and the test of an `else if` the line of its `if`. Our `} else if (x) {` has
# both on one line; where the original starts a statement at the test, its `if` is on a later line
# than that `}`. Three spellings do that and only the last changes the syntax tree:
#
#     if (a) x = 1;          }                  } else {           (or `}` / `else {` / `if`,
#     else if (b) y = 1;     else if (b) {          if (b) {        `}` / `else` / `{` / `if`)
#                                                   ...
#                                                   }
#                                               }
#
# A nested `if` needs a line for the `}` of the `else` block, and puts the `if` where a plain
# `else`'s first statement goes; an `else if` puts it on the `else`'s line, before that.


def else_evidence(ours, orig, text):
    """How the original lays out a function's `else`s where the line table shows it: Counter of
    ('else', own, gap) for each plain `} else {` of ours whose jump and first statement both start
    lines in the original (own: the jump starts a line of its own, i.e. the `}` has one; gap:
    lines from the jump's line, or the line of the last statement start before it, to the first
    statement of the `else`), and ('else-if', own, 0) for each `} else if` of ours whose test starts
    no line of its own in the original either (the original wrote `} else if` on one line)."""
    a = [(off, ln) for ln, _, off in ours if ln]
    at_orig = {off: ln for ln, _, off in orig if ln}
    orig_full = sorted(at_orig.items())
    ev = Counter()
    for i, (j, m) in enumerate(a[:-1]):
        s = strip_code(text.get(m, ""))
        if not ELSE_LINE.match(s):
            continue
        nxt = a[i + 1][0]
        before = [ln for o, ln in orig_full if o < j]
        own = j in at_orig
        base = at_orig[j] if own else (before[-1] if before else None)
        inside = [o for o in at_orig if j < o < nxt]
        if base is None or inside:
            continue
        if ELSE_IF_LINE.match(s):
            ev[("else-if", own, 0)] += 1
        elif s == "} else {" and nxt in at_orig and at_orig[nxt] > base:
            ev[("else", own, at_orig[nxt] - base)] += 1
    return ev


def chain_end(m, text):
    """The line of ours with the `}` that ends the if/else chain that the `} else` on line m
    continues (None if not found)."""
    depth = 0
    lines = sorted(ln for ln in text if ln >= m)
    for k, ln in enumerate(lines):
        s = strip_code(text[ln])
        for ci, ch in enumerate(s):
            if ch == "{":
                depth += 1
            elif ch == "}":
                depth -= 1
                if depth > 0:
                    continue
                if depth < 0 and ln != m:
                    return None
                rest = s[ci + 1:].strip()
                if not rest:
                    rest = next((strip_code(text[n]) for n in lines[k + 1:] if strip_code(text[n])), "")
                if not re.match(r"else\b", rest):
                    return ln
                depth = 0
    return None


def else_if(off, jump, m, ours, orig, text, local, unit_style):
    """(kind, reason) for the original's statement start at the test of our `} else if` on line m
    (jump: the offset of our entry for that line, the jump over the else). 'layout' when the
    original's line numbers rule out a nested `if` (no line for the `}` it needs; `if` on the line
    after a one-line branch) or put the `if` before where the function's (else the file's) plain
    `else`s start their first statement; 'structure' when the `if` follows a `}` of its own and is
    just there, or the original writes `} else if` on one line elsewhere and has the `if` on the
    line after the `}`. Otherwise unsettled ('structure', reason starting "`else if` or").
    """
    at_orig = {o: ln for ln, _, o in orig if ln}
    orig_full = sorted(at_orig.items())
    own = jump in at_orig
    before = [ln for o, ln in orig_full if o < off]
    if not before or at_orig[off] <= before[-1]:
        return "structure", tree_hint(text.get(m, ""))
    gap = at_orig[off] - before[-1]
    after = "the `}`" if own else "the branch's one-line statement"
    # no line for the `}` that closes the `else` block around a nested `if`
    end = chain_end(m, text)
    if end is not None:
        a = sorted((o, ln) for ln, _, o in ours if ln)
        later = [o for o, ln in a if o > off and not m <= ln <= end]
        if later and later[0] in at_orig:
            nxt = at_orig[later[0]]
            last = max(ln for o, ln in orig_full if off <= o < later[0])
            need = 2 if own else 1
            if nxt > last and nxt - last - 1 < need:
                return "layout", (f"`else if` in the original: {nxt - last - 1} line(s) between the chain's last "
                                  f"statement and the next, a nested `if` needs {need}")
    if not own and gap == 1:
        return "layout", ("`else if` in the original: its `if` is on the line after a one-line branch "
                          "(nested, it would need `else { if` on one line)")
    for scope, ev in (("function", local), ("file", unit_style)):
        gaps = Counter({g: n for (k, o, g), n in ev.items() if k == "else" and o == own})
        if gaps:
            top = max(gaps.values())
            tied = sorted(k for k, n in gaps.items() if n == top)
            g = tied[0]
            if gap >= g and len(tied) > 1:
                return "structure", (f"`else if` or `}} else {{` + nested `if`: its `if` is {gap} line(s) after "
                                     f"{after}; plain `else`s start their first statement "
                                     + " or ".join(map(str, tied)) + f" lines after it (this {scope})")
            if gap < g:
                return "layout", (f"`else if` in the original: its `if` is {gap} line(s) after {after}, a plain "
                                  f"`else`'s first statement {g} (this {scope})")
            if gap == g and g != 2:
                return "structure", (f"`}} else {{` + nested `if` in the original: its `if` is {gap} line(s) after "
                                     f"{after}, like a plain `else`'s first statement (this {scope})")
            if gap == g:
                return "structure", (f"`else if` or `}} else {{` + nested `if`: its `if` is {gap} line(s) after "
                                     f"{after}, like a plain `else`'s first statement (this {scope}), but `else` / "
                                     "`if` on two lines (the same tree) puts it there too")
            return "structure", (f"`else if` or `}} else {{` + nested `if`: its `if` is {gap} line(s) after "
                                 f"{after}, a plain `else`'s first statement {g} (this {scope}; comments between?)")
        if ev[("else-if", own, 0)]:
            if gap == 1:
                return "structure", (f"`}} else {{` + nested `if` in the original: its `if` is on the line after "
                                     f"{after}, and its other `else if`s are on that line (this {scope})")
            return "structure", (f"`else if` or `}} else {{` + nested `if`: its `if` is {gap} line(s) after "
                                 f"{after}, other `else if`s on that line (this {scope}; comments between?)")
    return "structure", (f"`else if` or `}} else {{` + nested `if`: its `if` is {gap} line(s) after {after}; "
                         "no plain `else` in this file shows the original's layout")


# --- compiling and matching up functions -------------------------------------------------------

TABLE = None
HELPERS = None
HELPER_DEF = re.compile(r"\binline\b[^;{}()]*?\b(\w+)\s*\([^;{}]*\)\s*\{|#define\s+(\w+)\(")


def helper_names(texts):
    """{name: 'inline' or 'macro'} of the inline functions and function-like macros defined in
    some C texts (a name that is both counts as inline)."""
    out = {}
    for t in texts:
        for a, b in HELPER_DEF.findall(t):
            if a:
                out[a] = "inline"
            else:
                out.setdefault(b, "macro")
    return out


def unit_list():
    units = re.findall(r"(?m)^  unit = (\S+)$", (ROOT / "build.ninja").read_text())
    return sorted(set(units))


def originals():
    """{(path under src/, function): entries} and {function: [entries]} of the original."""
    by_file, by_name = {}, {}
    for src, name, _lo, entries in dwarf_lines.original():
        rel = src.split("/src/")[-1]
        by_file.setdefault((rel, name), entries)
        by_name.setdefault(name, []).append(entries)
    return by_file, by_name


def find(name, cunit):
    by_file, by_name = TABLE
    hit = by_file.get((cunit + ".c", name))
    if hit is None and len(by_name.get(name, [])) == 1:
        hit = by_name[name][0]
    return hit


def unit_source(cunit):
    """src/<unit>.c, or $UNIT_SRC: a variant of the one unit being checked (see the usage above)."""
    return Path(os.environ["UNIT_SRC"]) if os.environ.get("UNIT_SRC") else ROOT / "src" / f"{cunit}.c"


def compile_g(unit, d):
    import configure
    src = unit_source(unit.split(":")[-1])
    obj = Path(d) / "g.o"
    r = subprocess.run([str(ROOT / configure.WIBO), str(ROOT / configure.MWCC), "-c", "-g",
                        *configure.unit_cflags(unit), "-nostdinc", "-Iinclude", "-stderr", str(src), "-o", str(obj)],
                       cwd=ROOT, capture_output=True)
    return obj if r.returncode == 0 else None


def work(unit):
    """(unit, [(function, differences)]) or (unit, None) on a compile error."""
    global TABLE, HELPERS
    if TABLE is None:
        TABLE = originals()
        HELPERS = helper_names(p.read_text(encoding="utf-8", errors="replace")
                               for p in (ROOT / "include").rglob("*.h"))
    cunit = unit.split(":")[-1]
    d = tempfile.mkdtemp()
    try:
        obj = compile_g(unit, d)
        if obj is None:
            return unit, None
        mine = dwarf_lines.from_object(obj)
    finally:
        shutil.rmtree(d, ignore_errors=True)
    raw = unit_source(cunit).read_text(encoding="utf-8").split("\n")
    places = logical_places(raw)
    helpers = dict(HELPERS)
    for k, v in helper_names(["\n".join(raw)]).items():
        if helpers.get(k) != "inline":
            helpers[k] = v
    funcs, style = [], Counter()
    for name, entries in mine.items():
        orig = find(name, cunit)
        if orig is None or name.startswith("__stripped"):
            continue
        text, truncated, has_asm = function_text(name, entries, raw, places)
        funcs.append((name, entries, orig, text, truncated, has_asm))
        style += else_evidence(entries, orig, text)
    res = [(name, classify(entries, orig, text, truncated, has_asm, helpers, style))
           for name, entries, orig, text, truncated, has_asm in funcs]
    return unit, res


def logical_places(raw):
    """{logical line: [physical indices]} of a C file, honouring `#line N` (like
    dwarf_lines.logical_lines, but a `#line` that goes back makes a logical line occur twice)."""
    out, cur = {}, 1
    for i, line in enumerate(raw):
        if line.strip().startswith("#line "):
            cur = int(line.split()[1])
            continue
        out.setdefault(cur, []).append(i)
        cur += 1
    return out


def function_text(name, entries, raw, places):
    """({logical line: source text}, truncated, has_asm) for one function: each of its lines and
    each other line its table names. Where `#line` repeats a logical line, the text is the
    occurrence nearest after the function's definition. truncated: a
    `#line` inside the function goes back before the function's first line; MWCC then writes no
    more entries (not even the closing brace), so the rest of the function can't be compared.
    has_asm: the function has an `asm` block or is an `asm` function."""
    lines = [ln for ln, _, _ in entries if ln]
    if not lines:
        return {}, False, False
    heads = places.get(lines[0], [])
    named = [i for i in heads if re.search(rf"\b{re.escape(name)}\s*\(", raw[i])]
    start = named[0] if named else (heads[0] if heads else 0)
    text = {}
    for ln in set(lines):
        cands = places.get(ln, [])
        after = [i for i in cands if i >= start]
        if cands:
            text[ln] = raw[min(after) if after else max(cands)]
    logical = {i: ln for ln, idx in places.items() for i in idx}
    depth, truncated, has_asm = 0, False, False
    for i in range(start, len(raw)):
        if i in logical:
            text.setdefault(logical[i], raw[i])
        s = raw[i].strip()
        if s.startswith("#line ") and int(s.split()[1]) < lines[0]:
            truncated = True
        code = strip_code(raw[i])
        has_asm = has_asm or bool(re.search(r"\basm\b", code))
        depth += code.count("{") - code.count("}")
        if depth <= 0 and "}" in code:
            break
    return text, truncated, has_asm


def fallback():
    out = set()
    for line in (ROOT / "config/asm_functions.txt").read_text(encoding="utf-8").splitlines():
        p = line.split("#")[0].split()
        if len(p) == 2:
            out.add((p[0], p[1]))
    return out


TAGS = {  # kind: [(pattern at the start of the reason, short tag for the report)]
    "structure": [
        ("our helper", "helper"), (r"`\} else \{` \+ nested", "else-if"), ("`else if`", "else-if?"),
        ("`else \\{ if` of ours", "else-nest"), ("`&&`", "&&"), ("`\\|\\|`", "||"),
        ("comma in a `for`", "for-comma"), ("loop", "loop"), ("`\\?:`", "?:"), ("chained", "chain"),
        ("same value", "chain"), ("initializer", "init"), ("nested call", "call"), ("inline assembly", "asm"),
        ("nested test", "nested-if"), ("a test", "test"), ("statement of ours", "merged"),
    ],
    "macro": [("macro", "macro")],
    "layout": [
        ("`else if` in the original", "else-if"), ("body on the header", "body"), ("a control statement", "header"),
        ("our brace", "brace"), (r"\d+ statements on", "crowded"), ("our line holds", "split"),
        ("on the previous statement", "joined"),
    ],
}
TAG_TEXT = {
    "structure": {
        "helper": "a call of our inline function where the original wrote the statements out",
        "else-if": "`} else if (x)` where the original has `} else {` and a nested `if`: its `if` is where its "
                   "plain `else`s start their first statement",
        "else-if?": "`} else if (x)` where the original's `if` starts a later line and its other `else`s don't "
                    "settle whether it is nested (unsettled: an `else if` with comments between, or nested)",
        "else-nest": "`} else {` and a nested `if` of ours where the original has `else if` on one line",
        "&&": "an `if (a && b)` of ours where the original tests separately (nested `if`s)",
        "chain": "one chained assignment on one side, separate assignments on the other",
        "loop": "a loop written differently (`for` vs `while`, the increment or test elsewhere)",
        "merged": "another statement of ours that merges statements of the original",
        "test": "a test of ours that the original folds into the previous statement",
        "asm": "an inline-assembly instruction of ours the original's source doesn't have",
        "call": "a nested call where the original used a temporary",
        "nested-if": "a nested `if` of ours where the original has one test (`&&`)",
        "for-comma": "a comma expression in a `for` header where the original has statements",
        "?:": "a `?:` of ours where the original has `if`/`else`",
        "||": "an `||` of ours where the original tests separately",
        "init": "an initializer of ours where the original assigns",
    },
    "macro": {
        "macro": "a call of our function-like macro where the original wrote its statements out (the same "
                 "tokens, over several lines)",
    },
    "layout": {
        "else-if": "`} else if (x)` where the original has `else if` on a later line than the `}` (`}` / "
                   "`else if`, or a one-line branch before it)",
        "body": "a body of ours on its own line where the original has it on the header's line",
        "header": "a control statement of ours on its own line where the original has it on its parent's header "
                  "line (`for (...) for (...)`)",
        "brace": "a jump or loop test of ours on a `}` the original's unbraced body didn't have",
        "crowded": "statements of ours on separate lines where the original has fewer lines for them",
        "split": "several statements on one line of ours where the original breaks the line",
        "joined": "a statement of ours on its own line, on the previous statement's line in the original",
    },
}
KINDS = ("structure", "macro", "layout")


def tag(why, kind="structure"):
    return next((t for pat, t in TAGS.get(kind, []) if re.match(pat, why)), "other")


def compact(diffs, kinds=("structure",)):
    """`orig-only 607 else-if (orig 863), 2150 helper ×3 (orig 2160-2164); ours-only 164 nested-if,
    60-84 chain ×25` for the rows of the given kinds: our line, tag and, for orig-only, the
    original's line; rows of one tag at one line of ours (orig-only) or in a run (ours-only) are
    grouped."""
    parts = []
    for side in ("orig", "ours"):
        groups = {}
        for _, s, kind, o, m, _, why in diffs:
            if s == side and kind in kinds:
                key = (m, tag(why, kind)) if side == "orig" else (tag(why, kind),)
                groups.setdefault(key, []).append((m, o))
        items = []
        for key, rows in groups.items():
            ms, os_ = [m or 0 for m, _ in rows], [o or 0 for _, o in rows]
            n = f" ×{len(rows)}" if len(rows) > 1 else ""
            if side == "orig":
                where = f"{os_[0]}" if len(rows) == 1 else f"{min(os_)}-{max(os_)}"
                items.append(f"{key[0] or '?'} {key[1]}{n} (orig {where})")
            elif len(rows) <= 3:
                items += [f"{m} {key[0]}" for m in ms]
            else:
                items.append(f"{min(ms)}-{max(ms)} {key[0]}{n}")
        if items:
            parts.append(f"{side}-only " + ", ".join(items))
    return "; ".join(parts)


def tag_table(kind, tags):
    rows = ["| kind | count | what differs |", "|---|---:|---|"]
    rows += [("| " + t + f" | {n} | " + TAG_TEXT[kind].get(t, "") + " |").replace("||", "\\|\\|")
             for t, n in sorted(tags.items(), key=lambda x: (-x[1], x[0]))]
    return rows


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    args = sys.argv[1:]
    jobs = 8
    if args and re.fullmatch(r"-j\d+", args[0]):
        jobs = int(args.pop(0)[2:])
    only = args[0] if args else None
    units = [u for u in unit_list() if not only or u.split(":")[-1] == only or u == only]
    if os.environ.get("UNIT_SRC") and not only:
        sys.exit("UNIT_SRC needs the unit it is a variant of")
    if only and not units:
        print(f"no unit {only}")
        return 1
    nonmatching = fallback()
    with Pool(jobs) as pool:
        results = pool.map(work, units)
    rows, errors = [], []
    n_funcs = n_same = n_trunc = 0
    kinds, per_unit = Counter(), Counter()
    tags = {k: Counter() for k in KINDS}
    funcs_with = Counter()  # functions whose worst difference is structure / macro / layout
    for unit, res in results:
        if res is None:
            errors.append(unit)
            continue
        cunit = unit.split(":")[-1]
        for name, diffs in sorted(res):
            if (cunit, name) in nonmatching or (unit, name) in nonmatching:
                continue
            n_funcs += 1
            n_trunc += any(d[2] == "untracked" for d in diffs)
            for d in diffs:
                kinds[(d[1], d[2])] += 1
                if d[2] in tags:
                    tags[d[2]][tag(d[6], d[2])] += 1
            per_unit[unit] += sum(d[2] == "structure" for d in diffs)
            present = {d[2] for d in diffs}
            worst = next((k for k in KINDS if k in present), None)
            if worst:
                funcs_with[worst] += 1
            else:
                n_same += 1
            rows.append((unit, name, diffs))
    n_struct, n_macro, n_layout = (funcs_with[k] for k in KINDS)
    total = {k: kinds[("orig", k)] + kinds[("ours", k)] for k in KINDS}
    summary = (f"{n_funcs - n_struct}/{n_funcs} matched functions have the original's statement structure "
               f"({n_same} also its line breaks); structural differences: {kinds[('orig', 'structure')]} orig-only, "
               f"{kinds[('ours', 'structure')]} ours-only in {n_struct} functions; macro: {total['macro']} "
               f"(in {n_macro} functions otherwise structurally the same); layout: "
               f"{kinds[('orig', 'layout')]} orig-only, {kinds[('ours', 'layout')]} ours-only")
    if only:
        for unit, name, diffs in rows:
            if not diffs:
                continue
            print(f"{unit} {name}:")
            for off, side, kind, o, m, code, why in diffs:
                print(f"  +{off:#06x} {side + '-only':9} {kind:9} orig {o or '-':>5} ours {m or '-':>5}  "
                      f"{code}   [{why}]")
        for unit in errors:
            print(f"{unit}: compile error")
        print(summary)
        return 0
    lines = ["# Statement structure of matched functions", "",
             "Generated by `tools/layout_compare.py`. For every matched function, the line table of our",
             "`-g` compile compared with the original's, offset by offset (see the tool for the rules).",
             "A statement start only one side has is one of three things:", "",
             "- **structural**: the original's line numbers show a different syntax tree (statements we",
             "  merged or split, an `&&` written as nested `if`s, `} else {` + nested `if` written as",
             "  `else if`, an inline function where the original wrote the statements out). Besides",
             "  source fidelity, the tree decides what MWCC leaves in its arena (the order of float",
             "  constants), so these can matter to matching elsewhere in a file;",
             "- **macro**: a call of our function-like macro where the original wrote the same statements",
             "  out (or its call over several lines). The tokens, and so the tree, are the same: this",
             "  matters for source fidelity only;",
             "- **layout**: only the line breaks differ (`if (x) y = 1;` on one line, braces the original's",
             "  body didn't have, `}` / `else if (x)` on two lines).", "",
             "The bytes match either way; structural and macro differences are where the C still differs",
             "from the original's source (STYLE.md, \"Evidence decides\"). Some are guesses the line table",
             "can't settle; the tool's docstring says which.", "",
             f"{n_funcs - n_struct} of {n_funcs} matched functions have the original's statement structure "
             f"({n_same} also its line breaks, {n_macro} differ only in macro calls and layout, {n_layout} only "
             f"in layout). {n_struct} functions have {total['structure']} structural differences: "
             f"{kinds[('orig', 'structure')]} orig-only (the original starts a statement where we don't) and "
             f"{kinds[('ours', 'structure')]} ours-only (the reverse). Macro calls account for {total['macro']} "
             f"differences, line breaks for {total['layout']} ({kinds[('orig', 'layout')]} orig-only, "
             f"{kinds[('ours', 'layout')]} ours-only).", ""]
    if n_trunc:
        lines += [f"In {n_trunc} function(s) a `#line` that goes back before the function's first line ends our",
                  "line table early (MWCC writes no entries after it), so the rest isn't compared.", ""]
    if errors:
        lines += ["Not compiled: " + ", ".join(f"`{u}`" for u in errors) + ".", ""]
    lines += ["## Structural differences (the syntax tree differs)", ""] + tag_table("structure", tags["structure"])
    lines += ["", "## Macro calls (the same tokens)", ""] + tag_table("macro", tags["macro"])
    lines += ["", "## Layout (line breaks only)", ""] + tag_table("layout", tags["layout"])
    lines += ["", "## Per function", "",
              "Our line at each structural difference and macro call: for orig-only, the statement of ours to",
              "split (and the original's line of the statement it lacks); for ours-only, the statement the",
              "original doesn't start. `python tools/layout_compare.py <unit>` lists every difference, layout",
              "included, with our source text and the reason.", "",
              "| unit | function | structural differences | macro calls |", "|---|---|---|---|"]
    for unit, name, diffs in rows:
        c, mc = compact(diffs), compact(diffs, ("macro",))
        if c or mc:
            lines.append(f"| `{unit}` | `{name}` | " + c.replace("|", "\\|") + " | " + mc.replace("|", "\\|") + " |")
    (ROOT / "docs" / "layout-fidelity.md").write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")
    print(summary)
    print("most: " + ", ".join(f"{u} {n}" for u, n in per_unit.most_common(10)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
