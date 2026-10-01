/*
 * Provisional ("kari") collision data table: CLDadr holds up to 16 collision data blocks.
 */

#include "sh2.h"

unsigned char *CLDadr[16];
static int atari_data_num;
static int atari_head_index;

/** Clears the collision data table and its counters. */
void kari_init_colidata(void) {
    int i;

    atari_head_index = 0;
    atari_data_num = 0;
    for (i = 0; i < 16; i++) {
        CLDadr[i] = 0;
    }
}
