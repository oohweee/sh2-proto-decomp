/*
 * Character model system entry points (Chacter_Draw): start-up of the model work areas.
 */
#include "sh2.h"

/** Initializes the shared model work and the Model3 renderer. */
void ModelInit(void) {
    ModelCommonWorkInit();
    Model3Init();
}

/** Post-draw hook for the model system; empty in this build. */
void ModelDrawPost(void) {
}
