/*
 * wrap_printf.c: sets up the interrupt-safe printf buffer.
 */

#include "sh2.h"
#include "lib/sh_kernel.h"

/** Gives printf a 64 KB buffer from the debug printf area. */
void wrap_printf_init(void) {
    void *iprintf_buf;

    iprintf_buf = dbAllocatePrintf(0x10000);
    printf_init(iprintf_buf, 0x10000, 1);
}
