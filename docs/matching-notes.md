# Matching notes

The full text of the longer `Matching:`, `FAKEMATCH:` and `NON_MATCHING:` notes. The source keeps a
short version of each (what is unusual, and whether it is fitted, fake or inferred) with a link to
its entry here; the labels are explained in [reading-the-source.md](reading-the-source.md).

Each entry is the full text of a note whose short form is in the source; the source's short
version may be worded differently. Where a note says "above", "below", "here" or "see there", it means its place in the
source file: the entries are listed by file and, within a file, in source order, under the
function (or other definition) the note stands next to. The anchors (`#<unit>-<function>`, lower
case) are stable; link to them.

## Chacter/anime.c

<a id="anime-inlineapplymatrix_1"></a>
### inlineApplyMatrix_1

Matching: the original has out-of-line copies of fi_libvu0_inline.h's _sceVu0ApplyMatrix_1 (after
shCharacterAnimeReconstruct) and _sceVu0Normalize (after EigenVector; EigenVec2Mat calls it too),
while other inline functions of that header are inlined here, so inlining was on. MWCC doesn't
inline a call made inside another inline function: it emits the callee out of line right after the
function being compiled ([docs/headers.md](headers.md), section 4). _sceVu0Normalize is reached
through fi_calc.h's ktVectorNormal (a real function: the DWARF has its copy in lens_flare.c).
_sceVu0ApplyMatrix_1 is reached the same way through the wrapper below: that a wrapper existed is
inferred from the copy's placement; its name (invented) and header are unknown.

## Chacter/m3_bgobj.c

<a id="m3_bgobj-robjectfunction"></a>
### RObjectFunction

Matching: why the bug and not `this`: the original copies the shCharacterGetSubCharacter() result to
a0 (`paddub a0, v0`) and keeps scp there, where the twins, which are the same source statement for
statement (line table), keep it in v0. Reading scp before it's set makes it live on entry, so the
allocator merges it with the incoming a0, and it can't then share v0 with the call result: exactly
the original's code. With `this`, scp is merged into v0 as in the twins (FUN_004a3730); no other
spelling found changes that. The line table can't tell scp from `this`, so this is the reading that
fits the code, not a proof. MWCC warns that scp isn't initialized.

## Chacter/m3_play_3d.c

<a id="m3_play_3d-lower_walk_3d"></a>
### lower_walk_3d

NON_MATCHING: ana_spd is `short` (the DWARF's type). Our compiler build sign-extends a short
register variable again before the int add when a call and a branch come in between; the original's
adds the register directly. That is a compiler-build difference
([docs/nonmatching.md](nonmatching.md), "Short arithmetic"), so the function is linked from the
original code. Declaring ana_spd `int` with a (short) cast matches, but contradicts the DWARF. The
case-0 argument 0x200 also loads in another order than the original's (its float-constant order,
[docs/toolchain.md](toolchain.md), "Root cause"); `(0, 0x200)` would match it but isn't plausible C.

<a id="m3_play_3d-lower_back_3d"></a>
### lower_back_3d

NON_MATCHING: ana_spd is `short` (the DWARF's type). Our compiler build sign-extends a short
register variable again before the int add when a call and a branch come in between; the original's
adds the register directly. That is a compiler-build difference
([docs/nonmatching.md](nonmatching.md), "Short arithmetic"), so the function is linked from the
original code. Declaring ana_spd `int` with a (short) cast matches, but contradicts the DWARF.

<a id="m3_play_3d-upper_back_3d"></a>
### upper_back_3d

NON_MATCHING: ana_spd is `short` (the DWARF's type). Our compiler build sign-extends a short
register variable again before the int add when a call and a branch come in between; the original's
adds the register directly. That is a compiler-build difference
([docs/nonmatching.md](nonmatching.md), "Short arithmetic"), so the function is linked from the
original code. Declaring ana_spd `int` with a (short) cast matches, but contradicts the DWARF.

## Chacter_Draw/sh2_JmsSpot_Man.c

<a id="sh2_jmsspot_man-programed_light_set"></a>
### Programed_Light_Set (the sh_vu0.h helper declarations)

Matching: Programed_Light_Set calls the sh_vu0.h helpers below instead of inlining them, and their
out-of-line copies follow it at the end of the file (the DWARF places them in sh_vu0.h). MWCC does
that for a helper declared before its first caller and defined after it (as sh_character_status.c
does with _shLength), so they are declared here and sh_vu0.h is included after Programed_Light_Set.
The copies come out in the header's definition order.

## DBG/shDBG_fontHandle.c

<a id="shdbg_fonthandle-shdbg_initfontenv"></a>
### shDBG_InitFontEnv

Matching: qwd is a Q_WORDDATA * (the 16-byte-aligned typedef,
[include/common.h](../include/common.h)), not a plain union Q_WORDDATA *: the original keeps it in a
16-byte-aligned stack slot at 0x10(sp), which only a pointer to the aligned typedef gets. The
original's DWARF also has an int id that no code survives for (v0); it is reconstructed as the GS
context added to the context-1 register addresses.

## Effect/ef_rain.c

<a id="ef_rain-efraindropdrawline"></a>
### efRainDropDrawLINE

FAKEMATCH: rgba is declared through an aligned(4) typedef of its row type. The original's stack has
it 4-aligned between q and w in declaration order (q 0x60, rgba 0x64, w 0x6C); our compiler gives
every local over 4 bytes 8-byte alignment and allocates it first, and so did the original everywhere
else (all 139 other small-element stack arrays of more than 4 bytes in the DWARF are 8-aligned). A
typedef leaves no trace in this DWARF, so the declaration type is still the DWARF's unsigned char
[2][4], but nothing shows that the original had such a typedef; it may instead be a 2.4.1.01 rule
for arrays of arrays (rgba is the only 8-byte one in the game). The locals, their order and scopes
are the DWARF's (z is declared and unused there); the fog value is one statement in the original's
line table, hence the fog_param.h helpers.

## Enemy/en_common.c

<a id="en_common-enroomforbiddenarea"></a>
### enRoomForbiddenArea

FAKEMATCH: the float-constant argument order of the enSetForbiddenArea calls comes from the stand-in
above, fitted so that its syntax-tree nodes leave the right flag byte
([docs/toolchain.md](toolchain.md), "Root cause") under each of the 40 constants' parse nodes; its
no-effect `1000.0f;` and its constants' values (zero or not in byte 1 of the double's high word) are
part of the fit. The original had no code there ([docs/stand-ins.md](stand-ins.md)), so this is a
fake match; without the stand-in 7 of the 10 calls load their constants in a different order (57
words).

## Enemy/en_ike.c

<a id="en_ike-file"></a>
### The file (level changes and constant order)

Matching: the order in which enSetSize/enSetNewSize calls load their float constants depends on
memory the compiler leaves over from the previous function ([docs/toolchain.md](toolchain.md), "Root
cause"), so it depends on how neighbouring functions are spelled. The level changes use EN_SET_LEVEL
where the original's DWARF line table has both assignments on one line and the macro matches
(enIKECtrlSwing, enIKECtrlAttack); that also made the stand-in before enIKECtrlAttack unnecessary.

## Enemy/en_nse.c

<a id="en_nse-ennsectrlattack"></a>
### enNSECtrlAttack

FAKEMATCH: the float-constant argument order of the enSetNewSize calls depends on leftovers of
earlier syntax trees ([docs/toolchain.md](toolchain.md), "Root cause"); it matches only with
spellings chosen for it, found by a search over ordinary alternatives, none of them evidenced:
`enCheckDamage(dp) != 0` and the written-out `dp->flag = dp->flag & ~0x400` here, and three in
enNSECtrlChase and enNSECtrlPrecaution (see there). Evidenced: the first two level changes are one
line each in the original's line table (EN_SET_LEVEL, and the two stores on one line); the last is
EN_SET_LEVEL (its do/while nop is in the original's code).

## Enemy/en_pap.c

<a id="en_pap-enpapctrlattack"></a>
### enPAPCtrlAttack

FAKEMATCH: the float-constant order ([docs/toolchain.md](toolchain.md), "Root cause") is fitted, not
recovered: both `enCheckHuggedPlayer() != 0`, the damage check's level changes 6 and 5 as
`dp->slv = N; dp->sslv = 0;`, and the written-out `dp->flag = dp->flag | 0x8000` / `& ~0x8000`; the
code is the same either way. Evidenced: the `?:` operands are unsigned char, as the original's
`daddiu` (not `addiu`) loads of them show; the `} else {` + nested `if (enCheckSpray(dp))` is the
original's line table.

## Enemy/en_red.c

<a id="en_red-enredctrlseize"></a>
### enREDCtrlSeize

FAKEMATCH: the float-constant order of the two enSetNewSize calls comes from compiler leftovers
([docs/toolchain.md](toolchain.md), "Root cause"); it is fitted, not recovered: the `!= 0` tests
here and in enREDCtrlAttack, the `(void)0;` on the line the original leaves without code (656),
`attack_count = attack_count + 1`, `flag = flag | 8`, and enREDCtrlAttack's `sslv += 1` and plain
level change 7. `t =` for enREDCanSeePlayer is the DWARF's.

## Enemy/en_scu.c

<a id="en_scu-enscuctrlcrawl"></a>
### enSCUCtrlCrawl

FAKEMATCH: the float-constant order ([docs/toolchain.md](toolchain.md), "Root cause") is fitted, not
recovered: `enCheckInstantDeath(dp) != 0`, `dp->slv = 11; dp->sslv = 0;` (not EN_SET_LEVEL) in the
damage check and in case 3, and case 3's `enCheckFinishedByHuman(dp) == 0`; the code is the same
either way. Evidenced: the two `} else if (shRandF() < 0.1f)` chains, the one-line hp assignments
and case 2's one-line level changes follow the original's line table.

## Event/chara_admin.c

<a id="chara_admin-connectcharaworkadminout"></a>
### ConnectCharaWorkAdminOut

FAKEMATCH: 8000 + work, the near-check radius, is assigned to near_r (first loop) and near_r2
(CHARA_ADMIN_IS_NEAR's t) where it is first compared, and sqr() squares the local. The DWARF has
neither local. Written as a repeated `8000.0f + work`, the CSE temporary is propagated into sqr()'s
asm operands, so the square goes to a new FPR; the original keeps the sum in a fresh FPR (f6, and f5
in the two later loops, hence two locals) and squares it in place.

## Event/demoview.c

<a id="demoview-ddsreadfloat2"></a>
### DdsReadFloat2

Matching: compiled without the global optimizer (with it on, sig and coe swap registers); the pragma
reproduces the function exactly, as it does the same decoder in event.c (CharToFloat2). MWCC also
compiles a function containing inline asm this way, but nothing in the code shows one. The line
table has each read and its increment on one line, as the `*((char *)adr_dds)++` reads elsewhere in
this file, but under the pragma that spelling compiles differently, so they stay split here. Neither
setting matches with the locals in the DWARF's order (sig, exp, coe, work): sig and coe swap
registers.

## Event/picture.c

<a id="picture-picturedraw"></a>
### PictureDraw

Matching: the ST coordinates and Q are written as float bits through fbits() (asm_helpers.h). Its
mfc1 folds to an integer constant, but as inline asm it compiles the function without the global
optimizer, which the original shows: pic->status reloaded for every test, 0.0f's bits zeroed through
a register (move; nop), Q's bits evaluated first into a temporary and sign-extended in
GS_SET_RGBAQ.

## Event/stage/stg_ovservation.c

<a id="stg_ovservation-game_flag"></a>
### GAME_FLAG

Matching: the flags through a helper rather than written out as word and bit: the code is the same,
but the helper's `n >> 5` / `n & 31` (EvBgmControl's GAME_FLAG(517) is the one that counts) leave
the compiler state that gives OB_DemoBlur its float-constant order
([docs/toolchain.md](toolchain.md), "Root cause"), which a fitted stand-in reproduced before.

## GFW/sh2gfw_2d_filters.c

<a id="sh2gfw_2d_filters-sh2gfw_swap_glowsoft"></a>
### sh2gfw_Swap_GlowSoft

Matching: `i` holds GetTexTBP0(...) >> 5 across the RegFrame[0] copy, as `ty` does in
sh2gfw_Swap_Soft: the original's line table has the same three statements (lines 1575, 1584, 1587)
and its DWARF has an `int i` this function otherwise never uses; with `i` the s-register order of
the parameters and the a0 temporary come out as the original's, where counter, ix, iy, cent or sz as
the temporary (and `i` as the fill-loop index) all reorder them.

<a id="sh2gfw_2d_filters-stripped_double_code"></a>
### STRIPPED_DOUBLE_CODE() before sh2gfw_FadeOut_Retain

Matching: a fitted stand-in for software-double code ([docs/stand-ins.md](stand-ins.md)).
sh2gfw_FadeOut_Retain keeps pfp->TargetSec in a2 (ours took a1): the software-double register mode,
which makes MWCC keep a0/a1 out of the temporaries of the functions after it
([docs/decomp-workflow.md](decomp-workflow.md)). The original's line table leaves 206 lines between
sh2gfw_Swap_GlowSoft (ends 1741) and sh2gfw_FadeOut_Retain (starts 1948), against the file's usual
3-6, so there is room for a stripped function here (the file has three other such gaps: after
sh2gfw_test_MakeNoise, sh2gfw_SendDraw_Noise and sh2gfw_Fade3; placed in either of the earlier two
the stand-in gives the same result). It changes no other function of the unit.

<a id="sh2gfw_2d_filters-sh2gfw_fadeout_retain"></a>
### sh2gfw_FadeOut_Retain

Matching: pfp->TargetSec's temporary is a2 because of the software-double stand-in above (a later
call's argument-register use keeps a0/a1 live through the TargetSec tests). Locals in the DWARF's
order; line 2041 computes faderatio / 8 and discards it (lines 2042-2043 have no code), reproduced
by a branch whose body only assigns the dead faderatio; MWCC deletes a bare `faderatio / 8;` (and
`faderatio /= 8;`). The condition and body are reconstructed, not recovered.

## GFW/sh2gfw_Vertexpacket.c

<a id="sh2gfw_vertexpacket-sh2gfw_geom_makepacket"></a>
### sh2gfw_Geom_MakePacket

Matching: the loop's `curr_vernor += (packsize - 2) * 16` spells next's expression out, so the
optimizer's CSE temporary for it absorbs `next` (the coalesced pair keeps the temporary's lower
number), which is why the original's DWARF gives next no register of its own (v0) and why xgkick is
coloured before it (s3, then reused for (packsize << 17) | 0x6D008004; next s4). With `+= next` at
all three sites next keeps its own number and takes s3 (12 words). The expression is equal to next
there (the else branch has just set it); which of the two in-branch increments was written this way
is not known (either one matches; the first one is the other candidate). The locals are declared in
the order that gives the original's s0-s2; the DWARF's order gives 67 words. Matching: the SPR[2]
words go through qwd (a DWARF local with no register), so the constant pointer stores xgkick through
lui at as the original does. The header copies are Q_WORDDATA assignments (lq through CSE'd address
registers, as in the original). The `rest_vNum < packsize` branch holds a reconstructed dead
statement: the original branches around a then-block with no code (line 258 has no entry);
`vNum = rest_vNum;` (vNum is otherwise unused) stands in for it.

<a id="sh2gfw_vertexpacket-sh2gfw_semitrans_geompacket"></a>
### sh2gfw_SemiTrans_GeomPacket

Matching: the GIF tag's words are written and copied as SPR[3], not through giftag (unused, as in
sh2gfw_Geom_MakePacket): the front end's CSE then keeps the address from its first store
(`ori t3, t0, 0x30; sw t0, 0(t3)`) for the two packet blocks' copies, as the original does. Written
through a pointer assigned before the call, the address is constant-folded at every use (the call
kills the CSE'd value) and the first block forms it again, 4 bytes later, so the loop-test label
needs no `.p2alignl` nop. The loop's `curr_vernor += (packsize - 2) * 16` spells next's expression
out so that the CSE temporary absorbs next (no register of its own in the DWARF: v0), as in
sh2gfw_Geom_MakePacket. The final block's `(unsigned int)rest_vNum` cast is reconstructed from the
DWARF, which gives rest_vNum (s0 throughout) no register of its own either: the front end makes a
temporary for the cast expression (it recurs, implicitly, in `g_pack[1].ui32[1] = rest_vNum`), the
temporary absorbs rest_vNum, and its low number colours rest_vNum before the parameter copies (s0,
then pkhead s1, inf s2, gid s3; without the cast rest_vNum is s3, 22 words). One cast on either `|`
word does it; which one had it is not known.

## Item/otn_itemmain.c

<a id="otn_itemmain-set_position"></a>
### set_position

FAKEMATCH: which constant move_near argument is loaded first (lui/mtc1 order) depends on byte 5 of
the constants' parse nodes, left over in the compiler's arena by item_main_setup
([docs/toolchain.md](toolchain.md), "Root cause"); within a case, repeated constants share the flag
of their first occurrence. How the original's spelling produced its order is unknown. These
spellings are fitted only to move where the nodes land (without them only that order differs):

- every box is a block of its own, one line per box as in the original's line table, except the
  first box of case 8;
- a comma expression joins a box's two assignments in the last box of case 3 and 5/9 and the first
  box of case 8 and 13;
- a redundant (float) cast on the third argument (already a float): case 4 box 1 (y); case 3 box 0
  (x, y), box 2 (y), box 3 (x, y); case 5/9 box 0 (x, y); case 6/7 box 3 (x, y); case 8 box 3 (y);
  case 11 box 0 (x, y).

## Item/otn_option.c

<a id="otn_option-key_conf"></a>
### key_conf

Matching: ~x is written `x ^ 0xFFFFFFFF` at all three places, as the original's `li v1,-1; xor` (the
only such pair in the game) shows. The unsigned constant is above the signed 16-bit range, so the
optimizer keeps it in a temporary when it is used more than once after CSE, and the xor with that
register survives. `~x`, `x ^ -1`, or 0xFFFFFFFF at fewer than all three places give `not` (the code
generator turns an immediate ^ -1 into nor), 5-7 words different. The last statement's xor still
comes out as the original's `not`. `(unsigned)x ^ 0xFFFFFFFFU` compiles the same.

## MC/mc_menu.c

<a id="mc_menu-mcselectdata"></a>
### mcSelectData

Matching: the two int-to-float conversions are itof() (inline asm), which compiles the function
without the global optimizer, as the original's code shows (port/num/base/fmax re-sign-extended at
every use). In the 0x100 (page down) branch, `num = fmax` inside `if (fmax < 5)` reuses the test's
sign-extended fmax and leaves no code, but keeps the then-block, so the original branches around an
empty block; the line table has the test and a statement line with no code there.

<a id="mc_menu-mcdrawslot"></a>
### mcDrawSlot

Matching: the clamp at the end is an if/else whose then-block held a statement the optimizer
removed: the original branches around an empty then-block (slti; beqz; b; addiu), which an empty
`{ }`, `if (fmax >= 5)` or `!(fmax < 5)` doesn't give (and the plain spellings rotate the
s-registers, 53 words), and which mcSelectData's page-down clamp gives the same way. The statement
itself is lost (it shares the test's line; any dead statement compiles to the same code): `n = fmax`
is reconstructed after mcSelectData's `num = fmax`, not recovered.

## SH2_common/pad.c

<a id="pad-shpadset"></a>
### shPadSet

Matching: the `work = ...` statement is a reconstruction. The original's line table has a statement
at the top of the stick-byte copy loop whose only surviving code is the addresses of pad[i][2] and
pad[i][3] (then reused by the last two copies), and the DWARF has a local `work` that nothing else
uses: a dead read of those two bytes into work, removed by the optimizer. What exactly it computed
is lost; any one statement reading pad[i][2] then pad[i][3] into work compiles the same.

## view/vc_main.c

<a id="vc_main-vcmakefarwatchtgtpos"></a>
### vcMakeFarWatchTgtPos

NON_MATCHING: one kept copy: sqr_c(cir_r) squares a copy (mov.s f2,f20; mul.s f2,f2,f20), unique in
the binary; ours folds the copy into mul.s f2,f20,f20. Its 4-byte shift makes the tail differ (116
words). A compiler-build difference: our build folds it at every -O level and under every pragma,
and never keeps a copy with one operand still read from f20; 3.0.1b44/b51 emit the original's
sequence from this very C ([docs/toolchain.md](toolchain.md), "Compiler-version test"). Tried: every
sqr/sqr_c/x*x spelling, inline helpers split by do/while(0), goto, loops or empty asm (even a "+f"
operand), the copy through actual_dist (the DWARF's f2), the permuter (16.5k iterations). The
real_ang_x copy (f0 -> mov.s f12) is the original's two statements (line table).

<a id="vc_main-vcmakefarwatchtgtpos-locals"></a>
### vcMakeFarWatchTgtPos: its locals

Matching: several locals per declaration. The original's line table leaves 7 lines between the
header and the first statement for these 16 locals, so they were grouped; which grouping is unknown.
This one follows the DWARF order. The grouping doesn't decide the float-constant order of
vcAdjustWatchYLimitHighWhenFarView (see DEG2RAD): the six groupings tried all match it, including
three that didn't with the constants written as literals.
