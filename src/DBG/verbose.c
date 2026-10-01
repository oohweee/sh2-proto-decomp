/*
 * verbose.c: verbose(), the game's leveled debug printf.
 */

#include "sh2.h"
#include "libc/stdarg.h"
#include "libc/stdio.h"

/**
 * printf()s `format` when `level` (clamped to 1..9) is within execEnv_verbose_level. Halts on a
 * message longer than 1 KB. @param level verbosity level of the message @param format printf format
 */
int verbose(int level, char *format, ...) {
    int len;
    char buf[1024];
    char *argp;

    va_start(argp, format);
    if (level < 1) {
        level = 1;
    }
    if (level >= 9) {
        level = 9;
    }
    if (level <= execEnv_verbose_level) {
        len = vsprintf(buf, format, argp);
        if (len >= sizeof(buf)) {
            printf("verbose.c:28> verbose strbuf overflow!!\n"
                   "verbose.c:29> %s"
                   "verbose.c:30> halted by error\n", buf);
            while (1) {}
        }
        printf("%s", buf);
    }
}
