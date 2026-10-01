#ifndef LIBC_MATH_H
#define LIBC_MATH_H

/*
 * <math.h>: the math functions the game calls from its C library (linked as assembly: lib/sinf,
 * lib/__ieee754_*...; no DWARF). ISO C signatures; only what the game uses.
 *
 * Provenance: the function names are the binary's symbols, the prototypes are the C standard's.
 * No SDK or C-library header was used.
 *
 * asm_libm.h defines fabsf (and fmaxf, fminf) as inline assembly; files that include it get the
 * single instruction, the others (anime.c, lake_wave.c...) call the library's fabsf.
 */

double ceil(double x);
double cos(double x);
double fabs(double x);
double floor(double x);
double sin(double x);
double sqrt(double x);

float acosf(float x);
float asinf(float x);
float atan2f(float y, float x);
float cosf(float x);
float fabsf(float x);
float fmodf(float x, float y);
float powf(float x, float y);
float sinf(float x);
float sqrtf(float x);

#endif
