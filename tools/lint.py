#!/usr/bin/env python3
"""Mechanical checks for tracked source files: rules that never move a line.

- every tracked file is UTF-8 text (the repo tracks no binaries);
- LF line endings, a final newline, no tabs, no trailing whitespace (C sources and headers,
  Python tools, docs and the other text files outside include/sh2/ and config/);
- no absolute home-directory paths in any tracked text file (the repo stays free of personal
  paths; scripts derive their location instead);
- every FAKEMATCH note is one tools/progress.py parses (`FAKEMATCH:` or
  `FAKEMATCH (for <function>...):` at the start of a comment line), and it is attributed to
  functions defined in its file, so none is silently left out of the fake count;
- braces on every if/else/for/while body in C sources (STYLE.md, "Formatting");
- upper-case digits in hex literals in C code (`0x1F`, not `0x1f`; the prefix stays `0x`), not
  in comments, strings or the arguments of a stringizing macro such as assert() (the original's
  spelling is in the binary) (STYLE.md, "Formatting").
- a C file's `#define SH2_LOCAL_<function>` names a prototype that include/sh2/api/ wraps in
  `#ifndef SH2_LOCAL_<function>` (config/prototype_overrides.txt, `local` lines), so a file can't
  leave out a declaration nobody listed (STYLE.md, "Headers").
- every `#line` in a C file has a use of `__LINE__` after it before the next `#line` (directly or
  through a macro of the file or include/*.h, such as assert()): a `#line` changes the binary
  only through `__LINE__` (STYLE.md, "Line numbers are part of the output").

Formatting that could add or remove lines is not done here: line numbers are part of the matching
build (see STYLE.md).

    python tools/lint.py          # exit 1 on any problem
    python tools/lint.py --fix    # fix trailing whitespace, CRLF, missing final newlines and
                                  # lower-case hex digits
"""
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
STYLED = (".c", ".h", ".py", ".md", ".txt", ".yaml", ".ld", ".sh", "")
HOME_PATH = re.compile(r"([A-Za-z]:[\\/]+Users[\\/]+|/home/|/mnt/[a-z]/Users/|/c/Users/)[^\s\"'`)]*")
HEX_LOWER = re.compile(r"\b0[xX][0-9A-Fa-f]*[a-f][0-9A-Fa-f]*")


def strip_c(t, keep_pp=False):
    """Comments, string/char literals and (unless keep_pp) preprocessor lines blanked; offsets and
    newlines outside string literals kept."""
    out, i, n = [], 0, len(t)
    while i < n:
        if t.startswith("/*", i):
            j = t.find("*/", i + 2)
            j = n if j < 0 else j + 2
            out.append("".join(ch if ch == "\n" else " " for ch in t[i:j]))
            i = j
        elif t.startswith("//", i):
            j = t.find("\n", i)
            j = n if j < 0 else j
            out.append(" " * (j - i))
            i = j
        elif t[i] in "\"'":
            q, j = t[i], i + 1
            while j < n and t[j] != q:
                j += 2 if t[j] == "\\" else 1
            out.append(q + " " * (j - i - 1) + q)
            i = j + 1
        else:
            out.append(t[i])
            i += 1
    lines, cont = [], False
    for line in "".join(out).split("\n"):
        if not keep_pp and (cont or line.lstrip().startswith("#")):
            cont = line.rstrip().endswith("\\")
            lines.append(" " * len(line))
        else:
            lines.append(line)
    return "\n".join(lines)


def unbraced(text):
    """Line numbers of if/else/for/while statements whose body is not a braced block."""
    t = strip_c(text)
    found = []
    for m in re.finditer(r"\b(if|for|while|else)\b", t):
        j = m.end()
        while j < len(t) and t[j] in " \t\n":
            j += 1
        if m.group(1) == "else":
            if re.match(r"if\b", t[j:]):
                continue
        else:
            if j >= len(t) or t[j] != "(":
                continue
            depth = 0
            while j < len(t):
                depth += {"(": 1, ")": -1}.get(t[j], 0)
                if depth == 0:
                    break
                j += 1
            j += 1
            while j < len(t) and t[j] in " \t\n":
                j += 1
            if m.group(1) == "while" and t[j:j + 1] == ";":
                continue  # the end of do { } while (x);
        if j < len(t) and t[j] != "{":
            found.append(t.count("\n", 0, m.start()) + 1)
    return found


DEFINE = re.compile(r"^[ \t]*#[ \t]*define[ \t]+(\w+)\(([^)]*)\)((?:[^\n]*\\\n)*[^\n]*)", re.M)
_HEADER_MACROS = None


def stringizing_macros(text):
    """Names of the function-like macros (defined in `text` or include/*.h) that stringize an
    argument (`#x`), directly or by passing their arguments on to such a macro: the spelling of
    those arguments is in the binary (an assert's text), so their hex literals keep their case."""
    global _HEADER_MACROS
    if _HEADER_MACROS is None:
        _HEADER_MACROS = {}
        for h in sorted((ROOT / "include").glob("*.h")):
            _HEADER_MACROS.update(macro_defs(h.read_text(encoding="utf-8")))
    defs = dict(_HEADER_MACROS)
    defs.update(macro_defs(text))
    names = {n for n, (params, body) in defs.items()
             if any(re.search(r"(?<!#)#\s*%s\b" % re.escape(p), body) for p in params if p)}
    grown = True
    while grown:
        grown = False
        for n, (params, body) in defs.items():
            if n not in names and any(re.search(r"\b%s\s*\(" % re.escape(m), body) for m in names):
                names.add(n)
                grown = True
    return names


def macro_defs(text):
    """{name: (parameters, body)} of the function-like macros defined in `text`."""
    return {m.group(1): ([p.strip() for p in m.group(2).split(",")], m.group(3))
            for m in DEFINE.finditer(text)}


def lowercase_hex(text):
    """[(offset, literal)] of the hex literals in C code (outside comments and strings, macro
    definitions included) that have a lower-case digit, except in the arguments of a macro that
    stringizes them (stringizing_macros)."""
    t = strip_c(text, keep_pp=True)
    kept = []
    for name in stringizing_macros(text):
        for m in re.finditer(r"\b%s\s*\(" % re.escape(name), t):
            depth, j = 0, m.end() - 1
            while j < len(t):
                depth += {"(": 1, ")": -1}.get(t[j], 0)
                if depth == 0:
                    break
                j += 1
            kept.append((m.end(), j))
    return [(m.start(), m.group()) for m in HEX_LOWER.finditer(t)
            if not any(a <= m.start() < b for a, b in kept)]


def upper_hex(text):
    """text with the digits of lowercase_hex()'s literals in upper case (the `0x` kept)."""
    for pos, lit in reversed(lowercase_hex(text)):
        text = text[:pos] + "0x" + lit[2:].upper() + text[pos + len(lit):]
    return text


ANY_DEFINE = re.compile(r"^[ \t]*#[ \t]*define[ \t]+(\w+)((?:[^\n]*\\\n)*[^\n]*)", re.M)
LINE_DIRECTIVE = re.compile(r"^[ \t]*#[ \t]*line[ \t]+\d+", re.M)
_HEADER_DEFINES = None


def unused_line_directives(text):
    """Line numbers of the `#line` directives in a C file's text with no use of `__LINE__` after
    them before the next `#line`, directly or through a macro (of the file or include/*.h) whose
    expansion uses it."""
    global _HEADER_DEFINES
    if _HEADER_DEFINES is None:
        _HEADER_DEFINES = {}
        for h in sorted((ROOT / "include").glob("*.h")):
            _HEADER_DEFINES.update(m.groups() for m in ANY_DEFINE.finditer(h.read_text(encoding="utf-8")))
    defs = dict(_HEADER_DEFINES)
    defs.update(m.groups() for m in ANY_DEFINE.finditer(text))
    names = {"__LINE__"}
    grown = True
    while grown:
        grown = False
        for n, body in defs.items():
            if n not in names and re.search(r"\b(%s)\b" % "|".join(sorted(names)), body):
                names.add(n)
                grown = True
    use = re.compile(r"\b(%s)\b" % "|".join(sorted(names)))
    code = strip_c(text)  # comments, literals and preprocessor lines blanked; same offsets
    starts = [m.start() for m in LINE_DIRECTIVE.finditer(text)]
    out = []
    for k, a in enumerate(starts):
        b = starts[k + 1] if k + 1 < len(starts) else len(text)
        if not use.search(code, text.index("\n", a), b):
            out.append(text.count("\n", 0, a) + 1)
    return out


_FUNCTIONS_H = None


def local_guards():
    """The text of the generated prototypes, include/sh2/api/ (for their SH2_LOCAL_ guards)."""
    global _FUNCTIONS_H
    if _FUNCTIONS_H is None:
        _FUNCTIONS_H = "".join(p.read_text(encoding="utf-8")
                               for p in sorted((ROOT / "include" / "sh2" / "api").rglob("*.h")))
    return _FUNCTIONS_H


def tracked():
    out = subprocess.run(["git", "ls-files", "-z"], cwd=ROOT, capture_output=True, check=True).stdout
    return [ROOT / p for p in out.decode().split("\0") if p]


def main():
    fix = "--fix" in sys.argv
    problems = 0
    for path in tracked():
        rel = path.relative_to(ROOT).as_posix()
        if not path.is_file():
            continue
        data = path.read_bytes()
        try:
            text = data.decode("utf-8")
        except UnicodeDecodeError as e:
            print(f"{rel}: not UTF-8 (byte {e.start}): the repo tracks only UTF-8 text")
            problems += 1
            continue
        for n, line in enumerate(text.split("\n"), 1):
            if HOME_PATH.search(line) and rel != "tools/lint.py":
                print(f"{rel}:{n}: absolute home path")
                problems += 1
        if path.suffix not in STYLED or rel.startswith(("include/sh2/", "config/")):
            continue
        if path.suffix == ".c":
            for n in unbraced(text):
                print(f"{rel}:{n}: body without braces")
                problems += 1
            for n in unused_line_directives(text):
                print(f"{rel}:{n}: #line with no __LINE__ use after it (it changes no bytes)")
                problems += 1
            for m in re.finditer(r"^#define (SH2_LOCAL_\w+)", text, re.M):
                if f"#ifndef {m.group(1)}\n" not in local_guards():
                    print(f"{rel}:{text.count(chr(10), 0, m.start()) + 1}: {m.group(1)} guards no prototype")
                    problems += 1
        if path.suffix in (".c", ".h") and not fix:
            for pos, lit in lowercase_hex(text):
                print(f"{rel}:{text.count(chr(10), 0, pos) + 1}: lower-case hex digits in {lit}")
                problems += 1
        new = text.replace("\r\n", "\n")
        lines = new.split("\n")
        for n, line in enumerate(lines, 1):
            if "\t" in line:
                print(f"{rel}:{n}: tab")
                problems += 1
            if line != line.rstrip() and not fix:
                print(f"{rel}:{n}: trailing whitespace")
                problems += 1
        new = "\n".join(l.rstrip() for l in lines)
        if not new.endswith("\n"):
            new += "\n"
            if not fix:
                print(f"{rel}: no final newline")
                problems += 1
        if fix and path.suffix in (".c", ".h"):
            new = upper_hex(new)
        if "\r\n" in text and not fix:
            print(f"{rel}: CRLF line endings")
            problems += 1
        if fix and new != text:
            path.write_bytes(new.encode("utf-8"))
    import progress
    for rel, n, problem in progress.fakematch_problems():
        print(f"{rel}:{n}: {problem}")
        problems += 1
    print(f"lint: {problems} problem(s)" if problems or not fix else "lint: fixed")
    return 1 if problems and not fix else 0


if __name__ == "__main__":
    sys.exit(main())
