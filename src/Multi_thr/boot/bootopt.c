/*
 * bootopt.c: parses the executable's command line against the boot option
 * table (bootoptitem.c).
 */

#include "sh2.h"
#include "libc/unistd.h"

static int key_index(int key) {
    if (key >= 'a' && key <= 'z') {
        key = key - 'a' + 26;
    } else if (key >= 'A' && key <= 'Z') {
        key = key - 'A';
    } else {
        key = -1;
    }
    return key;
}

/**
 * Parses the options in `argc`/`argv` with getopt(): every option in BootOptItemList is marked as
 * set and its variable is set from its parser, from the argument, or to 1.
 */
void BootOptGet(int argc, char **argv) {
    char keylist[256];
    struct BootOptItem *keyitem[62] = { NULL };
    char *dst;
    int len;
    int rem;
    int key;
    int idx;
    struct BootOptItem *item;
    int (*get)(char *);
    int var;
    char *param;

    rem = sizeof(keylist) - 1;
    item = BootOptItemList;
    dst = keylist;
    for (; rem > 0 && item->var != NULL; item++) {
        key = key_index(item->key[0]);
        if (key >= 0) {
            keyitem[key] = item;
            len = UtilStrCpyL(dst, item->key, rem);
            dst += len;
            rem -= len;
        }
    }

    for (key = getopt(argc, argv, keylist); key != -1; key = getopt(argc, argv, keylist)) {
        param = optarg;
        idx = key_index(key);
        if (idx >= 0 && (item = keyitem[idx]) != NULL) {
            if ((get = item->get) != NULL) {
                var = get(param);
            } else if (item->key[1] == ':') {
                var = *(int *)param;
            } else {
                var = 1;
            }
            item->set = 1;
            if (item->var != NULL) {
                *(int *)item->var = var;
            }
        }
    }

    while (optind < argc) {
        optind++;
    }
}
