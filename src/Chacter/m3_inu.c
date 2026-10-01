/*
 * INU character (model 0x10D): animation tables, the update function, and the drama and
 * gameplay animation setters.
 * The dog of the Dog ending (verified; docs/characters.md).
 */

#include "sh2.h"
#include "asm_helpers.h"

static int dinu_anime_adr_list[3] = { 0, 0, 0x15584 };

static const struct _AnimeInfo inu_anim[1] = {
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0 },
};

static const struct _AnimeInfo d_inu_anim[3] = {
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0 },
    { 0x1325, 0x0215, 0x0800, 0x0000, 0x0214, 0 },
    { 0x1326, 0x0151, 0x0800, 0x0000, 0x0150, 0 },
};

static void HumanINUFunction(struct SubCharacter *this) {
    float pos[4];
    float rot[4];

    switch (this->step) {
    case 0:
        vcopy(&this->pos, pos);
        vcopy(&this->rot, rot);
        if (!PlayerNowDemoEventMode()) {
            SCAnimeTypeSwitch(this, 1);
            shCharacterHumanINUAnimeSetP(this, 0x12F3, 1);
        }
        vcopy(pos, &this->pos);
        vcopy(rot, &this->rot);
        this->step++;
    case 1:
        break;
    }
}

/** Installs the INU update function on a sub-character. @param scp the character. */
void shCharacterSetHumanINULow(struct SubCharacter *scp) {
    shCharacterSetFunction(scp, HumanINUFunction);
}

/**
 * Plays drama (event) animation @p anime_id on a INU model.
 * @param scp the character. @param anime_id animation ID in the drama table.
 * @return 0, or -1 if @p scp isn't a INU model.
 */
int shCharacterHumanINUAnimeSet(struct SubCharacter *scp, int anime_id) {
    struct _AnimeInfo *aip;

    if (shCharacterGetModelID(scp) == 0x10D) {
        SCAnimeTypeSwitch(scp, 0);
        aip = (struct _AnimeInfo *)&d_inu_anim[anime_id - 0x1324];
        shCharacterAnimeSet(scp, 0, 2, aip,
                            dinu_anime_adr_list[anime_id - 0x1324] +
                                (int)shCharacterGetAnimeAdrForDrama(scp, anime_id - 0x1324));
        return 0;
    }
    return -1;
}

/**
 * Plays gameplay animation @p anime_id on a INU model.
 * @param scp the character. @param anime_id animation ID in the gameplay table.
 * @param comp 1 selects shCharacterAnimeSet mode 4, anything else mode 2.
 * @return 0, or -1 if @p scp isn't a INU model.
 */
int shCharacterHumanINUAnimeSetP(struct SubCharacter *scp, int anime_id, int comp) {
    struct _AnimeInfo *aip;

    if (shCharacterGetModelID(scp) == 0x10D) {
        aip = (struct _AnimeInfo *)&inu_anim[anime_id - 0x12F2];
        shCharacterAnimeSet(scp, 0, (char)(comp == 1 ? 4 : 2), aip, (int)shCharacterGetAnimeAdrForPlay(scp));
        return 0;
    }
    return -1;
}
