#ifndef SDK_LIBCDVD_H
#define SDK_LIBCDVD_H

/*
 * libcdvd, the EE library for the disc drive (the binary's "PsIIlibcdvd 2240" stamp): the
 * functions and constants the game uses. The library is linked as assembly (no DWARF).
 *
 * Provenance: the function names are the binary's symbols. The parameter and return types are
 * inferred from the game's call sites; the parameter names are ours. The disc types' names
 * (SCECdNODISC...) are the ones ps2sdk's libcdvd uses; the other constant names (CDVD_*) are
 * ours. Every value agrees with the game's code. The layouts of sceCdRMode, sceCdCLOCK and
 * sceCdlFILE are the DWARF's (sh2/types.h). No SDK header or other SDK file was used.
 */

#include "sh2/types.h"

/* sceCdGetDiskType() */
#define SCECdNODISC 0x00
#define SCECdDETCT 0x01
#define SCECdPSCD 0x10
#define SCECdPSCDDA 0x11
#define SCECdPS2CD 0x12
#define SCECdPS2CDDA 0x13
#define SCECdPS2DVD 0x14
#define SCECdCDDA 0xFD
#define SCECdDVDV 0xFE
#define CDVD_TYPE_ILLEGAL 0xFF /* the game asserts "illegal media." on it */

/* sceCdMmode() */
#define CDVD_MMODE_CD 1
#define CDVD_MMODE_DVD 2

/*
 * sceCdDiskReady(): the drive is ready. fsCmdCdCheckDisk2 (filecmd.c) only goes on to read the
 * media when the result is 2; the library's code returns 6 when its command can't be sent.
 */
#define CDVD_READY 2

void *sceCdCallback(void (*fn)(void));
int sceCdDiskReady(int mode);
int sceCdGetDiskType(void);
int sceCdGetError(void);
int sceCdInit(int mode);
int sceCdInitEeCB(int priority, void *stack, int stack_size);
int sceCdMmode(int media);
int sceCdRead(unsigned int lsn, unsigned int sectors, void *buf, sceCdRMode *mode);
int sceCdReadClock(sceCdCLOCK *clock);
int sceCdSearchFile(sceCdlFILE *file, const char *path);
int sceCdSeek(unsigned int lsn);
int sceCdStatus(void);
int sceCdSync(int mode);
int sceCdTrayReq(int mode, unsigned int *count);

#endif
