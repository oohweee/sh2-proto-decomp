# Strategy

## Scope (decided 2026-09-28)

- **Target: the July 2001 prototype**, not retail. It carries full DWARF 1 (names, types, locals,
  line numbers, the original source tree), which removes most of the guesswork of a normal
  decompilation. Retail would be a separate target; this DWARF doesn't describe it.
- **Independent and LLM-written.** The project is an experiment in what an LLM can do alone. An
  existing human SH2/SH3 decompilation project has a no-LLM policy, so nothing is taken from it,
  and this project doesn't contribute there.
- **Purely a decompilation**: no port or other derived project is planned here.

## Matching gate (defined 2026-09-28)

Byte-identical **loadable image**:
1. `main` section contents (file bytes `0x700`–`0x2C8480`, loaded at `0x100000`), and
2. each `GX/*.BIN` overlay present on the disc.

`ninja` (or `ninja check`) verifies all 13 targets. The full 13 MB ELF (symtab, DWARF, `.mwcats`,
relocations) is not part of the gate; matching it would need `mwldps2` and `-sym on` output to
match exactly.

## Risks

1. **Compiler build.** 2.4.1.01 isn't among the public builds; 2.4 build 0017 (`mwcps2-2.4-001213`) is used. The known
   differences (data alignment, which the build models, and most of the functions linked from the
   original code) are in [toolchain.md](toolchain.md) and [nonmatching.md](nonmatching.md).
2. **Missing overlays.** 39 of 51 overlays have no binary on this disc, so their code can't be
   matched ([rom-map.md](rom-map.md)).
3. **Libraries.** 937 functions in `main` have no DWARF: Sony's EE libraries, the C library and
   the game's own `libSh*` libraries. They stay assembly, as do the Metrowerks runtime (7
   functions) and the sound driver's EE side (`sd0712`, 25 functions), which have DWARF.
4. **VU microcode and inline asm.** VU1 microprograms stay as data in the assembly (`.vutext`); the game's VU0 inline headers
   are written as GCC-style asm ([decomp-workflow.md](decomp-workflow.md), "Inline assembly").

## Phase 0: recon

- [x] Compiler family identified (Metrowerks MIPS C 2.4.1.01).
- [x] DWARF readable (`tools/dwarf1.py`: all 4,346 distinct function names render). The DWARF
      describes 4,670 functions: 4,189 in `main` (4,157 of the game's, 25 of `sd0712`, 7 of the
      Metrowerks runtime) and 481 in the 51 overlays, where many names repeat (`EvRoomInit`,
      ...).
- [x] WSL + wibo set up; MWCC runs.
- [x] Test functions match byte-for-byte: `mwcps2-2.4-001213`, `-O2,p -sdatathreshold 0` (8/8).
- [x] Where the 39 missing overlays are: the file index lists them as loose files, not `.MGF`
      members, so they aren't on this disc ([formats.md](formats.md)).
- [x] Gate defined (2026-09-28): loadable image of `main` + 12 on-disc overlays.

## Phase 1: matching build from assembly

- [x] `main` rebuilds byte-identical from asm (2026-09-28). 541 code units: crt0, 328 source
      files with DWARF (325 of the game's, the sound driver's `sd_call.c` and two Metrowerks
      runtime files) and 212 library objects (from their `.text` section symbols), plus the VU
      microcode as a binary blob. The DWARF also attributes out-of-line functions to 5 headers
      (`fi_calc.h`, `sh_vu0.h`, ...); they belong to the files that include them.
- [x] The 12 `GX/*.BIN` overlays on disc rebuild byte-identical (2026-09-28). Each is one stage
      source file; the Metrowerks overlay header `"MWo3"` + padding is kept as a binary blob.
- [x] Data/rodata/sdata/bss split per unit (2026-09-28, `tools/datasplit.py`): each code unit owns one
      contiguous range per section; code-less data files and assembler objects are separate blocks.
- [x] objdiff config + `tools/progress.py` (2026-09-29; `objdiff.json` from `configure.py`; the
      `game` category of objdiff's report agrees with `progress.py`).

## Phase 2: decompilation (status 2026-09-30)

- [x] Every game source file is C and linked (337/337 units: 325 in `main`, one per overlay);
      the match counts, by kind, are in [PROGRESS.md](../PROGRESS.md). Functions that don't match
      are linked from the original code ([nonmatching.md](nonmatching.md)).
- [x] Honesty accounting in `PROGRESS.md`: fake matches, stand-in fits (measured,
      `config/standin_deps.txt`) and order fits (`config/order_fits.txt`).
- [ ] Fewer fitted stand-ins and fake matches ([stand-ins.md](stand-ins.md)).
- [ ] Libraries (Sony SDK, Metrowerks runtime, sound driver) stay assembly for now.
