/*
 * Double-buffered work area shared by the character model renderers (Chacter_Draw).
 * model_common_work points at the page being built; ModelCommonWorkFlip swaps pages.
 */
#include "sh2.h"

/* Matching: bss objects are laid out in reverse declaration order; the double buffer is 128-aligned. */
struct ModelCommonWork *model_common_work;
static struct ModelCommonWork model_common_work_db[2] __attribute__((aligned(128)));
static int model_common_work_page;

/** Points model_common_work at the first page, once. */
void ModelCommonWorkInit(void) {
    static int initialized;

    if (!initialized) {
        model_common_work_page = 0;
        model_common_work = &model_common_work_db[0];
        initialized = 1;
    }
}

/** Switches model_common_work to the other page of the double buffer. */
void ModelCommonWorkFlip(void) {
    model_common_work = &model_common_work_db[model_common_work_page ^= 1];
}
