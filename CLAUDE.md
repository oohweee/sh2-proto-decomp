> These are the instructions the LLM (Claude) worked under in this repository, published for
> transparency. They are not documentation of the project: see README.md and `docs/`.

# Project: Silent Hill 2 (PS2 prototype, 2001-07-13) Decompilation

## Mission
Produce a **matching decompilation** of the Silent Hill 2 PS2 prototype dated 2001-07-13
(`SLUS_202.28`, SHA1 `888eff71606ff4c1c610e30111b3ca5da647edcc`, 13,126,924 bytes, unstripped with
DWARF 1): C source that compiles with the original toolchain to byte-identical code. This is purely a
decompilation: ports or other derived projects are left to others.

This project is an experiment in what an LLM can do: **all code, tooling and docs are written by
Claude**. Say so in the README.

Scope and status: `docs/strategy.md`. Toolchain and build model: `docs/toolchain.md`. Layout of
the executable: `docs/rom-map.md`. Workflow: `docs/decomp-workflow.md`. Code style: `STYLE.md`.

## Legal & hygiene rules (non-negotiable)
- The disc files are user-supplied and live in `baserom/` (gitignored). Never commit them, never commit
  extracted assets, never commit anything derived byte-for-byte from them (the game's own tables written as C
  initializers excepted, as in other matching decompilations; see README, "Legal").
- Assets are extracted at build time from the user's own files.
- Never use, reference, or search for leaked/official source code or SDK leaks. All code is written
  from disassembly, the prototype's own debug info, and public hardware documentation.
- **Independence.** An existing human SH2/SH3 decompilation project has a no-LLM policy and does
  not want its work used by AI projects. Nothing is taken from it: do not read, copy, or derive from
  its source, configs, or symbol files, and don't contribute there. Derive everything here from the
  binary and DWARF ourselves.

## Toolchain (details in `docs/toolchain.md`)
- Compiler: Metrowerks CodeWarrior for PS2; the original is `MW MIPS C Compiler (2.4.1.01)`, which
  isn't available. The build uses `mwcps2-2.4-001213` from decompme/compilers
  (`tools/download_tools.py`). Pure C, no C++.
- Binutils: decompals `binutils-mips-ps2-decompals` (assembling, objdump, linking).
- Run MWCC under **wibo** on Linux (WSL), as decomp.me does. Don't patch or modify the compiler.
- Build: `configure.py` + ninja. Keep it runnable under WSL.

## Methodology

### The gate
The build reproduces the loadable image byte-for-byte: `main` and the 12 overlays on the disc
(`docs/strategy.md`). `ninja` verifies it and `tools/check_all.sh` runs every check. The gate must
stay green on every commit.

### Function-by-function decompilation
1. Pick a function or unit (leaves and small functions first; batch by original source file).
2. Start from `tools/dwarf1.py func <name>`: real prototype, parameter names, locals, struct layouts.
   Read the original's line table (`tools/dwarf_lines.py`) before guessing at statement structure.
3. Compare with `tools/diff_unit.py` (or objdiff) until the diff is empty.
4. If stuck, try decomp-permuter (`tools/permute.py`). If still non-matching, list the function
   in `config/asm_functions.txt` (linked from the original code) and keep its best equivalent C
   with a `NON_MATCHING:` comment on the remaining diff (`docs/nonmatching.md`).

Rules:
- **Never** tweak the build, linker script, or diff tooling to fake a match.
- Write code the way Team Silent plausibly wrote it (`STYLE.md`). Use the DWARF names for
  functions, params, locals, struct fields. Anonymous DWARF types are named in
  `config/type_names.txt`, with the evidence; unnamed symbols keep address names until there is
  evidence.
- Label everything that exists only for matching (`Matching:`), fake matches (`FAKEMATCH:`),
  fitted stand-ins and order fits, and keep `tools/progress.py` counting them separately.
- Mirror the original source tree: `src/` follows `E:\work\sh2(CVS全取得)\src\...` from the DWARF
  (`src/Chacter/anime.c` etc., original spellings kept).

### Documentation & progress
- `tools/progress.py` → `PROGRESS.md`.
- Keep `docs/` current and consistent with the code.

## Repo layout
```
baserom/        # user's disc files (gitignored); baserom/disc/SLUS_202.28
asm/            # splitter output (gitignored)
config/         # unit lists, names, overrides (splat configs and symbol files are generated)
src/ include/   # decompiled C, headers (mirrors the original tree)
tools/          # dwarf1.py, download_tools.py, diff and progress tools, extractors
docs/
```

## Environment notes
- The build runs on Linux or WSL; MWCC runs under wibo there.
- `tools/download_tools.py` fetches compilers and binutils into `tools/` (gitignored).

## How to work with me
- Before a session, run the build check and `tools/progress.py`; report status in one line.
- Propose the next batch of functions and why (dependencies, size, subsystem).
- Show me the final diff state for each function, not the whole iteration history.
- Stop and ask before: changing compiler flags, re-splitting segments, restructuring headers
  broadly, or anything that could break the matching build.
- If you suspect the toolchain guess is wrong (systematic diffs across many functions), say so and
  propose a test rather than grinding on individual functions.
