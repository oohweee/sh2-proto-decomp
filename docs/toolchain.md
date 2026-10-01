# Toolchain

The compiler, the flags, how the build reproduces the original's layout, and the known differences
between the compiler build we have and the one the game was built with. Codegen patterns useful
while writing C are in [decomp-workflow.md](decomp-workflow.md).

## Compiler: Metrowerks CodeWarrior for PS2

Evidence from `SLUS_202.28`:
- `.comment` section: `MW MIPS C Compiler (2.4.1.01)` / `PlayStation2`.
- 52 `.mwcats` sections (type `0xCA2A82C2`), written by the Metrowerks linker.
- DWARF 1 (`.debug` + `.line`), one compile unit per function, producer `MW MIPS C Compiler`,
  every unit `LANG_C` (no C++).
- Globals are addressed with `lui at,hi` / `op reg,lo(at)`, never through `$gp`
  (`-sdatathreshold 0`). Functions are padded with `nop` to 16-byte alignment.

### Choosing a build (2026-09-28)

Public builds (decompme/compilers; `tools/download_tools.py` fetches the one the build uses,
`--sweep` the others):

| id | version string | runtime built |
|---|---|---|
| `mwcps2-2.3.3-000906` | 2.3.3 | Sep 6 2000 |
| `mwcps2-2.4-001213` | 2.4 (Engineering Build 0017) | Dec 13 2000 |
| `mwcps2-3.0-011126` | 3.0 | Nov 26 2001 |

The target's 2.4.1.01 falls between Dec 2000 and Nov 2001 and isn't available.
`tools/compiler_test/sweep.py` compiles test files with every build at `-O0`…`-O4` (plus `,p`/`,s`)
and compares each function's instruction words with the target, masking relocated fields:

| test | functions | matching combinations |
|---|---|---|
| `test.c` | 4 leaf accessors (≤7 insns) | all 3 builds at `-O1`, `-O2`, `-O2,p` |
| `test2.c` | `enReduceHP` (float), `UtilStrConvertCdPath` (char loop), `titleGetBattleLevelFromCursor` (switch + assert), `enMKNGetRotSpeed` (local array init + call) | **only 2.4 at `-O2,p`** |

- `-O3` and higher fill the `jr ra` delay slot. The target's compiled code never does: of the
  4,294 `jr ra` in game code, the 18 with a filled delay slot are all in hand-written assembly
  (`SH2_common/sh_vu0`, `Fog/spack`, `fontGetData`).
- `-O2` without `,p` gets `enMKNGetRotSpeed` wrong (21 vs 22 words).
- 2.3.3 and 3.0 fail the medium functions at every level.

So the project uses `mwcps2-2.4-001213`. Where it differs from 2.4.1.01, see "Differences between
2.4.1.01 and build 0017" below.

### Flags

`configure.py`: `-O2,p -sdatathreshold 0 -str readonly -enum min`.

| flag | evidence |
|---|---|
| `-O2,p` | the sweep above |
| `-sdatathreshold 0` | no `$gp`-relative accesses in game code |
| `-str readonly` | string literals are in `.rodata`, one 8-aligned section each |
| `-enum min` | the DWARF's enums are 1 or 2 bytes |

Other options (`-char`, `-fp_contract`, inline depth) stay at their defaults; no matched function
has needed another value. A file the original compiled with other settings gets extra flags in
`config/file_flags.txt`, with the evidence (currently only `Lens/lens_flare -inline off`,
[headers.md](headers.md) section 5).

`-g` adds DWARF and a line table without changing code or data, so `tools/dwarf_lines.py`,
`tools/dwarf_compare.py` and `tools/layout_compare.py` compile with it and compare the result with
the original's debug information.

### Running it

The build runs MWCC under [wibo](https://github.com/decompals/wibo), as decomp.me does, on Linux
or WSL (Ubuntu 24.04, set up by `tools/setup_wsl.sh`). Running the compiler natively on Windows
isn't supported.

Command: `tools/wibo tools/mwcc/mwcps2-2.4-001213/mwccps2.exe -c -O2,p -sdatathreshold 0 -str readonly -enum min -nostdinc -stderr in.c -o out.o`

## Binutils

`binutils-mips-ps2-decompals` v0.10 (GNU Binutils 2.40 with R5900 fixes): assembling, linking,
`objdump`. There's a Windows build, used for `objdump`.

## Metrowerks DWARF 1 quirks (handled in `tools/dwarf1.py`)

- **No typedef DIEs.** Anonymous structs appear as `@anonN`, so typedef names are lost.
- Struct/union/enum DIEs sit in type-only compile units and are referenced across units.
- Parameter locations are always `OP_REG 2`, a placeholder. Local register locations look real
  (`s0`–`s7` common).
- Location opcode `0x80` = floating-point register (`f20`–`f23` common for locals).
- Fundamental type `0xA510` = 16-byte unsigned integer (Sony `u_long128`); plain `int`/`short`/`long`
  are emitted as the `FT_signed_*` codes.
- Vendor attribute `0x2013` (block) on subroutines, not decoded.

What the DWARF records about headers is in [headers.md](headers.md).

## Splitting and linking (splat 0.50, spimdisasm 1.42)

`tools/gen_splat.py` reads the ELF and writes, for each target T (`main` and the 12 overlays),
`config/T.yaml`, `config/symbols_T.txt`, `config/relocs_T.txt`, `config/align_T.txt` and
`config/relocated_T.txt`, plus `config/symbols_extern.txt` (`configure.py` runs it; the yaml,
symbol and relocation files are derived from the disc and not committed). Settings that matter
for a byte-exact rebuild:
- **Relocations from `.relmain`** → `config/relocs_main.txt` (HI16/LO16/26/32; 90k entries).
  Without them spimdisasm mis-symbolizes `table - k` addressing. Example: `Sd3dPlay` (sound
  driver) reads four consecutive 8-byte entries at `0x17DD30` as `Sd3dData - 0x138800`,
  `- 0x1387F8`, `- 0x1387F0` and `- 0x1387E8` (each a HI16/LO16 pair), addresses that fall inside
  another file's text.
- **No SUBALIGN** (`emit_subalign: False`). MWCC puts every function and every variable in its
  own section with its own alignment (functions 16, doubles 8, ...), and the linker has to honor
  those. Each assembled section gets `objcopy --set-section-alignment` = the largest power of two
  dividing its original start (`config/align_<target>.txt`), which lands it exactly after an MWCC
  object that ends without trailing padding. bss alignments are capped at the bss start's
  alignment, since the bss output section has no fixed address. In `main`, the asm (library)
  objects use the smallest alignments that reproduce the layout instead
  (`config/min_align_main.txt`, see "Shiftability").
- `string_encoding: ASCII` (and data): otherwise binary constants get decoded as EUC-JP/SJIS text
  and re-emitted as UTF-8 (the `huge` double in the math library became "呆Iq").
- `allow_data_addends: False`: stops guessed `sym + offset` pointers in data (file tables of
  sector/size pairs looked like bss pointers). Real pointers with addends still come from relocs.
- Relocation types 121/123 (DVP/VU labels) and 7/8 (`gp`-relative, handled via `gp_value`) aren't
  converted.

Section layout of `main`: `.text` 0x100000, `.vutext` 0x299300, `.data` 0x2A7400
(`__data_start`), `.rodata` 0x389000, `.sdata` 0x3C7D50, `.sbss`/`.bss` 0x3C7D80–0x1F01E00.
`gp` = 0x3CFC70.

### Data ownership (`tools/datasplit.py`)

The linker lays out every section in link order, so each section is a sequence of per-object
ranges in the same order as `.text`. Evidence: relocations from code (file-local symbols like `@N`
literals and `name$N` statics are strong; globals weak), DWARF variable claims (weak: a global is
listed by every file that uses it), data-to-data references, and library section symbols. A
dynamic program picks the best monotonic assignment for code units. Globals defined in code-less
files (`data_bg_*.c`, `exec_env.c`, ...) become separate blocks. Ranges are verified per unit
when it moves to C: the compiled object has to fit exactly.

### Overlays

The Metrowerks overlay linker starts and ends every overlay section on a 0x80 boundary: every
`GX/*.BIN` is a multiple of 0x80, zero-padded. `configure.py` inserts `. = ALIGN(., 0x80);` before
each overlay's data and bss in the generated linker scripts and aligns `<t>_DATA_END` /
`<t>_RODATA_END`; `diff_unit.py` models the same. Linker-defined absolute symbols (`_gp`, `_end`,
`_ovl_start_addr`) have no relocation in the original; `diff_unit.py` resolves them to their
values (`config/symbols_extern.txt`). Header-defined inline functions that come before an
overlay's first .c function belong to that .c unit (gx_ast, gx_awy). The overlay file format is in
[rom-map.md](rom-map.md).

### Dead-stripping

The Metrowerks linker drops functions nothing references, plus data only they referenced. A
string literal that a surviving function shares stays in `.rodata`, at the position where the
compiler first emitted it for the stripped function; `.bss` isn't stripped (a stripped function's
zero-initializer template survives). GNU ld doesn't dead-strip, so such functions are written in
C, listed in `config/stripped_functions.txt`, and `tools/mwcc_fixup.py` empties their `.text` (and
relocations) plus every data section only they referenced. The build and `diff_unit.py` both run
it. Stripped functions have no DWARF: their names and bodies are guesses constrained by the
surviving literals. The same mechanism removes the fitted stand-ins (see "Float-constant argument
order").

### Checking a C unit (`tools/diff_unit.py`)

Compares against the **original binary**, not the assembled asm (whose spimdisasm guesses are
stored as zero plus a relocation). Every relocation is resolved as the linker will and compared
with the original: a field may differ only where the original ELF has a relocation
(`config/relocated_<target>.txt`), and there it must name the same target symbol. Each section is
laid out from the unit's original address, every data symbol's address is checked, and the unit's
first data object must land on its original address after the preceding object.

## Differences between 2.4.1.01 and build 0017

### Data alignment (2026-09-28)

In the prototype, every data object from Metrowerks-compiled game code starts on an 8-byte
boundary (3,147/3,224 `.data` objects, the rest being GCC-built Sony objects; 433/433 `.bss`;
702/702 `.rodata`), and every such object of 16 bytes or more on a 16-byte boundary (2,443/2,462
in `.data`, all in `.rodata` and `.bss`). The public builds (2.3.3, 2.4 build 0017, 3.0) give a
4-byte variable a 4-aligned section. No compiler option controls it (`-sym on` and `-g` don't),
and the Metrowerks linker `mwldps2` keeps 4-byte alignment, so it's a difference in the 2.4.1.01
build. `tools/mwcc_fixup.py` models it: each data section is raised to 16 when it is 16 bytes or
larger, else to 8. The build and `diff_unit.py` both apply it.

### Compiler-version test

First run 2026-09-28. Two residual patterns recurred across units: address-taken pointer locals
at 16-byte aligned stack slots in the original, and float-constant call arguments loaded earlier
than any C spelling reproduced. The affected units (sh2gfw_drawloop_main, shDBG_fontHandle,
sh2gfw_fogtest_main, model3_sub_n, m3_play_event, en_fly, gx_toi stg_toilet) were compiled with
every build then available (`diff_unit.py --cc`): 2.3.3, 2.4 build 0017, 3.0 (Nov 2001), 3.0.1
(Jan 2002), 3.0.3 (Jul 2002). Build 0017 was the best on every unit by a wide margin, and no build
fixed the residual functions. Both patterns were explained later, and neither is a compiler-build
difference: aligned typedefs ([decomp-workflow.md](decomp-workflow.md), "Locals, stack and
registers") and arena leftovers ("Root cause" below).

Repeated on 2026-09-30 for the functions still not matching, with all 21 public builds
(decompme/compilers: the five above plus sixteen 3.0 betas and 3.0.1 builds from 2003-2006; none
exists between 2.4 and 3.0), all with the project's flags. 2.3.3 can't compile vc_main.c (its
inline assembler rejects register-variable operands). The builds' decompme IDs, to reproduce it:
`tools/download_tools.py --sweep` fetches the first four (with the build's `mwcps2-2.4-001213`);
the others download the same way from the decompme/compilers release (`<id>.tar.gz`, unpacked into
`tools/mwcc/<id>/` for `diff_unit.py --cc <id>`).

- `mwcps2-2.3.3-000906`, `mwcps2-2.4-001213`, `mwcps2-3.0-011126`, `mwcps2-3.0.1-020123`,
  `mwcps2-3.0.3-020716`;
- `mwcps2-3.0b38-030307`, `mwcps2-3.0b50-030527`, `mwcps2-3.0b52-030722`;
- `mwcps2-3.0.1b44-030325`, `mwcps2-3.0.1b51-030512`, `mwcps2-3.0.1b74-030811`,
  `mwcps2-3.0.1b75-030916`, `mwcps2-3.0.1b87-031208`, `mwcps2-3.0.1b95-040309`,
  `mwcps2-3.0.1b103-040528`, `mwcps2-3.0.1b119-040914`, `mwcps2-3.0.1b145-050209`,
  `mwcps2-3.0.1b151-050317`, `mwcps2-3.0.1b198-051011`, `mwcps2-3.0.1b205-051227`,
  `mwcps2-3.0.1b210-060308`.

- `vcMakeFarWatchTgtPos`: whether the copy of cir_r survives before squaring depends on the build,
  not the C. With the same C, 3.0.1b44 (Mar 2003) and b51 (May 2003) emit the original's
  `mov.s f2, fX; mul.s f2, f2, fX`, and 3.0b38/b50/b52 keep a copy in another form; every other
  build folds it like build 0017. Those builds match 2-3 of vc_main's 62 functions (build 0017:
  61), so they are evidence, not a toolchain.
- `RObjectFunction` (since matched as an original bug, [nonmatching.md](nonmatching.md),
  "Resolved"): no build puts scp in a0; all read it through v0 like build 0017.

The three `m3_play_3d` functions show a third difference, in the sign extension of `short`
register variables ([nonmatching.md](nonmatching.md), "Short arithmetic").

## Float-constant argument order

When a call has several constant `float` arguments, MWCC sometimes materializes one of them ahead
of the others, and which one depends on code earlier in the file. This was the largest group of
residual diffs.

The first model (2026-09-28) was a count: with the same function, adding float constants earlier
in the file changed the order (enIKEInitData hoists 500.0f with 0, 4, 10, 14 or 16 earlier
constants and 600.0f, like the original, with 9 or 20), and the constants' values made no
difference. `tools/constcount.py` searches on that basis: it inserts stand-ins
(`static float __stripped_float_code_<n>(float x) { return x + 3.0f + ...; }`, removed from the
build like dead-stripped functions) before each non-matching function and keeps the smallest
constant count that fixes it. Its first sweep fixed 25 functions. The model was incomplete (the
constants' values, earlier non-float code and argument spelling also move the order), and the
root cause below replaced it. The stand-ins in `src/` have up to 199 constants, and some have
other shapes; all are fitted, not recovered.

### Root cause (2026-09-29): an uninitialized flag in recycled memory

Found by reverse-engineering our compiler (mwccps2.exe 2.4 build 0017, analysed in Ghidra and
traced under gdb; nothing from the compiler is in the repository, and the trace scripts are not
published). Function addresses are in that executable.

- Code generation for a call (`FUN_004a1ad0`) evaluates the arguments in two passes: first every
  argument whose expression node has **byte 5** set, then the rest. Float constants are
  materialized in that order, which becomes the `lui`/`mtc1` order in the output.
- Expression nodes come from a bump allocator (`FUN_00432aa0`) that never clears memory, and the
  constant nodes' constructors set bytes 0-3 only. So byte 5 of a float constant is whatever an
  earlier allocation left at that address.
- The allocator is rewound (`FUN_00432b90`) after every function, without clearing, and the next
  function's syntax tree is built from the same base address. So the flag bytes of a function's
  constants are usually leftovers of the **previous function's syntax tree** (any code, not only
  float code; names and comments are stored elsewhere), and which leftover a constant lands on
  depends on everything the function allocated before it. A big enough function fills the region
  with its own leftovers, which is why large stand-ins stop changing anything.
- Clearing the allocator at each rewind (done in the debugger) makes enIKECtrlAttack2's first call
  come out in plain argument order like the original, but not its later calls: the original's code
  shows flagged arguments there too. So the original compiler has the same behaviour, and the
  original file left different leftovers.

Both the leftovers and where a constant lands depend on source spelling, not just on the code
produced: in en_ike.c, writing a level change as `EN_SET_LEVEL(dp, 9)` instead of two assignments
(the same code) in enIKECtrlAttack moves enIKECtrlAttack2's flags. That makes the flags evidence
for choosing between spellings that compile identically ([STYLE.md](../STYLE.md), "Comments";
`config/order_fits.txt`). A stand-in between two functions only helps when its own allocations
reach the relevant addresses; for enIKECtrlAttack2 they don't (those nodes sit in a later arena
block).

Consequences:
- The `__stripped_float_code*` stand-ins work by recreating leftovers, so they are fitted by
  nature, and the dead-stripped-code explanation holds at most for some of them. A stripped
  function still takes up source lines, but at 32 of the 78 stand-in positions the original's line
  table leaves at most the file's usual gap between functions plus 2 lines, so no code stood
  there. The functions matched through those stand-ins are counted as fake matches
  ([stand-ins.md](stand-ins.md), `tools/standins.py`, `tools/progress.py`).
- Matching the remaining float-order functions means reproducing the original's leftovers: a
  search problem, but a precise one, since the flag of every argument can be read out of the
  compiler instead of inferred from the output.

## Register allocation (2026-09-29)

Traced in gdb on `mwcps2-2.4-001213` and re-implemented in a simulator (not published) that
reproduces the traced assignments exactly (InitAllDataOne, CmdQueuePut4, mcDrawSlot). Addresses
are in mwccps2.exe.

- Per function (`FUN_00435610`), per register class, `FUN_004ab420` loops: build the interference
  graph (`FUN_004a3410`) and coalesce copies (`FUN_004a3730`) until stable, simplify
  (`FUN_004aae60`), select (`FUN_004ab190`); on failure insert spill code (`FUN_00504430`) and
  retry (up to 10 times).
- Virtual registers are numbered: parameters in order; then the locals list, which holds the
  optimizer's temporaries (newest first) and the declared locals in reverse declaration order,
  with block-scope locals and CSE temporaries interleaved by scope; then expression temporaries in
  code order. A coalesced pair keeps the lower number.
- Simplify: K = 26 integer registers (all but zero, at, k0, k1, gp, sp). Degrees count physical
  registers too (`zero` touches almost everything; `v0` everything after the last call). Passes
  scan numbers upwards and remove any node of degree < K at once; when stuck, the node with the
  lowest spill cost / degree goes (ties: highest number).
- Select colours in reverse removal order, normally descending number, except that a node whose
  degree was >= K at its first-pass turn is coloured before all others. Each gets the
  lowest-numbered free register of v0, v1, a0-a3, t0-t7, t8, t9, ra; when none is free the next
  saved register (s0...s7, fp) is opened for every later node.
- Spill cost: the sum over uses (2) and definitions (1) of 8^(syntactic loop depth); a
  `do { } while (0)` counts as a loop.
- Constants outside the signed 16-bit range (0x8000, 0xEEEEE, `~7U`) become optimizer temporaries
  with low numbers (coloured late); small constants are rematerialized in fresh, high-numbered
  registers (coloured early). `x = c ? a : b` makes a temporary too.
- No list scheduler runs at `-O2,p`: instruction order is code order plus peepholes.
- The peephole that drops a redundant sign extension of a `short` (`FUN_00503000`, opcodes
  0x43c/0x43d) tracks definitions only within a basic block, and calls end blocks, so a short read
  after a call or branch is extended again. The original compiler didn't
  ([nonmatching.md](nonmatching.md), "Short arithmetic").

Source levers that follow: declaration order and scopes (numbering), a `?:` or a large constant
(low-numbered temporaries), loop or `do { } while (0)` nesting (spill costs, hence s-register order
and which value is spilled), and splitting or merging statements (which temporaries exist).
CmdQueuePut4 and sh2gfw_GsExecStoreImage were matched this way.

## Shiftability

Besides byte-identical, the build is code-shiftable within main's `.text` slack: main's code can
move, while its data, its bss and the overlays' load address stay where they are (first reached
2026-09-28; "Limit" below). What makes it work:
- `tools/unsymbolize.py` (after every split) turns symbol references spimdisasm guessed where the
  original has no relocation back into numbers (357 in main: file sizes, constants like 0xEEEEE,
  coincidental matches), so they don't move with a symbol.
- `tools/resymbolize.py` does the reverse for instructions spimdisasm wrote as plain words although
  the original has a relocation: each gets an explicit one
  (`.reloc ., R_MIPS_HI16, _copyRefImage+0x48`). There are 8 in main: two in the MPEG library's
  `_copyRefImage`, which addresses a table inside its own code, and six `lui` of the C library's
  .bss addresses. A boot test found the first of these: without the relocation, movies decoded
  green in a shifted build.
- `tools/label_symbols.py` defines the original's symbols that live inside asm data and binary
  blobs (VU microcode entry points, `FontData`, crt0's `_start`, splat's auto-assigned `func_`/`D_`
  addresses) as labels at their place in the asm.
- configure.py defines the original linker's symbols relative to their sections in main.ld
  (`_fbss`, `_gp`, `_ovl_start_addr`, `_end`, `_stack`, and the C library's common variables `errno`
  and getopt's state after all .bss). main's PROVIDE list keeps only the overlay-area symbols (the
  overlays' entry tables at the fixed overlay address) and `__data_size`; anything else missing is
  a link error.
- Overlays link against `build/main_syms.ld`, exported from this build's main.elf
  (`tools/export_syms.py`), not against original addresses.
- `config/min_align_main.txt` (`tools/min_align.py`) replaces gen_splat's address-derived section
  alignments of the asm (library) objects with the smallest ones that reproduce the layout. Only
  four need more than 16 bytes: the VU microcode and VU data (128), one DMA buffer (64), one table
  (32).

`tools/shift_test.py` inserts padding (up to 0x58 bytes, the slack before `.vutext`'s alignment;
0x40 by default) right after crt0, so every function after it in main's `.text` moves, relinks main
and all overlays, and checks that outside the moved code nothing but relocated words changes, that
no original relocation is missing from a linked asm object, and that all 92,067 relocated fields
(including lui/lo pairs, judged by each relocation's symbol, as in `Sd3dData - 0x138800` above)
resolve to their targets' new addresses. With `--iso` it writes the result into a copy of the disc
image. A build shifted by 0x40 bytes, checked by hand in PCSX2 2.6.3, boots, starts a new game,
loads stages and plays the first cutscenes.

Limit: the overlays load at a fixed address right after main's .bss, and main names their entry
tables (stage_*) at fixed addresses. A change that grows main's data or .bss would also need the
overlays relinked at the new base and main linked against their entry tables (two-pass link).
