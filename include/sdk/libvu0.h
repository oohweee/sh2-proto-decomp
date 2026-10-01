#ifndef SDK_LIBVU0_H
#define SDK_LIBVU0_H

/*
 * The EE library of VU0 macro-mode vector and matrix functions (sceVu0*): the ones the game
 * calls. The library is linked as assembly (no DWARF); these functions are one object in the
 * binary, sceVu0... to sceVpu0Reset.
 *
 * Provenance: the function names are the binary's symbols. The parameter and return types are
 * inferred from the game's call sites; the parameter names are ours (`dst` is the output). Vectors
 * and matrices are written as the plain arrays they are (`float v[4]`, `float m[4][4]`); as
 * parameters both are pointers. No SDK header or other SDK file was used.
 */

void sceVpu0Reset(void);

void sceVu0AddVector(float dst[4], float a[4], float b[4]);
void sceVu0ApplyMatrix(float dst[4], float m[4][4], float v[4]);
void sceVu0ClampVector(float dst[4], float v[4], float min, float max);
void sceVu0CopyMatrix(float dst[4][4], float m[4][4]);
void sceVu0CopyVector(float dst[4], float v[4]);
void sceVu0DropShadowMatrix(float dst[4][4], float *light, float a, float b, float c, int mode);
void sceVu0FTOI0Vector(int dst[4], float v[4]);
float sceVu0InnerProduct(float a[4], float b[4]);
void sceVu0InterVector(float dst[4], float a[4], float b[4], float t);
void sceVu0InterVectorXYZ(float dst[4], float a[4], float b[4], float t);
void sceVu0InversMatrix(float dst[4][4], float m[4][4]);
void sceVu0MulMatrix(float dst[4][4], float a[4][4], float b[4][4]);
void sceVu0MulVector(float dst[4], float a[4], float b[4]);
void sceVu0Normalize(float dst[4], float v[4]);
void sceVu0OuterProduct(float dst[4], float a[4], float b[4]);
void sceVu0RotMatrix(float dst[4][4], float m[4][4], float angles[4]);
void sceVu0RotMatrixX(float dst[4][4], float m[4][4], float angle);
void sceVu0RotMatrixY(float dst[4][4], float m[4][4], float angle);
void sceVu0RotTransPers(int dst[4], float m[4][4], float v[4], int mode);
void sceVu0ScaleVector(float dst[4], float v[4], float s);
void sceVu0ScaleVectorXYZ(float dst[4], float v[4], float s);
void sceVu0SubVector(float dst[4], float a[4], float b[4]);
void sceVu0TransMatrix(float dst[4][4], float m[4][4], float t[4]);
void sceVu0TransposeMatrix(float dst[4][4], float m[4][4]);
void sceVu0UnitMatrix(float dst[4][4]);
void sceVu0ViewScreenMatrix(float dst[4][4], float scrz, float ax, float ay, float cx, float cy, float zmin,
                            float zmax, float nearz, float farz);

#endif
