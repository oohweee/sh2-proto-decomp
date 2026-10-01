#ifndef LIB_LIBSHPAD_H
#define LIB_LIBSHPAD_H

/*
 * libShPad, the game's controller library over libpad. It is linked as assembly (the object
 * lib/libShPadEnable00 in config/main.yaml) and has no DWARF, so its original header is unknown
 * and the name of this one is ours. The function names are the binary's symbols; the signatures
 * are inferred from the call sites (Multi_thr/pad, SH2_common/pad.c, DBG, and the vibration in
 * m3_play.c and m3_boat.c), and the parameter names are ours or the callers' (pow0, pow1).
 */

int libShPadEnable00(void);
int libShPadEnable10(void);
int libShPadInit(void);
int libShPadRead(int port, int slot, char *data);
int libShPadSend(int port, int slot, unsigned short pow0, unsigned short pow1);
int libShPadSetMode(int port, int slot, int mode, int lock, int act, int press);
int libShPadStart(int port, int slot);
int libShPadTrans(void);

#endif
