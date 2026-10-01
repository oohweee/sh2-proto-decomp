/*
 * Shadow packet buffers (sh2shd): a write cursor over a packet buffer.
 */
#include "sh2.h"

/** Sets up a packet buffer on buf, empty. */
void sh2shd_init_packet_buf(struct SHADOW_PACKET_BUF *packet, union Q_WORDDATA *buf) {
    packet->head = buf;
    packet->curr = buf;
}

/** Empties a packet buffer. */
void sh2shd_reset_packet_buf(struct SHADOW_PACKET_BUF *packet) {
    packet->curr = packet->head;
}
