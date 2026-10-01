/*
 * MRY human character (Mary, model 0x109): animation tables, the update function,
 * and the drama and gameplay animation setters.
 */

#include "sh2.h"

static int dmary_anime_adr_list[21] = {
    0, 0, 0x10D0C, 0x21110, 0x37E74, 0x4FD60, 0x606E0, 0x11110, 0x1DB28, 0x38250, 0x4B838,
    0x691E4, 0x10D0C, 0x25884, 0x3D014, 0x4FDBC, 0x678FC, 0x10D0C, 0x1F7A4, 0x41508, 0x4FDBC
};

static int dmary_clani_adr_list[21] = {
    0, 0, 0, 0xDC, 0x1F7C, 0x269C, 0x29E8, 0, 0x274, 0xBC8, 0x1988, 0x2A64, 0, 0x684,
    0x31B4, 0x5894, 0x81C0, 0, 0x70, 0x514C, 0x9C2C
};

static const struct _AnimeInfo dmary_anim[21] = {
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0 },
    { 0x0FA1, 0x0009, 0x0800, 0x0000, 0x0000, 1 },
    { 0x0FA2, 0x0191, 0x0800, 0x0000, 0x0190, 0 },
    { 0x0FA3, 0x028D, 0x0800, 0x0000, 0x028C, 0 },
    { 0x0FA4, 0x0201, 0x0800, 0x0000, 0x0200, 0 },
    { 0x0FA5, 0x0156, 0x0800, 0x0000, 0x0155, 0 },
    { 0x0FA6, 0x00BC, 0x0800, 0x0000, 0x00BB, 0 },
    { 0x0FA7, 0x019A, 0x0800, 0x0000, 0x0199, 0 },
    { 0x0FA8, 0x034C, 0x0800, 0x0000, 0x034B, 0 },
    { 0x0FA9, 0x02A4, 0x0800, 0x0000, 0x02A3, 0 },
    { 0x0FAA, 0x0342, 0x0800, 0x0000, 0x0340, 0 },
    { 0x0FAB, 0x00BC, 0x0800, 0x0000, 0x00BB, 0 },
    { 0x0FAC, 0x01ED, 0x0800, 0x0000, 0x01EC, 0 },
    { 0x0FAD, 0x0263, 0x0800, 0x0000, 0x0262, 0 },
    { 0x0FAE, 0x0184, 0x0800, 0x0000, 0x0183, 0 },
    { 0x0FAF, 0x01E3, 0x0800, 0x0000, 0x01E2, 0 },
    { 0x0FB0, 0x00BC, 0x0800, 0x0000, 0x00BB, 0 },
    { 0x0FB1, 0x014E, 0x0800, 0x0000, 0x014D, 0 },
    { 0x0FB2, 0x02C2, 0x0800, 0x0000, 0x02C1, 0 },
    { 0x0FB3, 0x02F4, 0x0800, 0x0000, 0x02F3, 0 },
    { 0x0FB4, 0x00BC, 0x0800, 0x0000, 0x00BB, 0 },
};

static const struct _AnimeInfo pmary_anim[3] = {
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0 },
    { 0x0FFB, 0x0043, 0x0800, 0x0000, 0x0042, 1 },
    { 0x0FFC, 0x0044, 0x0800, 0x0000, 0x0043, 1 },
};

static int HumanMRYInit() { /* Matching: K&R definition; called with arguments it ignores (the DWARF shows none). */
    return 0;
}

static void HumanMRYFunction(struct SubCharacter *this) {
    struct _AnimeInfo *aip;
    short id;

    switch (this->step) {
    case 0:
        id = shCharacterGetModelID(this);
        if (id == 0x109) {
            HumanMRYInit(this);
            aip = (struct _AnimeInfo *)&dmary_anim[1];
            shCharacterAnimeSet(this, 0, 0, aip, (int)shCharacterGetAnimeAdrForDrama(this, 1));
        }
        this->step++;
    case 1:
        break;
    }
}

/** Installs the MRY update function on a sub-character. @param scp the character. */
void shCharacterSetHumanMRYLow(struct SubCharacter *scp) {
    shCharacterSetFunction(scp, HumanMRYFunction);
}

/**
 * Plays drama (event) animation @p anime_id and its cluster animation on a MRY model.
 * @param scp the character. @param anime_id animation ID in the drama table.
 * @return 0, or -1 if @p scp isn't a MRY model.
 */
int shCharacterHumanMRYAnimeSet(struct SubCharacter *scp, int anime_id) {
    struct _AnimeInfo *aip;

    SCAnimeTypeSwitch(scp, 0);
    if (shCharacterGetModelID(scp) == 0x109) {
        aip = (struct _AnimeInfo *)&dmary_anim[anime_id - 0xFA0];
        shCharacterAnimeSet(scp, 0, 2, aip,
                            dmary_anime_adr_list[anime_id - 0xFA0] +
                                (int)shCharacterGetAnimeAdrForDrama(scp, anime_id - 0xFA0));
        shCharacterClusterAnimeSet(scp, dmary_clani_adr_list[anime_id - 0xFA0] +
                                            (int)shCharacterGetClusterAnimeAdr(scp));
        return 0;
    }
    return -1;
}

/**
 * Plays gameplay animation @p anime_id on a MRY model.
 * @param scp the character. @param anime_id animation ID in the gameplay table.
 * @return 0, or -1 if @p scp isn't a MRY model.
 */
int shCharacterHumanMRYAnimeSetP(struct SubCharacter *scp, int anime_id) {
    struct _AnimeInfo *aip;

    SCAnimeTypeSwitch(scp, 1);
    if (shCharacterGetModelID(scp) == 0x109) {
        aip = (struct _AnimeInfo *)&pmary_anim[anime_id - 0xFFA];
        shCharacterAnimeSet(scp, 0, 0, aip, (int)shCharacterGetAnimeAdrForPlay(scp));
        return 0;
    }
    return -1;
}
