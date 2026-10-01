/*
 * DMAR human character (Maria (drama), model 0x106): animation tables, the update function,
 * and the drama and gameplay animation setters.
 */

#include "sh2.h"

static int dmaria_anime_adr_list[31] = {
    0, 0, 0, 0x413DC, 0, 0, 0x3660, 0x24F78, 0x3F3BC, 0x6BAAC, 0, 0x24C64, 0x10500, 0x6FDF0,
    0x26100, 0x4EAD0, 0x76AD4, 0, 0xA6A24, 0xC0154, 0, 0, 0, 0, 0x1318, 0x9B88, 0, 0x77E0,
    0x34DF0, 0x70D70, 0
};

static int dmaria_clani_adr_list[30] = {
    0, 0, 0, 0x514, 0, 0, 0, 0x17C4, 0x2E0C, 0x34D8, 0, 0x304, 0, 0x134C, 0, 0x2C8, 0x704,
    0, 0x1C20, 0x2038, 0, 0, 0, 0, 0, 0, 0, 0x6C, 0x880, 0x36F4
};

static const struct _AnimeInfo dmaria_anim[30] = {
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0 },
    { 0x0BB9, 0x0001, 0x0800, 0x0000, 0x0000, 1 },
    { 0x0BBA, 0x030A, 0x0800, 0x0000, 0x0309, 0 },
    { 0x0BBB, 0x0132, 0x0800, 0x0000, 0x0131, 0 },
    { 0x0BBC, 0x015F, 0x0800, 0x0000, 0x015E, 0 },
    { 0x0BBD, 0x014F, 0x0800, 0x0000, 0x014E, 0 },
    { 0x0BBE, 0x02DA, 0x0800, 0x0000, 0x02D9, 0 },
    { 0x0BBF, 0x0330, 0x0800, 0x0000, 0x032F, 0 },
    { 0x0BC0, 0x03C6, 0x0800, 0x0000, 0x03C5, 0 },
    { 0x0BC1, 0x044F, 0x0800, 0x0000, 0x044E, 0 },
    { 0x0BC2, 0x0230, 0x0800, 0x0000, 0x022F, 0 },
    { 0x0BC3, 0x0346, 0x0800, 0x0000, 0x0345, 0 },
    { 0x0BC4, 0x0385, 0x0800, 0x0000, 0x0384, 0 },
    { 0x0BC5, 0x0209, 0x0800, 0x0000, 0x0208, 0 },
    { 0x0BC6, 0x0264, 0x0800, 0x0000, 0x0263, 0 },
    { 0x0BC7, 0x025E, 0x0800, 0x0000, 0x025D, 0 },
    { 0x0BC8, 0x0226, 0x0800, 0x0000, 0x0225, 0 },
    { 0x0BC9, 0x031A, 0x0800, 0x0000, 0x0319, 0 },
    { 0x0BCA, 0x0137, 0x0800, 0x0000, 0x0136, 0 },
    { 0x0BCB, 0x0214, 0x0800, 0x0000, 0x0213, 0 },
    { 0x0BCC, 0x0230, 0x0800, 0x0000, 0x022F, 0 },
    { 0x0BCD, 0x0249, 0x0800, 0x0000, 0x0248, 0 },
    { 0x0BCE, 0x0107, 0x0800, 0x0000, 0x0106, 0 },
    { 0x0BCF, 0x0064, 0x0800, 0x0000, 0x0063, 0 },
    { 0x0BD0, 0x006E, 0x0800, 0x0000, 0x006D, 0 },
    { 0x0BD1, 0x0095, 0x0800, 0x0000, 0x0094, 0 },
    { 0x0BD2, 0x02E9, 0x0800, 0x0000, 0x02E8, 0 },
    { 0x0BD3, 0x023E, 0x0800, 0x0000, 0x023D, 0 },
    { 0x0BD4, 0x0260, 0x0800, 0x0000, 0x025F, 0 },
    { 0x0BD5, 0x0235, 0x0800, 0x0000, 0x0234, 0 },
};

static const struct _AnimeInfo p_hhh_mar_anim[6] = {
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0 },
    { 0x0D49, 0x0048, 0x0800, 0x0000, 0x0047, 1 },
    { 0x0D4A, 0x0065, 0x0300, 0x0000, 0x0064, 0 },
    { 0x0D4B, 0x0043, 0x0300, 0x0065, 0x00A7, 1 },
    { 0x0D4C, 0x0002, 0x0800, 0x0000, 0x0001, 1 },
    { 0x0D4D, 0x000F, 0x0800, 0x0000, 0x000E, 1 },
};

static int HumanDMARInit() { /* Matching: K&R definition; called with arguments it ignores (the DWARF shows none). */
    return 0;
}

static void HumanDMARFunction(struct SubCharacter *this) {
    struct _AnimeInfo *aip;

    switch (this->step) {
    case 0:
        if (shCharacterGetModelID(this) == 0x106) {
            HumanDMARInit(this);
            aip = (struct _AnimeInfo *)&dmaria_anim[1];
            shCharacterAnimeSet(this, 0, 0, aip, (int)shCharacterGetAnimeAdrForDrama(this, 1));
        }
        this->step++;
    case 1:
        break;
    }
}

/** Installs the DMAR update function on a sub-character. @param scp the character. */
void shCharacterSetHumanDMARLow(struct SubCharacter *scp) {
    shCharacterSetFunction(scp, HumanDMARFunction);
}

/**
 * Plays drama (event) animation @p anime_id and its cluster animation on a DMAR model.
 * @param scp the character. @param anime_id animation ID in the drama table.
 * @return 0, or -1 if @p scp isn't a DMAR model.
 */
int shCharacterHumanDMARAnimeSet(struct SubCharacter *scp, int anime_id) {
    struct _AnimeInfo *aip;

    SCAnimeTypeSwitch(scp, 0);
    if (shCharacterGetModelID(scp) == 0x106) {
        aip = (struct _AnimeInfo *)&dmaria_anim[anime_id - 0xBB8];
        shCharacterAnimeSet(scp, 0, 2, aip,
                            dmaria_anime_adr_list[anime_id - 0xBB8] +
                                (int)shCharacterGetAnimeAdrForDrama(scp, anime_id - 0xBB8));
        shCharacterClusterAnimeSet(scp, dmaria_clani_adr_list[anime_id - 0xBB8] +
                                            (int)shCharacterGetClusterAnimeAdr(scp));
        return 0;
    }
    return -1;
}

/**
 * Plays gameplay animation @p anime_id on a DMAR model. Some IDs also set the eye/center height.
 * @param scp the character. @param anime_id animation ID in the gameplay table.
 * @return 0, or -1 if @p scp isn't a DMAR model.
 */
int shCharacterHumanDMARAnimeSetP(struct SubCharacter *scp, int anime_id) {
    struct _AnimeInfo *aip;

    SCAnimeTypeSwitch(scp, 1);
    if (shCharacterGetModelID(scp) == 0x106) {
        switch (anime_id) {
        case 0xD49:
        case 0xD4A:
        case 0xD4B:
        case 0xD4C:
            break;
        case 0xD4D:
            scp->eye_y = 2250.0f;
            scp->center_y = 2250.0f;
            break;
        }
        aip = (struct _AnimeInfo *)&p_hhh_mar_anim[anime_id - 0xD48];
        shCharacterAnimeSet(scp, 0, 0, aip, (int)shCharacterGetAnimeAdrForPlay(scp));
        return 0;
    }
    return -1;
}
