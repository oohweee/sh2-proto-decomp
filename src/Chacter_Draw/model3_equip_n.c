/*
 * Model3 equipment flags (Chacter_Draw): the 128-bit per-model mask that selects which
 * equipment parts (weapons, flashlight...) are drawn.
 */
#include "sh2.h"

/*
 * Matching: the 128-bit equipment flags are handled in inline asm (MWCC C can't operate on
 * u_long128: "illegal data size"). `addi` marks the hand-written part. The locals only
 * appear as asm operands, so their declaration order is whatever gives the original
 * register allocation.
 */
/**
 * Tests one equipment flag of a model work.
 * @param work_ the model's struct ModelWork
 * @param id    flag number, 0-127
 * @return 1 if the flag is set, else 0
 */
int Model3WorkEquipmentFlag(void *work_, int id) {
    struct ModelWork *work;
    u_long128 data;
    unsigned long mask;
    unsigned long result;

    work = work_;
    data = work->equipment_flag;
    asm {
        slti    v0, id, 0x40
        bnez    v0, lbl1
        pcpyud  data, data, data
    lbl1:
        addi    mask, zero, 1
        dsllv   mask, mask, id
        and     result, data, mask
    }
    if (result) {
        return 1;
    }
    return 0;
}

/**
 * Replaces the equipment flags selected by mask: flags = (flags & ~mask) | (flag & mask).
 * @param work_ the model's struct ModelWork
 * @param mask  flags to change
 * @param flag  their new values
 */
/* Matching: locals declared in an order fitted to the asm operands' registers (tmp0 before data); the
 * DWARF's order (work, data, result, tmp0, tmp1) doesn't match (docs/dwarf-fidelity.md). */
/* Original asm: in a C function's body, as here: the original's line table has an entry per asm
 * instruction and the return on the closing brace. */
void Model3WorkSetEquipmentFlagMulti(void *work_, u_long128 mask, u_long128 flag) {
    struct ModelWork *work;
    u_long128 tmp0;
    u_long128 tmp1;
    u_long128 result;
    u_long128 data;

    work = work_;
    asm {
        lq      data, 0x10(work)
        pnor    tmp0, zero, mask
        pand    tmp0, data, tmp0
        pand    tmp1, flag, mask
        por     result, tmp0, tmp1
        sq      result, 0x10(work)
    }
}

/**
 * Sets James's equipment: flag 0 plus the left-hand, right-hand and spotlight parts; clears
 * the rest. Matching: declared int in the DWARF, but no value is returned.
 * @param scp       the character's struct SubCharacterDisp
 * @param lhand     flag number of the left-hand item
 * @param rhand     flag number of the right-hand item
 * @param spotlight flag number of the spotlight part
 */
int sh2gfw_Set_JMSequip(void *scp, int lhand, int rhand, int spotlight) {
    u_long128 flg;
    u_long128 msk;
    struct SubCharacterDisp *scp_d;

    scp_d = scp;
    msk = -1;
    flg = 1 | (1 << lhand) | (1 << rhand) | (1 << spotlight);
    Model3WorkSetEquipmentFlagMulti(scp_d->work, msk, flg);
}
