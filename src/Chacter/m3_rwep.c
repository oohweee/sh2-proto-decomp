/*
 * WeaponR character: a reflected weapon model (model_type 1) with no behaviour of its own.
 */

#include "sh2.h"

static void WeaponRFunction(struct SubCharacter *this) {
    switch (this->step) {
    case 0:
        this->model_type = 1;
        this->step++;
    case 1:
        break;
    }
}

/** Installs the WeaponR update function on a sub-character. @param scp the character. */
void shCharacterSetWeaponRLow(struct SubCharacter *scp) {
    shCharacterSetFunction(scp, WeaponRFunction);
}
