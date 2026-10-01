/*
 * Item models: items shown on the item screen, and pickup items placed in the world (static
 * models drawn at a per-model scale).
 */

#include "sh2.h"
#include "fi_libvu0_inline.h"

/* Like sceVu0RotMatrix: rotate by z, then y, then x. Local: not one of the shared helpers. */
static inline void shRotMatrix(float (*m0)[4], float (*m1)[4], float *rot) {
    shRotMatrixZ(m0, m1, rot[2]);
    shRotMatrixY(m0, m0, rot[1]);
    shRotMatrixX(m0, m0, rot[0]);
}

static struct shItemScreenObjectSettingData item_screen_obj_data[58] = {
    { 0x748, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x600, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x601, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x602, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x603, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x604, 1.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x605, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x606, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x607, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x608, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x609, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x60A, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x60B, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x60C, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x60D, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x60E, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x60F, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x610, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x611, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x612, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x613, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x614, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x615, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x616, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x617, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x618, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x619, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x61A, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x61B, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x61C, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x61D, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x61E, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x61F, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x620, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x621, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x622, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x623, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x624, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x625, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x626, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x627, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x628, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x629, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x62A, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0x62B, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
    { 0, 10.0f, { 0.0f, 0.0f, 1.5707964f, 0.0f } },
};

static int shCharacterItemScreenItemInit(struct SubCharacter *scp) {
    SCStayModelSwitch(scp, 1);
    return 0;
}

static int shCharacterWorldScreenItemInit(struct SubCharacter *scp) {
    SCStayModelSwitch(scp, 1);
    return 0;
}

static void ItemScreenItemFunction(struct SubCharacter *this) {
    int i1;

    switch (this->step) {
    case 0:
        shCharacterItemScreenItemInit(this);
        for (i1 = 0; item_screen_obj_data[i1].chara_id != 0; i1++) {
            if (this->kind == item_screen_obj_data[i1].chara_id) {
                shCharacterItemScreenObjectSet(this, &item_screen_obj_data[i1]);
                _sceVu0UnitMatrix(this->mat.d);
                break;
            }
        }
        this->step++;
    case 1:
        break;
    }
}

static void WorldScreenItemFunction(struct SubCharacter *this) {
    float scale;

    switch (this->step) {
    case 0:
        shCharacterWorldScreenItemInit(this);
        switch (shCharacterGetModelID(this)) {
        case 0x706:
            scale = 2.0f;
            break;
        case 0x707:
            scale = 2.0f;
            break;
        case 0x709:
            scale = 2.0f;
            break;
        case 0x70A:
            scale = 2.0f;
            break;
        case 0x72C:
            scale = 1.5f;
            break;
        case 0x72F:
            scale = 1.5f;
            break;
        case 0x732:
            scale = 1.5f;
            break;
        case 0x730:
            scale = 1.5f;
            break;
        case 0x71C:
            scale = 1.5f;
            break;
        case 0x736:
            scale = 1.5f;
            break;
        case 0x73D:
            scale = 1.5f;
            break;
        case 0x73E:
            scale = 1.5f;
            break;
        case 0x747:
            scale = 1.5f;
            break;
        case 0x735:
            scale = 1.5f;
            break;
        case 0x73F:
            scale = 2.0f;
            break;
        default:
            scale = 1.0f;
            break;
        }
        shCharacterWorldScreenObjectSetNew(this, scale);
        this->pos_spd.x = this->pos.x;
        this->pos_spd.z = this->pos.z;
        this->eye_y = this->center_y = this->pos.y - 100.0f;
        switch (this->kind) {
        case 0x72F:
            this->pos_spd.z -= 125.0f;
            break;
        }
        this->step++;
    case 1:
        break;
    }
}

static void getRmat(float (*mat)[4]) {
    float rot[4][4] = {
        { 0.0f, 3.1415927f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f },
    };

    _sceVu0UnitMatrix(mat);
    shRotMatrix(mat, mat, (float *)rot);
    _sceVu0TransposeMatrix(mat, mat);
}

/**
 * Sets up a static model placed in the world: bakes the model's bone matrices, turned by the
 * inverse of a PI rotation about y, into its skeleton, then applies @p scale.
 * @param this the character. @param scale uniform scale.
 */
void shCharacterWorldScreenObjectSetNew(struct SubCharacter *this, float scale) {
    struct SubCharacterDisp *scp_d;
    struct shSkelton *sp;
    struct ModelWork *work;
    int i;
    float rm[4][4];

    scp_d = (struct SubCharacterDisp *)this;
    sp = scp_d->anime.top;
    work = scp_d->work;
    getRmat(rm);
    SCSetRot(this, &this->rot);
    for (i = 0; sp != NULL; sp = sp->next, i++) {
        sp->src_m = *(struct FMAT *)work->matrices[i];
        shMulMatrix((float (*)[4])&sp->src_m, rm, (float (*)[4])&sp->src_m);
        sp->des_m = sp->src_m;
        sp->src_t = *(struct FVEC *)sp->src_m.d[3];
        sp->des_t = sp->src_t;
    }
    shCharacterStayObjectScaleSet(this, scale);
}

/** Installs the item-screen item update function. @param scp the character. */
void shCharacterSetItemScreenItemLow(struct SubCharacter *scp) {
    shCharacterSetFunction(scp, ItemScreenItemFunction);
}

/** Installs the world item update function. @param scp the character. */
void shCharacterSetWorldScreenItemLow(struct SubCharacter *scp) {
    shCharacterSetFunction(scp, WorldScreenItemFunction);
}
