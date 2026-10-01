/*
 * NIK object character: picks a start animation and frame per instance id, registers its
 * enemy AI, and (debug pad port 7) prints its current animation frame.
 * The hanging meat in the Eddie boss fight (verified; docs/characters.md).
 */

#include "sh2.h"
#include "asm_helpers.h"
#include "libc/stdio.h"

static const struct _AnimeInfo nik_anim[4] = {
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0 },
    { 0x206D, 0x0092, 0x0800, 0x0000, 0x0091, 1 },
    { 0x206E, 0x00C8, 0x0800, 0x0092, 0x0159, 1 },
    { 0x206F, 0x0096, 0x0800, 0x015A, 0x01EF, 1 },
};

static int ObjectNIKInit(void) {
    return 0;
}

static void ObjectNIKFunction(struct SubCharacter *this) {
    struct _AnimeInfo *aip;
    short init_frame[11] = { 0x34, 0x13, 0, 0x63, 0x17, 7, 0x47, 0xA, 0x37, 0, 0x1B };
    short init_anime[11] = { 2, 1, 2, 0, 0, 0, 2, 0, 0, 2, 1 };
    float pos[4];
    float rot[4];
    struct EnLOCAL_DATA *dp;
    struct SubCharacterDisp *scp_d;
    char buf[128];

    switch (this->step) {
    case 0:
        vcopy(&this->pos, pos);
        vcopy(&this->rot, rot);
        ObjectNIKInit();
        SCAnimeTypeSwitch(this, 1);
        aip = (struct _AnimeInfo *)&nik_anim[init_anime[this->id] + 1];
        shCharacterAnimeSet(this, 0, 2, aip, (int)shCharacterGetAnimeAdrForPlay(this));
        shCharacterAnimeFrameSet(this, aip->start + init_frame[this->id]);
        if (this->status & 4) {
            if ((dp = enEntryEnemy(0xC)) != NULL) {
                this->enemy_p = dp;
                enInitData(dp, this);
            } else {
                this->enemy_p = NULL;
            }
        }
        this->step++;
        break;
    case 1:
        if (shPadGetPort() == 7) {
            scp_d = (struct SubCharacterDisp *)this;
            sprintf(buf, "Frame %d", scp_d->anime.cur_frame.x);
            shDBG_print_string(buf, 8, this->id * 8 + 200);
        }
        break;
    }
}

/** Installs the NIK update function on a sub-character. @param scp the character. */
void shCharacterSetObjectNIKLow(struct SubCharacter *scp) {
    shCharacterSetFunction(scp, ObjectNIKFunction);
}
