/*
 * James's weapon models (0x801-0x808): the weapon character's animation, attaching it to the
 * player's hand, and the weapon's start/end points for hit checks.
 */

#include "sh2.h"
#include "asm_helpers.h"
#include "sh_vu0.h"
#include "sdk/libvu0.h"

static const struct _AnimeInfo weapon_anim[9] = {
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0 },
    { 0x1B59, 0x0001, 0x0000, 0x0000, 0x0000, 0 },
    { 0x1B5A, 0x0001, 0x0000, 0x0001, 0x0001, 0 },
    { 0x1B5B, 0x0001, 0x0000, 0x0002, 0x0002, 0 },
    { 0x1B5C, 0x0001, 0x0000, 0x0003, 0x0003, 0 },
    { 0x1B5D, 0x0001, 0x0000, 0x0004, 0x0004, 0 },
    { 0x1B5E, 0x0001, 0x0000, 0x0005, 0x0005, 0 },
    { 0x1B5F, 0x0001, 0x0000, 0x0006, 0x0006, 0 },
    { 0x1B60, 0x0001, 0x0000, 0x0007, 0x0007, 0 },
};

static void WeaponFunction(struct SubCharacter *this) {
    struct _AnimeInfo *aip;
    float scale;

    switch (this->step) {
    case 0:
        SCAnimeTypeSwitch(this, 0);
        switch (shCharacterGetModelID(this)) {
        case 0x801:
            aip = (struct _AnimeInfo *)&weapon_anim[1];
            break;
        case 0x802:
            aip = (struct _AnimeInfo *)&weapon_anim[2];
            break;
        case 0x803:
            aip = (struct _AnimeInfo *)&weapon_anim[3];
            break;
        case 0x805:
            aip = (struct _AnimeInfo *)&weapon_anim[5];
            break;
        case 0x806:
            aip = (struct _AnimeInfo *)&weapon_anim[6];
            break;
        case 0x804:
            aip = (struct _AnimeInfo *)&weapon_anim[4];
            break;
        case 0x807:
            aip = (struct _AnimeInfo *)&weapon_anim[8];
            break;
        case 0x808:
            aip = (struct _AnimeInfo *)&weapon_anim[7];
            break;
        }
        shCharacterAnimeSet(this, 0, 0, aip, (int)shCharacterGetAnimeAdrForPlay(this));
        this->step++;
    case 1:
        break;
    }
}

/**
 * Puts the weapon on the player's hand bone (part 0x23) after the player's animation ran.
 * @param this the weapon character (may be NULL). @param kind kind of the character holding it.
 */
void shUpdateWeaponMatrixAfterAnime(struct SubCharacter *this, short kind) {
    struct FMAT mat;
    struct SubCharacter *player;

    if (this != NULL) {
        GetPlayerPartsLocalMatrix(&mat, 0x23);
        this->sk_top->src_m = mat;
        this->sk_top->src_t = *(struct FVEC *)mat.d[3];
        if ((player = shCharacterGetSubCharacter(kind, -1)) != NULL) {
            this->mat = player->mat;
            vcopy_dst_first(&this->pos, this->mat.d[3]);
        }
    }
}

static void shGetJamesWeaponPos(float *pos, float *vec, int kind) {
    float pos0[4];
    float pos1[4];
    float vec0[4];
    struct FMAT lw_mat;
    struct FMAT mat;
    unsigned char weapon;
    struct SubCharacter *scp;
    struct shSkelton *stp;
    float wep_range_test[9][4] = {
        { 0.0f, 0.0f, 0.0f, 0.0f },
        { 500.0f, 0.0f, 0.0f, 0.0f },
        { 500.0f, 0.0f, 0.0f, 0.0f },
        { 500.0f, 0.0f, 0.0f, 0.0f },
        { 500.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 500.0f, 0.0f, 0.0f },
        { 0.0f, 500.0f, 0.0f, 0.0f },
        { 0.0f, 500.0f, 0.0f, 0.0f },
        { 0.0f, 500.0f, 0.0f, 0.0f },
    };

    weapon = PlayerGetJamesWeapon();
    if ((scp = shCharacterGetSubCharacter(weapon + 0x800, -1)) != NULL) {
        lw_mat = scp->mat;
        stp = scp->sk_top;
        switch (kind) {
        case 0:
            mat = stp->src_m;
            vcopy_dst_first(pos0, stp->src_m.d[3]);
            break;
        case 1:
            mat = stp->next->src_m;
            vcopy_dst_first(pos0, stp->next->src_m.d[3]);
            break;
        }
        mat.d[3][0] = mat.d[3][1] = mat.d[3][2] = 0.0f;
        mat.d[3][3] = 1.0f;
        sceVu0ApplyMatrix(pos0, (float (*)[4])&lw_mat, pos0);
        vcopy_dst_first(pos, pos0);
        switch (kind) {
        case 0:
            mat = stp->next->src_m;
            vcopy_dst_first(pos1, stp->next->src_m.d[3]);
            mat.d[3][0] = mat.d[3][1] = mat.d[3][2] = 0.0f;
            mat.d[3][3] = 1.0f;
            break;
        case 1:
            sceVu0CopyVector(pos1, wep_range_test[sh2jms.weapon]);
            sceVu0ApplyMatrix(pos1, (float (*)[4])&mat, pos1);
            pos1[0] += stp->next->src_m.d[3][0];
            pos1[1] += stp->next->src_m.d[3][1];
            pos1[2] += stp->next->src_m.d[3][2];
            pos1[3] = stp->next->src_m.d[3][3];
            break;
        }
        sceVu0ApplyMatrix(pos1, (float (*)[4])&lw_mat, pos1);
        _shSubVector(vec0, pos1, pos0);
        _shNormalize(vec, vec0);
    }
}

/** Gets the current weapon's start point and direction. @param pos out: position. @param vec out: unit direction. */
void shGetJamesWeaponStartPos(float *pos, float *vec) {
    shGetJamesWeaponPos(pos, vec, 0);
}

/** Gets the current weapon's end point and direction. @param pos out: position. @param vec out: unit direction. */
void shGetJamesWeaponEndPos(float *pos, float *vec) {
    shGetJamesWeaponPos(pos, vec, 1);
}

/** Installs the weapon update function on a sub-character. @param scp the character. */
void shCharacterSetWeaponLow(struct SubCharacter *scp) {
    shCharacterSetFunction(scp, WeaponFunction);
}
