# sh2-proto-decomp

A matching decompilation of the **Silent Hill 2** PlayStation 2 prototype dated 2001-07-13
(`SLUS_202.28`).

**This project is written entirely by an LLM (Claude).** It's an experiment in how far that can go.
It isn't affiliated with any other Silent Hill decompilation. [CLAUDE.md](CLAUDE.md) holds the
instructions the LLM worked under; it is kept in the repository for transparency, not as
documentation.

New to the code? [docs/reading-the-source.md](docs/reading-the-source.md) explains the markers in
it (`Matching:`, `FAKEMATCH:`, stand-ins and the rest) and what you can skip when reading for game
logic.

## Status

All of the game's code on the disc is decompiled. 91.71% of it matches from plain C; everything else is counted below and in
[PROGRESS.md](PROGRESS.md) (`tools/progress.py`), where each matched function falls in exactly one
category.

- Every game source file (337 units) is decompiled, and the build reproduces the original byte for
  byte: `main` (SHA1 `b3ca4fac404052dcce59405c6aea766e87d11609`) and the 12 stage overlays on the
  disc (`GX/*.BIN`).
- 4114 of the game's 4307 functions (91.71% of its code) match from plain C: no fitted stand-in,
  fake match, order fit or assembly. Counting those, 4302 match (99.68%).
- 5 are linked from the original code (`config/asm_functions.txt`), each with its best equivalent C
  kept and a `NON_MATCHING` note ([docs/nonmatching.md](docs/nonmatching.md)). The game was built
  with MWCC 2.4.1.01, which isn't available; for most of these differences the evidence points to
  that compiler build, and the page says how far each is shown.
- 86 matched functions are written as assembly, each with a note giving the evidence that the
  original was assembly too.
- 46 matches are fake matches: C marked `FAKEMATCH` that nobody would have written, or code that
  depends on a stand-in fitted to the compiler's state at a place where the original had no code.
  39 more depend on stand-ins where the original had room for code (19) or where that can't be
  told (20), and 17 on a spelling chosen for the constant order among spellings that compile the
  same (order fits, `config/order_fits.txt`). See "Honesty notes" below.
- A unit counts as complete (objdiff's "complete" flag) only when none of its functions is
  linked from the original code and none is a fake match: 312 of the 337 units, 76.38% of the
  game's code.
- The build is code-shiftable within main's `.text` slack: `tools/shift_test.py` inserts padding
  (up to 0x58 bytes; 0x40 by default) right after crt0, which moves every function after it in
  main's `.text`, relinks main and every overlay against the moved symbols, and verifies all
  relocations. main's data and bss and the overlays' load address don't move
  ([docs/toolchain.md](docs/toolchain.md), "Shiftability"). The test can write the result into a
  copy of the disc image; checked by hand in PCSX2, the shifted image boots, starts a new game,
  loads stages and plays the first cutscenes.
- Libraries stay as assembly and aren't counted as game code: 937 functions without debug
  information (Sony's EE libraries, the C library and the game's own `libSh*` libraries), the
  Metrowerks runtime (7 functions) and the EE side of the sound driver (`sd0712`, 25 functions,
  4 KB, built outside the game's source tree, from `M:\select\sound\sd0712\ee\`). The last two
  have DWARF: `include/sh2/api/sd0712.h` and `mwcc_runtime.h` are generated from it.

## How it was made

Everything here (code, tools, docs) was written by Claude in Claude Code, from goals set and
results reviewed in conversation; no code was written by hand. The figures below are
totals from Claude Code's local session logs, measured just before publication:

| | |
|---|---|
| Time from first message to publication | 2 days 18 hours (2026-09-28 to 2026-10-01) |
| Active time in the main session | about 37 hours (gaps over 15 minutes left out) |
| Subagents run in parallel | 180 (171 Claude Opus 5.5, 9 Claude Fable 5.1), about 124 agent-hours combined |
| Input tokens processed | 7.58 billion: 7.47 billion read from the prompt cache, 110 million written to it, 76 thousand uncached |
| Output tokens | at least 5.7 million (the text and tool calls written; thinking isn't counted) |

Most input tokens are cache reads: each step of an agent re-reads its whole conversation. Output
is a lower bound, because the logs don't keep a reliable output count; it is estimated from the
characters of the text and tool calls the model wrote, about 4 characters per token.

## Building

The build runs on x86-64 Linux, natively or in WSL on Windows. macOS should work but hasn't been
tested. It needs Python 3.10 or later in a release whose `tarfile` supports `filter="data"`
(3.10.12, 3.11.4, 3.12 or later; tested with 3.12), ninja and git. Everything else is fetched by
`tools/download_tools.py` into `tools/` (gitignored): the Metrowerks compiler the build uses
(`--sweep` adds the others), the MIPS binutils, [wibo](https://github.com/decompals/wibo) (which
runs the Windows compiler on Linux and macOS) and optionally objdiff-cli.

### Getting the disc files

You need your own copy of the prototype disc. The build reads only the executable `SLUS_202.28` and
the twelve stage overlays in `GX/`. Copy them into `baserom/disc/` (gitignored), keeping the disc's
upper-case names:

```
baserom/disc/SLUS_202.28
baserom/disc/GX/AEE.BIN  AEW.BIN  AEX.BIN  AEY.BIN  AOT.BIN  AST.BIN
                AWX.BIN  AWY.BIN  FST.BIN  OBS.BIN  TOI.BIN  TWE.BIN
```

To get them out of a disc image, either mount the image (Windows Explorer and macOS mount an `.iso`
when you open it; on Linux, `sudo mount -o loop,ro image.iso <dir>`) or open it in 7-Zip, and copy
the files. If they show up in lower case, rename them. Copying the whole disc is fine too; the rest
(`DATA/`, `IOP/`, `SYSTEM.CNF`, `GX/XXX.BIN`) isn't read.

`tools/extract.py`, which `configure.py` runs, checks each file's SHA1 and stops at the first
mismatch:

| file | SHA1 |
|---|---|
| `SLUS_202.28` | `888eff71606ff4c1c610e30111b3ca5da647edcc` |
| `GX/AEE.BIN` | `3bfa8ef006f02c616262327a9ff3f8b6c9d97192` |
| `GX/AEW.BIN` | `c867cbccef33b2f4541a3a3ffced05eb0934bf05` |
| `GX/AEX.BIN` | `261d15bdc6b929ddd776c4c3841fdf1fdbc2b5c4` |
| `GX/AEY.BIN` | `e5c4abaea73e3e19f4d22fa1bb5f93818b0dd46a` |
| `GX/AOT.BIN` | `5c4942e4276abb1d10cc28e750682ab65aedd8dc` |
| `GX/AST.BIN` | `4054ca0d501328bd595a28384603b34257004c8f` |
| `GX/AWX.BIN` | `4c1f71ff4519f701202f251fa7233a51c176c7c1` |
| `GX/AWY.BIN` | `5d7853013882a9cc610c32f13c2bf93f986ac993` |
| `GX/FST.BIN` | `f2dddb6a7e3b86a9ec4bcdd0d91ab240ea188697` |
| `GX/OBS.BIN` | `f132348a6257cb2bf54fcd3b47764aa7185c8ce0` |
| `GX/TOI.BIN` | `52ff0a97c6b240db7ca01575826b64ceddfd314a` |
| `GX/TWE.BIN` | `981fc444233e9c601a8c7e07bb7645ed19813b5d` |

It writes what the build compares against to `build/orig/`, and the data files the original linked
into its code (so far one, the drama-demo script of the fishing scene in `stg_tgs_trial.c`) to
`include/assets/`, where the C source `#include`s them. Both are gitignored: the game's files are
never committed. The shift test's `--iso` option (see "Checking") also needs the disc image
itself, a plain ISO 9660 image with 2048-byte sectors.

### Setting up

**Debian and Ubuntu** (tested on Ubuntu 24.04). Once, from the repo root:

```sh
sh tools/setup_wsl.sh
```

It installs `python3`, `python3-venv`, `python3-pip`, `ninja-build`, `git` and `curl` with
`apt-get` (through `sudo`), creates the Python virtual environment `.venv`, installs
`requirements.txt` into it, runs `tools/download_tools.py --objdiff`, and checks that the compiler
starts.

**Windows**: build inside WSL. Install Ubuntu 24.04 (`wsl --install -d Ubuntu-24.04`), clone the
repository onto a Windows drive, and run the Debian/Ubuntu steps in a WSL shell in the repository's
`/mnt/<drive>/...` directory. From Git Bash, `tools/wsl <command>` runs a command there, e.g.
`tools/wsl sh tools/setup_wsl.sh` or `tools/wsl ninja`. It assumes the distribution is named
`Ubuntu-24.04`, runs the command as `root`, and translates the Git Bash path `/c/...` to
`/mnt/c/...`, so it only works for a clone on a Windows drive.

**Other Linux distributions and macOS**: install Python (see above), ninja and git with your
package manager (on macOS, e.g. `brew install python ninja git`), then do what `setup_wsl.sh` does
after its `apt-get` lines:

```sh
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
.venv/bin/python tools/download_tools.py --objdiff
tools/wibo tools/mwcc/mwcps2-2.4-001213/mwccps2.exe -version   # the compiler starts
```

On macOS, `download_tools.py` fetches the arm64 (Apple silicon) builds of binutils and objdiff-cli.

### Build

```sh
.venv/bin/python configure.py  # extract, generate the splat configs, split, write build.ninja
ninja                          # compile, assemble, link, and verify every target
```

`configure.py` ends with `build.ninja: 13 targets, 337 C files`. `ninja` checks each of the 13
targets against the original as its last step. A successful build prints these 13 lines among its
progress output, in any order, and exits with status 0:

```
OK build/main.bin sha1 b3ca4fac404052dcce59405c6aea766e87d11609
OK build/gx_aee.bin sha1 3bfa8ef006f02c616262327a9ff3f8b6c9d97192
OK build/gx_aew.bin sha1 c867cbccef33b2f4541a3a3ffced05eb0934bf05
OK build/gx_aex.bin sha1 261d15bdc6b929ddd776c4c3841fdf1fdbc2b5c4
OK build/gx_aey.bin sha1 e5c4abaea73e3e19f4d22fa1bb5f93818b0dd46a
OK build/gx_aot.bin sha1 5c4942e4276abb1d10cc28e750682ab65aedd8dc
OK build/gx_ast.bin sha1 4054ca0d501328bd595a28384603b34257004c8f
OK build/gx_awx.bin sha1 4c1f71ff4519f701202f251fa7233a51c176c7c1
OK build/gx_awy.bin sha1 5d7853013882a9cc610c32f13c2bf93f986ac993
OK build/gx_fst.bin sha1 f2dddb6a7e3b86a9ec4bcdd0d91ab240ea188697
OK build/gx_obs.bin sha1 f132348a6257cb2bf54fcd3b47764aa7185c8ce0
OK build/gx_toi.bin sha1 52ff0a97c6b240db7ca01575826b64ceddfd314a
OK build/gx_twe.bin sha1 981fc444233e9c601a8c7e07bb7645ed19813b5d
```

`main.bin` is the executable's loaded image (its `main` section), so its hash differs from that of
`SLUS_202.28`; each overlay is rebuilt whole, so its hash is the disc file's. A target that differs
prints `MISMATCH` with its first differing addresses, and `ninja` fails.

The compiler is Metrowerks CodeWarrior for PS2 (`mwcps2-2.4-001213`, flags
`-O2,p -sdatathreshold 0 -str readonly -enum min`); the game was built with 2.4.1.01, which isn't
available. See [docs/toolchain.md](docs/toolchain.md). A file the original compiled with other
settings gets extra flags in `config/file_flags.txt`, with the evidence; the build and every tool
that compiles a unit use them.

## Checking

```sh
sh tools/check_all.sh     # lint, build, every unit, shift test, progress
```

Every commit passes `tools/check_all.sh`, the gate other decompilation projects run in CI (a
byte-identical build, never broken). This repository has no CI that builds: that needs the game
disc, so the checks run locally.

The tools below run with the virtual environment's Python: `.venv/bin/python tools/<tool>.py`
(from Git Bash on Windows: `tools/wsl .venv/bin/python tools/<tool>.py`).

- `tools/diff_unit.py <unit>`: compile one C file and compare it with the original, function by
  function and section by section (including where every relocation resolves).
- `tools/shift_test.py [--iso disc.iso]`: the shiftability test. With `--iso`, it also writes a copy
  of the disc image with the shifted executable and overlays in place, `build/shift/sh2_shift.iso`.
- `tools/lint.py`: mechanical source rules (see [STYLE.md](STYLE.md)).
- `objdiff.json` (written by `configure.py`) for [objdiff](https://github.com/encounter/objdiff):
  `ninja objdiff` builds the objects it needs; `tools/download_tools.py --objdiff` fetches
  objdiff-cli for reports (`tools/objdiff-cli report generate`). Its `game` category agrees with
  `tools/progress.py`; its totals over all code don't, because it counts the library and data
  objects differently.
- `tools/nonmatching.py`: how far each function linked from the original code is from its C.
- `tools/dwarf_compare.py`: which matched functions' locals still differ from the original's DWARF
  ([docs/dwarf-fidelity.md](docs/dwarf-fidelity.md)); `tools/standins.py`: where the original had
  room for the code each fitted stand-in stands in for ([docs/stand-ins.md](docs/stand-ins.md));
  `tools/layout_compare.py`: which matched functions' statements still differ from the original's
  line table ([docs/layout-fidelity.md](docs/layout-fidelity.md)).
- `tools/permute.py <unit> <function>`: set up [decomp-permuter](https://github.com/simonlindholm/decomp-permuter)
  for a function (`tools/download_tools.py --permuter` fetches it).

## Layout

| path | what |
|---|---|
| `src/` | the decompiled C, one file per original source file, in the original directory layout |
| `include/` | shared headers; `include/sh2/` is generated from the DWARF by `tools/gen_headers.py`; `include/assets/` is extracted from the disc (gitignored) |
| `config/` | unit list, symbol names, type names, prototype corrections, stripped/fallback functions |
| `tools/` | splitter configuration, build helpers, diffing, progress, tests |
| `docs/` | documentation (below) |

## Documentation

About the code and the game:

- [docs/reading-the-source.md](docs/reading-the-source.md): the markers in the source, what to
  skip when reading for game logic, and where names come from.
- [STYLE.md](STYLE.md): code style, and the rules behind every marker.
- [docs/matching-notes.md](docs/matching-notes.md): the full text of the longer matching notes,
  linked from the short notes in the source.
- [docs/characters.md](docs/characters.md): the code's character and enemy names (SCU, RED, IKE, ...)
  mapped to the game's (Lying Figure, Pyramid Head, Flesh Lip, ...), with evidence and confidence.
- [docs/endings-and-stages.md](docs/endings-and-stages.md): the endings, all 52 stages and the map and
  room IDs, mapped to the game's names where the evidence allows.
- [docs/missing-overlays.md](docs/missing-overlays.md): the function names of the 39 stages whose
  code isn't on the disc, from the executable's symbol table.
- [docs/game-ids.md](docs/game-ids.md): item, weapon and other ID spaces the code uses.
- [docs/formats.md](docs/formats.md): the file index and `.mgf` archives, textures, messages, background, character and demo-script formats, and the save-data cipher, from the loaders.
- [docs/prototype.md](docs/prototype.md): what is notable about this prototype: debug features and how to reach them, where the playable build ends, unused content.

About the decompilation:

- [docs/strategy.md](docs/strategy.md): the target, the matching gate, risks and open items.
- [docs/toolchain.md](docs/toolchain.md): the compiler, the build model, dead-stripping,
  shiftability, and the known compiler-version differences.
- [docs/rom-map.md](docs/rom-map.md): the disc, the executable and the overlay layout.
- [docs/decomp-workflow.md](docs/decomp-workflow.md): how to decompile and verify a unit; the
  compiler's codegen patterns (source conventions are in STYLE.md).
- [docs/headers.md](docs/headers.md): what the DWARF shows about the original's headers.
- [docs/nonmatching.md](docs/nonmatching.md): the functions whose C doesn't match yet, by cause.
- [docs/stand-ins.md](docs/stand-ins.md) (generated): every fitted stand-in, whether the original
  had room for code there, and which functions depend on it.
- [docs/dwarf-fidelity.md](docs/dwarf-fidelity.md) (generated): matched functions whose locals
  still differ from the original's DWARF.
- [docs/layout-fidelity.md](docs/layout-fidelity.md) (generated): matched functions whose
  statements still differ from the original's line table.
- [PROGRESS.md](PROGRESS.md) (generated): the figures.

## Help wanted

The code matches; what's left is making it more faithful, and the parts the prototype disc doesn't
cover.

- **Fewer fitted stand-ins and fake matches.** Each stand-in and `FAKEMATCH` stands for code or a
  spelling of the original that hasn't been found. [docs/stand-ins.md](docs/stand-ins.md) lists the
  stand-ins and their dependents; [docs/toolchain.md](docs/toolchain.md) ("Root cause") explains
  what decides the constant order, and [docs/headers.md](docs/headers.md) how the original's
  headers may have contributed.
- **File formats.** Documenting the formats of the game's data files (the `DATA/*.MGF` archives
  and what is in them) from the code that reads them: [docs/formats.md](docs/formats.md).
- **Comparison with retail.** The prototype is the target because its debug information describes
  it; how the retail release differs is a separate, later question
  ([docs/strategy.md](docs/strategy.md)).
- **The 39 missing overlays.** The executable has symbols, relocations and debug information for
  51 overlays, but only 12 binaries are on this disc; where the other 39 are, if anywhere, is open
  ([docs/rom-map.md](docs/rom-map.md); their function names are in
  [docs/missing-overlays.md](docs/missing-overlays.md)).
- **Event-scripting formats.** Decoding the stage event data (`src/Event/stage/*.c`) for modders:
  the bitfields of `Event_List` (`EventListElement` in `src/Event/event.c` reads them; a start is
  in [docs/endings-and-stages.md](docs/endings-and-stages.md), Notes), the trigger shapes of the
  `ev_pos` tables (still raw byte arrays), and the values in `Item_List`, `Model_List` and
  `Enemy_List`.
- **A demo catalog.** The 116 `.dds` cutscene scripts in the file index, each with its cast (the
  `anim_info` table in `src/Event/demoview.c` maps cast names to character kinds) and the stage
  that plays it where that is known.
- **Flag and item tables.** Complete tables of game flags and item IDs
  ([docs/game-ids.md](docs/game-ids.md) has the ones known so far).
- **A retail cross-reference.** A plan for comparing with the retail release (NTSC-U v1.0): which
  functions, tables and IDs changed.
- **Modder tools.** An `.mgf` unpacker driven by the file index, a `.mes` decoder, and a tool to
  decrypt and re-encrypt save data ([docs/formats.md](docs/formats.md) has the formats).
- **Library code.** Sony's SDK, the Metrowerks runtime and the sound driver's EE side stay assembly,
  split from the user's disc ([docs/strategy.md](docs/strategy.md)).

A good first task: pick a function in [docs/dwarf-fidelity.md](docs/dwarf-fidelity.md) with a
missing local and bring its C closer to the original's (the local declared and used) while it
still matches; check with `.venv/bin/python tools/diff_unit.py <unit>` and
`.venv/bin/python tools/dwarf_compare.py <unit>`.

## Honesty notes

- Names come from the prototype's DWARF debug information (functions, variables, parameters,
  locals, struct members). Anonymous types were named by us from their use, with the evidence in
  `config/type_names.txt`. For the libraries, which have no DWARF, function names come from the
  executable's symbol table; types and layouts from the DWARF (where the game's own structs hold
  them) or from how the game uses them; hardware register layouts from public hardware
  documentation. `sce*` type names follow the SDK's naming; that name is the only part not taken
  from the binary. No SDK file was used.
- Functions the original linker dead-stripped left traces (string literals, .bss templates) but
  no code; they're reconstructed as stand-ins whose names and bodies are guesses
  (`config/stripped_functions.txt`).
- Stand-ins such as `STRIPPED_DOUBLE_CODE()` and `__stripped_float_code_*` reproduce a compiler
  state (float-constant order, register mode) that the original's earlier code produced. They are
  fitted, not recovered. The original's line table shows that at 32 of the 78 stand-ins no code
  stood at all, so those represent nothing of the original, and the functions matched through
  them are counted as fake matches ([docs/stand-ins.md](docs/stand-ins.md)).
- Where such a stand-in has been removed, the constant order now comes from how earlier code is
  spelled (docs/toolchain.md, "Root cause"): the original's line table decides where it can
  (a merged statement, a macro, a loop's form), and otherwise one of several spellings that compile
  to the same instructions (`x += 1` or `x = x + 1`) was chosen because it gives the original's
  order. Each such choice has a `Matching:` note, and the functions whose order it sets are listed
  in `config/order_fits.txt` and counted in `PROGRESS.md`: the source is plausible, but that exact
  spelling is inferred, not recovered.
- Every compiled C object is post-processed by `tools/mwcc_fixup.py` before linking, to model two
  things the original toolchain did and ours doesn't. First, it empties the code of the functions
  listed in `config/stripped_functions.txt` (with their relocations and any data only they
  referenced), as the Metrowerks linker's dead-stripping did; GNU ld, which links this build,
  doesn't. The list holds the reconstructed dead-stripped functions and every stand-in: the
  wildcard `__stripped_float_code*` covers all the fitted float stand-ins. So stand-ins add no
  bytes to the build; they only change how the compiler compiles what follows, and the functions
  that depend on them are counted as fitted or fake matches ([docs/stand-ins.md](docs/stand-ins.md),
  `PROGRESS.md`). Second, it raises the alignment of every data section to 8 bytes (16 for sections
  of 16 bytes or more): 2.4.1.01 aligns every data object that way, as every game object in the
  prototype shows, while the public 2.4 build aligns 4-byte variables to 4 bytes and no compiler
  option or linker setting changes that ([docs/toolchain.md](docs/toolchain.md)). Neither step
  changes the bytes of anything that is kept.
- Every construct that exists only to reproduce the original's output is marked `Matching:`;
  bugs present in the original are marked `@bug`.

## Legal

The disc's files are not in this repository. The disc is user-supplied, and the build extracts what
it needs from it at build time: the images it compares against, the library code (as assembly),
and the data files the original linked into its code. The C is written from the binary and its
debug information only (no leaked source or NDA material). Symbol names, types and struct layouts
come from the prototype's debug information (for the libraries: names from the symbol table,
layouts from the DWARF or the game's usage, hardware registers from public hardware
documentation; no SDK file was used), and the game's initialized variables and tables are written
as C initializers, as in other matching decompilations.

Library code linked into the game (Sony's SDK libraries, the Metrowerks runtime, the sound
driver) is not decompiled or committed: like other console decompilations, the build splits it
from the user's disc into `asm/` (gitignored) and assembles it as is. Only symbol names and the
layout the build needs are in `config/`.

This project is not affiliated with or endorsed by Konami or Sony Interactive Entertainment.
Silent Hill is a trademark of Konami Digital Entertainment. PlayStation is a trademark of Sony
Interactive Entertainment.
