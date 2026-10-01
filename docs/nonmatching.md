# Non-matching functions

The game functions below have C that doesn't compile to the original with our compiler build
(the count is in [PROGRESS.md](../PROGRESS.md)). Each section says how far the cause is known:
for some differences it is shown to come from the compiler build (we have MWCC 2.4 build 0017, `mwcps2-2.4-001213`;
the game was built with 2.4.1.01, [toolchain.md](toolchain.md)), not from the C; for others it
isn't.

The matching build links these functions from the original code (`config/asm_functions.txt`,
`tools/asm_fallback.py`), so the build stays byte-identical and shiftable, and `PROGRESS.md` counts
them as linked, not matched. The units they are in don't count as complete in objdiff's report
(`objdiff.json`). Each function keeps its best equivalent C in place, with a `NON_MATCHING:` note
saying what differs and what was tried.

To re-measure (in the build environment, after `ninja`):

```sh
.venv/bin/python tools/nonmatching.py              # all of them, as a Markdown table
.venv/bin/python tools/nonmatching.py view/vc_main # one unit
```

"Words" is the number of 4-byte words that differ from the original when the function's own C is
compiled (measured 2026-09-29). A size change shifts everything after it, so a large count doesn't
necessarily mean a large difference.

## Values recomputed or reloaded

The original keeps a copy of a value where our compiler reuses the one it already has.

| unit | function | words | what differs |
|---|---|---:|---|
| `view/vc_main` | `vcMakeFarWatchTgtPos` | 116 | the original squares a copy of cir_r (`mov.s f2, f20; mul.s f2, f2, f20`, unique in the binary); ours folds the copy into `mul.s f2, f20, f20`, and the 4-byte shift makes the rest differ |

Why it is the compiler build: traced (~60 probes), our build folds every copy of a
register-allocated local into the instruction that reads it, keeping one only when the source is
precoloured (a parameter before the first call, a call result in f0). It does so at every
optimization level and under every pragma (the fold is in the code generator), and never produces
the original's mixed state (the copy kept, one operand still read from cir_r). With the same C,
two 2003 builds (3.0.1b44, b51) emit the original's instructions ([toolchain.md](toolchain.md),
"Compiler-version test"). The only C that matches names `$f2` in an asm helper that emits those two
instructions, which would be transcribed assembly; it isn't used.

## Short arithmetic

A `short` (the DWARF's type) added to an `int`: our compiler sign-extends it again before the add,
the original adds the register directly.

| unit | function | what differs |
|---|---|---|
| `Chacter/m3_play_3d` | `lower_walk_3d` | ana_spd re-extended before the add; the case-0 argument 0x200 also loads in another order |
| `Chacter/m3_play_3d` | `lower_back_3d` | ana_spd re-extended before the add |
| `Chacter/m3_play_3d` | `upper_back_3d` | ana_spd re-extended before the add |

Why the sign extension is the compiler build: all three compile without the global optimizer (their `fabsf` is the
inline-asm helper: `lwc1 f1; abs.s f1, f1`). Our build's conversion code (0x49d8f0 in
mwccps2.exe) emits a sign extension for every int <- short conversion, and the only thing that
removes one is a peephole that needs the operand's definition, which is forgotten at every basic
block boundary ([toolchain.md](toolchain.md), "Register allocation"); with the optimizer on, CSE
of an earlier `(int)x` removes it too. Between `ana_spd`'s definition and the add there are a call
and a branch, fixed by the line table. In the original's code, these three are the only short reads
across a block boundary in a function compiled without the optimizer, and the original doesn't
re-extend them, while it does in 17 optimizer-on functions. So the original compiler kept short
register variables normalized with the optimizer off, and ours doesn't.

`lower_walk_3d`'s second difference is not explained that way. The order in which its case-0 call
loads the argument 0x200 is the float-constant argument order ([toolchain.md](toolchain.md), "Root
cause"), which depends on the leftovers of the code compiled before the function, not on the
compiler build as far as is known. It is unexplained.

Declaring `ana_spd` as `int` and casting the assignment to `(short)` makes all three match
(`lower_walk_3d` with its constant also written `(0, 0x200)`): the same code, but the DWARF declares
`short`. That version was kept as a labelled `FAKEMATCH` for a while; since the difference is the
compiler build's, the functions are linked from the original code instead.

## A local coalesced with an asm register

The original's register allocator merged a C local with the register an asm block copies it to;
ours keeps both.

| unit | function | words | what differs |
|---|---|---:|---|
| `Fog/fog` | `fog_part_wall` | 73 | the original has wall in t1 and no code for the asm's `addu t1, wall, zero` (its line table has no entry for that line, where the twin fog_part_obj has its copy of od); ours puts wall in a1 and keeps the copy, 4 bytes longer. With the asm using wall instead of t1, only that register differs (6 words), but the line table fits the copy. Not yet shown to be the compiler build (measured 2026-09-30) |

## Resolved

Functions once listed here that match now: 51, 12 of them only as labelled fake matches
(**FAKEMATCH** below). Two methods found most of the fixes.

- **Inline asm helpers.** MWCC compiles a whole function without its global optimizer when it
  contains native inline assembly, including an inline helper such as `sqr()` from
  `asm_helpers.h`. A function whose original code looks unoptimized (values reloaded or
  recomputed, no copy propagation) may have used one of those helpers somewhere. When a function
  responds strongly to `#pragma global_optimizer off` ([decomp-workflow.md](decomp-workflow.md),
  "Optimizer settings and inline helpers"), look for such a helper before reaching for the pragma.
- **The original's line table.** It records where each statement starts, so a statement boundary
  the original has and ours doesn't (or the reverse) is direct evidence about how the source was
  written (`tools/dwarf_lines.py layout <unit> <function>`, or `tools/diff_unit.py --lines`).
  fontLoad and clCheckColumn2WallHit had asm in the function body (one line-table entry per
  instruction); sh2gfw_init_vctagbuf had an initialized declaration; shPadSet had a dead
  statement the optimizer removed.

The float-constant-order functions, once the largest group here, match through fitted stand-ins or
spellings fitted to the compiler's state ([toolchain.md](toolchain.md), "Root cause";
[stand-ins.md](stand-ins.md)); several of them are fake matches. Where the evidence shows how the
original was written, that version is kept, even where it moved the constant order further away.

| unit | function | fix | how it was found |
|---|---|---|---|
| `Chacter/m3_bgobj` | `RObjectFunction` | `@bug`: case 0 passes the still-unset scp instead of `this` (`AVOID_UB` guard for other builds) | the twins m3_angela/m3_djames have the same statements but keep scp in v0, so the source differed; reading scp before it is set makes it live on entry, so it coalesces with the incoming a0, as in the original (traced in the coalescer). Circumstantial: the line table can't tell scp from `this`. No compiler build reproduces the old code's allocation either |
| `Event/memo` | `MemoMessageWallet` | `work = game_flag.safe[i]; work++;` in the riddle-level-1 loop | the permuter split the statement |
| `Event/chara_data_load` | `CharaDataFreeSearch` | `#pragma opt_common_subs off` around the function; `j == size >> 13` | the original recomputes `size >> 13` at each use; the pragma reproduces the function exactly, while the rest of the file needs CSE on |
| `Event/event` | `CharToFloat2` | `#pragma global_optimizer off` around the function | the pragma sweep: it reproduces the function exactly; see its `Matching:` note |
| `Event/demoview` | `DdsReadFloat2` | the same as `CharToFloat2` (the same decoder) | the pragma sweep |
| `Event/event` | `EventCheckLookPoint` | `sqr(pos_x) + sqr(pos_z)` (the `asm_helpers.h` helper) | an inline asm helper makes MWCC compile the whole function without the global optimizer, which the original's code showed |
| `Event/event` | `ItemCheckLookPoint` | `pos_x = sqr(pos_x); pos_z = sqr(pos_z);` | the same |
| `gx_awx:Event/stage/stg_apart_w1f` | `EvProgSubCoinCursor` | `px = sqr(px); py = sqr(py);` | the pragma sweep showed it was compiled without the global optimizer (97 -> 1 word); the permuter's result for the twin function pointed at a helper |
| `Event/stage/stg_tgs_trial` | `EvProgElevatorButtonCheck` | the same (its twin) | the permuter replaced the squares with an inline function |
| `sh2shd/sh2shd_char_jms` | `sh2shd_make_reftag_pool_char` | `(unsigned int)&man->shape[shape_no] & 0x7FFFFFFF` without an extra `(unsigned long)` cast | the permuter |
| `sh2shd/sh2shd_char_jms` | `sh2shd_make_reftag_pool_char_p_maria` | the same | the permuter |
| `Multi_thr/util/cmd_serv` | `CmdQueuePut3` | the arguments are queued by a `PUTARG` macro (a `do { } while (0)` block, like the file's `GETARG`); the other `CmdQueuePut*` still match with it | the permuter wrapped the pushes in `do { } while (0)` |
| `GFW/sh2gfw_viewclip` | `sh2gfw_get_ViewRecTangle` | `i = abs(...); j = abs(...); if (i > j)` at the end (j reused) | the permuter assigned j there |
| `GFW/sh2gfw_viewclip` | `sh2gfw_init_vctagbuf` | `cleardata` as an initialized declaration (`union Q_WORDDATA cleardata = {...};`), then `VcBuf[0] = cleardata;` | the original's line table: a statement at `VcBuf[0]` that ours didn't have, after a three-line initializer |
| `Multi_thr/util/cmd_serv` | `CmdQueuePut4` | the queueing in `do { } while (0)` (all `CmdQueuePut*`), each argument pushed by two statements | the allocator's spill cost weights loop depth; the line table has two lines per argument and a nop-only statement where the loop closes |
| `Lens/lens_th_draw` | `sh2gfw_GsExecStoreImage` | unsigned masks (`~7U`), the rounding as one statement | the allocator's constant handling; the line table shows one statement |
| `Enemy/en_common` | `enRoomForbiddenArea` | **FAKEMATCH**: a shaped stand-in fitted before it where the original had no code (its statement shapes, a no-effect constant statement, and its constants' byte patterns are all fitted) | a model of which byte each of the stand-in's statement objects leaves under the 40 constant nodes, confirmed by a compile |
| `GFW/sh2gfw_Vertexpacket` | `sh2gfw_SemiTrans_GeomPacket` | giftag unused (the tag words and copies through SPR[3], as in Geom_MakePacket), the loop increment as in Geom_MakePacket, and `(unsigned int)rest_vNum` in the final tag word | MWCC's tree-level CSE: the temporary is made at the first occurrence; the cast recurs implicitly, and the temporary absorbing rest_vNum gives the DWARF's register-less rest_vNum and the original's s0; our -g DWARF now equals the original's |
| `Enemy/en_ike` | `enIKECtrlAttack2` | **FAKEMATCH**: 5 fits in Attack2 and Attack (listed in their notes) | an exact flag predictor over arena snapshots; a per-call DP |
| `Enemy/en_mkn` | `enMKNCtrlWaitFall`, `enMKNCtrlDown` | **FAKEMATCH**: fits in WaitFall, Mannequin2, Down and Confuse (Down's include no-op `(void)` casts), two stand-ins refitted | the same predictor; EN_SET_LEVEL's do/while is observable through MWCC's label numbering, so where the original used it is evidence |
| `Item/otn_itemmain` | `set_position` | **FAKEMATCH**: 18 edits in 11 of its boxes (comma joins, redundant (float) casts; listed in its note) on the line table's one-line-per-box layout, plus two of item_main_setup's statements in the line table's grouping | an allocator simulation that reproduces every flag; a DP search for the fewest edits |
| `Enemy/en_scu` | `enSCUCtrlCrawl` | **FAKEMATCH**: four fits inside the function (listed in its note) | a model of the flag bytes: no three-fit solution exists |
| `Enemy/en_pap` | `enPAPCtrlAttack` | **FAKEMATCH**: six fits inside the function (listed in its note) | the model: no solution with five or fewer |
| `Event/chara_admin` | `ConnectCharaWorkAdminOut` | **FAKEMATCH**: two float temporaries the DWARF lacks for `8000 + work` | the original's in-place squares; ours propagates the shared value into sqr()'s operands |
| `GFW/sh2gfw_2d_filters` | `sh2gfw_FadeOut_Retain` | a software-double stand-in in the 206-line gap before it (the original has room there) | the a1/a2 shift is the double register mode; our allocator trace shows a double helper call earlier in the file |
| `GFW/sh2gfw_Vertexpacket` | `sh2gfw_Geom_MakePacket` | the loop increment as `curr_vernor += (packsize - 2) * 16;` | the DWARF's register-less `next`: a local coalesced with a CSE temporary |
| `GFW/sh2gfw_2d_filters` | `sh2gfw_Swap_GlowSoft` | `qwd[id]` with `id++` after each quadword (the file's own style in SendDraw_Noise); MWCC folds the increments into the addresses, which gives the unfolded `(id + k) << 4` the original shows. The same idiom made the other five FAKEMATCHes in the file real (Fade2, Fade3, Filter_Glow_Blur, SendDraw_Noise, Swap_Soft); the DWARF's `i` holds GetTexTBP0 >> 5 (evidence: its line table and Swap_Soft's) | the original folds `p[i + k]` normally (sh2gfw_setVCTAG_DrawSys), so the unfolded index was source; layout_compare: 19 -> 29 of the file's 33 functions with the original's statement structure |
| `GFW/sh2gfw_2d_filters` | `sh2gfw_Filter_JustCopy2` | the same `id++` idiom (was a FAKEMATCH, `~~(id + 9)`) | as for Swap_GlowSoft |
| `Enemy/en_nse` | `enNSECtrlAttack` | **FAKEMATCH**: five spellings in Attack, Chase and Precaution fitted to the float-constant order (two one-line level changes are the line table's) | a flag predictor over the arena snapshot, ~1.5M combinations: a chance fit |
| `Enemy/en_red` | `enREDCtrlSeize` | **FAKEMATCH**: nine spellings fitted to the float-constant order (listed in its note) | flag-byte tracing: no uniform shift gives the pattern |
| `view/vc_main` | `vcAdjustWatchYLimitHighWhenFarView` | vcMakeFarWatchTgtPos's locals declared several per line | the line table leaves 7 lines for 16 locals; which grouping is fitted to the flags (noted) |
| `gx_twe:Event/stage/stg_town_east` | `CB_1stMoster_FogHosei` | the game-flag tests through inline helpers (names ours) | the file's own flag arithmetic; inline rather than macro is decided by the float order only (noted) |
| `Effect2/hh_effect_object_texture` | `TextureBinary_DesignateTexture_Load_toAlwaysBuffer` | file-local inline getters (names ours) | the DWARF's v0 locations for pContext and friends throughout the file, which only an inline result gives |
| `Chacter_Draw/model3_vu1_n` | `InitAllDataOne` | **FAKEMATCH**: one bdraw store through a second UNCACHED(); UncachedAddr as an inline with a local (evidence: Model3LoadMpg1's DWARF) | the allocator simulation |
| `Item/otn_option` | `key_conf` | `x ^ 0xFFFFFFFF` at all three places | the only `li -1; xor` in the game: an unsigned constant used more than once stays in a temporary, where `^ -1` becomes `nor` |
| `Effect/ef_rain` | `efRainDropDrawLINE` | **FAKEMATCH**: `rgba` through an aligned(4) row typedef, and a software-double stand-in before the file's first function, where whether the original had room can't be told ([stand-ins.md](stand-ins.md): unknown) | stack layout: the original has rgba 4-aligned in declaration order, unlike its 139 other small-element arrays; no evidence for the typedef |
| `sh2shd/sh2shd_shadow_model` | `sh2shd_exe_drop_shadow` | `qcopy` as a one-line asm macro rather than an inline function (no parameter temporary for the destination) | the allocator simulation: the copies' destination and source numbering |
| `view/vc_main` | `vcMakeIdealCamPosForThroughDoorCam` | three statements through `cos_ang_y` as the line table splits them | the line table (and 8 combinations tried: only this one matches) |
| `GFW/sh2gfw_parse_and_packet` | `sh2gfw_packet_Poly_LocalTexUsing` | a dead load of the texture header (reconstructed, noted) | the line table: a statement whose only code is `itex * 4` |
| `Event/picture` | `PictureDraw` | the float bits of constants through an inline-asm `fbits()` (mfc1), which also switches the global optimizer off as the original's reloads show | `move r, zero; nop` and `lui r, 0x3F80` for 0.0f/1.0f are what MWCC makes of mfc1 on a constant |
| `MC/mc_menu` | `mcSelectData` | in the page-down branch, `if (fmax < 5) { num = fmax; } else { ... }`: the then-block's `num = fmax` reuses the test's sign-extended fmax and leaves no code, so the original branches around an empty block | the line table: the test, then a statement line with no code; and itof() (inline asm), which compiles the function without the global optimizer, as the original's re-extended values show |
| `MC/mc_menu` | `mcDrawSlot` | the clamp as `if (fmax < 5) { n = fmax; } else { fmax = 5; }`: the then-block's statement was dead (the original branches around an empty block); its content is reconstructed after mcSelectData's `num = fmax`, not recovered (was a FAKEMATCH, `fmax < 5 ? 0 : (fmax = 5);`) | the branch shape, and mcSelectData's clamp, which compiles the same way |
| `Collision/cl_main` | `clCheckColumn2WallHit`, `clGetHitSectListMOVEOutDoor` | the VU0 code as asm in the function bodies instead of inline helpers | the original's line table has an entry per asm instruction there, which only in-body asm produces |
| `Font/font` | `fontLoad`, `fontPrintStrMain`, `fontPrintWord`, `fontPrintStrWide`, `fontGetMesWidth` | if/else for the code fetch, fontLoad's asm in the body, fontGetMesWidth's dead copies of fontPrintWord's sstr/sy statements, and two reconstructed spellings (noted in the source) | DWARF locals and locations, and the line table's statement entries and gaps |
| `SH2_common/pad` | `shPadSet` | a dead statement `work = pad[i][2] + pad[i][3];` at the top of the stick-byte loop (reconstructed: the expression itself is lost, see its `Matching:` note) | the original's line table has a statement there whose only code is those two addresses; `work` is in the DWARF and otherwise unused |

## What would count as a fix

The same rules as every other function ([STYLE.md](../STYLE.md)):

- C a programmer could plausibly have written: permuter output that only matches through
  meaningless temporaries, reordered no-op statements or casts that change nothing is not a fix.
  In the rare case where such code is the only match found and it is still clearly better than
  the assembly (for example, it is correct, equivalent C), it may be kept, marked
  `FAKEMATCH:` with what is fake about it, and counted separately.
- A pragma or per-function setting only where it reproduces the function exactly and is something
  the original's developers could have had in their source (see STYLE.md, "Pragmas").
- Stand-ins fitted to the compiler's state (`__stripped_float_code*`) stay a last resort and are
  counted as fitted in `PROGRESS.md`.

If the original compiler build (2.4.1.01) becomes available from a legitimate source, the functions
above are the first to try with it.
