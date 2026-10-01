/*
 * NSE enemy character (model 0x207): animation tables, the sub-character update function
 * that registers the enemy AI (enEntryEnemy/enInitData), animation setters, and the
 * attack-position query used by the battle code.
 * NSE is the Bubble Head Nurse (suspected); XOO, handled here too, is unknown (docs/characters.md).
 */

#include "sh2.h"
#include "asm_helpers.h"
#include "sh_vu0.h"
#include "sdk/libvu0.h"

static const struct _AnimeInfo nse_anim[27] = {
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0 },
    { 0x157D, 0x0011, 0x0800, 0x0000, 0x0010, 1 },
    { 0x157E, 0x0022, 0x0800, 0x0011, 0x0032, 1 },
    { 0x157F, 0x0020, 0x0800, 0x0033, 0x0052, 0 },
    { 0x1580, 0x0022, 0x0800, 0x0053, 0x0074, 0 },
    { 0x1581, 0x0013, 0x0800, 0x0075, 0x0087, 0 },
    { 0x1582, 0x0021, 0x0800, 0x0088, 0x00A8, 0 },
    { 0x1583, 0x0024, 0x0800, 0x00A9, 0x00CC, 0 },
    { 0x1584, 0x0018, 0x0800, 0x00CD, 0x00E4, 0 },
    { 0x1585, 0x0018, 0x0800, 0x00E5, 0x00FC, 0 },
    { 0x1586, 0x0019, 0x0800, 0x00FD, 0x0115, 0 },
    { 0x1587, 0x0021, 0x0800, 0x0116, 0x0136, 0 },
    { 0x1588, 0x0024, 0x0800, 0x0137, 0x015A, 0 },
    { 0x1589, 0x0015, 0x0800, 0x015B, 0x016F, 0 },
    { 0x158A, 0x001D, 0x0800, 0x0170, 0x018C, 0 },
    { 0x158B, 0x0017, 0x0800, 0x018D, 0x01A3, 0 },
    { 0x158C, 0x001F, 0x0800, 0x01A4, 0x01C2, 0 },
    { 0x158D, 0x001F, 0x0800, 0x01C3, 0x01E1, 0 },
    { 0x158E, 0x0011, 0x0800, 0x01E2, 0x01F2, 0 },
    { 0x158F, 0x001E, 0x0800, 0x01F3, 0x0210, 0 },
    { 0x1590, 0x001D, 0x0800, 0x0211, 0x022D, 0 },
    { 0x1591, 0x0023, 0x0800, 0x022E, 0x0250, 0 },
    { 0x1592, 0x0027, 0x0800, 0x0251, 0x0277, 0 },
    { 0x1593, 0x000B, 0x0800, 0x0278, 0x0282, 1 },
    { 0x1594, 0x000F, 0x0800, 0x0283, 0x0291, 1 },
    { 0x1595, 0x0001, 0x0800, 0x018B, 0x018B, 0 },
    { 0x1596, 0x0001, 0x0800, 0x01A2, 0x01A2, 0 },
};

/*
 * Matching: the DWARF drops this unused parameter. Passing `this` keeps it live in a0 up
 * to the call (MWCC knows this static callee clobbers nothing else), so the switch on
 * this->step is loaded into a1.
 */
static int EnemyNSEInit(struct SubCharacter *scp) {
    return 0;
}

static void EnemyNSEFunction(struct SubCharacter *this) {
    float pos[4];
    float rot[4];
    struct EnLOCAL_DATA *dp;

    switch (this->step) {
    case 0:
        if (this->battle.status & 0x400) {
            vcopy(&this->pos, pos);
            vcopy(&this->rot, rot);
            EnemyNSEInit(this);
            if (!PlayerNowDemoEventMode()) {
                SCAnimeTypeSwitch(this, 1);
                shCharacterEnemyNSEAnimeSetP(this, 0x157D, 1);
                vcopy(pos, &this->pos);
                vcopy(rot, &this->rot);
                if (this->status & 4) {
                    if ((dp = enEntryEnemy(6)) != NULL) {
                        this->enemy_p = dp;
                        enInitData(dp, this);
                    } else {
                        this->enemy_p = NULL;
                    }
                }
            }
            this->step++;
        }
    case 1:
    case 2:
        break;
    }
}

/** Installs the NSE update function on a sub-character. @param scp the character. */
void shCharacterSetEnemyNSELow(struct SubCharacter *scp) {
    shCharacterSetFunction(scp, EnemyNSEFunction);
}

/**
 * Plays gameplay animation @p anime_id on a NSE model.
 * @param scp the character. @param anime_id animation ID in the gameplay table.
 * @param comp 1 selects shCharacterAnimeSet mode 4, anything else mode 2.
 * @return 0, or -1 if @p scp isn't a NSE model.
 */
int shCharacterEnemyNSEAnimeSetP(struct SubCharacter *scp, int anime_id, int comp) {
    struct _AnimeInfo *aip;

    if (shCharacterGetModelID(scp) == 0x207 || shCharacterGetModelID(scp) == 0x20B) {
        aip = (struct _AnimeInfo *)&nse_anim[anime_id - 0x157C];
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
void shGetEnemyNSEAttackPos(struct SubCharacter *scp, float *s_pos, float *s_vec, unsigned short atk) {
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
    sk_num = 24;
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
