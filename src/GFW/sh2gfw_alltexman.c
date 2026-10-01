/*
 * Texture transfer bookkeeping (GFW): the texture sync manager's transfer counter.
 */
#include "sh2.h"

/** Resets the count of texture transfers in flight. */
void sh2gfw_init_Trans(struct sh2gfw_ALLTEXSYNC_MAN *pATSM) {
    pATSM->trans_NOW_num = 0;
}
