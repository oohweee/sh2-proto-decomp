/* ef_packet.c: the VIF1 packet the effects are built into. */
#include "sh2.h"
#include "sdk/libvifpk.h"

static sceVif1Packet efctVif1Packet;

/** Initializes the effect packet on the buffer at packet; returns the packet. */
sceVif1Packet *shEfctPkInit(void *packet) {
    sceVif1PkInit(&efctVif1Packet, packet);
    return &efctVif1Packet;
}

/** Empties the effect packet (at the start of a frame); returns the packet. */
sceVif1Packet *shEfctPkReset(void) {
    sceVif1PkReset(&efctVif1Packet);
    return &efctVif1Packet;
}

/**
 * Starts an effect task's data: a DMA cnt tag, then an open DIRECT (GIF) code. Returns the packet.
 */
sceVif1Packet *shEfctPkTaskHead(void) {
    sceVif1PkCnt(&efctVif1Packet, 0);
    sceVif1PkOpenDirectCode(&efctVif1Packet, 0);
    return &efctVif1Packet;
}

/** Ends an effect task's data: closes the DIRECT code. Returns the packet. */
sceVif1Packet *shEfctPkTaskTail(void) {
    sceVif1PkCloseDirectCode(&efctVif1Packet);
    return &efctVif1Packet;
}

/** Terminates the packet and returns its start, the address to kick with d1cSend. */
void *shEfctPkGetKickAddrByd1cSend(void) {
    sceVif1PkEnd(&efctVif1Packet, 0);
    sceVif1PkTerminate(&efctVif1Packet);
    return efctVif1Packet.pBase;
}
