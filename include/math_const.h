#ifndef MATH_CONST_H
#define MATH_CONST_H

/*
 * Math constants. Where the original defined PI is unknown (a macro leaves no DWARF), so this
 * header and its name are ours. Macros only: including it defines no functions, so it can't
 * change MWCC's float-constant order (docs/toolchain.md, "Root cause").
 */

/* pi as a float (0x40490FDB) */
#define PI 3.1415927f

#endif
