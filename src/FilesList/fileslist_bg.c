/*
 * fileslist_bg.c: the background file table: per background stage, its list of blocks and their
 * count (FilesBgBlockList_* / FilesBgBlockMax_*; the room lists are unset in this build), and
 * FilesGetBgBlock, which looks a block up by stage and block number.
 */
#include "sh2.h"

static struct FilesBgStage FilesBgStage_ap[1] = {{FilesBgBlockList_ap, FilesBgBlockMax_ap, NULL, NULL}};
static struct FilesBgStage FilesBgStage_bw[1] = {{FilesBgBlockList_bw, FilesBgBlockMax_bw, NULL, NULL}};
static struct FilesBgStage FilesBgStage_ca[1] = {{FilesBgBlockList_ca, FilesBgBlockMax_ca, NULL, NULL}};
static struct FilesBgStage FilesBgStage_cb[1] = {{FilesBgBlockList_cb, FilesBgBlockMax_cb, NULL, NULL}};
static struct FilesBgStage FilesBgStage_cc[1] = {{FilesBgBlockList_cc, FilesBgBlockMax_cc, NULL, NULL}};
static struct FilesBgStage FilesBgStage_cd[1] = {{FilesBgBlockList_cd, FilesBgBlockMax_cd, NULL, NULL}};
static struct FilesBgStage FilesBgStage_er[1] = {{FilesBgBlockList_er, FilesBgBlockMax_er, NULL, NULL}};
static struct FilesBgStage FilesBgStage_hp[1] = {{FilesBgBlockList_hp, FilesBgBlockMax_hp, NULL, NULL}};
static struct FilesBgStage FilesBgStage_ps[1] = {{FilesBgBlockList_ps, FilesBgBlockMax_ps, NULL, NULL}};
static struct FilesBgStage FilesBgStage_qp[1] = {{FilesBgBlockList_qp, FilesBgBlockMax_qp, NULL, NULL}};
static struct FilesBgStage FilesBgStage_qt[1] = {{FilesBgBlockList_qt, FilesBgBlockMax_qt, NULL, NULL}};
static struct FilesBgStage FilesBgStage_rr[1] = {{FilesBgBlockList_rr, FilesBgBlockMax_rr, NULL, NULL}};
static struct FilesBgStage FilesBgStage_ru[1] = {{FilesBgBlockList_ru, FilesBgBlockMax_ru, NULL, NULL}};
static struct FilesBgStage FilesBgStage_th[1] = {{FilesBgBlockList_th, FilesBgBlockMax_th, NULL, NULL}};
static struct FilesBgStage FilesBgStage_ob[1] = {{FilesBgBlockList_ob, FilesBgBlockMax_ob, NULL, NULL}};
static struct FilesBgStage FilesBgStage_ma[1] = {{FilesBgBlockList_ma, FilesBgBlockMax_ma, NULL, NULL}};

static struct FilesBgStage *FilesBgStageList[BG_ID_num] = {
    NULL,
    FilesBgStage_ca,
    FilesBgStage_cb,
    FilesBgStage_cc,
    FilesBgStage_cd,
    FilesBgStage_ob,
    FilesBgStage_er,
    FilesBgStage_bw,
    FilesBgStage_th,
    FilesBgStage_ap,
    FilesBgStage_hp,
    FilesBgStage_ps,
    FilesBgStage_rr,
    FilesBgStage_ru,
    FilesBgStage_qp,
    FilesBgStage_qt,
    FilesBgStage_ma,
};
static int FilesBgStageMax[1] = {BG_ID_num};

/** Returns block blk_id of background stage stg_id, or NULL if either is out of range. */
struct FilesBgBlock *FilesGetBgBlock(enum STAGE_ID stg_id, int blk_id) {
    struct FilesBgStage *stg;
    int max_id;

    max_id = FilesBgStageMax[0];
    if (stg_id <= 0 || max_id <= stg_id) {
        return NULL;
    }
    stg = FilesBgStageList[stg_id];
    max_id = *stg->block_max;
    if (blk_id < 0 || max_id <= blk_id) {
        return NULL;
    }
    return stg->block_list[blk_id];
}
