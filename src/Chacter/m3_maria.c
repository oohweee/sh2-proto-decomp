/*
 * MAR: Maria as a gameplay character (model 0x105). The update function runs the Maria AI in
 * m3_maria_sub; this file also owns her work area sh2mar.
 */

#include "sh2.h"
#include "asm_helpers.h"

static int dmar_anime_adr_list[7] = { 0, 0, 0, 0, 0, 0, 0 };

static const struct _AnimeInfo pmaria_anim[40] = {
    { 0, 0, 0, 0, 0, 0 },
    { 0xB, 0xA, 0x180, 0, 9, 1 },
    { 0xC, 0x18, 0x600, 0xA, 0x21, 1 },
    { 0xD, 0x18, 0x700, 0x22, 0x39, 1 },
    { 0xE, 0x14, 0x400, 0x3A, 0x4D, 1 },
    { 0xF, 0x1F, 0x200, 0x4E, 0x6C, 0 },
    { 0x10, 0x1E, 0x400, 0x6D, 0x8A, 0 },
    { 0x11, 0x1E, -0xC00, 0x6D, 0x8A, 0 },
    { 0x12, 0xA, 0x80, 0x8B, 0x94, 1 },
    { 0x13, 0x37, 0x280, 0x95, 0xCB, 0 },
    { 0x14, 0xF, 0x400, 0xCC, 0xDA, 0 },
    { 0x15, 0xF, 0x400, 0xDB, 0xE9, 0 },
    { 0x16, 0xF, 0x400, 0xEA, 0xF8, 0 },
    { 0x17, 0xF, 0x400, 0xF9, 0x107, 0 },
    { 0x18, 0x1E, 0x200, 0x108, 0x125, 0 },
    { 0x19, 0x1E, 0x200, 0x126, 0x143, 0 },
    { 0x1A, 0xF, 0x400, 0x144, 0x152, 0 },
    { 0x1B, 0xF, 0x400, 0x153, 0x161, 0 },
    { 0x1C, 0x1E, 0x200, 0x162, 0x17F, 0 },
    { 0x1D, 0x1E, 0x200, 0x180, 0x19D, 0 },
    { 0x1E, 0xF, 0x400, 0x19E, 0x1AC, 0 },
    { 0x1F, 0xF, 0x400, 0x1AD, 0x1BB, 0 },
    { 0x20, 0x1E, 0x200, 0x1BC, 0x1D9, 0 },
    { 0x21, 0x1F, 0x200, 0x1DA, 0x1F8, 0 },
    { 0x22, 0xF, 0x400, 0x1F9, 0x207, 0 },
    { 0x23, 0xF, 0x400, 0x208, 0x216, 0 },
    { 0x24, 0x1F, 0x200, 0x217, 0x235, 0 },
    { 0x25, 0x1E, 0x200, 0x236, 0x253, 0 },
    { 0x26, 0x1E, 0x200, 0x254, 0x271, 0 },
    { 0x27, 0xF, 0x400, 0x272, 0x280, 0 },
    { 0x28, 0xF, 0x400, 0x281, 0x28F, 0 },
    { 0x29, 0x1E, 0x200, 0x290, 0x2AD, 0 },
    { 0x2A, 0x1E, 0x200, 0x2AE, 0x2CB, 0 },
    { 0x2B, 0xF, 0x400, 0x2CC, 0x2DA, 0 },
    { 0x2C, 0xF, 0x400, 0x2DB, 0x2E9, 0 },
    { 0x2D, 0x1E, 0x200, 0x2EA, 0x307, 0 },
    { 0x2E, 0x1E, 0x200, 0x308, 0x325, 0 },
    { 0x2F, 0xF, 0x400, 0x326, 0x334, 0 },
    { 0x30, 0xF, 0x400, 0x335, 0x344, 0 },
    { 0x31, 0xF, 0x400, 0x345, 0x353, 0 },
};

static const struct _AnimeInfo d_mar_anim[7] = {
    { 0, 0, 0, 0, 0, 0 },
    { 0x29, 0xA, 0x800, 0, 9, 1 },
    { 0x2A, 0x105, 0x800, 0, 0x104, 0 },
    { 0x2B, 0x137, 0x800, 0, 0x136, 0 },
    { 0x2C, 0x140, 0x800, 0, 0x13F, 0 },
    { 0x2D, 0xBF, 0x800, 0, 0xBE, 0 },
    { 0x2D, 0xBA, 0x800, 0, 0xB9, 0 },
};

struct shMariaWork sh2mar;

static int HumanMARInit(struct SubCharacter *scp) {
    SCAnimeTypeSwitch(scp, 1);
    SCLightOnNowSwitch(scp, 1);
    scp->battle.status |= 0x400;
    sh2mar.active_type = 2;
    return 0;
}

static void HumanMARFunction(struct SubCharacter *this) {
    struct _AnimeInfo *aip;
    float pos[4];
    float rot[4];
    void MariaCheckAnime(); /* Matching: called without a prototype in the original (arguments passed unconverted). */
    void MariaCheckSound(); /* Matching: likewise. */

    switch (this->step) {
    case 0:
        vcopy_dst_first(pos, (float *)&this->pos);
        vcopy_dst_first(rot, (float *)&this->rot);
        HumanMARInit(this);
        SCAnimeTypeSwitch(this, 1);
        aip = (struct _AnimeInfo *)&pmaria_anim[1];
        shCharacterAnimeSet(this, 0, 0, aip, (int)shCharacterGetAnimeAdrForPlay(this));
        vcopy_dst_first((float *)&this->pos, pos);
        vcopy_dst_first((float *)&this->rot, rot);
        MariaCheckSetParameterPhase2(this);
        this->step++;
        break;
    case 1:
        if ((this->status & 4) && !(this->status & 0x2000)) {
            MariaCheckDamage(this);
            MariaCheckSetParameterPhase1(this);
            MariaCheckControl(this);
            MariaCheckAnime(this);
            MariaUpdatePosition(this);
            MariaCheckSetParameterPhase2(this);
            MariaCheckSound();
        }
        break;
    }
}

/** Installs Maria's update function and gives her sh2mar's HP. @param scp the character. */
void shCharacterSetHumanMARLow(struct SubCharacter *scp) {
    shCharacterSetFunction(scp, HumanMARFunction);
    sh2mar.mar_p = scp;
    sh2mar.mar_p->battle.hp = sh2mar.hp;
    sh2mar.mar_p->battle.hp_max = sh2mar.hp_max;
}

/**
 * Plays drama (event) animation @p anime_id on a MAR model.
 * @param scp the character. @param anime_id animation ID in the drama table.
 * @return 0, or -1 if @p scp isn't a MAR model.
 */
int shCharacterHumanMARAnimeSet(struct SubCharacter *scp, int anime_id) {
    struct _AnimeInfo *aip;

    if (shCharacterGetModelID(scp) == 0x105) {
        SCAnimeTypeSwitch(scp, 0);
        aip = (struct _AnimeInfo *)&d_mar_anim[anime_id - 0x28];
        shCharacterAnimeSet(scp, 0, 2, aip,
                            dmar_anime_adr_list[anime_id - 0x28] +
                                (int)shCharacterGetAnimeAdrForDrama(scp, anime_id - 0x28));
        return 0;
    }
    return -1;
}

/** Clears sh2mar. */
void shCharacterMariaWorkInit(void) {
    void shQzero(); /* Matching: called without a prototype in the original (arguments passed unconverted). */

    shQzero(&sh2mar, sizeof(struct shMariaWork));
}

/** Sets Maria's starting HP (by battle level) and stamina, and resets her status. */
void shCharacterMariaWorkInitAtGameStart(void) {
    switch (playing.battle_level) {
    case 0:
    case 1:
        sh2mar.hp = sh2mar.hp_max = 160.0f;
        break;
    case 2:
    case 3:
        sh2mar.hp = sh2mar.hp_max = 80.0f;
        break;
    }
    sh2mar.tired_max = 600;
    sh2mar.main_status_now = 0;
    sh2mar.main_status_prev = 0;
    sh2mar.sub_status_now = 0;
    sh2mar.sub_status_prev = 0;
    mar_flg_on(&sh2mar.sub_st_flg, 1);
    mar_sub_st_set(0, &sh2mar);
    mar_sub_flg_set(0, &sh2mar);
    sh2mar.column_mov.kind = sh2mar.column_atk.kind = 1;
    sh2mar.column_mov.weight = sh2mar.column_atk.weight = 2;
    sh2mar.column_mov.material = sh2mar.column_atk.material = 6;
    sh2mar.column_mov.shape = sh2mar.column_atk.shape = 3;
    sh2mar.column_mov.p[1][3] = sh2mar.column_atk.p[1][3] = 150.0f;
}
