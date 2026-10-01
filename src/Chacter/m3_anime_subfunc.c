/*
 * Animation helpers: set or clear the `untouchable` flag on groups of skeleton nodes (upper
 * body, lower body, all but the knees or neck) of James's (PJMS) and Maria's skeletons.
 */

#include "sh2.h"

/** Marks James's upper-body nodes untouchable. @param stp the skeleton's node array. */
void sh_PJMS_SetUntouchUpper(struct shSkelton *stp) {
    stp[40].untouchable = (void *)1;
    stp[39].untouchable = (void *)1;
    stp[38].untouchable = (void *)1;
    stp[37].untouchable = (void *)1;
    stp[36].untouchable = (void *)1;
    stp[35].untouchable = (void *)1;
    stp[34].untouchable = (void *)1;
    stp[33].untouchable = (void *)1;
    stp[32].untouchable = (void *)1;
    stp[31].untouchable = (void *)1;
    stp[28].untouchable = (void *)1;
    stp[27].untouchable = (void *)1;
    stp[22].untouchable = (void *)1;
    stp[21].untouchable = (void *)1;
    stp[20].untouchable = (void *)1;
    stp[19].untouchable = (void *)1;
    stp[18].untouchable = (void *)1;
    stp[15].untouchable = (void *)1;
    stp[12].untouchable = (void *)1;
    stp[11].untouchable = (void *)1;
    stp[10].untouchable = (void *)1;
    stp[9].untouchable = (void *)1;
    stp[8].untouchable = (void *)1;
    stp[3].untouchable = (void *)1;
    stp[2].untouchable = (void *)1;
    stp[0].untouchable = (void *)1;
}

/** Marks James's lower-body nodes untouchable. @param stp the skeleton's node array. */
void sh_PJMS_SetUntouchUnder(struct shSkelton *stp) {
    stp[30].untouchable = (void *)1;
    stp[29].untouchable = (void *)1;
    stp[26].untouchable = (void *)1;
    stp[25].untouchable = (void *)1;
    stp[24].untouchable = (void *)1;
    stp[23].untouchable = (void *)1;
    stp[17].untouchable = (void *)1;
    stp[16].untouchable = (void *)1;
    stp[14].untouchable = (void *)1;
    stp[13].untouchable = (void *)1;
    stp[7].untouchable = (void *)1;
    stp[6].untouchable = (void *)1;
    stp[5].untouchable = (void *)1;
    stp[4].untouchable = (void *)1;
    stp[1].untouchable = (void *)1;
}

/** Clears the untouchable mark on James's upper-body nodes. @param stp the skeleton's node array. */
void sh_PJMS_ResetUntouchUpper(struct shSkelton *stp) {
    stp[40].untouchable = 0;
    stp[39].untouchable = 0;
    stp[38].untouchable = 0;
    stp[37].untouchable = 0;
    stp[36].untouchable = 0;
    stp[35].untouchable = 0;
    stp[34].untouchable = 0;
    stp[33].untouchable = 0;
    stp[32].untouchable = 0;
    stp[31].untouchable = 0;
    stp[28].untouchable = 0;
    stp[27].untouchable = 0;
    stp[22].untouchable = 0;
    stp[21].untouchable = 0;
    stp[20].untouchable = 0;
    stp[19].untouchable = 0;
    stp[18].untouchable = 0;
    stp[15].untouchable = 0;
    stp[12].untouchable = 0;
    stp[11].untouchable = 0;
    stp[10].untouchable = 0;
    stp[9].untouchable = 0;
    stp[8].untouchable = 0;
    stp[3].untouchable = 0;
    stp[2].untouchable = 0;
    stp[0].untouchable = 0;
}

/** Clears the untouchable mark on James's lower-body nodes. @param stp the skeleton's node array. */
void sh_PJMS_ResetUntouchUnder(struct shSkelton *stp) {
    stp[30].untouchable = 0;
    stp[29].untouchable = 0;
    stp[26].untouchable = 0;
    stp[25].untouchable = 0;
    stp[24].untouchable = 0;
    stp[23].untouchable = 0;
    stp[17].untouchable = 0;
    stp[16].untouchable = 0;
    stp[14].untouchable = 0;
    stp[13].untouchable = 0;
    stp[7].untouchable = 0;
    stp[6].untouchable = 0;
    stp[5].untouchable = 0;
    stp[4].untouchable = 0;
    stp[1].untouchable = 0;
}

/** Marks every node of James's skeleton but nodes 6 and 7 (the knees) untouchable. */
void mizSetUntouchWithoutKnee(struct shSkelton *stp) {
    stp[40].untouchable = (void *)1;
    stp[39].untouchable = (void *)1;
    stp[38].untouchable = (void *)1;
    stp[37].untouchable = (void *)1;
    stp[36].untouchable = (void *)1;
    stp[35].untouchable = (void *)1;
    stp[34].untouchable = (void *)1;
    stp[33].untouchable = (void *)1;
    stp[32].untouchable = (void *)1;
    stp[31].untouchable = (void *)1;
    stp[30].untouchable = (void *)1;
    stp[29].untouchable = (void *)1;
    stp[28].untouchable = (void *)1;
    stp[27].untouchable = (void *)1;
    stp[26].untouchable = (void *)1;
    stp[25].untouchable = (void *)1;
    stp[24].untouchable = (void *)1;
    stp[23].untouchable = (void *)1;
    stp[22].untouchable = (void *)1;
    stp[21].untouchable = (void *)1;
    stp[20].untouchable = (void *)1;
    stp[19].untouchable = (void *)1;
    stp[18].untouchable = (void *)1;
    stp[17].untouchable = (void *)1;
    stp[16].untouchable = (void *)1;
    stp[15].untouchable = (void *)1;
    stp[14].untouchable = (void *)1;
    stp[13].untouchable = (void *)1;
    stp[12].untouchable = (void *)1;
    stp[11].untouchable = (void *)1;
    stp[10].untouchable = (void *)1;
    stp[9].untouchable = (void *)1;
    stp[8].untouchable = (void *)1;
    stp[5].untouchable = (void *)1;
    stp[4].untouchable = (void *)1;
    stp[3].untouchable = (void *)1;
    stp[2].untouchable = (void *)1;
    stp[1].untouchable = (void *)1;
    stp[0].untouchable = (void *)1;
}

/** Clears the untouchable mark set by mizSetUntouchWithoutKnee. */
void mizResetUntouchWithoutKnee(struct shSkelton *stp) {
    stp[40].untouchable = 0;
    stp[39].untouchable = 0;
    stp[38].untouchable = 0;
    stp[37].untouchable = 0;
    stp[36].untouchable = 0;
    stp[35].untouchable = 0;
    stp[34].untouchable = 0;
    stp[33].untouchable = 0;
    stp[32].untouchable = 0;
    stp[31].untouchable = 0;
    stp[30].untouchable = 0;
    stp[29].untouchable = 0;
    stp[28].untouchable = 0;
    stp[27].untouchable = 0;
    stp[26].untouchable = 0;
    stp[25].untouchable = 0;
    stp[24].untouchable = 0;
    stp[23].untouchable = 0;
    stp[22].untouchable = 0;
    stp[21].untouchable = 0;
    stp[20].untouchable = 0;
    stp[19].untouchable = 0;
    stp[18].untouchable = 0;
    stp[17].untouchable = 0;
    stp[16].untouchable = 0;
    stp[15].untouchable = 0;
    stp[14].untouchable = 0;
    stp[13].untouchable = 0;
    stp[12].untouchable = 0;
    stp[11].untouchable = 0;
    stp[10].untouchable = 0;
    stp[9].untouchable = 0;
    stp[8].untouchable = 0;
    stp[5].untouchable = 0;
    stp[4].untouchable = 0;
    stp[3].untouchable = 0;
    stp[2].untouchable = 0;
    stp[1].untouchable = 0;
    stp[0].untouchable = 0;
}

/** Marks every node of the chain at @p stp but nodes 6 and 9 (the neck) untouchable. */
void MariaSetUntouchWithoutNeck(struct shSkelton *stp) {
    int i;

    for (i = 0; stp->next; stp = stp->next, i++) {
        if (i != 6 && i != 9) {
            stp->untouchable = (void *)1;
        }
    }
}

/** Clears the untouchable mark set by MariaSetUntouchWithoutNeck. */
void MariaResetUntouchWithoutNeck(struct shSkelton *stp) {
    int i;

    for (i = 0; stp->next; stp = stp->next, i++) {
        if (i != 6 && i != 9) {
            stp->untouchable = 0;
        }
    }
}
