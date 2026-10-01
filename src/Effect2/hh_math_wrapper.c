/* hh_math_wrapper.c: the math functions the HH effect code uses: sqrt.s, and sine and cosine as
 * Taylor polynomials (accurate near 0; callers pass angles in [-PI, PI]). */
#include "sh2.h"

/**
 * Returns the square root of Value, with the FPU's sqrt.s (an inline asm block in the original).
 */
float HH_MathWrapper_Sqrtf(float Value) {
    asm {
        sqrt.s Value, Value
    }
    return Value;
}

/** Returns sin(x) from its Taylor polynomial up to x^9. */
float HH_MathWrapper_Sinf(float x) {
    float x2;
    float x3;
    float x5;
    float x7;
    float x9;
    float result;

    x2 = x * x;
    x3 = x2 * x;
    x5 = x3 * x2;
    x7 = x5 * x2;
    x9 = x7 * x2;
    result = x + (-1.0f / 6.0f) * x3 + (1.0f / 120.0f) * x5 + (-1.0f / 5040.0f) * x7 + (1.0f / 362880.0f) * x9;
    return result;
}

/** Returns cos(x) from its Taylor polynomial up to x^8. */
float HH_MathWrapper_Cosf(float x) {
    float x2;
    float x4;
    float x6;
    float x8;
    float result;

    x2 = x * x;
    x4 = x2 * x2;
    x6 = x4 * x2;
    x8 = x6 * x2;
    result = 1.0f + (-1.0f / 2.0f) * x2 + (1.0f / 24.0f) * x4 + (-1.0f / 720.0f) * x6 + (1.0f / 40320.0f) * x8;
    return result;
}
