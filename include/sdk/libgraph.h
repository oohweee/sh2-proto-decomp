#ifndef SDK_LIBGRAPH_H
#define SDK_LIBGRAPH_H

/*
 * libgraph, the EE library for the GS (the binary's "PsIIlibgraph2200" stamp): the functions the
 * game calls, the GS registers it writes through A+D packets and the macros that build their
 * 64-bit values. The library is linked as assembly (no DWARF).
 *
 * Provenance: the function names are the binary's symbols. The parameter and return types are
 * inferred from the game's call sites, and each `short` and `unsigned char` parameter from the
 * library's own code, which sign-extends it from 16 bits (sll/sra by 16) or reads only its low
 * 8 bits (andi 0xFF, or lbu from the stack); the parameter names are ours. The structures
 * (sceGsDBuff, sceGsClear...) have the DWARF's layouts (sh2/types.h). struct GsGraphParam is ours
 * (no DWARF): its layout is what sceGsResetGraph stores, and its fields are named after the
 * parameters and the register field stored there. Register numbers and bit positions are the GS
 * hardware's (public GS documentation: PCSX2, ps2sdk) and agree with the values in the binary.
 * The macro names GS_REG_*, GS_SET_* and GS_PSM_* are the ones ps2sdk's GS headers use (but these
 * don't mask the fields); GS_PRIM_TRISTRIP and GS_FIELD are ours. The macros' parameters are the
 * register fields' names, as in the DWARF's register structs (sh2/types.h) where it has them. No
 * SDK header or other SDK file was used.
 */

#include "sh2/types.h"

/*
 * What sceGsResetGraph(0, ...) stored, as its code shows: its interlace and out_mode arguments,
 * field_mode != 0, and bits 16-23 of the GS CSR register (REV, the GS revision), which the game
 * doesn't read. The library's structure goes on past these 8 bytes; the game reads only these.
 */
struct GsGraphParam {
    short interlace;
    short out_mode;
    short field_mode;
    short csr_rev;
};

struct GsGraphParam *sceGsGetGParam(void);
unsigned long sceGsGetIMR(void);
void sceGsPutDispEnv(struct sceGsDispEnv *disp);
unsigned long sceGsPutIMR(unsigned long imr);
void sceGsResetGraph(short mode, short interlace, short out_mode, short field_mode);
void sceGsResetPath(void);
int sceGsSetDefClear(struct sceGsClear *clear, short ztest, short x, short y, short width, short height,
                     unsigned char r, unsigned char g, unsigned char b, unsigned char a, unsigned int z);
int sceGsSetDefDBuff(sceGsDBuff *dbuf, short psm, short width, short height, short ztest, short zpsm,
                     short clear);
int sceGsSetHalfOffset(sceGsDrawEnv1 *draw, short x, short y, short half);
int sceGsSwapDBuff(sceGsDBuff *dbuf, int which);
int sceGsSyncPath(int mode, int unused); /* the library never reads the second argument */
int sceGsSyncV(int mode);

/* GS register numbers (A+D addresses). */
#define GS_REG_PRIM 0x00
#define GS_REG_RGBAQ 0x01
#define GS_REG_ST 0x02
#define GS_REG_UV 0x03
#define GS_REG_XYZF2 0x04
#define GS_REG_XYZ2 0x05
#define GS_REG_TEX0_1 0x06
#define GS_REG_CLAMP_1 0x08
#define GS_REG_XYZF3 0x0C
#define GS_REG_TEX1_1 0x14
#define GS_REG_XYOFFSET_1 0x18
#define GS_REG_TEXFLUSH 0x3F
#define GS_REG_ALPHA_1 0x42
#define GS_REG_TEST_1 0x47
#define GS_REG_FRAME_1 0x4C
#define GS_REG_ZBUF_1 0x4E
#define GS_REG_FINISH 0x61

/* PRIM's primitive type 4 (triangle strip), and the pixel storage formats 0 (32-bit) and 0x14
   (4-bit indexed). */
#define GS_PRIM_TRISTRIP 4
#define GS_PSM_32 0x00
#define GS_PSM_4 0x14

/*
 * Register values: each field cast to 64 bits and shifted to its bit position, ORed lowest field
 * first. Matching: keep this expression shape (a bare cast for the bit-0 field); another shape can
 * change the float-constant order of the functions using them (docs/toolchain.md, "Root cause").
 */
#define GS_FIELD(v, pos) ((unsigned long)(v) << (pos))

#define GS_SET_ALPHA(a, b, c, d, fix) \
    ((unsigned long)(a) | GS_FIELD(b, 2) | GS_FIELD(c, 4) | GS_FIELD(d, 6) | GS_FIELD(fix, 32))
#define GS_SET_BITBLTBUF(sbp, sbw, spsm, dbp, dbw, dpsm) \
    ((unsigned long)(sbp) | GS_FIELD(sbw, 16) | GS_FIELD(spsm, 24) | GS_FIELD(dbp, 32) | \
     GS_FIELD(dbw, 48) | GS_FIELD(dpsm, 56))
#define GS_SET_CLAMP(wms, wmt, minu, maxu, minv, maxv) \
    ((unsigned long)(wms) | GS_FIELD(wmt, 2) | GS_FIELD(minu, 4) | GS_FIELD(maxu, 14) | \
     GS_FIELD(minv, 24) | GS_FIELD(maxv, 34))
#define GS_SET_FRAME(fbp, fbw, psm, fbmsk) \
    ((unsigned long)(fbp) | GS_FIELD(fbw, 16) | GS_FIELD(psm, 24) | GS_FIELD(fbmsk, 32))
/* CRTMD (bit 2) is always 1. */
#define GS_SET_PMODE(en1, en2, mmod, amod, slbg, alp) \
    ((unsigned long)(en1) | GS_FIELD(en2, 1) | GS_FIELD(1, 2) | GS_FIELD(mmod, 5) | GS_FIELD(amod, 6) | \
     GS_FIELD(slbg, 7) | GS_FIELD(alp, 8))
#define GS_SET_PRIM(prim, iip, tme, fge, abe, aa1, fst, ctxt, fix) \
    ((unsigned long)(prim) | GS_FIELD(iip, 3) | GS_FIELD(tme, 4) | GS_FIELD(fge, 5) | GS_FIELD(abe, 6) | \
     GS_FIELD(aa1, 7) | GS_FIELD(fst, 8) | GS_FIELD(ctxt, 9) | GS_FIELD(fix, 10))
#define GS_SET_RGBAQ(r, g, b, a, q) \
    ((unsigned long)(r) | GS_FIELD(g, 8) | GS_FIELD(b, 16) | GS_FIELD(a, 24) | GS_FIELD(q, 32))
#define GS_SET_SCISSOR(scax0, scax1, scay0, scay1) \
    ((unsigned long)(scax0) | GS_FIELD(scax1, 16) | GS_FIELD(scay0, 32) | GS_FIELD(scay1, 48))
#define GS_SET_SMODE2(int_, ffmd, dpms) ((unsigned long)(int_) | GS_FIELD(ffmd, 1) | GS_FIELD(dpms, 2))
#define GS_SET_ST(s, t) ((unsigned long)(s) | GS_FIELD(t, 32))
#define GS_SET_TEST(ate, atst, aref, afail, date, datm, zte, ztst) \
    ((unsigned long)(ate) | GS_FIELD(atst, 1) | GS_FIELD(aref, 4) | GS_FIELD(afail, 12) | \
     GS_FIELD(date, 14) | GS_FIELD(datm, 15) | GS_FIELD(zte, 16) | GS_FIELD(ztst, 17))
#define GS_SET_TEX0(tbp0, tbw, psm, tw, th, tcc, tfx, cbp, cpsm, csm, csa, cld) \
    ((unsigned long)(tbp0) | GS_FIELD(tbw, 14) | GS_FIELD(psm, 20) | GS_FIELD(tw, 26) | GS_FIELD(th, 30) | \
     GS_FIELD(tcc, 34) | GS_FIELD(tfx, 35) | GS_FIELD(cbp, 37) | GS_FIELD(cpsm, 51) | GS_FIELD(csm, 55) | \
     GS_FIELD(csa, 56) | GS_FIELD(cld, 61))
#define GS_SET_TEX1(lcm, mxl, mmag, mmin, mtba, l, k) \
    ((unsigned long)(lcm) | GS_FIELD(mxl, 2) | GS_FIELD(mmag, 5) | GS_FIELD(mmin, 6) | GS_FIELD(mtba, 9) | \
     GS_FIELD(l, 19) | GS_FIELD(k, 32))
#define GS_SET_UV(u, v) ((unsigned long)(u) | GS_FIELD(v, 16))
#define GS_SET_XYOFFSET(ofx, ofy) ((unsigned long)(ofx) | GS_FIELD(ofy, 32))
#define GS_SET_XYZ(x, y, z) ((unsigned long)(x) | GS_FIELD(y, 16) | GS_FIELD(z, 32))
#define GS_SET_XYZF(x, y, z, f) ((unsigned long)(x) | GS_FIELD(y, 16) | GS_FIELD(z, 32) | GS_FIELD(f, 56))
#define GS_SET_ZBUF(zbp, psm, zmsk) ((unsigned long)(zbp) | GS_FIELD(psm, 24) | GS_FIELD(zmsk, 32))

#endif
