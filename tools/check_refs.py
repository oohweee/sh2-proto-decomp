#!/usr/bin/env python3
"""Check the `path:N` and `path:N-M` citations in README.md, STYLE.md and docs/*.md.

For every citation (a path, or a bare file name, followed by `:N` or `:N-M`, and any `, N` or
`, N-M` right after it, which cite the same file):

- the file exists and is tracked by git. A bare name (`filecmd.c:300`) must name one tracked file,
  or one the same document names by its path elsewhere; otherwise it is ambiguous;
- the lines are within the file (1 <= N <= M <= its number of lines);
- a citation of a file git ignores (generated, e.g. config/symbols_main.txt or
  config/relocs_main.txt, which are made from the disc) is an error unless its sentence says
  "generated from the disc": readers without the disc can't follow it otherwise;
- every backticked identifier in the same sentence (a function, type or variable: `name`,
  `name()`, `struct name`, `union name`, `enum name`) appears in one of the sentence's cited
  ranges, give or take 3 lines, or in the C definition a cited range lies inside (the function
  whose name, local or parameter it is, the struct/union/enum whose member it is). A sentence is
  one sentence of a paragraph or list item, or one cell of a table row. Where one of the
  sentence's citations is already wrong, its identifiers aren't checked.

Problems are printed like lint's (`file:line: problem`); the exit status is 1 if there are any.
It needs only the tracked files, not the build or the disc.

    python3 tools/check_refs.py
"""
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
EXTENSIONS = "c|h|s|py|md|txt|sh|yaml|ld|json|inc"
CITATION = re.compile(r"(?<![\w/.-])((?:[\w.-]+/)*[\w-]+\.(?:%s)):(\d+)(?:-(\d+))?" % EXTENSIONS)
# `, N` or `, N-M` after a citation (the same file), when the number ends the item.
MORE = re.compile(r",\s*(\d+)(?:-(\d+))?(?=\s*(?:[,;)|]|\.(?:\s|$)|$|\s+and\b))")
IDENT = re.compile(r"^(?:(?:struct|union|enum)\s+)?([A-Za-z_]\w*)(?:\(\))?$")
GENERATED_NOTE = "generated from the disc"
SLACK = 3


def git_files(*args):
    out = subprocess.run(["git", "ls-files", "-z", *args], cwd=ROOT, capture_output=True, check=True).stdout
    return {p for p in out.decode().split("\0") if p}


def documents():
    return [p for p in ["README.md", "STYLE.md"] + sorted(f"docs/{q.name}" for q in (ROOT / "docs").glob("*.md"))
            if (ROOT / p).exists()]


def sentences(text):
    """[(sentence text, [(offset in text, ...)])]: yields (sentence, start offset) pairs of a
    Markdown document, outside fenced code blocks. Table cells, list items, headings and
    paragraphs are split into sentences."""
    out = []
    lines = text.split("\n")
    offs, pos = [], 0
    for line in lines:
        offs.append(pos)
        pos += len(line) + 1
    fenced = False
    block = []  # (offset, line)

    def flush():
        if not block:
            return
        start = block[0][0]
        joined = text[start:block[-1][0] + len(block[-1][1])]
        for m in re.finditer(r"(?:[^.!?]|[.!?](?!\s+[A-Z(`*\"]))+[.!?]?", joined):
            if m.group().strip():
                out.append((m.group(), start + m.start()))
        block.clear()

    for off, line in zip(offs, lines):
        s = line.strip()
        if s.startswith("```"):
            flush()
            fenced = not fenced
            continue
        if fenced:
            continue
        if not s:
            flush()
            continue
        if s.startswith("|"):
            flush()
            for m in re.finditer(r"(?:\\\||[^|])+", line):
                if m.group().strip():
                    out.append((m.group(), off + m.start()))
            continue
        if s.startswith("#") or re.match(r"([-*+]|\d+\.)\s", s):
            flush()
        block.append((off, line))
    flush()
    return out


def definitions(lines):
    """[(first line, last line, text)] of the top-level braced definitions (functions, struct,
    union and enum types, initialized tables) of a C file, from the line their declaration starts
    on to their closing brace."""
    sys.path.insert(0, str(ROOT / "tools"))
    import lint
    raw = "\n".join(lines)
    code = lint.strip_c(raw)
    out, depth, decl, open_at = [], 0, 0, 0
    for i, c in enumerate(code):
        if c == "{":
            if depth == 0:
                open_at = i
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0:
                start = decl + len(code[decl:open_at]) - len(code[decl:open_at].lstrip())
                out.append((code.count("\n", 0, start) + 1, code.count("\n", 0, i) + 1, raw[start:i + 1]))
                decl = i + 1
        elif c == ";" and depth == 0:
            decl = i + 1
    return out


def problems():
    """[(document, line, problem)] for every citation problem."""
    tracked = git_files()
    ignored = git_files("--others", "--ignored", "--exclude-standard")
    by_name = {}
    for p in tracked:
        by_name.setdefault(p.rsplit("/", 1)[-1], []).append(p)
    cache = {}

    def file_lines(path):
        if path not in cache:
            cache[path] = (ROOT / path).read_text(encoding="utf-8", errors="replace").split("\n")
            if cache[path] and cache[path][-1] == "":
                cache[path].pop()
        return cache[path]

    out = []
    for doc in documents():
        text = (ROOT / doc).read_text(encoding="utf-8")
        named = {}  # file name -> paths this document names in full
        for m in CITATION.finditer(text):
            if "/" in m.group(1):
                named.setdefault(m.group(1).rsplit("/", 1)[-1], set()).add(m.group(1))
        for m in re.finditer(r"(?<![\w/.-])((?:[\w.-]+/)+[\w-]+\.(?:%s))\b" % EXTENSIONS, text):
            named.setdefault(m.group(1).rsplit("/", 1)[-1], set()).add(m.group(1))
        for sentence, start in sentences(text):
            cites = []  # (line in doc, path or None, [(first, last)])
            failed = False  # a citation that doesn't resolve: its identifiers can't be checked
            for m in CITATION.finditer(sentence):
                line = text.count("\n", 0, start + m.start()) + 1
                ranges = [(int(m.group(2)), int(m.group(3) or m.group(2)))]
                end = m.end()
                while True:
                    more = MORE.match(sentence, end)
                    if not more:
                        break
                    ranges.append((int(more.group(1)), int(more.group(2) or more.group(1))))
                    end = more.end()
                cited = m.group(1)
                path, problem = resolve(cited, tracked, ignored, by_name, named)
                if problem and "ignores" in problem and GENERATED_NOTE in " ".join(sentence.split()):
                    problem = None
                if path is None:
                    failed = True
                    if problem:
                        out.append((doc, line, f"{cited}: {problem}"))
                    continue
                n = len(file_lines(path))
                for a, b in ranges:
                    if not 1 <= a <= b <= n:
                        failed = True
                        out.append((doc, line, f"{cited}:{a}{'' if a == b else f'-{b}'}: outside the file "
                                               f"({path} has {n} lines)"))
                cites.append((line, path, [(a, b) for a, b in ranges if 1 <= a <= b <= n]))
            if not cites or failed:
                continue
            for tick in re.finditer(r"`([^`\n]+)`", sentence):
                im = IDENT.match(tick.group(1).strip())
                if not im or CITATION.search(tick.group(1)):
                    continue
                name = im.group(1)
                if not any(covered(file_lines(path), name, ranges, path) for _, path, ranges in cites):
                    where = ", ".join(f"{p}:{r[0][0]}" + (f"-{r[0][1]}" if r and r[0][0] != r[0][1] else "")
                                      for _, p, r in cites if r)
                    line = text.count("\n", 0, start + tick.start()) + 1
                    out.append((doc, line, f"`{name}` is not in the cited lines ({where or 'none valid'}, "
                                           f"+-{SLACK}) or a function around them"))
    return out


def resolve(cited, tracked, ignored, by_name, named):
    """(tracked path, None), (None, problem), or (None, None) for a file nothing can check."""
    if "/" in cited:
        for cand in (cited, f"src/{cited}", f"include/{cited}"):
            if cand in tracked:
                return cand, None
        for cand in (cited, f"src/{cited}", f"include/{cited}"):
            if cand in ignored or any(re.fullmatch(pat, cand) for pat in GENERATED):
                return None, "a file git ignores (generated): say it is generated from the disc, or cite a tracked file"
        return None, "no such tracked file"
    cands = by_name.get(cited, [])
    if len(cands) == 1:
        return cands[0], None
    if len(cands) > 1:
        full = [p for p in named.get(cited, ()) for p in [p if p in tracked else f"src/{p}"] if p in tracked]
        if len(set(full)) == 1:
            return full[0], None
        return None, f"ambiguous: {len(cands)} tracked files are called {cited}; cite it by its path"
    if any(re.fullmatch(pat, f"config/{cited}") for pat in GENERATED) or \
            any(p.rsplit("/", 1)[-1] == cited for p in ignored):
        return None, "a file git ignores (generated): say it is generated from the disc, or cite a tracked file"
    return None, "no such tracked file"


# Generated from the disc by configure.py/tools/gen_splat.py (gitignored, so absent from a clone).
GENERATED = [r"config/[\w.]+\.yaml", r"config/symbols_\w+\.txt", r"config/relocs_\w+\.txt",
             r"config/relocated_\w+\.txt", r"include/macro\.inc", r"objdiff\.json", r"build\.ninja"]


_DEFS = {}


def covered(lines, name, ranges, path):
    """Whether `name` appears in one of the ranges (+-SLACK lines), or in the C definition (a
    function, or a struct/union/enum type) that a range lies inside: the function's name, one of
    its locals, a member of the type."""
    word = re.compile(r"\b%s\b" % re.escape(name))
    for a, b in ranges:
        if any(word.search(line) for line in lines[max(0, a - 1 - SLACK):b + SLACK]):
            return True
    if path.endswith((".c", ".h")):
        if path not in _DEFS:
            _DEFS[path] = definitions(lines)
        for first, last, text in _DEFS[path]:
            if any(first <= a and b <= last for a, b in ranges) and word.search(text):
                return True
    return False


def main():
    found = problems()
    for doc, line, problem in found:
        print(f"{doc}:{line}: {problem}")
    print(f"check_refs: {len(found)} problem(s)")
    return 1 if found else 0


if __name__ == "__main__":
    sys.exit(main())
