/*
 * Stay objects: static background models registered as world-screen objects.
 */

#include "sh2.h"

static int shCharacterStayObjectInit(struct SubCharacter *scp) {
    SCStayModelSwitch(scp, 1);
    return 0;
}

static void StayObjectFunction(struct SubCharacter *this) {
    float scale;

    switch (this->step) {
    case 0:
        shCharacterStayObjectInit(this);
        scale = 1.0f;
        shCharacterWorldScreenObjectSetNew(this, scale);
        this->eye_y = this->center_y = this->pos.y - 150.0f;
        this->step++;
    case 1:
        break;
    }
}

/** Installs the stay-object update function on a sub-character. @param scp the character. */
void shCharacterSetStayObjectLow(struct SubCharacter *scp) {
    shCharacterSetFunction(scp, StayObjectFunction);
}
