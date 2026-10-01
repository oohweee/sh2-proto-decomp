#ifndef SDK_LIBMC_H
#define SDK_LIBMC_H

/*
 * libmc, the EE library for memory cards (the binary's "PsIIlibmc   2240" stamp): the functions
 * the game calls. The library is linked as assembly (no DWARF).
 *
 * Provenance: the function names are the binary's symbols. The parameter and return types are
 * inferred from the game's call sites; the parameter names are ours. sceMcTblGetDir's layout is
 * the DWARF's (sh2/types.h). No SDK header or other SDK file was used.
 */

#include "sh2/types.h"

int sceMcClose(int fd);
int sceMcDelete(int port, int slot, const char *path);
int sceMcFormat(int port, int slot);
int sceMcGetDir(int port, int slot, const char *path, unsigned int flags, int max, sceMcTblGetDir *out);
int sceMcGetEntSpace(int port, int slot, const char *path);
int sceMcGetInfo(int port, int slot, int *type, int *free_space, int *formatted);
int sceMcInit(void);
int sceMcMkdir(int port, int slot, const char *path);
int sceMcOpen(int port, int slot, const char *path, int mode);
int sceMcRead(int fd, void *buf, int size);
int sceMcSeek(int fd, int offset, int whence);
int sceMcSync(int mode, int *cmd, int *result);
int sceMcWrite(int fd, const void *buf, int size);

#endif
