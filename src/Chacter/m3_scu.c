/*
 * SCU enemy character (model 0x200): animation tables, the sub-character update function
 * that registers the enemy AI (enEntryEnemy/enInitData), animation setters, and the
 * attack-position query used by the battle code.
 * The Lying Figure (verified; docs/characters.md).
 */

#include "sh2.h"
#include "asm_helpers.h"
#include "sh_vu0.h"
#include "sdk/libvu0.h"

static int dscu_anime_adr_list[10] = { 0, 0, 0, 0, 0x16818, 0x1BEBC, 0x1C4B0, 0x1E47C, 0x2CE90, 0 };

static const struct _AnimeInfo scu_anim[36] = {
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0 },
    { 0x1389, 0x000F, 0x0800, 0x0000, 0x000E, 1 },
    { 0x138A, 0x001F, 0x0800, 0x000F, 0x002D, 1 },
    { 0x138B, 0x0011, 0x0800, 0x002E, 0x003E, 1 },
    { 0x138C, 0x000E, 0x0800, 0x003F, 0x004C, 0 },
    { 0x138D, 0x0018, 0x0800, 0x004D, 0x0064, 0 },
    { 0x138E, 0x0018, 0x0800, 0x004D, 0x0064, 0 },
    { 0x138F, 0x0014, 0x0800, 0x0065, 0x0078, 0 },
    { 0x1390, 0x0021, 0x0800, 0x0079, 0x0099, 0 },
    { 0x1391, 0x001E, 0x0800, 0x009A, 0x00B7, 0 },
    { 0x1392, 0x0014, 0x0800, 0x00B8, 0x00CB, 0 },
    { 0x1393, 0x0015, 0x0800, 0x00CC, 0x00E0, 0 },
    { 0x1394, 0x0013, 0x0800, 0x00E1, 0x00F3, 0 },
    { 0x1395, 0x0022, 0x0800, 0x00F4, 0x0115, 0 },
    { 0x1396, 0x0025, 0x0800, 0x0116, 0x013A, 0 },
    { 0x1397, 0x0017, 0x0800, 0x013B, 0x0151, 0 },
    { 0x1398, 0x000F, 0x0800, 0x0152, 0x0160, 0 },
    { 0x1399, 0x0010, 0x0800, 0x0161, 0x0170, 0 },
    { 0x139A, 0x001D, 0x0800, 0x0171, 0x018D, 0 },
    { 0x139B, 0x001D, 0x0800, 0x018E, 0x01AA, 0 },
    { 0x139C, 0x000F, 0x06A4, 0x01AB, 0x01B9, 0 },
    { 0x139D, 0x001D, 0x0800, 0x01BA, 0x01D6, 0 },
    { 0x139E, 0x001E, 0x0800, 0x01D7, 0x01F4, 0 },
    { 0x139F, 0x0024, 0x0800, 0x01F5, 0x0218, 0 },
    { 0x13A0, 0x002F, 0x0800, 0x0219, 0x0247, 0 },
    { 0x13A1, 0x0026, 0x0800, 0x0248, 0x026D, 0 },
    { 0x13A2, 0x0026, 0x0800, 0x026E, 0x0293, 0 },
    { 0x13A3, 0x000F, 0x0800, 0x0294, 0x02A2, 1 },
    { 0x13A4, 0x000F, 0x0800, 0x02A3, 0x02B1, 1 },
    { 0x13A5, 0x000C, 0x0800, 0x02B2, 0x02BD, 1 },
    { 0x13A6, 0x000C, 0x0800, 0x02BE, 0x02C9, 1 },
    { 0x13A7, 0x0002, 0x0800, 0x02CA, 0x02CB, 0 },
    { 0x13A8, 0x0001, 0x0800, 0x015F, 0x015F, 0 },
    { 0x13A9, 0x0001, 0x0800, 0x016F, 0x016F, 0 },
    { 0x13AA, 0x0002, 0x0800, 0x02CC, 0x02CD, 0 },
    { 0x13A6, 0x003D, 0x0800, 0x02CE, 0x030A, 1 },
};

static const struct _AnimeInfo d_scu_anim[10] = {
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0 },
    { 0x1771, 0x002A, 0x0800, 0x0000, 0x0029, 1 },
    { 0x1772, 0x01FE, 0x0800, 0x0000, 0x01FD, 0 },
    { 0x1773, 0x02C2, 0x0800, 0x0000, 0x02C1, 0 },
    { 0x1774, 0x00A0, 0x0800, 0x0000, 0x009F, 0 },
    { 0x1775, 0x0048, 0x0800, 0x0000, 0x0047, 0 },
    { 0x1776, 0x003A, 0x0800, 0x0000, 0x0039, 0 },
    { 0x1777, 0x01B6, 0x0800, 0x0000, 0x01B5, 0 },
    { 0x1778, 0x0157, 0x0800, 0x0000, 0x0156, 0 },
    { 0x1779, 0x01D5, 0x0800, 0x0000, 0x01D4, 0 },
};

/*
 * Matching: the DWARF drops this unused parameter. Passing `this` keeps it live in a0 up
 * to the call (MWCC knows this static callee clobbers nothing else), so the switch on
 * this->step is loaded into a1.
 */
static int EnemySCUInit(struct SubCharacter *scp) {
    return 0;
}

static void EnemySCUFunction(struct SubCharacter *this) {
    float pos[4];
    float rot[4];
    struct EnLOCAL_DATA *dp;

    switch (this->step) {
    case 0:
        if (this->battle.status & 0x400) {
            vcopy(&this->pos, pos);
            vcopy(&this->rot, rot);
            EnemySCUInit(this);
            if (!PlayerNowDemoEventMode()) {
                SCAnimeTypeSwitch(this, 1);
                shCharacterEnemySCUAnimeSetP(this, 0x138A, 1);
                if (this->status & 4) {
                    if ((dp = enEntryEnemy(1)) != NULL) {
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

/** Installs the SCU update function on a sub-character. @param scp the character. */
void shCharacterSetEnemySCULow(struct SubCharacter *scp) {
    shCharacterSetFunction(scp, EnemySCUFunction);
}

/**
 * Plays drama (event) animation @p anime_id on a SCU model.
 * @param scp the character. @param anime_id animation ID in the drama table.
 * @return 0, or -1 if @p scp isn't a SCU model.
 */
int shCharacterEnemySCUAnimeSet(struct SubCharacter *scp, int anime_id) {
    struct _AnimeInfo *aip;

    if (shCharacterGetModelID(scp) == 0x200) {
        SCAnimeTypeSwitch(scp, 0);
        aip = (struct _AnimeInfo *)&d_scu_anim[anime_id - 0x1770];
        shCharacterAnimeSet(scp, 0, 2, aip,
                            dscu_anime_adr_list[anime_id - 0x1770] +
                                (int)shCharacterGetAnimeAdrForDrama(scp, anime_id - 0x1770));
        return 0;
    }
    return -1;
}

/**
 * Plays gameplay animation @p anime_id on a SCU model.
 * @param scp the character. @param anime_id animation ID in the gameplay table.
 * @param comp 1 selects shCharacterAnimeSet mode 4, anything else mode 2.
 * @return 0, or -1 if @p scp isn't a SCU model.
 */
int shCharacterEnemySCUAnimeSetP(struct SubCharacter *scp, int anime_id, int comp) {
    struct _AnimeInfo *aip;

    if (shCharacterGetModelID(scp) == 0x200) {
        aip = (struct _AnimeInfo *)&scu_anim[anime_id - 0x1388];
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
void shGetEnemySCUAttackPos(struct SubCharacter *scp, float *s_pos, float *s_vec, unsigned short atk) {
    float pos0[4];
    float pos1[4];
    float vec[4];

    pos0[0] = scp->pos.x;
    pos0[1] = scp->pos.y + -650.0f;
    pos0[2] = scp->pos.z;
    vcopy(pos0, s_pos);
    switch (atk) {
    case 0:
        pos1[0] = pos0[0] + 900.0f * shSinF(scp->rot.y);
        pos1[2] = pos0[2] + 1000.0f * shCosF(scp->rot.y);
        pos1[1] = pos0[1];
        break;
    case 1:
        pos1[0] = pos0[0] + 900.0f * shSinF(scp->rot.y);
        pos1[2] = pos0[2] + 1000.0f * shCosF(scp->rot.y);
        pos1[1] = pos0[1] - 900.0f * shCosF(0.41887903f);
        break;
    case 2:
        pos1[0] = pos0[0] + 900.0f * shSinF(scp->rot.y + 0.41887903f);
        pos1[2] = pos0[2] + 1000.0f * shCosF(scp->rot.y + 0.41887903f);
        pos1[1] = pos0[1];
        break;
    case 3:
        pos1[0] = pos0[0] + 900.0f * shSinF(scp->rot.y);
        pos1[2] = pos0[2] + 1000.0f * shCosF(scp->rot.y);
        pos1[1] = pos0[1] + 900.0f * shCosF(0.41887903f);
        break;
    case 4:
        pos1[0] = pos0[0] + 900.0f * shSinF(scp->rot.y - 0.41887903f);
        pos1[2] = pos0[2] + 1000.0f * shCosF(scp->rot.y - 0.41887903f);
        pos1[1] = pos0[1];
        break;
    }
    vec[0] = pos1[0] - pos0[0];
    vec[1] = pos1[1] - pos0[1];
    vec[2] = pos1[2] - pos0[2];
    vec[3] = 0.0f;
    _shNormalize(s_vec, vec);
}

/**
 * Spawns a ground-impact effect at one of the SCU's feet.
 * @param scp the enemy. @param type 0 for one foot bone, non-zero for the other (also passed
 *        on as the effect type).
 */
void shEnemySCU_EffectFoot(struct SubCharacter *scp, int type) {
    float mat[4][4];
    int i1;
    int foot;
    struct shSkelton *sk;

    foot = type ? 20 : 21;
    sk = scp->sk_top;
    for (i1 = 0; i1 < foot; i1++) {
        sk = sk->next;
    }
    sceVu0MulMatrix(mat, (float (*)[4])&scp->mat, (float (*)[4])&sk->src_m);
    mat[3][1] += 50.0f;
    HH_Effect_Object_Ground_Impact_Post_forEnemy(mat[3], type);
}
