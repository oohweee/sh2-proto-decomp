/*
 * ONI enemy character (model 0x209): animation tables, the sub-character update function
 * that registers the enemy AI (enEntryEnemy/enInitData), animation setters, and the
 * attack-position query used by the battle code.
 * Pyramid Head, the spear pair at the end of the hotel (verified; the hospital chase is suspected; docs/characters.md).
 */

#include "sh2.h"
#include "asm_helpers.h"
#include "sh_vu0.h"
#include "sdk/libvu0.h"

static int doni_anime_adr_list[10] = { 0, 0, 0, 0x9D40, 0, 0x163F8, 0, 0x28458, 0x4A60C, 0xBC24 };

static const struct _AnimeInfo oni_anim[16] = {
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0 },
    { 0x1519, 0x0012, 0x0800, 0x0000, 0x0011, 1 },
    { 0x151A, 0x002E, 0x0800, 0x0012, 0x003F, 1 },
    { 0x151B, 0x0027, 0x0800, 0x0040, 0x0066, 0 },
    { 0x151C, 0x0019, 0x0800, 0x0067, 0x007F, 0 },
    { 0x151D, 0x0016, 0x0800, 0x0080, 0x0095, 0 },
    { 0x151E, 0x0016, 0x0800, 0x0096, 0x00AB, 0 },
    { 0x151F, 0x001D, 0x0800, 0x00AC, 0x00C8, 0 },
    { 0x1520, 0x0016, 0x0800, 0x00C9, 0x00DE, 0 },
    { 0x1521, 0x000D, 0x0800, 0x00DF, 0x00EB, 0 },
    { 0x1522, 0x001D, 0x0800, 0x00EC, 0x0108, 1 },
    { 0x1523, 0x000D, 0x0800, 0x0109, 0x0115, 0 },
    { 0x1524, 0x001D, 0x0800, 0x0116, 0x0132, 0 },
    { 0x1525, 0x001D, 0x0800, 0x0133, 0x014F, 0 },
    { 0x1526, 0x0016, 0x0800, 0x0150, 0x0165, 0 },
    { 0x1527, 0x0002, 0x0800, 0x0166, 0x0167, 0 },
};

static const struct _AnimeInfo d_oni_anim[10] = {
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0 },
    { 0x18CF, 0x0001, 0x0800, 0x0000, 0x0000, 1 },
    { 0x18D0, 0x031A, 0x0800, 0x0000, 0x0319, 0 },
    { 0x18D1, 0x02CC, 0x0800, 0x0000, 0x02CB, 0 },
    { 0x18D2, 0x0686, 0x0800, 0x0000, 0x0685, 0 },
    { 0x18D3, 0x0303, 0x0800, 0x0000, 0x0302, 0 },
    { 0x18D4, 0x00AA, 0x0800, 0x0000, 0x00A9, 0 },
    { 0x18D5, 0x0686, 0x0800, 0x0000, 0x0685, 0 },
    { 0x18D6, 0x0303, 0x0800, 0x0000, 0x0302, 0 },
    { 0x18D7, 0x00AA, 0x0800, 0x0000, 0x00A9, 0 },
};

/*
 * Matching: the DWARF drops this unused parameter. Passing `this` keeps it live in a0 up
 * to the call (MWCC knows this static callee clobbers nothing else), so the switch on
 * this->step is loaded into a1.
 */
static int EnemyONIInit(struct SubCharacter *scp) {
    return 0;
}

static void EnemyONIFunction(struct SubCharacter *this) {
    float pos[4];
    float rot[4];
    struct EnLOCAL_DATA *dp;

    switch (this->step) {
    case 0:
        if (this->battle.status & 0x400) {
            vcopy(&this->pos, pos);
            vcopy(&this->rot, rot);
            EnemyONIInit(this);
            if (!PlayerNowDemoEventMode()) {
                SCAnimeTypeSwitch(this, 1);
                shCharacterEnemyONIAnimeSetP(this, 0x1519, 1);
                if (this->status & 4) {
                    if ((dp = enEntryEnemy(5)) != NULL) {
                        this->enemy_p = dp;
                        enInitData(dp, this);
                    } else {
                        this->enemy_p = NULL;
                    }
                }
            }
            vcopy(pos, &this->pos);
            vcopy(rot, &this->rot);
            this->step++;
        }
    case 1:
        break;
    }
}

/** Installs the ONI update function on a sub-character. @param scp the character. */
void shCharacterSetEnemyONILow(struct SubCharacter *scp) {
    shCharacterSetFunction(scp, EnemyONIFunction);
}

/**
 * Plays drama (event) animation @p anime_id on a ONI model.
 * @param scp the character. @param anime_id animation ID in the drama table.
 * @return 0, or -1 if @p scp isn't a ONI model.
 */
int shCharacterEnemyONIAnimeSet(struct SubCharacter *scp, int anime_id) {
    struct _AnimeInfo *aip;

    if (shCharacterGetModelID(scp) == 0x209) {
        SCAnimeTypeSwitch(scp, 0);
        aip = (struct _AnimeInfo *)&d_oni_anim[anime_id - 0x18CE];
        shCharacterAnimeSet(scp, 0, 2, aip,
                            doni_anime_adr_list[anime_id - 0x18CE] +
                                (int)shCharacterGetAnimeAdrForDrama(scp, anime_id - 0x18CE));
        return 0;
    }
    return -1;
}

/**
 * Plays gameplay animation @p anime_id on a ONI model.
 * @param scp the character. @param anime_id animation ID in the gameplay table.
 * @param comp 1 selects shCharacterAnimeSet mode 4, anything else mode 2.
 * @return 0, or -1 if @p scp isn't a ONI model.
 */
int shCharacterEnemyONIAnimeSetP(struct SubCharacter *scp, int anime_id, int comp) {
    struct _AnimeInfo *aip;

    if (shCharacterGetModelID(scp) == 0x209) {
        aip = (struct _AnimeInfo *)&oni_anim[anime_id - 0x1518];
        shCharacterAnimeSet(scp, 0, (char)(comp == 1 ? 4 : 2), aip, (int)shCharacterGetAnimeAdrForPlay(scp));
        return 0;
    }
    return -1;
}

/**
 * Gets where attack @p atk starts and which way it points, from the attacking bone.
 * @param scp the enemy. @param s_pos out: world position. @param s_vec out: unit direction.
 * @param atk attack (battle) ID.
 */
void shGetEnemyONIAttackPos(struct SubCharacter *scp, float *s_pos, float *s_vec, unsigned short atk) {
    float pos0[4];
    float pos1[4];
    float vec[4];
    struct FMAT lw_mat;
    struct FMAT mat;
    int i;
    int sk_num;
    float wep_range[4] = { 500.0f, 0.0f, 0.0f, 0.0f };
    struct shSkelton *stp;

    lw_mat = scp->mat;
    stp = scp->sk_top;
    sk_num = 36;
    for (i = 0; i < sk_num; i++) {
        stp = stp->next;
    }
    mat = stp->src_m;
    pos0[0] = stp->src_m.d[3][0];
    pos0[1] = stp->src_m.d[3][1];
    pos0[2] = stp->src_m.d[3][2];
    pos0[3] = stp->src_m.d[3][3];
    mat.d[3][0] = 0.0f;
    mat.d[3][3] = 1.0f;
    sceVu0ApplyMatrix(pos0, (float (*)[4])&lw_mat, pos0);
    vcopy(pos0, s_pos);
    sceVu0ApplyMatrix(pos1, (float (*)[4])&mat, wep_range);
    pos1[0] += stp->src_m.d[3][0];
    pos1[1] += stp->src_m.d[3][1];
    pos1[2] += stp->src_m.d[3][2];
    pos1[3] = stp->src_m.d[3][3];
    sceVu0ApplyMatrix(pos1, (float (*)[4])&lw_mat, pos1);
    vec[0] = pos1[0] - pos0[0];
    vec[1] = pos1[1] - pos0[1];
    vec[2] = pos1[2] - pos0[2];
    vec[3] = 0.0f;
    _shNormalize(s_vec, vec);
}
