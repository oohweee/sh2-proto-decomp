# Reading the source

Every file in `src/` compiles to the original bytes, so a few things in it exist only for the
compiler: they decide register allocation, the order of constants, or a line number baked into a
string, and change nothing about what the game does. They are always labelled. This page explains
the labels, what you can skip when you only want to know how the game works, and where the names
come from. The rules themselves are in [STYLE.md](../STYLE.md).

`src/` mirrors the original source tree recorded in the prototype's DWARF
(`E:\work\sh2(CVS全取得)\src\...`), one C file per original file, with the original spellings
(`src/Chacter/`, `src/Event/stage/stg_ovservation.c`). Each of the twelve stage overlays on the
disc is one ordinary file under `src/Event/stage/`; `config/c_units.txt` lists which target
(`main` or an overlay) each file belongs to.

## Where names come from

The prototype's executable is unstripped and carries DWARF 1 debug information, so most names are
the developers' own:

- **From the executable**: function and global names (symbol table); parameters, locals, struct
  and union members, enumerators and the source file paths (DWARF). These are never renamed.
  `tools/dwarf1.py func <name>` prints a function's original prototype and locals,
  `tools/dwarf1.py type <name>` a type.
- **Named by us, with evidence**: the DWARF has no typedefs, so anonymous structs appear as
  `anon_<size>_<hash>` until they are named in `config/type_names.txt`, one line of evidence each
  (the names marked provisional there rest on thin evidence). Library types whose layout the
  DWARF or the game's use shows take the SDK's `sce*` name; the name is the only part not taken
  from the binary.
- **Invented**: the helper headers whose original name is unknown (`include/asm_helpers.h`,
  `include/m3_helpers.h`, `include/enemy.h` and the other subsystem helpers; the comment at the
  top of each says what is known), the macros and inline functions in them, the inline
  functions defined in `src/` files (ours unless a note cites the DWARF or the symbol table, as
  for the out-of-line copies `_shLength` and `check_self_spot`), and the functions the linker
  dead-stripped (below), which have no symbol and no DWARF. Their names are guesses unless
  `config/stripped_functions.txt` says otherwise.
- **Unknown**: anything with neither a symbol nor DWARF keeps an address name (`func_XXXXXXXX`,
  `D_XXXXXXXX`) rather than a guess.

Comments are ours. The Doxygen comment on a function describes what its code does; it is not the
developers' documentation.

## The labels

A long note keeps a short version in the source (what is unusual, and whether it is fitted, fake
or inferred) and links to its full text in [matching-notes.md](matching-notes.md).

### `Matching:`

Marks any construct that exists only to reproduce the original's output: `#line`, the stand-ins
below, a local declaration order fitted to the stack or registers, `do { } while (0)` macros kept
for the `nop` they produce, K&R definitions, block-scope prototypes, a call made without a
prototype in scope (the file declares the function itself and defines `SH2_LOCAL_<function>`
before `#include "sh2.h"` to leave out the generated prototype; STYLE.md, "Headers"), volatile
casts, pragmas, aligned typedefs. The note says what the construct is for and, where there is one, the evidence:

```c
/* Matching: locals declared in an order fitted to the asm operands' registers (tmp0 before data);
   the DWARF's order (work, data, result, tmp0, tmp1) doesn't match (docs/dwarf-fidelity.md). */
```

Most `Matching:` constructs are ordinary C of the kind the original could have contained, not fake
matches; the stand-ins below are the exception, and are counted separately. Where one departs from the original's DWARF, `tools/dwarf_compare.py` lists it in
[dwarf-fidelity.md](dwarf-fidelity.md).

### `#line N`

Asserts and log messages bake `__LINE__` into a string in the binary, so the statement has to sit
on its original line. Where our file is shorter than the original was, a commented `#line` puts
the next line back on its number (`src/Event/title.c`):

```c
        case 1:
            /* Matching: the log string bakes its original line number into the object. */
#line 642
            printf(TLOG("title.tex: read finished(%d.%02d)\n"), wait_loop / 60,
                   wait_loop % 60 * 100 / 60);
```

The number tells you roughly how long the original file was at that point; nothing else.

Every `#line` in `src/` has such a string after it, before the next `#line`; that is the only
reason one is there, and `tools/lint.py` checks it. `#line`s that no string followed were tested
and removed: 138 in 77 files. Five of them, in m3_play.c and vc_main.c, restored our own numbering
from before a stand-in was inserted or declarations were merged, some going backwards
(`tools/constcount.py` writes such a `#line` after a stand-in it inserts). Many were labelled as
keeping "the original line numbers", but of those with code after them, all but two put that code
on another line than the original's line table has for it. With all of them removed, each file
compiles to the same bytes as before, and its DWARF and layout fidelity
([dwarf-fidelity.md](dwarf-fidelity.md), [layout-fidelity.md](layout-fidelity.md)) is unchanged.
A `#line` changes the binary only through `__LINE__`.

### Dead-stripped functions

The original linker removed functions nothing called. Their code is gone, but their string
literals (and in one case a `.bss` template) stayed behind. Where those traces show a function
existed, it is written back into the file so the literals land where they did, and listed in
`config/stripped_functions.txt`; the build then empties its code (`tools/mwcc_fixup.py`), so it
produces no bytes. Its name and body are guesses, and none of it is in the game's binary.

### Stand-ins: `STRIPPED_DOUBLE_CODE()` and `__stripped_float_code_*`

Some functions match only if the compiler is in a particular state when it reaches them:

- **Software-double mode.** Once MWCC has compiled `double` arithmetic in a file, it uses `a2`
  rather than `a0`/`a1` for temporaries in the rest of that file. `STRIPPED_DOUBLE_CODE()`
  (`include/common.h`) expands to a small `static double __stripped_double_code(double)` placed
  where the original's register use changes.
- **Float-constant order.** Which float constant of a call's arguments MWCC loads first depends on
  leftover bytes in the compiler's own memory, left there by whatever it compiled before
  ([toolchain.md](toolchain.md), "Root cause"). A `static float __stripped_float_code_<n>(...)`
  placed before a function recreates leftovers that give the original's order.

Both are listed in `config/stripped_functions.txt` and emptied like dead-stripped functions, so
they add nothing to the binary. Their bodies are **fitted, not recovered**: nobody knows what code
stood there, if any. `src/Enemy/en_arm.c`:

```c
/* Matching: fitted stand-in for float code (docs/stand-ins.md); it makes enARMCanSeePlayer materialize
 * 1000.0f before loading dp->scp->rot.y for shSinCosV_Scale, as the original does. */
static float __stripped_float_code_1(float x) { return x + 3.0f + 5.0f + 7.0f + 9.0f + 11.0f + 13.0f + 15.0f; }
static int enARMCanSeePlayer(struct EnLOCAL_DATA *dp) {
```

[stand-ins.md](stand-ins.md) checks each stand-in against the original's line table. Where the
gap between the neighbouring functions leaves room for a dead-stripped function, the functions
that depend on the stand-in count as fitted matches. Where it leaves no room, the stand-in stands
for nothing that existed, and its dependents count as fake matches. `PROGRESS.md` reports both.

### Order fits

Some spellings compile to the same instructions but leave different leftovers for the next
function (`x += 1` or `x = x + 1`, a macro or the statements it expands to). Where nothing else
decides between them, the one that gives a later function the original's constant order was
chosen. The `Matching:` note says so and names the function it serves, and that function is
listed in `config/order_fits.txt` (`src/Fog/fog_blow.c`):

```c
 * Matching: `if (pd->erase != 0)` rather than `if (pd->erase)` (the same code) is chosen for the
 * float-constant order: with it the shSway1f() call below loads -alp / 32.0f before 0.02f, as the
 * original does (docs/toolchain.md, "Root cause").
```

The source is plausible either way; only the exact spelling is inferred.

### `FAKEMATCH:`

Above a function whose match relies on C nobody would have written: a meaningless temporary, a
no-op statement, a cast kept only for codegen. The note says which part is fake
(`src/Enemy/en_fly.c`):

```c
/* FAKEMATCH: the (float) cast of a float sum is there only for codegen: it loads the angle before 500.0f for
 * shSinCosV_Scale. ...
```

The code still means what it says (a cast of a `float` to `float` does nothing), so you can read
through it. `tools/progress.py` counts these functions as fake matches.

### `NON_MATCHING:`

Above a function whose C doesn't compile to the original yet. The C stays in the file as the
function's best equivalent version and is compiled, but the build then replaces its code with the
original's (`tools/asm_fallback.py`, listed in `config/asm_functions.txt`). There is no
`#ifdef NON_MATCHING` / `INCLUDE_ASM` pair in the source as in some other projects. The note
describes the remaining difference and what was tried; [nonmatching.md](nonmatching.md) has all
of them. The C behaves like the original, so for reading purposes it is as good as the rest.

### `@bug`

A bug that is in the original game: an operator precedence mistake, an uninitialized read, an
out-of-bounds access. It is kept, because the matching build must reproduce it
(`src/Chacter/bg_chara.c`):

```c
    if (!(chara_id >> 8) == 3) { /* @bug always false (precedence); kept as in the original */
```

A fix for other builds goes behind a flag that is off by default: `#ifdef BUGFIX` for behaviour,
`#ifdef AVOID_UB` for undefined behaviour (one so far, in `src/Chacter/m3_bgobj.c`). Read these;
they are real behaviour of the game.

## What you can skip when reading for game logic

- Stand-ins (`__stripped_*`, `STRIPPED_DOUBLE_CODE()`) and the functions listed in
  `config/stripped_functions.txt`: none of them is in the game's binary.
- `#line` directives, and the line numbers inside assert and log strings.
- `Matching:` notes about registers, declaration order, constant order and line layout. The
  construct they describe behaves like its plainer spelling (`x = x + 1` is `x += 1`).
- The code parts of `FAKEMATCH:` notes: casts and temporaries that change nothing.
- The mechanics of `include/asm_helpers.h`, `include/asm_libm.h` and the VU0 inline assembly in
  `include/sh_vu0.h` and `include/fi_libvu0_inline.h`: the helper names say what they compute
  (float/int conversion, square root, clamp, vector length and copy, VU0 vector and matrix
  operations).
- `include/sh2/`: generated from the DWARF by `tools/gen_headers.py`. Look types up there; don't
  read it top to bottom.

Don't skip `@bug` notes, `NON_MATCHING:` functions' C, or data tables: they are the game.
