/*
 * AGL human character (Angela, model 0x107): animation tables, the update function,
 * and the drama and gameplay animation setters.
 * Also RAGL, a mirrored copy: it follows the character whose kind is 0x20 lower, reflected
 * across a wall plane chosen by room.
 */

#include "sh2.h"

static int dangela_anime_adr_list[32] = {
    0, 0, 0, 0x10098, 0x24EFC, 0x4A6DC, 0x72680, 0x90A98,
    0xB23D0, 0x24420, 0x44918, 0x6040C, 0x77B0C, 0x9D6D8, 0xBB8B8, 0xD12C4,
    0, 0x1A7AC, 0x40F24, 0x640DC, 0x85368, 0xA9008, 0xD0714, 0xE86AC,
    0x1027A8, 0x118164, 0x26598, 0x46C98, 0x62240, 0x83134, 0xBB67C, 0xDF29C
};

static int dangela_clani_adr_list[32] = {
    0, 0, 0, 0, 0x1A8, 0x3D8, 0x3044, 0x62B0,
    0x8924, 0, 0x1368, 0x1D1C, 0x314C, 0x466C, 0x4BBC, 0x5C28,
    0, 0x8CC, 0xD50, 0x105C, 0x37C0, 0x58FC, 0x7A3C, 0x9080,
    0xB298, 0xC2C0, 0, 0x2660, 0x5694, 0x6F74, 0xA674, 0xB250
};

static const struct _AnimeInfo dangela_anim[32] = {
    { 0, 0, 0, 0, 0, 0 },
    { 0xDAD, 1, 0x800, 0, 0, 1 },
    { 0xDAE, 0x373, 0x800, 0, 0x372, 1 },
    { 0xDAF, 0x1B6, 0x800, 0, 0x1B5, 0 },
    { 0xDB0, 0x31F, 0x800, 0, 0x31E, 0 },
    { 0xDB1, 0x352, 0x800, 0, 0x351, 0 },
    { 0xDB2, 0x283, 0x800, 0, 0x283, 0 },
    { 0xDB3, 0x2C9, 0x800, 0, 0x2C8, 0 },
    { 0xDB4, 0x483, 0x800, 0, 0x482, 0 },
    { 0xDB5, 0x2AC, 0x800, 0, 0x2AB, 0 },
    { 0xDB6, 0x297, 0x800, 0, 0x296, 0 },
    { 0xDB7, 0x1F0, 0x800, 0, 0x1EF, 0 },
    { 0xDB8, 0x321, 0x800, 0, 0x320, 0 },
    { 0xDB9, 0x280, 0x800, 0, 0x27F, 0 },
    { 0xDBA, 0x1C9, 0x800, 0, 0x1C8, 0 },
    { 0xDBB, 0x110, 0x800, 0, 0x10F, 0 },
    { 0xDBC, 0x230, 0x800, 0, 0x22F, 0 },
    { 0xDBD, 0x334, 0x800, 0, 0x333, 0 },
    { 0xDBE, 0x2EA, 0x800, 0, 0x2E9, 0 },
    { 0xDBF, 0x2C0, 0x800, 0, 0x2BF, 0 },
    { 0xDC0, 0x2F7, 0x800, 0, 0x2F6, 0 },
    { 0xDC1, 0x349, 0x800, 0, 0x348, 0 },
    { 0xDC2, 0x1FA, 0x800, 0, 0x1F9, 0 },
    { 0xDC3, 0x227, 0x800, 0, 0x226, 0 },
    { 0xDC4, 0x1C7, 0x800, 0, 0x1C6, 0 },
    { 0xDC5, 0x1AD, 0x800, 0, 0x1AC, 0 },
    { 0xDC6, 0x3B1, 0x800, 0, 0x3B0, 0 },
    { 0xDC7, 0x29F, 0x800, 0, 0x29E, 0 },
    { 0xDC8, 0x369, 0x800, 0, 0x368, 0 },
    { 0xDC9, 0x520, 0x800, 0, 0x51F, 0 },
    { 0xDCA, 0x35C, 0x800, 0, 0x35B, 0 },
    { 0xDCB, 0x4FE, 0x800, 0, 0x4FD, 0 },
};

static const struct _AnimeInfo pangela_anim[6] = {
    { 0, 0, 0, 0, 0, 0 },
    { 0xF3D, 0x40, 0x800, 0, 0x3F, 1 },
    { 0xF3E, 0x5C, 0x800, 0x40, 0x9B, 1 },
    { 0xF3F, 0x45, 0x800, 0, 0x44, 1 },
    { 0xF40, 0x4A, 0x80, 0, 0x49, 1 },
    { 0xF41, 0x5B, 0x80, 0x4A, 0xA4, 0 },
};

static float wall_pos;
static int mirror_mode;

static void HumanAGLFunction(struct SubCharacter *this) {
    struct _AnimeInfo *aip;
    short id;

    switch (this->step) {
    case 0:
        id = shCharacterGetModelID(this);
        if (id == 0x107) {
            aip = (struct _AnimeInfo *)&dangela_anim[1];
            shCharacterAnimeSet(this, 0, 0, aip, (int)shCharacterGetAnimeAdrForDrama(this, 1));
        }
        this->step++;
    case 1:
        break;
    }
}

/** Installs the AGL update function on a sub-character. @param scp the character. */
void shCharacterSetHumanAGLLow(struct SubCharacter *scp) {
    shCharacterSetFunction(scp, HumanAGLFunction);
}

/**
 * Plays drama (event) animation @p anime_id and its cluster animation on a AGL model.
 * @param scp the character. @param anime_id animation ID in the drama table.
 * @return 0, or -1 if @p scp isn't a AGL model.
 */
int shCharacterHumanAGLAnimeSet(struct SubCharacter *scp, int anime_id) {
    struct _AnimeInfo *aip;

    SCAnimeTypeSwitch(scp, 0);
    if (shCharacterGetModelID(scp) == 0x107) {
        aip = (struct _AnimeInfo *)&dangela_anim[anime_id - 0xDAC];
        shCharacterAnimeSet(scp, 0, 2, aip,
                            dangela_anime_adr_list[anime_id - 0xDAC] +
                                (int)shCharacterGetAnimeAdrForDrama(scp, anime_id - 0xDAC));
        shCharacterClusterAnimeSet(scp, dangela_clani_adr_list[anime_id - 0xDAC] +
                                            (int)shCharacterGetClusterAnimeAdr(scp));
        return 0;
    }
    return -1;
}

/**
 * Plays gameplay animation @p anime_id on a AGL model. Some IDs also set the eye/center height.
 * @param scp the character. @param anime_id animation ID in the gameplay table.
 * @return 0, or -1 if @p scp isn't a AGL model.
 */
int shCharacterHumanAGLAnimeSetP(struct SubCharacter *scp, int anime_id) {
    struct _AnimeInfo *aip;

    SCAnimeTypeSwitch(scp, 1);
    if (shCharacterGetModelID(scp) == 0x107) {
        switch (anime_id) {
        case 0xF3D:
            scp->center_y = scp->eye_y = 1986.6311f;
            break;
        case 0xF3E:
            scp->center_y = scp->eye_y = 1755.6741f;
            break;
        case 0xF3F:
            scp->center_y = scp->eye_y = -500.0f;
            break;
        case 0xF40:
        case 0xF41:
            break;
        }
        aip = (struct _AnimeInfo *)&pangela_anim[anime_id - 0xF3C];
        shCharacterAnimeSet(scp, 0, 0, aip, (int)shCharacterGetAnimeAdrForPlay(scp));
        return 0;
    }
    return -1;
}

static int HumanRAGLInit(struct SubCharacter *scp) {
    SCAnimeTypeSwitch(scp, 0);
    scp->model_type = 1;
    switch (RoomNameJms()) {
    case 1:
        mirror_mode = 0;
        wall_pos = -20000.0f;
        break;
    case 0x24:
        mirror_mode = 2;
        wall_pos = -99995.0f;
        break;
    }
    return 0;
}

static void HumanRAGLFunction(struct SubCharacter *this) {
    struct SubCharacter *scp;

    switch (this->step) {
    case 0:
        HumanRAGLInit(this);
        this->step++;
        break;
    case 1:
        scp = shCharacterGetSubCharacter(this->kind - 0x20, -1);
        if (scp) {
            switch (mirror_mode) {
            case 0:
                this->pos.x = wall_pos + (wall_pos - scp->pos.x);
                this->pos.y = scp->pos.y;
                this->pos.z = scp->pos.z;
                this->rot.y = -scp->rot.y;
                break;
            case 1:
                this->pos.x = scp->pos.x;
                this->pos.y = wall_pos + (wall_pos - scp->pos.y);
                this->pos.z = scp->pos.z;
                this->rot.y = scp->rot.y;
                this->rot.z = 3.1415927f;
                break;
            case 2:
                this->pos.x = scp->pos.x;
                this->pos.y = scp->pos.y;
                this->pos.z = wall_pos + (wall_pos - scp->pos.z);
                if (scp->rot.y >= 0.0f) {
                    this->rot.y = 3.1415927f - scp->rot.y;
                } else {
                    this->rot.y = -3.1415927f - scp->rot.y;
                }
                break;
            }
        }
        break;
    }
}

/** Installs the RAGL (mirrored copy) update function on a sub-character. @param scp the character. */
void shCharacterSetHumanRAGLLow(struct SubCharacter *scp) {
    shCharacterSetFunction(scp, HumanRAGLFunction);
}
