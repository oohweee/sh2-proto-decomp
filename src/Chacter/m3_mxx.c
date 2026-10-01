/*
 * MXX human character (model 0x10A): plays its first drama animation when created.
 * Mary, in the ending scenes with her letter (suspected; docs/characters.md).
 */

#include "sh2.h"

static const struct _AnimeInfo dmxx_anim[11] = {
    { 0x0000, 0x0000, 0x0000, 0, 0x0000, 0 },
    { 0x1005, 0x0009, 0x0800, 0, 0x0000, 1 },
    { 0x1006, 0x043D, 0x0800, 0, 0x043C, 0 },
    { 0x1007, 0x01FA, 0x0800, 0, 0x01F9, 0 },
    { 0x1008, 0x0286, 0x0800, 0, 0x0285, 0 },
    { 0x1009, 0x0366, 0x0800, 0, 0x0365, 0 },
    { 0x100A, 0x044C, 0x0800, 0, 0x044B, 0 },
    { 0x100B, 0x026F, 0x0800, 0, 0x026E, 0 },
    { 0x100C, 0x02C6, 0x0800, 0, 0x02C5, 0 },
    { 0x100D, 0x035C, 0x0800, 0, 0x035B, 0 },
    { 0x100E, 0x0384, 0x0800, 0, 0x0383, 0 },
};

static int HumanMXXInit(struct SubCharacter *scp) {
    scp->kind = 0x10A;
    SCAnimeTypeSwitch(scp, 0);
    return 0;
}

static void HumanMXXFunction(struct SubCharacter *this) {
    struct _AnimeInfo *aip;
    short id;

    switch (this->step) {
    case 0:
        id = shCharacterGetModelID(this);
        if (id == 0x10A) {
            HumanMXXInit(this);
            aip = (struct _AnimeInfo *)&dmxx_anim[1];
            shCharacterAnimeSet(this, 0, 0, aip, (int)shCharacterGetAnimeAdrForDrama(this, 1));
        }
        this->step++;
    case 1:
        break;
    }
}

/** Installs the MXX update function on a sub-character. @param scp the character. */
void shCharacterSetHumanMXXLow(struct SubCharacter *scp) {
    shCharacterSetFunction(scp, HumanMXXFunction);
}
