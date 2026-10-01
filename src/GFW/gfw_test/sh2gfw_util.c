/*
 * Polygon scissoring against the view volume (GFW test code, used by the game): builds GS
 * packets for triangle lists/strips and, for triangles that cross the clip volume, clips them
 * against its six planes on VU0 and emits the clipped polygons as a second packet.
 */
#include "sh2.h"
#include "gfw_helpers.h"
#include "sdk/libvu0.h"

static float world_view[4][4];
static float world_screen[4][4];
static float view_clip[4][4];
static float world_clip[4][4];

/**
 * Transforms vertex into clip space and returns the VU0 clip flags (vclipw; the .word is
 * vclipw.xyz vf12, vf12w, which the assembler doesn't know).
 * @param clip       receives the clip-space vertex
 * @param local_clip local-to-clip matrix
 * @param vertex     the vertex
 * @return the clip flag register (lowest 6 bits: this vertex)
 */
/* Original asm: in a C function's body, as here: the original's line table has an entry per asm
 * instruction and the return on the closing brace. */
int sceVu0ViewVolumeClip(float *clip, float (*local_clip)[4], float *vertex) {
    int ret;

    __asm__ __volatile__("
    lqc2         vf4, 0x0(%2)
    lqc2         vf5, 0x10(%2)
    lqc2         vf6, 0x20(%2)
    lqc2         vf7, 0x30(%2)
    lqc2         vf8, 0x0(%3)
    vmulax.xyzw  ACC, vf4, vf8x
    vmadday.xyzw ACC, vf5, vf8y
    vmaddaz.xyzw ACC, vf6, vf8z
    vmaddw.xyzw  vf12, vf7, vf8w
    .word        0x4BCC61FF
    vnop
    vnop
    vnop
    sqc2         vf12, 0x0(%1)
    vnop
    cfc2         %0, vi18
    " : "=r"(ret) : "r"(clip), "r"(local_clip), "r"(vertex));
    return ret;
}

/** Copies node nod1 to nod0. */
void CopyNode(struct ScissorNode *nod0, struct ScissorNode *nod1) {
    sceVu0CopyVector(nod0->vertex, nod1->vertex);
    sceVu0CopyVector(nod0->normal, nod1->normal);
    sceVu0CopyVector(nod0->color, nod1->color);
    sceVu0CopyVector(nod0->texUV, nod1->texUV);
    sceVu0CopyVector(nod0->clipV, nod1->clipV);
}

/** Resets a scissor work area: empty in/out arrays, triangle rotation at 0. */
void InitNodeArraySet(struct ScissorNodeArraySet *scissorflip) {
    scissorflip->rotflag = 0;
    scissorflip->flipflag = 0;
    scissorflip->in = &scissorflip->narray[scissorflip->flipflag];
    scissorflip->out = &scissorflip->narray[!scissorflip->flipflag];
    scissorflip->in->nodeNum = 0;
    scissorflip->out->nodeNum = 0;
    scissorflip->triangle.nodeNum = 3;
}

/** Empties the in/out node arrays of a scissor work area (the triangle is kept). */
void ResetNodeArraySet(struct ScissorNodeArraySet *scissorflip) {
    scissorflip->flipflag = 0;
    scissorflip->in = &scissorflip->narray[scissorflip->flipflag];
    scissorflip->out = &scissorflip->narray[!scissorflip->flipflag];
    scissorflip->in->nodeNum = 0;
    scissorflip->out->nodeNum = 0;
}

/** Swaps the in and out node arrays and empties the new out array. */
void FlipNodeArray(struct ScissorNodeArraySet *scissorflip) {
    scissorflip->flipflag = !scissorflip->flipflag;
    scissorflip->in = &scissorflip->narray[scissorflip->flipflag];
    scissorflip->out = &scissorflip->narray[!scissorflip->flipflag];
    scissorflip->out->nodeNum = 0;
}

/**
 * Adds a vertex to the three-vertex triangle window (replacing the oldest).
 * @param mode non-zero for flat shading: nod's normal and color go to all three vertices
 */
void PushTriangleNodeArray(struct ScissorNodeArraySet *scissorflip, struct ScissorNode *nod, int mode) {
    if (mode) {
        sceVu0CopyVector(scissorflip->triangle.node[0].normal, nod->normal);
        sceVu0CopyVector(scissorflip->triangle.node[1].normal, nod->normal);
        sceVu0CopyVector(scissorflip->triangle.node[2].normal, nod->normal);
        sceVu0CopyVector(scissorflip->triangle.node[0].color, nod->color);
        sceVu0CopyVector(scissorflip->triangle.node[1].color, nod->color);
        sceVu0CopyVector(scissorflip->triangle.node[2].color, nod->color);
    }
    CopyNode(&scissorflip->triangle.node[scissorflip->rotflag++], nod);
    scissorflip->rotflag = scissorflip->rotflag % 3;
}

/** Sets up the six clip planes (+-x, +-y, +-z): the clip-flag mask and component/sign of each. */
void InitScissorPlane(struct ScissorPlaneSet *sp) {
    sp->planeNum = 6;
    sp->plane[0].clipmask = 0x820;
    sp->plane[0].xyzflag = 0x12;
    sp->plane[1].clipmask = 0x410;
    sp->plane[1].xyzflag = 0x2;
    sp->plane[2].clipmask = 0x208;
    sp->plane[2].xyzflag = 0x11;
    sp->plane[3].clipmask = 0x104;
    sp->plane[3].xyzflag = 0x1;
    sp->plane[4].clipmask = 0x82;
    sp->plane[4].xyzflag = 0x10;
    sp->plane[5].clipmask = 0x41;
    sp->plane[5].xyzflag = 0x0;
}

/**
 * Returns the VU0 clip flags of the edge a-b (the .words are vclipw.xyz of each vertex's clip
 * position; bits 0-5 are b's, 6-11 a's).
 */
/* Original asm: in a C function's body, as here: the original's line table has an entry per asm
 * instruction and the return on the closing brace, and its DWARF has a, b and the local ret. */
int ClipCheck(struct ScissorNode *a, struct ScissorNode *b) {
    int ret;

    asm {
        .set noreorder
        lqc2         vf10, 0x40(a)
        lqc2         vf11, 0x40(b)
        .word        0x4BCA51FF
        .word        0x4BCB59FF
        vnop
        vnop
        vnop
        vnop
        vnop
        cfc2.ni      ret, vi18
        .set reorder
    }
    return ret;
}

/**
 * Computes the node where the edge from inside to outside crosses a clip plane.
 * @param inter   receives the interpolated node
 * @param inside  the vertex inside the plane
 * @param outside the vertex outside
 * @param sgn     the plane's sign (+1 or -1: x = w or x = -w)
 * @param xyzflag the clipped component (0 x, 1 y, 2 z)
 */
/* Original asm: in a C function's body, as here: the original's line table has an entry per asm
 * instruction and the return on the closing brace, and its DWARF has the five parameters. */
void InterpNode(struct ScissorNode *inter, struct ScissorNode *inside, struct ScissorNode *outside, float sgn, int xyzflag) {
    asm {
        .set noreorder
        lqc2         vf10, 0x40(inside)
        lqc2         vf11, 0x40(outside)
        lqc2         vf21, 0x0(inside)
        lqc2         vf22, 0x10(inside)
        lqc2         vf23, 0x20(inside)
        lqc2         vf24, 0x30(inside)
        lqc2         vf25, 0x0(outside)
        lqc2         vf26, 0x10(outside)
        lqc2         vf27, 0x20(outside)
        lqc2         vf28, 0x30(outside)
        vsub.xyzw    vf15, vf11, vf10
        vsub.xyzw    vf25, vf25, vf21
        vsub.xyzw    vf26, vf26, vf22
        vsub.xyzw    vf27, vf27, vf23
        vsub.xyzw    vf28, vf28, vf24
        mfc1         t0, sgn
        nop
        qmtc2.ni     t0, vf9
        vmulx.w      vf12, vf10, vf9x
        vmulx.w      vf13, vf11, vf9x
        vsubw.xyzw   vf16, vf10, vf12w
        vsubw.xyzw   vf17, vf11, vf13w
        vabs.xyzw    vf16, vf16
        vabs.xyzw    vf17, vf17
        qmfc2.ni     t1, vf16
        qmfc2.ni     t2, vf17
    @rot:
        beqz         xyzflag, @done
        nop
        prot3w       t1, t1
        prot3w       t2, t2
        addi         xyzflag, xyzflag, -0x1
        j            @rot
        nop
    @done:
        qmtc2.ni     t1, vf16
        qmtc2.ni     t2, vf17
        vadd.xyz     vf18, vf17, vf16
        vdiv         Q, vf16x, vf18x
        vwaitq
        vaddq.xyzw   vf20, vf0, Q
        vabs.x       vf20, vf20
        vmulx.xyzw   vf15, vf15, vf20x
        vmulx.xyzw   vf25, vf25, vf20x
        vmulx.xyzw   vf26, vf26, vf20x
        vmulx.xyzw   vf27, vf27, vf20x
        vmulx.xyzw   vf28, vf28, vf20x
        vadd.xyzw    vf19, vf10, vf15
        vadd.xyzw    vf25, vf25, vf21
        vadd.xyzw    vf26, vf26, vf22
        vadd.xyzw    vf27, vf27, vf23
        vadd.xyzw    vf28, vf28, vf24
        sqc2         vf25, 0x0(inter)
        sqc2         vf26, 0x10(inter)
        sqc2         vf27, 0x20(inter)
        sqc2         vf28, 0x30(inter)
        sqc2         vf19, 0x40(inter)
        .set reorder
    }
}

/**
 * Clips the current triangle against every plane of planeset (Sutherland-Hodgman); the
 * resulting polygon is left in scissorflip->in.
 */
void ScissorTriangle(struct ScissorNodeArraySet *scissorflip, struct ScissorPlaneSet *planeset) {
    int i;
    int j;
    int clip;
    int mask;
    int xyz;
    float sgn;
    struct ScissorPlane *plane;
    struct ScissorNodeArray *inarray;
    struct ScissorNodeArray *outarray;
    struct ScissorNode *currN;
    struct ScissorNode *nextN;
    struct ScissorNode interN;

    inarray = &scissorflip->triangle;
    outarray = scissorflip->out;
    for (i = 0; i < planeset->planeNum; i++) {
        plane = &planeset->plane[i];
        sgn = (plane->xyzflag & 0x10) ? -1.0 : 1.0;
        mask = plane->clipmask;
        xyz = plane->xyzflag & 0xF;
        for (j = 0; j < inarray->nodeNum; j++) {
            currN = &inarray->node[j];
            nextN = &inarray->node[(j + 1) % inarray->nodeNum];
            clip = mask & ClipCheck(currN, nextN);
            if (clip == 0) {
                CopyNode(&outarray->node[outarray->nodeNum++], currN);
            } else if ((clip & 0x3F) && (clip & 0xFC0)) {
            } else if ((clip & 0x3F) && !(clip & 0xFC0)) {
                InterpNode(&interN, currN, nextN, sgn, xyz);
                CopyNode(&outarray->node[outarray->nodeNum++], currN);
                CopyNode(&outarray->node[outarray->nodeNum++], &interN);
            } else if (!(clip & 0x3F) && (clip & 0xFC0)) {
                InterpNode(&interN, nextN, currN, sgn, xyz);
                CopyNode(&outarray->node[outarray->nodeNum++], &interN);
            }
        }
        FlipNodeArray(scissorflip);
        inarray = scissorflip->in;
        outarray = scissorflip->out;
    }
}

/**
 * Writes the clipped polygon in scissorflip->in as a GIF packet (a triangle fan, or a line
 * strip when flag is set), if it has any vertices.
 * @param Prim  the GS PRIM value of the source primitive
 * @param flag  0: triangle fan; else line strip
 * @param ppqwd write pointer, advanced past the packet
 */
void MakeScissorPolygon(struct ScissorNodeArraySet *scissorflip, int Prim, int flag, union Q_WORDDATA **ppqwd) {
    int j;
    int outsize;
    float Q;
    struct ScissorNodeArray *in;
    int v01[4];
    int c01[4];
    float tex[4];
    union Q_WORDDATA *qwd;

    outsize = 0;
    in = scissorflip->in;
    qwd = *ppqwd;
    if (flag == 0) {
        Prim = (Prim & 0xFFF8) | 5;
    } else {
        Prim = (Prim & 0xFFE8) | 2;
    }
    outsize++;
    for (j = 0; j < in->nodeNum; j++) {
        sceVu0RotTransPers(v01, world_screen, in->node[j].vertex, 1);
        sceVu0ApplyMatrix(tex, world_screen, in->node[j].vertex);
        Q = 1.0 / tex[3];
        sceVu0ScaleVector(tex, in->node[j].texUV, 1.0 / tex[3]);
        sceVu0FTOI0Vector(c01, in->node[j].color);
        CopyQword(&qwd[outsize], tex);
        CopyQword(&qwd[outsize + 1], c01);
        CopyQword(&qwd[outsize + 2], v01);
        outsize += 3;
    }
    if (outsize > 1) {
        qwd[0].ul64[1] = 0x512;
        qwd[0].ul64[0] = (unsigned long)in->nodeNum | ((unsigned long)1 << 15) | ((unsigned long)1 << 46) | ((unsigned long)Prim << 47) | ((unsigned long)0 << 58) | ((unsigned long)3 << 60);
        *ppqwd = qwd + outsize;
    } else {
        *ppqwd = qwd;
    }
}

/**
 * Builds the GS packet of a primitive with per-vertex STQ, color and XYZ, and a second packet
 * of the clipped polygons for the triangles that cross the view volume (those vertices get
 * the ADC bit in the first packet).
 * @param DataBuf   vertex data: position, normal, st and color quadwords per vertex
 * @param OutputBuf write pointer of the main packet, advanced
 * @param SciBuf    write pointer of the clipped-polygon packet, advanced (by 0 if none)
 * @param flag      passed to MakeScissorPolygon
 * @param Prim      GS PRIM value
 * @param VertexNum number of vertices
 */
void MakeScissorPacket(void *DataBuf, void **OutputBuf, void **SciBuf, int flag, int Prim, int VertexNum) {
    int j;
    int outsize;
    unsigned int clipflag;
    float Q;
    struct ScissorNode node;
    union Q_WORDDATA *qwd;
    union Q_WORDDATA *outqwd;
    Q_WORDDATA *sciqwd;
    Q_WORDDATA *scitop;
    sceVu0IVECTOR Vertex01;
    sceVu0IVECTOR Color01;
    sceVu0FVECTOR tex;
    sceVu0FVECTOR clip;
    sceVu0FVECTOR GScolor;
    sceVu0FVECTOR *vertex;
    sceVu0FVECTOR *normal;
    sceVu0FVECTOR *st;
    sceVu0FVECTOR *color;
    struct ScissorNodeArraySet *scissorflip;
    struct ScissorPlaneSet *planeset;
    int k;

    outsize = 0;
    clipflag = 0;
    outqwd = *OutputBuf;
    sciqwd = *SciBuf;
    scitop = *SciBuf;
    vertex = (float (*)[4])DataBuf;
    normal = (float (*)[4])DataBuf + 1;
    st = (float (*)[4])DataBuf + 2;
    color = (float (*)[4])DataBuf + 3;
    sceVu0CopyMatrix(world_view, VbWvsMatrix.wvm);
    sceVu0CopyMatrix(world_screen, VbWvsMatrix.wsm);
    sceVu0CopyMatrix(view_clip, cam0.view_clip);
    sceVu0MulMatrix(world_clip, view_clip, world_view);
    scissorflip = (struct ScissorNodeArraySet *)0x70000000;
    planeset = (struct ScissorPlaneSet *)(scissorflip + 1);
    InitNodeArraySet(scissorflip);
    InitScissorPlane(planeset);
    outsize += 2;
    sciqwd++;
    for (j = 0; j < VertexNum; j++) {
        clipflag |= sceVu0ViewVolumeClip(clip, world_clip, vertex[j * 4]) & 0x3F;
        sceVu0RotTransPers(Vertex01, world_screen, vertex[j * 4], 1);
        sceVu0CopyVector(GScolor, color[j * 4]);
        sceVu0FTOI0Vector(Color01, GScolor);
        sceVu0CopyVector(node.vertex, vertex[j * 4]);
        sceVu0CopyVector(node.normal, normal[j * 4]);
        sceVu0CopyVector(node.color, GScolor);
        sceVu0CopyVector(node.texUV, st[j * 4]);
        sceVu0CopyVector(node.clipV, clip);
        if (Prim & 8) {
            PushTriangleNodeArray(scissorflip, &node, 0);
        } else {
            PushTriangleNodeArray(scissorflip, &node, 1);
        }
        if (clipflag & 0x3FFFF) {
            if (((Prim & 4) && j > 1) || ((Prim & 3) && (j + 1) % 3 == 0)) {
                Vertex01[3] |= 0x8000;
                ScissorTriangle(scissorflip, planeset);
                MakeScissorPolygon(scissorflip, Prim, flag, &sciqwd);
                ResetNodeArraySet(scissorflip);
            }
        }
        clipflag <<= 6;
        sceVu0ApplyMatrix(tex, world_screen, vertex[j * 4]);
        Q = 1.0 / tex[3];
        sceVu0ScaleVector(tex, st[j * 4], 1.0 / tex[3]);
        CopyQword(&outqwd[outsize], tex);
        CopyQword(&outqwd[outsize + 1], Color01);
        CopyQword(&outqwd[outsize + 2], Vertex01);
        outsize += 3;
    }
    outqwd[0].ui32[0] = (outsize - 1) | 0x10000000;
    outqwd[0].ui32[1] = 0;
    outqwd[0].ui32[2] = 0;
    outqwd[0].ui32[3] = (outsize - 1) | 0x50000000;
    outqwd[1].ul64[1] = 0x512;
    outqwd[1].ul64[0] = (unsigned long)VertexNum | ((unsigned long)1 << 15) | ((unsigned long)1 << 46) | ((unsigned long)Prim << 47) | ((unsigned long)0 << 58) | ((unsigned long)3 << 60);
    j = ((int)sciqwd - (int)scitop) >> 4;
    if (j <= 3) {
        j = 0;
        scitop->ui32[0] = 0x10000000;
        scitop->ui32[1] = 0;
        scitop->ui32[2] = 0;
        scitop->ui32[3] = 0;
    } else {
        scitop->ui32[0] = (j - 1) | 0x10000000;
        scitop->ui32[1] = 0;
        scitop->ui32[2] = 0;
        scitop->ui32[3] = (j - 1) | 0x50000000;
    }
    *(union Q_WORDDATA **)OutputBuf += outsize;
    *(union Q_WORDDATA **)SciBuf += j;
}

/** Clears qsize quadwords at qwd. */
void sh2gfw_util_zeroq(union Q_WORDDATA *qwd, int qsize) {
    u_long128 zero128;

    zero128 = 0;
    while (qsize > 0) {
        qsize--;
        qwd[qsize].ul128 = zero128;
    }
}
