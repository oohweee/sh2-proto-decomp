# The original's headers: what the DWARF shows

Our sources include one generated `sh2.h` (every type, prototype and global from the DWARF). The
original files included their own headers, in their own order, with inline functions and static
tables in them. That matters for matching in three ways: header code leaves compiler state behind
(the arena leftovers that decide float-constant order, [toolchain.md](toolchain.md), "Root cause"),
inline functions defined in headers produce out-of-line copies at particular places, and a header's
`inline` functions get their own ELF symbol binding. This file collects what the prototype's DWARF 1
says about the headers, separating facts (read from the ELF) from inferences, and records a pilot
that rebuilt four units with that header structure (2026-09-29).

**Reproducing.** Sections 1 and 2 can be checked with published tools: `tools/dwarf_lines.py orig
<function>` prints a function's compile-unit name (the header's path) and its line table, and
`tools/binutils/<os>/mips-ps2-decompals-readelf -s` shows the symbol bindings. Section 4 was tested by
compiling small files with the project's compiler. The rest was produced with scratch scripts that
are not in the repository: the cross-unit statics counts of section 3, the pilot of section 5, and
the traces in section 5 (gdb scripts on the compiler, as in toolchain.md, "Root cause").

## 1. What MWCC's DWARF 1 records about includes

Facts:

- One compile unit per function. Its `AT_name` is the path of the file the function's body is in,
  so a function defined in a header gets a compile unit named after the header
  (`E:\work\sh2(CVS全取得)\src\Chacter\fi_libvu0_inline.h`). 24 of the 5,244 compile units are
  named after `.h` files (section 2). Every object also has one compile unit without code (named
  after the `.c` file) holding its types and file-scope variables.
- No declaration file or line on any DIE. The attributes present are name, sibling, location,
  types, byte size, bit fields, element lists, subscripts, low/high pc, `AT_stmt_list`,
  `AT_language`, `AT_producer` ("MW MIPS C Compiler"), `AT_prototyped` and Metrowerks extensions
  (0x2013-0x2173, 0x2303 on subprograms: register save information; tag 0x4080 with 0x2296/0x22A8
  for overlays). Types carry no file information.
- The line table (`.line`) is per compile unit: (line, column, address) entries with no file index.
  In `mwcps2-2.4-001213`, code inlined from a header or from the same file gets the call's line (tested
  with `-g`); in the original only 2 of 4,684 functions have an entry before their first line, and
  none after their closing brace. So line tables carry no include information.
- ELF binding 13 (`STB_LOPROC`) on 4 symbols: `sh2gde_getWorldScreenMatrix`,
  `sh2gde_getWorldViewMatrix` (header compile units) and `check_self_spot`, `check_self_para`
  (`sh2shd/sh2shd_shadow_model.c`). Our compiler gives exactly this binding to the out-of-line copy of
  a non-static `inline` function (tested); a plain function gets binding 1. So these four were
  declared `inline` in the original, and `src/` declares them so (the two `sh2gde_` functions in
  `include/GFW/sh2_get_drawenv.h`, the other two in `sh2shd_shadow_model.c`).

What survives of the headers is therefore: the header-named compile units (section 2), statics that
headers defined in many units (section 3), the binding above, and the line of each unit's first
function, which bounds the size of its include/declaration block (median line 80; lens_flare.c 388).

## 2. Header-named compile units

Facts (address order; "after" is the unit's function the copy follows):

| header (`src\...`) | function | binding | header lines | unit | after |
|---|---|---|---|---|---|
| `Chacter\fi_libvu0_inline.h` | `_sceVu0ApplyMatrix_1` | local | 209-223 | Chacter/anime.c | shCharacterAnimeReconstruct |
| `Chacter\fi_libvu0_inline.h` | `_sceVu0Normalize` | local | 109-122 | Chacter/anime.c | EigenVector |
| `SH2_common\sh_vu0.h` | `_shLength` | local | 1196-1211 | Chacter/sh_character_status.c | shBattleCheckTargetMyArea |
| `Chacter\fi_libvu0_inline.h` | `_sceVu0RotTransPers` | local | 227-250 | Lens/lens_flare.c | shLensFlareSetLightSeed |
| `GFW\sh2_get_drawenv.h` | `sh2gde_getWorldScreenMatrix` | 13 | 142-148 | Lens/lens_flare.c | (same) |
| `Chacter\fi_calc.h` | `ktVectorNormal` | local | 197-199 | Lens/lens_flare.c | shLensFlareMakeScreenAngle |
| `Chacter\fi_libvu0_inline.h` | `_sceVu0Normalize` | local | 109-122 | Lens/lens_flare.c | (same) |
| `GFW\sh2_get_drawenv.h` | `sh2gde_getWorldViewMatrix` | 13 | 157-159 | Lens/lens_flare.c | shLensFlareMakeScreenInfo |
| `Fog\spack.h` | `spkSetFakeData128` | local | 168-171 | GFW/sh2gfw_SemiTrans_FrameWork.c | TrimSet_Packet_SprtoMemBuff |
| `Fog\spack.h` | `spkSetData128P` | local | 160-161 | GFW/sh2gfw_SemiTrans_FrameWork.c | (same) |
| `SH2_common\sh_vu0.h` | `_shAddVector` | local | 1262-1267 | Chacter_Draw/sh2_JmsSpot_Man.c | Programed_Light_Set (end of file) |
| `SH2_common\sh_vu0.h` | `_shScaleVector` | local | 1359-1365 | Chacter_Draw/sh2_JmsSpot_Man.c | (same) |
| `SH2_common\sh_vu0.h` | `_shNormalize` | local | 1242-1251 | Chacter_Draw/sh2_JmsSpot_Man.c | (same) |
| `SH2_common\sh_vu0.h` | `_shOuterProduct` | local | 1119-1126 | Chacter_Draw/sh2_JmsSpot_Man.c | (same) |
| `SH2_common\sh_vu0.h` | `_shInnerProduct` | local | 1137-1148 | Chacter_Draw/sh2_JmsSpot_Man.c | (same) |
| `GFW\gfw_test\sh2gfw_OV_CharaDrawHook.h` | `LinearTrim` | local | 83-84 | Event/stage/stg_apart_w2f.c (gx_awy) | start of overlay |
| `GFW\gfw_test\sh2gfw_OV_CharaDrawHook.h` | `Parallel_Trim` | local | 87-137 | Event/stage/stg_apart_w2f.c | start of overlay |
| `GFW\gfw_test\sh2gfw_OV_PreDraw.h` | `AP_Hosei_Light` | local | 232-279 | Event/stage/stg_apart_stair.c | start of overlay |
| `GFW\gfw_test\sh2gfw_OV_PreDraw.h` | `PS89_ParallelLightSet` | local | 411-444 | Event/stage/stg_labyrinth_w.c | start of overlay |
| `GFW\gfw_test\sh2gfw_OV_PreDraw.h` | `PSLE_ParallelLightSet` | local | 467-503 | Event/stage/stg_labyrinth_e.c | start of overlay |
| `GFW\gfw_test\sh2gfw_OV_PreDraw.h` | `PSLN_ParallelLightSet` | local | 530-563 | Event/stage/stg_labyrinth_n.c | start of overlay |
| `GFW\gfw_test\sh2gfw_OV_SpecialDraw.h` | `Draw_rr41Window` | local | 97-145 | Event/stage/stg_hotel_1f_f.c | start of overlay |
| `GFW\gfw_test\sh2gfw_OV_SpecialDraw.h` | `Draw_rr91_rr94Window` | local | 286-333 | Event/stage/stg_hotel_3f_f.c | start of overlay |
| `GFW\gfw_test\sh2gfw_OV_PreDraw.h` | `ru21_ru24_LightFlame` | global (1) | 33-124 | Event/stage/stg_hotel_fire.c | start of overlay |

### Header map

Contents known from the table (facts: names, lines, binding, unit; bodies are the binary's), and
inferences marked as such.

- **`Chacter/fi_libvu0_inline.h`** (at least 250 lines): `_sceVu0Normalize` 109-122,
  `_sceVu0ApplyMatrix_1` 209-223, `_sceVu0RotTransPers` 227-250, all `static`. Used by anime.c,
  lens_flare.c. Inference: the inline-only `_sceVu0*` helpers in our `include/fi_libvu0_inline.h`
  (which leave no DWARF) were here too; their order and lines are unknown.
- **`Chacter/fi_calc.h`** (at least 199 lines): `ktVectorNormal` 197-199, `static`, a call of
  `_sceVu0Normalize` (so fi_libvu0_inline.h came first). Used by lens_flare.c; inference: anime.c
  (section 4).
- **`GFW/sh2_get_drawenv.h`** (at least 159 lines): `sh2gde_getWorldScreenMatrix` 142-148 and
  `sh2gde_getWorldViewMatrix` 157-159, non-static `inline` (binding 13). Used by lens_flare.c.
  Inference: it also declares `GFW/sh2_get_drawenv.c`'s `sh2gde_*` functions.
- **`SH2_common/sh_vu0.h`** (at least 1,365 lines): `_shOuterProduct` 1119-1126, `_shInnerProduct`
  1137-1148, `_shLength` 1196-1211, `_shNormalize` 1242-1251, `_shAddVector` 1262-1267,
  `_shScaleVector` 1359-1365. Used by sh_character_status.c, sh2_JmsSpot_Man.c. Our
  `include/sh_vu0.h` has 340 lines: most of the original's content is unknown.
- **`Fog/spack.h`** (at least 171 lines): `spkSetData128P` 160-161, `spkSetFakeData128` 168-171. Used
  by sh2gfw_SemiTrans_FrameWork.c.
- **`GFW/gfw_test/sh2gfw_OV_CharaDrawHook.h`**: `LinearTrim` 83-84, `Parallel_Trim` 87-137 (static).
  stg_toilet.c has its own `LinearTrim` (its compile unit is the .c, lines 282-283).
- **`GFW/gfw_test/sh2gfw_OV_PreDraw.h`** (at least 563 lines): `ru21_ru24_LightFlame` 33-124 (global),
  `AP_Hosei_Light` 232-279, `PS89_ParallelLightSet` 411-444, `PSLE_ParallelLightSet` 467-503,
  `PSLN_ParallelLightSet` 530-563 (static). Inference: plain (not inline) functions, the header
  included at the top of these stage files, each overlay keeping the one its stage references
  (emitted at the start of `.text`, where the header was) and the linker dead-stripping the others.
  The statics `colvec`, `colref`, `DynamicLW` are defined (live) in exactly the four static-function
  users plus sh2_JmsSpot_Man.c, which suggests a shared definition too.
- **`GFW/gfw_test/sh2gfw_OV_SpecialDraw.h`** (at least 333 lines): `Draw_rr41Window` 97-145,
  `Draw_rr91_rr94Window` 286-333 (static); same pattern.

## 3. Header-defined statics

Fact: the code-less compile unit of each object lists the file-scope variables the unit defined or
referenced, with location 0 for a static that was defined but has no storage in the binary.
Inference (strong): a static defined in many units and kept in at most one was defined, with its
initializer, in a header those units included; the Metrowerks linker dropped the unused copies.
(Our compiler emits them: each gets its own `.rodata` section, which the linker model would have to
drop.)

| statics | units | kept in |
|---|---:|---|
| `sh2_attack_list` | 198 | Chacter/sh_character_battle.c |
| `human_skelton`, `enemy_skelton`, `obj_outdoor_skelton`, `obj_anime_skelton`, `obj_stay_skelton` | 144 | Chacter/m3_sc.c |
| `pjames_anim` ... `pjames_demo_anim` (10) | 57 | m3_play.c, m3_play_event.c (2 users) |
| `maria_apeear_point_list`, `pmaria_sub_status_flag` | 10 | Chacter/m3_maria_sub.c |
| `jms_walk_spd_ana` ... `pjames_lower_flag` (8) | 9 | Chacter/m3_play.c |
| per-enemy `*_anim`, `d_*_anim` pairs (red, mkn, scu, oni, nse, edb, pap, dmaria, dangela, ike, tyu, arm, bos, boat) | 2-9 each | the enemy's m3_*.c |
| `item_screen_obj_data` | 5 | Chacter/item_screen_obj.c |
| `watch_mv_prm_*` ... (19) | 4 | view/vc_main.c, vc_newest.c |
| `item_size` | 4 | Item/item_tgs_tmp.c |
| `_elastic_vec`; `_mass`, `_radius`; `_square_1x_stq_list`, `_square_00_rgba` | 5; 3; 2 | one Effect2 unit each |
| `adr_shNumber` | 2 | none |

Many other same-named statics are kept in every unit that has them (`ev_pos`, `ev_list`, `ev_prog`,
`gi_list`, `SpecialDrawFunctions`, the Effect2 `_square_*` tables...): per-file definitions with
conventional names, not header evidence.

Order (facts): `sh2_attack_list` is the first variable in 197 of its 198 units (not in
gamemain.c); the five skelton tables come right after it, in that order, in 143 of 144 (not in
chara_saveinfo.c). So those two headers were included before anything else that defined a
variable, the attack list's first. The per-enemy `*_anim` tables appear in different relative orders
in different units (e.g. red/nse, edb/pap), so they are not one header in a fixed order.

Variable order within the list (inference, checked on sh2gfw_fogtest_main.c and lens_flare.c's
shLensFlareInit and shLensFlareExec): statics with initializers when they are defined; then, per
function in code-generation order, the globals it references for the first time, in reverse order
of first reference; unreferenced tentative definitions last (lens_flare.c ends with `adr`,
`sh_lf_packet`, both location 0, which our source doesn't declare).

## 4. How out-of-line copies of header functions arise (tested)

Tested with `mwcps2-2.4-001213`:

1. **Inlining off** (`-inline off`, or `#pragma dont_inline on`): every inline function a function
   calls is emitted out of line right after that function (first caller only). This reproduces
   lens_flare.c's five copies and their order; either form gives identical output.
2. **Nested inlining**: with inlining on, an inline function's call to another inline function is
   not inlined; the callee is emitted out of line right after the function being compiled.
   anime.c matches as a whole with the `fi_libvu0_inline.h` functions defined `static inline` in
   the header when it calls `_sceVu0Normalize` through `ktVectorNormal` and `_sceVu0ApplyMatrix_1`
   through a similar wrapper (name unknown): the copies land after shCharacterAnimeReconstruct and
   EigenVector as in the original, and EigenVec2Mat calls the copy. anime.c can't have had
   inlining off: in the same functions, `_sceVu0InterVector` and `_sceVu0OuterProduct` are inlined.
3. **Calls before the definition**: an inline function declared before its caller and defined
   after it is emitted out of line after the caller; several such copies come out in definition
   order (`include/sh_vu0.h`). `sh_character_status.c` and `sh2_JmsSpot_Man.c` use this form (they
   include `sh_vu0.h` after the last caller). Whether the original used it is unknown; nested
   inlining may explain some of these copies too (untested), and neither form explains the
   end-of-file placement in sh2_JmsSpot_Man.c by itself.
4. **Plain static functions in the overlay headers** (section 2), emitted where included.

## 5. Pilot (2026-09-29)

Four units were rebuilt with the header structure above, in scratch copies outside `src/`
(headers `fi_libvu0_inline.h`, `fi_calc.h`, `sh2_get_drawenv.h`, `libvu0.h`, and two table headers
whose real names are unknown, holding the attack list and the skelton tables). A modified
`diff_unit.py` also dropped the unit's location-0 statics, as the Metrowerks linker did; without
that, the tables add `.rodata` and the unit isn't linkable.

| unit | header structure | result | stand-ins | in `src/` |
|---|---|---|---|---|
| Lens/lens_flare | attack-list header; fi_libvu0_inline.h, fi_calc.h, sh2_get_drawenv.h with the inline definitions; `-inline off`; no local copies | 17/17, linkable (also with the pragma) | still needed | yes, without the attack-list header |
| Fog/sh2gfw_fogtest_main | attack-list and skelton headers first | 3/3, linkable | still needed | no |
| Effect/ef_stage | attack-list and skelton headers | 27/27, linkable | both still needed | no |
| Chacter/anime | fi_libvu0_inline.h definitions, fi_calc.h, nested inlining; no local copies | all match, linkable | (none) | yes |

Why the stand-ins stay: each stand-in site was traced (allocations and rewinds of the arena, the
copy each call argument's constant node is made from, and a watchpoint on that node's flag byte),
in each unit with and without its stand-in:

- lens_flare (shLensFlareMakeEffectTargetRate), stg_toilet (Toilet_Dof_Filter), ef_stage (both
  sites), fog_blow (fogMoveParticle2): the flag byte is last written by the file's own earlier
  functions (their parse or code generation), after the headers. Header content can't change it.
  What differed in the original is in those functions or in the target function's own allocations.
- sh2gfw_fogtest_main (calcmain, the first function): the byte is never written before calcmain
  (fresh memory; only the parse of the local asm helper `vcopy_gcc_float` covers it without writing
  it). Here the original's include block did matter, but its content is unknown: none of 81
  arrangements of the known inline headers (each of 9 and every ordered pair, after the tables) fixes
  it (36 leave the 3-word difference, 45 make it 6). `tools/standins.py` reports this stand-in as
  unknown, since it comes before the file's first function; the 53 lines before calcmain could
  hold more than includes.

So header parsing doesn't decide these fake-matched functions: header structure only reaches a
unit's first function (and memory deeper than anything later code touches).

### lens_flare: `-inline off`, per file

Every header inline function lens_flare.c calls is out of line, each after its first caller,
across the whole file and inside a header function (`ktVectorNormal` calls `_sceVu0Normalize`
out of line); nothing in the file shows an inlined call. That is a setting for the whole file, so by
STYLE.md "Pragmas" it belongs in the build: `config/file_flags.txt` gives lens_flare `-inline off`,
the three headers are in `include/` (`fi_libvu0_inline.h`, `fi_calc.h`, `GFW/sh2_get_drawenv.h`),
and the two `sh2gde_` functions are `inline` (binding 13). anime.c uses the nested-inline form of
section 4.

## 6. Status and remaining work

In place: per-file compiler flags (`config/file_flags.txt`, used by the build and every tool that
compiles a unit); the headers lens_flare.c and anime.c need, with the original's inline
definitions on their original lines; the four binding-13 functions declared `inline`.

Also in place, with every unit rebuilt and checked against them:

- **Library headers.** `include/sdk/` (Sony's EE libraries: libvu0, libgraph with the GS register
  macros `GS_REG_*`/`GS_SET_*` (ps2sdk's names), libvifpk, libgifpk, libdma, eekernel, sifdev, sifrpc, libcdvd, libmc, libmpeg, libpad,
  libscf, libsdr), `include/libc/` (string, stdio, stdlib, math, stdarg, unistd) and `include/lib/`
  (the game's libShPad and kernel additions, which have no DWARF). Only what the game uses; each
  header names its sources: function names from the executable's symbol table, types and layouts
  from the DWARF or the call sites, hardware register layouts from public hardware documentation,
  library membership from the object each function is in; no SDK file was used). C files don't
  declare library functions or macros themselves, except three `void *memset();`/`void *memcpy();`
  for calls without a prototype (sh_character_status.c, sh_kt_vif1ot2.c, hh_class_manager.c).
  Including the headers changes no unit's code, so no file keeps local declarations for the
  float-constant order; common.h includes `libc/stdio.h` for printf.
- **No renamed-away declarations.** No file hides a generated declaration with a
  `#define foo foo_hdr` / `#include "sh2.h"` / `#undef foo` pattern. Where the DWARF was
  lossy for every unit, `config/prototype_overrides.txt` says what it lost: unused parameters
  (`EFCTSetGunSmoke`, `sh2gfw_Set_SpotLight`), `()` versus `(void)` (MWCC writes no
  `AT_prototyped`: `fog_part_newpos`, the callback of `clSetCharaHitColumn`), `volatile` (`isUp`,
  `vblankCount`, `shHdWork`); `sh2shd_init_outdoor_man2` and `sh2gfw_packet_Poly_LocalTexUsing`
  need nothing else. Where one unit needs its own declaration (calls without a prototype in
  m3_maria_sub.c, pss_audiodec.c and m3_sc.c, vc_calc.c's aligned by-value struct, and the static
  `close_to_value` and `font_print` of m3_boat.c and result.c), `gen_headers.py` wraps the
  prototype in `#ifndef SH2_LOCAL_<name>` with the reason, from a `local` line or, for the statics,
  from the DWARF; `tools/lint.py` checks every `SH2_LOCAL_` define. `Vertex_Infomeation_List`,
  whose layout differs between the nine units that define it, is only declared in `types.h`.
- **Game API by directory.** `sh2/functions.h` includes `sh2/api/<dir>.h`: each directory's
  prototypes by unit, in the order of their code.
- **Duplicates.** `fjAssert` (common.h) serves the enemy code too. `distXZ(b, a)` and
  `enDistXZ(a, b)` (and `distXYZ`/`enDist`) stay: with the other one's parameter order, 17 enemy
  functions (2 for enDist) change, since inline-call arguments are evaluated last to first.

Not done:

1. **Tooling**: a published extractor for sections 2-3 (tables, like `tools/standins.py`), and
   `mwcc_fixup.py` dropping a unit's static data objects that have location 0 in the original's
   DWARF and no reference, as the Metrowerks linker did.
2. **Include layout**: keep the generated `sh2.h` for declarations. Declarations leave no useful
   compiler state (in fogtest, everything before the first function body stayed within the first
   0x600 bytes of the arena). Add original-named headers only for content that has code or data:
   `SH2_common/sh_vu0.h` and `Fog/spack.h` under their original paths, the three `sh2gfw_OV_*.h`,
   and table headers with unknown names saying so. `gen_headers.py` would mark header-defined
   functions `inline` (binding 13) and leave their definitions to the headers; the tables' data
   comes from the unit that keeps them.
3. **Order**: the other out-of-line copies (sh_character_status, sh2_JmsSpot_Man, SemiTrans); the
   overlay headers (one shared header, unused functions dead-stripped through
   `stripped_functions.txt`); then the tables into the 198/144 units they belong to, one directory
   at a time, checking every unit.
4. **Stand-ins**: classify all 78 by the trace above (flag byte written by header-phase code, by
   earlier functions, or never); only the first two kinds say where the original differed. Improve
   `standins.py`'s "usual gap" for files with few functions.

Risks: the headers' content is mostly unknown (sh_vu0.h 340 of 1,365+ lines; helper order
invented), and filling it in until a stand-in disappears would only move the fitting into a
header. Adding tables or inline bodies changes leftovers for first functions that match today, so
every step needs all units rebuilt, and broad header changes can break the matching build.
Expected yield for the fake matches is small; the gains are fidelity (no local copies, original
headers, bindings) and a precise list of which stand-ins are about header content.
