/*
 * iopload.c: IOP module loading and IOP reboot, from the IOP module path.
 */

#include "sh2.h"
#include "lib/sh_kernel.h"

/**
 * Reboots the IOP with image `imgfile`; with no image, only enables printf. Returns non-zero on
 * success.
 */
int shIopReplaceMod(char *imgfile) {
    int ret = 0;
    char filename[256];

    if (imgfile != NULL) {
        if (shPathMakeIop(filename, imgfile)) {
            ret = shSifRebootIopR(filename);
        }
    } else {
        printf_enable();
    }
    return ret;
}

/**
 * Loads IOP module `module`; nothing for NULL or "". Returns the load result (undefined when
 * nothing was loaded).
 */
int shIopLoadMod(char *module) {
    int ret;
    char filename[256];

    if (module != NULL && *module != '\0') {
        if (shPathMakeIop(filename, module)) {
            ret = shSifLoadModuleR(filename, 0, "");
        }
    }
    return ret;
}
