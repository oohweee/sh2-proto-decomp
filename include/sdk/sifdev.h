#ifndef SDK_SIFDEV_H
#define SDK_SIFDEV_H

/*
 * The EE side of the IOP's services that the game uses: file I/O through the IOP, module loading,
 * IOP reboot and heap. The library code is linked as assembly, without DWARF.
 *
 * Provenance: the function names are the binary's symbols. The parameter and return types are
 * inferred from the game's call sites; the parameter names and the constant names (FIO_*) are
 * ours, with the values the game passes. No SDK header or other SDK file was used.
 */

/* sceOpen() flags and sceLseek() origins */
#define FIO_RDONLY 1
#define FIO_SEEK_END 2

int sceClose(int fd);
int sceFsReset(void);
int sceLseek(int fd, int offset, int whence);
int sceOpen(const char *path, int flags);
int sceRead(int fd, void *buf, int size);

void *sceSifAllocIopHeap(int size);
void sceSifLoadFileReset(void);
int sceSifLoadModule(const char *path, int arg_size, const char *args);
int sceSifRebootIop(const char *image);
int sceSifSyncIop(void);

#endif
