#!/usr/bin/env python3
"""Set up a decomp-permuter directory for one function (see docs/nonmatching.md).

decomp-permuter (https://github.com/simonlindholm/decomp-permuter; `tools/download_tools.py
--permuter` clones it into tools/decomp-permuter) randomly rewrites a function's C and keeps the
versions that compile closer to the original. Its own importer expects a Makefile project, so this
one builds the directory from our build:

- base.c:        the unit preprocessed by MWCC (-E), comments stripped. Only the function is
                 given to the permuter to parse and rewrite; the code before and after it is
                 passed through verbatim (PERM_IGNORE), because MWCC's output for a function
                 depends on what was compiled before it in the file (the permuter itself would
                 reduce the other functions to prototypes). The parser gets a declarations-only
                 copy of the code before the function instead (PERM_PRETEND).
- target.o:      the unit's object from the matching build (build/src/<unit>.c.o), whose copy of
                 the function is the original's (tools/asm_fallback.py), with the same symbols.
- compile.sh:    MWCC with the build's flags for the unit (configure.unit_cflags).
- settings.toml: scores only the function (objdump --disassemble=<function>).

    .venv/bin/python tools/permute.py <unit> <function>     # writes build/permuter/<function>/
    .venv/bin/python tools/decomp-permuter/permuter.py build/permuter/<function> -j8

It checks that the permuter's own rendering of base.c compiles the function to exactly the code
the unit's C gives, so the permuter's scores are about the function and not about the import.

Treat what the permuter finds as a hint: most of its outputs aren't code anyone would have
written. See STYLE.md (FAKEMATCH).
"""
import argparse
import contextlib
import io
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT))
import configure  # noqa: E402

PERMUTER = ROOT / "tools/decomp-permuter"
OBJDUMP = ROOT / (configure.BINUTILS + "objdump")


def mwcc(unit):
    """MWCC with the flags the build compiles `unit` with."""
    return [str(ROOT / configure.WIBO), str(ROOT / configure.MWCC)] + configure.unit_cflags(unit) + [
        "-nostdinc", "-Iinclude", "-stderr"]


# A string or character literal, or a comment.
TOKEN = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|/\*.*?\*/|//[^\n]*', re.S)
# Inline assembly in a function: an MWCC `asm { }` block or a GCC-style `__asm__(...);`.
ASM = re.compile(r"\basm\s*\{|\b__asm__(?:\s+(?:__volatile__|volatile))?\s*\(|\basm(?:\s+volatile)?\s*\(")


# The unit's own #pragma lines, which MWCC's preprocessor would drop, travel through it as this
# marker (one line each, so __LINE__ doesn't move) and become #pragma lines again in base.c.
PRAGMA_MARK = re.compile(r"__PERMUTE_PRAGMA__\(\s*\"((?:\\.|[^\"\\])*)\"\s*\);")


def preprocess(src, unit):
    """The unit as MWCC's preprocessor sees it, its #pragma lines kept as markers."""
    text = src.read_text(encoding="utf-8")
    marked = re.sub(r"^#pragma[ \t]+(.*)$", lambda m: '__PERMUTE_PRAGMA__("%s");' % m.group(1).strip(),
                    text, flags=re.M)
    copy = ROOT / "build/permuter" / f"{src.stem}.pragmas.c"
    copy.parent.mkdir(parents=True, exist_ok=True)
    copy.write_text(marked, encoding="utf-8", newline="\n")
    try:
        return subprocess.run(mwcc(unit) + [f"-I{src.parent}", "-E", str(copy)], cwd=ROOT, check=True,
                              capture_output=True, text=True).stdout
    finally:
        copy.unlink()


def restore_pragmas(text):
    return PRAGMA_MARK.sub(lambda m: f"\n#pragma {m.group(1)}\n", text)


def strip_comments(text):
    """Removes comments, keeping line breaks."""
    def repl(m):
        tok = m.group(0)
        if tok[0] in "\"'":
            return tok
        return "\n" * tok.count("\n") if "\n" in tok else " "
    return TOKEN.sub(repl, text)


def parseable(text):
    """Escapes line breaks inside string literals, for pycparser. Only for text that is parsed
    and not compiled: MWCC's GCC-style asm needs real line breaks in its template."""
    return TOKEN.sub(lambda m: m.group(0).replace("\n", "\\n"), text)


def balanced_end(text, i):
    """Index just past the bracket group that opens at text[i]."""
    close = {"(": ")", "{": "}", "[": "]"}
    stack = []
    while True:
        c = text[i]
        if c in close:
            stack.append(close[c])
        elif stack and c == stack[-1]:
            stack.pop()
            if not stack:
                return i + 1
        elif c in "\"'":
            i = TOKEN.match(text, i).end() - 1
        i += 1


def function_definitions(text):
    """[(name, start, body start, end)] for the function definitions at file scope."""
    out = []
    item = 0  # start of the current file-scope declaration
    i = 0
    while i < len(text):
        c = text[i]
        if c in "\"'":
            i = TOKEN.match(text, i).end()
            continue
        if c in "([":
            i = balanced_end(text, i)
            continue
        if c == ";":
            item = i + 1
        elif c == "{":
            end = balanced_end(text, i)
            before = text[:i].rstrip()
            if before.endswith(")"):
                name = re.search(r"(\w+)\s*\(", text[item:i]).group(1)
                out.append((name, item, i, end))
                item = end
            elif before.endswith(";"):
                raise SystemExit("K&R-style function definitions aren't supported yet")
            i = end
            continue
        i += 1
    return out


def declarations_only(text):
    """text with every function body replaced by `;` (and `asm` dropped from asm functions)."""
    out = []
    pos = 0
    for _, start, body, end in function_definitions(text):
        header = re.sub(r"\basm\s+", "", text[start:body], count=1)
        out += [text[pos:start], header.rstrip(), ";"]
        pos = end
    out.append(text[pos:])
    return "".join(out)


def protect_parens(text):
    """Writes the parentheses of string and character literals with unbalanced parentheses as
    octal escapes (same bytes), so that the permuter's PERM_ macro scanner, which counts
    parentheses, isn't misled. (Inline asm templates are balanced and must stay as they are.)"""
    def repl(m):
        tok = m.group(0)
        if tok[0] not in "\"'" or tok.count("(") == tok.count(")"):
            return tok
        return tok.replace("(", "\\050").replace(")", "\\051")
    return TOKEN.sub(repl, text)


def hide_asm(text):
    """Wraps inline assembly in PERM_IGNORE: the permuter passes it through and never moves it."""
    out = []
    pos = 0
    for m in ASM.finditer(text):
        if m.start() < pos:
            continue
        end = balanced_end(text, m.end() - 1)
        if text[m.end() - 1] == "(":
            end = text.index(";", end) + 1
        out += [text[pos:m.start()], f"PERM_IGNORE({protect_parens(text[m.start():end])})"]
        pos = end
    out.append(text[pos:])
    return "".join(out)


def make_base(text, function):
    """base.c for function from the preprocessed unit text."""
    found = [f for f in function_definitions(text) if f[0] == function]
    if len(found) != 1:
        raise SystemExit(f"{function}: {len(found)} definitions found")
    _, start, _, end = found[0]
    before, func, after = text[:start], text[start:end], text[end:]
    return (f"PERM_IGNORE({protect_parens(restore_pragmas(before))})\n"
            f"PERM_PRETEND({protect_parens(parseable(PRAGMA_MARK.sub('', declarations_only(before))))})\n"
            f"{hide_asm(restore_pragmas(func))}\n"
            f"PERM_IGNORE({protect_parens(restore_pragmas(after))})\n")


def render(base, function):
    """base.c as the permuter hands it to the compiler before randomizing (its cpp pass, its
    macro expansion, then its C printer)."""
    sys.path.insert(0, str(PERMUTER))
    from src.candidate import Candidate  # noqa: E402
    from src.helpers import get_default_randomization_weights  # noqa: E402
    from src.perm.eval import perm_evaluate_one  # noqa: E402
    from src.perm.parse import perm_parse  # noqa: E402
    from src.preprocess import preprocess as permuter_cpp  # noqa: E402
    with contextlib.redirect_stdout(io.StringIO()):  # "No perm macros found..."
        source, state = perm_evaluate_one(perm_parse(permuter_cpp(str(base))))
        weights = get_default_randomization_weights("mwcc")
        return Candidate.from_source(source, state, function, weights, 0).get_source()


def disassemble(obj, function):
    """The function's disassembly with relocations, without the file name line."""
    out = subprocess.run([str(OBJDUMP), "-drz", f"--disassemble={function}", str(obj)],
                         check=True, capture_output=True, text=True).stdout
    return out[out.index(f"<{function}>:"):]


def compile_c(src, obj, unit):
    result = subprocess.run(mwcc(unit) + ["-c", str(src), "-o", str(obj)], cwd=ROOT, capture_output=True,
                            text=True)
    if result.returncode:
        raise SystemExit(f"compiling {src} failed:\n{result.stdout}{result.stderr}")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("unit", help="[<target>:]<unit>, as in config/asm_functions.txt")
    ap.add_argument("function")
    ap.add_argument("--out", help="directory to write (default: build/permuter/<function>)")
    args = ap.parse_args()
    unit = args.unit.split(":")[-1]
    src = ROOT / "src" / f"{unit}.c"
    target = ROOT / "build/src" / f"{unit}.c.o"
    out = Path(args.out).resolve() if args.out else ROOT / "build/permuter" / args.function
    out.mkdir(parents=True, exist_ok=True)

    (out / "base.c").write_text(make_base(strip_comments(preprocess(src, unit)), args.function),
                                encoding="utf-8", newline="\n")
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        (tmp / "rendered.c").write_text(render(out / "base.c", args.function), encoding="utf-8")
        compile_c(tmp / "rendered.c", tmp / "rendered.o", unit)
        compile_c(src.relative_to(ROOT), tmp / "unit.o", unit)
        if disassemble(tmp / "rendered.o", args.function) != disassemble(tmp / "unit.o", args.function):
            raise SystemExit(f"{args.function}: base.c doesn't compile like {src.relative_to(ROOT)}"
                             f" (see {out / 'base.c'})")

    (out / "target.o").write_bytes(target.read_bytes())
    cc = " ".join(f'"{a}"' if " " in a else a for a in mwcc(unit))
    (out / "compile.sh").write_text(
        "#!/bin/sh\n"
        "# Written by tools/permute.py: MWCC with the matching build's flags.\n"
        f'exec {cc} -c "$1" -o "$3"\n', encoding="utf-8", newline="\n")
    (out / "compile.sh").chmod(0o755)
    (out / "settings.toml").write_text(
        f'func_name = "{args.function}"\n'
        'compiler_type = "mwcc"\n'
        f'objdump_command = "{OBJDUMP} -drz --disassemble={args.function}"\n',
        encoding="utf-8", newline="\n")
    print(f"{out.relative_to(ROOT)}: ready (base.c compiles {args.function} exactly like "
          f"{src.relative_to(ROOT)})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
