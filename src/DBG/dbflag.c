/*
 * dbflag.c: debug flags.
 *
 * execEnv_debug_flag is a bit set of debug features. dbFlagReserve() hands
 * the bits out, dbFlag() tests them and dbFlagSet() turns them on.
 * The flags themselves are set from the boot options.
 */

#include "sh2.h"

/* "<file>:<line>> " prefix of the verbose messages. */
#define VB(s) __FILE__ ":" SH_STRINGIFY(__LINE__) "> " s

/*
 * Shown for a reserved bit without an explanation. A named constant (not an
 * inline literal): it sits first in the original .rodata.
 */
static const char no_message[] = "(no message: why?)";

/* Who reserved each bit. */
static char *dbflag_explain[32] = { 0 };
/* Debug flag bits handed out so far. */
static unsigned int dbflag_reserved;

/**
 * Reserve the debug flag bits in `flag`.
 *
 * Each bit of execEnv_debug_flag (see dbFlag/dbFlagSet) belongs to one
 * debug feature. Code that wants a bit reserves it once at startup with a
 * short explanation so that clashes between two features are reported:
 *
 *   returns 1 when all the bits in `flag` were free (they are now reserved
 *   and remembered with `explain_message`),
 *   returns 0 when any of them was already reserved; nothing is reserved
 *   then, and the current owners of every reserved bit are listed.
 *
 * Messages go through verbose() at increasing verbosity levels.
 */
int dbFlagReserve(unsigned int flag, char *explain_message) {
    unsigned int check;
    char **explain;

    if (dbflag_reserved & flag) {
        verbose(1, VB("can't reserved debug flag : %08x : %s\n"), flag, explain_message);

        verbose(2, VB("alreday reserved          : %08x\n"), dbflag_reserved);
        for (check = 1, explain = dbflag_explain; check; check <<= 1, explain++) {
            if (check & dbflag_reserved) {
                /* report the owner of this bit */


                verbose(3, VB("flag report               : %08x : %s\n"), check,
                        *explain ? *explain : no_message);
            }
        }
        return 0;
    }
    for (check = 1, explain = dbflag_explain; check; check <<= 1, explain++) {
        if (check & flag) {
            dbflag_reserved |= check;
            *explain = explain_message;
        }
    }
    verbose(4, VB("reserved debug flag : %08x : %s\n"), flag, explain_message);
    return 1;
}

/** Returns the bits of `flag` that are set in execEnv_debug_flag. */
unsigned int dbFlag(unsigned int flag) {
    return execEnv_debug_flag & flag;
}

/** Sets the bits of `flag` in execEnv_debug_flag. */
void dbFlagSet(unsigned int flag) {
    execEnv_debug_flag |= flag;
}
