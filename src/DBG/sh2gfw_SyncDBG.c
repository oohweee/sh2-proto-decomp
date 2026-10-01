/*
 * sh2gfw_SyncDBG.c: debug counters of the graphics framework: vertices drawn
 * per pass (all and semi-transparent) and performance samples.
 */

#include "sh2.h"

int DBG_Perf[16];
int SemiTransTex_VertNum[6];
int Vertex_Num[6];
int Index_SemiTrans;
int Index_VertNum;
int DBG_Perf_index;

/** Clears the vertex counters of every pass and restarts at pass 0. */
void sh2gfw_Init_AllVertCounter(void) {
    int i;

    for (i = 0; i < 6; i++) {
        SemiTransTex_VertNum[i] = 0;
        Vertex_Num[i] = 0;
    }
    Index_SemiTrans = 0;
    Index_VertNum = 0;
}

/** Moves both vertex counters to the next pass. */
void sh2gfw_Incliment_VertNumIndices(void) {
    Index_SemiTrans++;
    Index_VertNum++;
}

/** Adds `num` semi-transparent vertices to the current pass. */
void sh2gfw_Add_SemiTransVertNum(int num) {
    SemiTransTex_VertNum[Index_SemiTrans] += num;
}

/** Adds `num` vertices to the current pass. */
void sh2gfw_Add_VertexNum(int num) {
    Vertex_Num[Index_VertNum] += num;
}

/** Restarts the performance samples at index 0. */
void sh2gfw_init_Perf(void) {
    DBG_Perf_index = 0;
}

/** Stores performance sample `count` in slot `md`. Returns nothing despite its int type. */
int sh2gfw_Store_Perf2(int count, int md) {
    DBG_Perf[md] = count;
}
