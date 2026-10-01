# Code style

The first rule is the matching build: the C must compile with the original toolchain to the
original bytes. Everything below applies within that constraint. When the two conflict, matching
wins, and the construct gets a `Matching:` comment saying why it is there.

These rules follow the established decompilation projects (zeldaret's Ocarina of Time and
Majora's Mask, pret's Pokémon games, Banjo-Kazooie). Where we differ, the reason is given: the
two big ones are that this game's DWARF gives us the original names and types, and that line
numbers are part of its binary.

## Line numbers are part of the output

Asserts and log macros bake `__LINE__` into strings in the binary. Adding or removing a line above
such a string changes the build. So:

- Don't reformat whole files. `.clang-format` is a reference for code you are writing
  (`clang-format --lines=...`); it cannot preserve line breaks, so unlike the Zelda projects we
  can't make a formatter a gate.
- Where our source is shorter than the original was, a commented `#line N` puts the next such
  string back on its original number. Only there: a `#line` with no such string after it (before
  the next `#line`) changes no bytes, so it isn't used.
- `tools/lint.py` enforces the rules that never move a line: UTF-8, LF, final newline, no tabs,
  no trailing whitespace, braces on every body.

## Formatting

- 4 spaces, no tabs. Braces on the same line (`if (x) {`), `} else {`.
- Every `if`/`else`/`for`/`while` body gets braces, even a single statement. Where the original had
  the body on the same line (the line table decides), keep it there: `if (x) { y = 1; }`.
- `case` labels at the same indentation as their `switch`.
- One declaration per line; pointer star next to the name (`int *p`). Where the original's line
  table shows several declarations sharing lines, follow it (with a `Matching:` note).
- Hex constants in upper case (`0x1F01E00`); float literals with `f` where the original is `float`.
- In code you write, decimal for counts, sizes, indices, timers and colours; hex for bit masks,
  addresses, register values and offsets (the Majora's Mask rule). Existing code isn't converted
  wholesale.
- `ARRAY_COUNT(a)` rather than `sizeof(a) / sizeof(a[0])`; `NULL` for pointers.
- Hardware registers by name from `include/eeregs.h` (`*D1_CHCR`, `*T0_COUNT`), never as raw
  addresses or file-local copies.

## Names

- Functions, globals, parameters, locals, struct members and enumerators keep their names from the
  prototype's DWARF. Don't rename them. (The `gFoo`/`sFoo`/camelCase conventions of the Zelda and
  Pokémon projects are for names those projects had to invent; ours are the developers' own.)
- Symbols with no name in the DWARF or symbol table keep an address name (`D_XXXXXXXX`,
  `func_XXXXXXXX`) until there is evidence for a real one: a wrong name is worse than none.
- Unnamed things get names only with evidence: anonymous types in `config/type_names.txt`
  (evidence per line), helper functions and macros by what they do. Invented names are never
  presented as original; say so in a comment where it matters.
- Library functions (`sce*`, the C library) keep their names from the executable's symbol table;
  `sce*` type names follow the SDK's naming (`config/type_names.txt`).
- Stand-ins for code that no longer exists are `__stripped_*`. A fitted stand-in where the
  original's line table shows no code stood (docs/stand-ins.md) stands for nothing: the functions
  matched through it are fake matches, and `tools/progress.py` counts them as such.

## Types

- Use the generated types from `sh2.h` (`include/sh2/`), and `u8`/`s16`/`u32`/`u_long128`... from
  `common.h` where the original's type is a plain integer of that width. Keep the DWARF's declared
  types for variables and parameters even when another type would match (it's evidence).
- Aligned typedefs (`Q_WORDDATA`, `sceVu0FVECTOR`) where the original's stack layout shows them.

## Headers

- The game's own declarations come from `sh2.h`: `include/sh2/types.h`, `variables.h` and
  `functions.h`, which includes one header per source directory (`include/sh2/api/<dir>.h`: the
  functions each unit defines, in the order of their code). They are generated
  (`tools/gen_headers.py`); fix declarations through `config/prototype_overrides.txt` and
  `config/type_names.txt`, never by hand.
- Library declarations come from the library headers, never from the C file:
  - `include/sdk/`: Sony's EE libraries (`libvu0.h`, `libgraph.h` with the GS register numbers and
    field macros, `GS_REG_*`/`GS_SET_*` as in ps2sdk, `libvifpk.h`, `libgifpk.h`, `libdma.h`, `eekernel.h`, `sifdev.h`, `sifrpc.h`,
    `libcdvd.h`, `libmc.h`, `libmpeg.h`, `libpad.h`, `libscf.h`, `libsdr.h`);
  - `include/libc/`: the C library (`string.h`, `stdio.h`, `stdlib.h`, `math.h`, `stdarg.h`,
    `unistd.h`); `common.h` includes `stdio.h`;
  - `include/lib/`: the game's own libraries that have no DWARF (`libShPad.h`, `sh_kernel.h`).

  They declare only what the game uses, and each says where its declarations come from: function
  names from the executable's symbol table, types and layouts from the DWARF or the game's call
  sites, hardware register layouts from public hardware documentation. No SDK header or other SDK
  file is used.
  A file includes the ones it uses, after `sh2.h` and the helper headers; a missing declaration is
  added to the header, not to the file.
- A file declares a function itself only where the original evidently did, with a `Matching:`
  note: a call without a prototype in scope (`void *memset();` where the arguments are passed
  unconverted), or another signature the generated one can't express. For a game function, a
  `local <function> <unit> <reason>` line in `config/prototype_overrides.txt` makes
  `gen_headers.py` wrap its prototype in `#ifndef SH2_LOCAL_<function>`, and the file defines
  `SH2_LOCAL_<function>` before including `sh2.h` (`tools/lint.py` checks that each such define has
  its guard). A game function that is also another unit's static function is wrapped the same way
  without a line (found in the DWARF). Don't rename a generated declaration away
  (`#define foo foo_hdr`). A type whose layout differs between the units that define it is only
  declared in `sh2/types.h`; those units define it.
- Shared inline helpers live in headers, never copied into C files:
  - `include/sh_vu0.h`, `include/fi_libvu0_inline.h`, `include/fi_calc.h`,
    `include/GFW/sh2_get_drawenv.h`: the game's own inline headers (names from the DWARF, whose
    paths are `src\SH2_common\`, `src\Chacter\` and `src\GFW\`; only `include/GFW/` mirrors
    its original directory, each header's comment gives its original path). The last three
    define the functions the original has out-of-line copies of, on their original lines
    (`#line`), `static inline` or `inline` as the copy's ELF binding says (local or 13);
  - `include/asm_helpers.h` (FPU, integer, vector and copy helpers), `include/asm_libm.h`
    (inline-asm `fabsf`/`fmaxf`/`fminf`), `include/math_const.h` (`PI`);
  - subsystem helpers: `include/enemy.h`, `include/hh_math.h`, `include/hh_vector.h`,
    `include/fog_helpers.h`, `include/fog_param.h`, `include/m3_helpers.h`,
    `include/model3_helpers.h`, `include/gfw_helpers.h`.

  Headers whose original name is unknown say so. A file keeps a local helper only when a call site
  needs a different variant; give it a distinct name and a comment. Two helpers that differ only
  in parameter order (`distXZ(b, a)` and `enDistXZ(a, b)`) are kept only when both are needed for
  matching (inline-call arguments are evaluated last to first), and both say so.
- If including a header changes a unit's code (header content before the first function can move
  the float-constant order of the first functions, docs/toolchain.md, "Root cause"), the file keeps
  its local declarations with a one-line `Matching:` note naming the reason.
- File-local declarations (static functions) go at the top of the C file.

## Comments

- Each file starts with a short comment: what the file is and what it does.
- Exported functions get a Doxygen comment: `/** Brief. @param x ... @return ... */`. Describe
  what the code does; don't guess at intent the code doesn't show.
- `Matching:` marks every construct that exists only to reproduce the original's output (stand-ins,
  `#line`, do/while(0) macros kept for their nop, K&R definitions, block-scope prototypes, volatile
  casts, pragmas, aligned typedefs).
- `NON_MATCHING:` above a function whose C doesn't match yet (it is then linked from the original
  code, `config/asm_functions.txt`), with the remaining difference. They are tracked in
  [docs/nonmatching.md](docs/nonmatching.md).
- `FAKEMATCH:` in the comment above a function whose match relies on C that nobody would have
  written (a meaningless temporary, a no-op statement or cast kept only for codegen), saying what
  is fake. It's a last resort, for when nothing plausible matches and the C is still clearly
  better than linking the original code; `tools/progress.py` counts these functions separately.
- Evidence decides between the two. When the original's DWARF or line table shows that code was
  there but the optimizer removed what it computed (a statement whose only surviving instructions
  are shared addresses, a local nothing uses), write the least code that fits the evidence and say
  in a `Matching:` note what is reconstructed and why. That is recovered structure, like the `pad`
  stack locals of the Zelda decompilations (known to exist, content unknown, honestly named), not a
  fake match. `FAKEMATCH:` is for codegen coaxing that no evidence supports.
- A declaration order fitted to the stack or register layout isn't a fake match (it is ordinary C,
  as in the Zelda projects, which permute declarations the same way); where it differs from the
  original's DWARF, `tools/dwarf_compare.py` lists it in docs/dwarf-fidelity.md, and the note says so.
- Where a local is declared follows the line table too. MWCC gives the prologue the line of the
  function's `{` and gives declarations without an initializer no entry, so the lines between the
  `{` and the first statement are the only room for top-level declarations. Where there are fewer
  lines than locals, the rest were declared in an inner block (or with an initializer) where the
  line table leaves room; the DWARF can't show it, since MWCC's DWARF lists every local flat.
- A choice between spellings that compile to the same instructions (`x += 1` or `x = x + 1`, a
  macro or its statements) may be made by its effect on the float-constant order of later code
  (docs/toolchain.md, "Root cause"), when nothing else decides. It is not a fake match, but the
  exact spelling is inferred: the `Matching:` note says it was chosen for that and names the
  function whose order it sets, and that function is listed in config/order_fits.txt (counted by
  `tools/progress.py`). Keep it to a choice or two per function whose order it sets, consistent
  with the file's own style: a combination of many choices found by searching thousands of them
  fits the compiler state as a stand-in does, only less visibly, so a labelled stand-in is better.
- `@bug` marks a bug that is in the original (the Zelda projects' `//! @bug`): out-of-bounds
  accesses, reads of uninitialized variables, wrong operators. Don't fix it in the matching build.
  A fix for other builds (a modern compiler, say) goes behind a flag that is off by default
  (`#ifdef BUGFIX` for behaviour, `#ifdef AVOID_UB` for undefined behaviour), as in the Pokémon and
  Zelda projects.
- The build has no warnings except ones that describe the original: the note on `$at` in inline
  assembly, "not initialized" for locals that only inline-assembly outputs write (MWCC doesn't count
  those as assignments), and `@bug`-marked reads of uninitialized variables. Don't add others.
- A `NON_MATCHING:` function's C is its best *equivalent* C, as in the Zelda, Pokémon and
  Banjo-Kazooie projects: correct and as close to the original's source as the evidence allows. When
  the DWARF or line table shows how a statement was written, follow it even if another spelling
  gets fewer differing words; the word count is a measure, not the goal. It must behave exactly like
  the original, because anything built from the C uses it; C that doesn't would be marked
  `NON_EQUIVALENT:` (Majora's Mask's term) and fixed first.

## Pragmas

CodeWarrior projects did use pragmas (`#pragma optimization_level`, `#pragma dont_inline`,
`#pragma opt_*`) around individual functions, so a pragma is a legitimate way to reproduce the
original, like a per-file flag in the build of an IDO or GCC decompilation. It is also an easy way
to fake a match, so:

- Only when it makes the function match exactly and nothing written plainly does.
- Scoped to the function: `#pragma push` / the setting / the function / `#pragma pop`.
- Marked `Matching:` with the evidence (what the original's code shows, e.g. no CSE or no
  scheduling in that function only).
- A setting for a whole file goes in the build (`config/file_flags.txt`: the unit, its extra MWCC
  flags, and the same evidence in a comment), not as a pragma at the top of the file.

## Inline assembly

Only where the original clearly used it: the game's VU0 macro-mode helpers (GCC-style extended
asm), FPU helpers whose instructions the compiler doesn't emit on its own
(`cvt.w.s`, `sqrt.s`, `max.s`/`min.s`), and hand-written functions. Not to paper over C that hasn't
been found.
