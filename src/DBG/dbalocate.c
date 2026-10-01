/*
 * dbalocate.c: debug memory allocation.
 *
 * dbAllocatePrintf hands out the fixed printf buffer at 0x2000000, just past the
 * 32 MiB EE RAM the retail machine has (debug units have 128 MiB).
 * Requests larger than UPPER_SIZE_PRINTF are a programming error.
 *
 * Matching: the assert message bakes in "dbalocate.c:28", so the assert has to
 * stay on line 28 of this file.
 */

#include "sh2.h"

/* Largest printf buffer a caller may request. */
#define UPPER_SIZE_PRINTF 0x10000

/* Address of the printf buffer. */
#define DB_PRINTF_BUFFER ((void *)0x2000000)

/**
 * Returns the printf buffer. `require_size` is only checked, never used to size
 * anything: the buffer is always the same, and callers must not keep two
 * allocations alive at once.
 */
void *dbAllocatePrintf(int require_size) {
    void *p;

    assert_dw(require_size<=UPPER_SIZE_PRINTF);
    p = DB_PRINTF_BUFFER;
    return p;
}
