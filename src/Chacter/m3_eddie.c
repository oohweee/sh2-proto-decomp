/*
 * EDI human character (Eddie, model 0x108): animation tables, the update function,
 * and the drama and gameplay animation setters.
 */

#include "sh2.h"

static int deddie_anime_adr_list[28] = {
    0, 0, 0, 0xE460, 0x35E68, 0x76B08, 0x96204, 0x9D554, 0, 0x3754C, 0x7EC90, 0, 0x26B78,
    0xE134, 0x2EEC8, 0x43B4C, 0x453D8, 0x59E5C, 0x794F8, 0, 0x406C, 0x3915C, 0x658, 0x5D24,
    0xC190, 0xFF5C, 0x1F990, 0x315D4
};

static int deddie_clani_adr_list[28] = {
    0, 0, 0, 0, 0x760, 0x189C, 0x1ED8, 0x1FF0, 0, 0x3AC4, 0x8E5C, 0, 0x2C9C, 0, 0xD54,
    0x2620, 0x268C, 0x464C, 0x46B0, 0, 0x6C, 0x4D2C, 0, 0x6C, 0xD8, 0x144, 0x578, 0x65C
};

static const struct _AnimeInfo eddie_anim[28] = {
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0 },
    { 0x1195, 0x0078, 0x0800, 0x0000, 0x0077, 1 },
    { 0x1196, 0x0055, 0x0800, 0x0000, 0x0054, 1 },
    { 0x1197, 0x03D8, 0x0800, 0x0000, 0x03D7, 0 },
    { 0x1198, 0x0661, 0x0800, 0x0000, 0x0660, 0 },
    { 0x1199, 0x0317, 0x0800, 0x0000, 0x0316, 0 },
    { 0x119A, 0x00AB, 0x0800, 0x0000, 0x00AA, 0 },
    { 0x119B, 0x01ED, 0x0800, 0x0000, 0x01EC, 0 },
    { 0x119C, 0x03C8, 0x0800, 0x0000, 0x03C7, 0 },
    { 0x119D, 0x03CC, 0x0800, 0x0000, 0x03CB, 0 },
    { 0x119E, 0x03A4, 0x0800, 0x0000, 0x03A3, 0 },
    { 0x119F, 0x02FC, 0x0800, 0x0000, 0x02FB, 0 },
    { 0x11A0, 0x01AA, 0x0800, 0x0000, 0x01A9, 0 },
    { 0x11A1, 0x0295, 0x0800, 0x0000, 0x0294, 0 },
    { 0x11A2, 0x017D, 0x0800, 0x0000, 0x017C, 0 },
    { 0x11A3, 0x00AC, 0x0800, 0x0000, 0x00AB, 0 },
    { 0x11A4, 0x0179, 0x0800, 0x0000, 0x0179, 0 },
    { 0x11A5, 0x01BD, 0x0800, 0x0000, 0x01BC, 0 },
    { 0x11A6, 0x00D3, 0x0800, 0x0000, 0x00D2, 0 },
    { 0x11A7, 0x01EB, 0x0800, 0x0000, 0x01EA, 0 },
    { 0x11A8, 0x032B, 0x0800, 0x0000, 0x032A, 0 },
    { 0x11A9, 0x0183, 0x0800, 0x0000, 0x0182, 0 },
    { 0x11AA, 0x029E, 0x0800, 0x0000, 0x029D, 0 },
    { 0x11AB, 0x030B, 0x0800, 0x0000, 0x030A, 0 },
    { 0x11AC, 0x01D6, 0x0800, 0x0000, 0x01D5, 0 },
    { 0x11AD, 0x028B, 0x0800, 0x0000, 0x028A, 0 },
    { 0x11AE, 0x00D3, 0x0800, 0x0000, 0x00D2, 0 },
    { 0x11AF, 0x04F7, 0x0800, 0x0000, 0x04F6, 0 },
};

static const struct _AnimeInfo peddie_anim[4] = {
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0 },
    { 0x12C1, 0x0048, 0x0200, 0x0000, 0x0047, 1 },
    { 0x12C2, 0x0047, 0x0200, 0x0000, 0x0046, 1 },
    { 0x12C3, 0x0002, 0x0800, 0x0000, 0x0001, 1 },
};

static void HumanEDIFunction(struct SubCharacter *this) {
    struct _AnimeInfo *aip;
    short id;

    switch (this->step) {
    case 0:
        id = shCharacterGetModelID(this);
        if (id == 0x108) {
            aip = (struct _AnimeInfo *)&eddie_anim[1];
            shCharacterAnimeSet(this, 0, 0, aip, (int)shCharacterGetAnimeAdrForDrama(this, 1));
        }
        this->step++;
    case 1:
        break;
    }
}

/** Installs the EDI update function on a sub-character. @param scp the character. */
void shCharacterSetHumanEDILow(struct SubCharacter *scp) {
    shCharacterSetFunction(scp, HumanEDIFunction);
}

/**
 * Plays drama (event) animation @p anime_id and its cluster animation on a EDI model.
 * @param scp the character. @param anime_id animation ID in the drama table.
 * @return 0, or -1 if @p scp isn't a EDI model.
 */
int shCharacterHumanEDIAnimeSet(struct SubCharacter *scp, int anime_id) {
    struct _AnimeInfo *aip;

    SCAnimeTypeSwitch(scp, 0);
    if (shCharacterGetModelID(scp) == 0x108) {
        aip = (struct _AnimeInfo *)&eddie_anim[anime_id - 0x1194];
        shCharacterAnimeSet(scp, 0, 2, aip,
                            deddie_anime_adr_list[anime_id - 0x1194] +
                                (int)shCharacterGetAnimeAdrForDrama(scp, anime_id - 0x1194));
        shCharacterClusterAnimeSet(scp, deddie_clani_adr_list[anime_id - 0x1194] +
                                            (int)shCharacterGetClusterAnimeAdr(scp));
        return 0;
    }
    return -1;
}

/**
 * Plays gameplay animation @p anime_id on a EDI model. Some IDs also set the eye/center height.
 * @param scp the character. @param anime_id animation ID in the gameplay table.
 * @return 0, or -1 if @p scp isn't a EDI model.
 */
int shCharacterHumanEDIAnimeSetP(struct SubCharacter *scp, int anime_id) {
    struct _AnimeInfo *aip;

    SCAnimeTypeSwitch(scp, 1);
    if (shCharacterGetModelID(scp) == 0x108) {
        switch (anime_id) {
        case 0x12C1:
            scp->eye_y = -242.1998f;
            scp->center_y = -242.1998f;
            break;
        case 0x12C2:
            scp->eye_y = -432.1998f;
            scp->center_y = -432.1998f;
            break;
        case 0x12C3:
            break;
        }
        aip = (struct _AnimeInfo *)&peddie_anim[anime_id - 0x12C0];
        shCharacterAnimeSet(scp, 0, 0, aip, (int)shCharacterGetAnimeAdrForPlay(scp));
        return 0;
    }
    return -1;
}
