/*
 * TY23 enemy object: a stay (static) model registered as a world-screen object, with an
 * enemy AI entry chosen by enTransID(kind).
 * TY2 and TY3: static variants of the Creeper (suspected; docs/characters.md).
 */

#include "sh2.h"

/*
 * Matching: the original calls shCharacterWorldScreenObjectSetNew without a prototype in scope
 * (the float is promoted to double with fptodp and passed in a1). sh2.h has the
 * prototype, and MWCC rejects a file-scope K&R redeclaration because of the float
 * parameter, so the old-style declaration sits at block scope.
 */

static int EnemyTY23Init(void) {
    return 0;
}

static void EnemyTY23Function(struct SubCharacter *this) {
    float scale;
    struct EnLOCAL_DATA *dp;
    void shCharacterWorldScreenObjectSetNew();

    switch (this->step) {
    case 0:
        if (this->battle.status & 0x400) {
            EnemyTY23Init();
            SCStayModelSwitch(this, 1);
            scale = 1.0f;
            shCharacterWorldScreenObjectSetNew(this, scale);
            dp = enEntryEnemy(enTransID(this->kind));
            this->enemy_p = dp;
            enInitData(dp, this);
            this->eye_y = this->center_y = this->pos.y - 50.0f;
            this->step++;
        }
    case 1:
        break;
    }
}

/** Installs the TY23 update function on a sub-character. @param scp the character. */
void shCharacterSetEnemyTY23Low(struct SubCharacter *scp) {
    shCharacterSetFunction(scp, EnemyTY23Function);
}
