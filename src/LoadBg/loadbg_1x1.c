/*
 * loadbg_1x1.c: background queries for the 1x1-block loader.
 */

#include "sh2.h"

/* loadbg_1x1.c: background queries for the 1x1-block loader. */

/**
 * Returns the draw environment ID of the current place: the map (glb_crd) in the high 16 bits,
 * block 1 outdoors or the first block of the room indoors in the low 16.
 */
/* Draw environment id: stage in the upper 16 bits, map (block) in the lower. */
int loadBg1x1_GetIdForDrawEnv(void) {
    struct _loadBgCommon_Info_T *info;
    int glb_crd;
    int mapid;

    info = _loadBgCommon_Info;
    glb_crd = _loadBgCommon_Info->glb_crd;
    mapid = BgIsOut(glb_crd) ? 1 : info->BlockID[0];
    return (glb_crd << 16) | mapid;
}

/** Translucent texture file of the current stage: block 1 outdoors, else the first loaded block that has one. */
void *loadBg1x1_GetTrTexFile(void) {
    struct _loadBgCommon_Info_T *info;
    int glb_crd;
    int map_no;
    struct FilesBgBlock *filesbg;
    void *file;
    int i;

    info = _loadBgCommon_Info;
    glb_crd = _loadBgCommon_Info->glb_crd;
    if (BgIsOut(glb_crd)) {
        filesbg = FilesGetBgBlock(glb_crd, 1);
        return filesbg->tex;
    }
    file = NULL;
    for (i = 0; i < 4; i++) {
        map_no = info->BlockID[i];
        if (map_no == 0) {
            break;
        }
        filesbg = FilesGetBgBlock(glb_crd, map_no);
        file = filesbg->tex;
        if (file != NULL) {
            break;
        }
    }
    return file;
}
