/*
 * fogdata.c: the fog's per-area data: the default and demo fog environments,
 * the collision walls near the player (from the stage's collision data) and the
 * choice of fog environment by area and position.
 */

#include "sh2.h"
#include "asm_helpers.h"
#include "asm_libm.h"
#include "sh_vu0.h"
#include "fog_helpers.h"

struct FOG_ENV_DATA fog_defset_data = { 500, 2000, 300, 100, 0, 0, 0, 10000, 0, 0, 1, 16, 2.0f };
struct FOG_ENV_DATA fog_demoenv_grave = { 200, 2000, 300, 100, 2500, 0, 0, 10000, 15, 8, 2, 32, 2.0f };

unsigned short fog_colis_data[6144] __attribute__((aligned(64)));

/**
 * Rebuilds the fog's collision walls from the stage collision data around the player, when the
 * player has moved to another collision block.
 */
void fogSetCollision(void) {
    int glb;

    if (stage == NULL) {
        return;
    }
    glb = stage->glb_crd;
    if (sh2jms.player == NULL) {
        return;
    }
    if (glb == 3 && fwork.WorldPosV[0] > -40000.0f &&
        (!(fwork.WorldPosV[2] < 80000.0f) || (fwork.WorldPosV[2] > 65500.0f && fwork.Global == 100))) {
        glb = 100;
    } else if (glb == 3 && ((fwork.WorldPosV[0] <= -55000.0f && fwork.WorldPosV[2] > 55000.0f) ||
                            (fwork.Global == 101 && fwork.WorldPosV[2] > 40000.0f && fwork.WorldPosV[0] <= -50000.0f) ||
                            (fwork.WorldPosV[0] < -40000.0f && fwork.WorldPosV[2] > 50000.0f))) {
        glb = 101;
    } else if (glb == 11) {
        switch (RoomNameJms()) {
        case 0x8E:
            glb = 0x67;
            if (fwork.LoadStep == 2) {
                fwork.LoadStep = 0;
            }
            break;
        case 0x8F:
            glb = 0x68;
            if (fwork.LoadStep == 2) {
                fwork.LoadStep = 0;
            }
            break;
        case 0x90:
            glb = 0x69;
            if (fwork.LoadStep == 2) {
                fwork.LoadStep = 0;
            }
            break;
        }
    } else if (glb == 12) {
        switch (RoomNameJms()) {
        case 0x98:
            glb = 0x6A;
            if (fwork.LoadStep == 2) {
                fwork.LoadStep = 0;
            }
            break;
        case 0x9B:
            glb = 0x6B;
            if (fwork.LoadStep == 2) {
                fwork.LoadStep = 0;
            }
            break;
        case 0xA2:
            glb = 0x6C;
            if (fwork.LoadStep == 2) {
                fwork.LoadStep = 0;
            }
            break;
        }
    } else if (glb == 9) {
        switch (RoomNameJms()) {
        case 0x28:
            glb = 0x66;
            if (fwork.LoadStep == 2) {
                fwork.LoadStep = 0;
            }
            break;
        }
    } else if (glb == 13) {
        switch (RoomNameJms()) {
        case 0xB0:
            glb = 0x6B;
            if (fwork.LoadStep == 2) {
                fwork.LoadStep = 0;
            }
            break;
        case 0xBB:
            if (fwork.WorldPosV[1] < -15000.0f) {
                glb = 0x6D;
                if (fwork.LoadStep == 2) {
                    fwork.LoadStep = 0;
                }
            }
            break;
        }
    } else if (glb == 14) {
        switch (RoomNameJms()) {
        case 0xBD:
            glb = 0x6E;
            if (fwork.LoadStep == 2) {
                fwork.LoadStep = 0;
            }
            break;
        }
    }
    if (fwork.LoadStep == 1) {
        if (fsSync(1, fwork.fid) >= 0) {
            fwork.ColisHead = (struct FOG_COLIS_HEAD *)fog_colis_data;
            fwork.LoadStep = 0;
            if (glb == 101) {
                fogSetCollisionMain2();
            } else {
                fogSetCollisionMain();
            }
        }
        return;
    }
    if (glb != fwork.Global && fwork.LoadStep == 0) {
        fwork.ColisHead = NULL;
        fwork.AreaTop = NULL;
        fwork.AreaMax = 0;
        fogResetWall();
        fogResetObj();
        fogSetEnvironment(&fog_defset_data);
        fwork.Flag &= ~0x100;
        switch (glb) {
        case 1:
            fwork.Global = glb;
            fwork.fid = FcRead(data_bg_ca_caGB_fcl, fog_colis_data);
            break;
        case 2:
            fwork.Global = glb;
            fwork.fid = FcRead(data_bg_cb_cbGB_fcl, fog_colis_data);
            break;
        case 3:
            fwork.Global = glb;
            fwork.fid = FcRead(data_bg_cc_ccGB_fcl, fog_colis_data);
            break;
        case 4:
            fwork.Global = glb;
            fwork.fid = FcRead(data_bg_cd_cdGB_fcl, fog_colis_data);
            break;
        case 5:
            fwork.Global = glb;
            fwork.fid = FcRead(data_bg_ob_obGB_fcl, fog_colis_data);
            break;
        case 100:
            fwork.Global = glb;
            fwork.fid = FcRead(data_bg_cc_park_fcl, fog_colis_data);
            break;
        case 101:
            fwork.Global = glb;
            fwork.fid = FcRead(data_bg_cc_ccnw_fcl, fog_colis_data);
            break;
        case 0x67:
        case 0x68:
        case 0x69:
            fwork.Global = glb;
            fwork.Flag |= 0x100;
            fwork.fid = FcRead(data_bg_ps_psGB_fcl, fog_colis_data);
            break;
        case 0x6A:
        case 0x6B:
        case 0x6C:
        case 0x66:
        case 0x6D:
        case 0x6E:
            fwork.Global = glb;
            fwork.fid = FcRead(data_bg_rr_rrGB_fcl, fog_colis_data);
            break;
        default:
            fwork.Global = -1;
            shQzero(fog_colis_data, 12);
            fwork.ColisHead = (struct FOG_COLIS_HEAD *)fog_colis_data;
            fogSetPartNum(0);
            fwork.LoadStep = 2;
            return;
        }
        fwork.LoadStep = 1;
    } else {
        switch (fwork.Global) {
        case 5:
            fogSetWorldPosV(&sh2jms.player->pos);
            fogSetLocalPosV();
            fwork.LocalPosV[1] = 1000.0f;
            fwork.LocalPosV[2] = 7000.0f;
            break;
        case 0x66:
            fwork.WorldPosV[0] = fwork.LocalPosV[0] = 16000.0f;
            fwork.WorldPosV[1] = fwork.LocalPosV[1] = 0.0f;
            fwork.WorldPosV[2] = fwork.LocalPosV[2] = -99000.0f;
            break;
        case 0x68:
            fwork.WorldPosV[0] = fwork.LocalPosV[0] = 100000.0f;
            fwork.WorldPosV[1] = fwork.LocalPosV[1] = 0.0f;
            fwork.WorldPosV[2] = fwork.LocalPosV[2] = -100000.0f;
            break;
        case 0x6A:
            fwork.WorldPosV[0] = fwork.LocalPosV[0] = -60000.0f;
            fwork.WorldPosV[1] = fwork.LocalPosV[1] = 0.0f;
            fwork.WorldPosV[2] = fwork.LocalPosV[2] = 56500.0f;
            break;
        case 0x6B:
            fwork.WorldPosV[0] = fwork.LocalPosV[0] = 20000.0f;
            fwork.WorldPosV[1] = fwork.LocalPosV[1] = 0.0f;
            fwork.WorldPosV[2] = fwork.LocalPosV[2] = -63300.0f;
            break;
        case 0x6C:
            fwork.WorldPosV[0] = fwork.LocalPosV[0] = 60000.0f;
            fwork.WorldPosV[1] = fwork.LocalPosV[1] = 0.0f;
            fwork.WorldPosV[2] = fwork.LocalPosV[2] = 16000.0f;
            break;
        case 0x6E:
            fwork.WorldPosV[0] = fwork.LocalPosV[0] = 10000.0f;
            fwork.WorldPosV[1] = fwork.LocalPosV[1] = 0.0f;
            fwork.WorldPosV[2] = fwork.LocalPosV[2] = 9600.0f;
            break;
        case 0x65:
            if (distXZ(fwork.MapPosV, fwork.WorldPosV) > 500.0f) {
                fogSetCollisionMain2();
            }
            return;
        case -1:
            fogSetPartNum(0);
            return;
        }
        if (distXZ(fwork.MapPosV, fwork.WorldPosV) > 500.0f) {
            fogSetCollisionMain();
        }
    }
}

/** Converts the collision data of the current block into fog walls (fogSetWall()). */
void fogSetCollisionMain(void) {
    int i;
    struct FOG_OBJ_DATA *od;
    struct FOG_OBJ_DATA *od2;
    float *wp;
    int wx;
    int wz;
    int max;
    int lim;
    float wxf;
    float wzf;
    float limf;
    struct FOG_COLIS_WALL *w1;
    struct FOG_COLIS_WALL2 *w2;
    float *pobj;
    float (*wall)[4];
    float *pos;
    struct FOG_WALL_DATA *pw;
    struct FOG_COLIS_HEAD *ch;

    wp = (float *)0x70003FA0;
    wall = (float (*)[4])0x70003FC0;
    pos = (float *)0x70003FB0;
    ch = fwork.ColisHead;
    if (ch == NULL) {
        return;
    }
    vcopy(fwork.WorldPosV, wp);
    wxf = wp[0];
    wzf = wp[2];
    wx = ftoi(wxf / 50.0f);
    wz = ftoi(wzf / 50.0f);
    vcopy(wp, fwork.MapPosV);
    fogResetWall();
    od = fwork.Obj;
    i = 0;
    max = fwork.ObjMax;
    od2 = &fwork.Obj[max];
    while (i < max) {
        if (od->id >= 100) {
            od2--;
            max--;
            if (od2 != od) {
                fogCopyObj(od2, od);
            }
        } else {
            od++;
            i++;
        }
    }
    fwork.ObjMax = max;

    max = ch->wall1;
    pobj = (float *)(ch + 1);
    lim = 100;
    if (fwork.Global == 5) {
        lim = 0x1000000;
    }
    i = 0;
    while (i < max) {
        w1 = (struct FOG_COLIS_WALL *)pobj;
        if ((iabs(w1->x0 - wx) <= lim && iabs(w1->z0 - wz) <= lim) ||
            (iabs(w1->x1 - wx) <= lim && iabs(w1->z1 - wz) <= lim)) {
            wall[0][0] = wall[2][0] = w1->x0;
            wall[0][2] = wall[2][2] = w1->z0;
            wall[1][0] = wall[3][0] = w1->x1;
            wall[1][2] = wall[3][2] = w1->z1;
            wall[0][1] = wall[1][1] = w1->y0;
            wall[2][1] = wall[3][1] = w1->y1;
            _shScaleVector(wall[0], wall[0], 50.0f);
            _shScaleVector(wall[1], wall[1], 50.0f);
            _shScaleVector(wall[2], wall[2], 50.0f);
            _shScaleVector(wall[3], wall[3], 50.0f);
            pw = &fwork.Wall[fwork.WallNum];
            fogSetWall(wall);
            ((int *)pw->normal)[3] = 0;
            ((float **)pw->v0)[3] = pobj;
        }
        i++;
        pobj += 3;
    }

    max = ch->wall2;
    i = 0;
    while (i < max) {
        w2 = (struct FOG_COLIS_WALL2 *)pobj;
        if ((iabs(w2->x0 - wx) <= lim && iabs(w2->z0 - wz) <= lim) ||
            (iabs(w2->x1 - wx) <= lim && iabs(w2->z1 - wz) <= lim) ||
            (iabs(w2->x2 - wx) <= lim && iabs(w2->z2 - wz) <= lim) ||
            (iabs(w2->x3 - wx) <= lim && iabs(w2->z3 - wz) <= lim)) {
            wall[0][0] = w2->x0;
            wall[0][1] = w2->y0;
            wall[0][2] = w2->z0;
            wall[1][0] = w2->x1;
            wall[1][1] = w2->y1;
            wall[1][2] = w2->z1;
            wall[2][0] = w2->x2;
            wall[2][1] = w2->y2;
            wall[2][2] = w2->z2;
            wall[3][0] = w2->x3;
            wall[3][1] = w2->y3;
            wall[3][2] = w2->z3;
            _shScaleVector(wall[0], wall[0], 50.0f);
            _shScaleVector(wall[1], wall[1], 50.0f);
            _shScaleVector(wall[2], wall[2], 50.0f);
            _shScaleVector(wall[3], wall[3], 50.0f);
            pw = &fwork.Wall[fwork.WallNum];
            fogSetWall(wall);
            ((int *)pw->normal)[3] = 1;
            ((float **)pw->v0)[3] = pobj;
        }
        i++;
        pobj += 6;
    }

    wxf = wp[0];
    wzf = wp[2];
    max = ch->obj1;
    limf = 3000.0f;
    if (fwork.Global == 5) {
        limf = 3.4028235e38f;
    }
    i = 0;
    while (i < max) {
        if (fabsf(pobj[0] - wxf) <= limf && fabsf(pobj[2] - wzf) <= limf) {
            pos[0] = pobj[0];
            pos[1] = pobj[1];
            pos[2] = pobj[2];
            fogSetObj(i + 100, pos, pobj[3]);
        }
        i++;
        pobj += 4;
    }
    max = ch->obj2;
    i = 0;
    while (i < max) {
        if (fabsf(pobj[0] - wxf) <= limf && fabsf(pobj[2] - wzf) <= limf) {
            pos[0] = pobj[0];
            pos[1] = pobj[1];
            pos[2] = pobj[2];
            fogSetObj2(i + 500, pos, pobj[3]);
        }
        i++;
        pobj += 4;
    }
    if (fwork.AreaTop == NULL) {
        fwork.AreaTop = (struct FOG_AREA_DATA *)pobj;
        fwork.AreaMax = ch->area;
        fwork.EnvTop = (struct FOG_ENV_DATA *)(((int)(fwork.AreaTop + ch->area) + 3) & ~3);
    }
}

/** fogSetCollisionMain() for the second collision set. */
void fogSetCollisionMain2(void) {
    int i;
    struct FOG_OBJ_DATA *od;
    struct FOG_OBJ_DATA *od2;
    float *wp;
    int wx;
    int wz;
    int max;
    int lim;
    float wxf;
    float wzf;
    float limf;
    struct FOG_COLIS_WALL *w1;
    struct FOG_COLIS_WALL2 *w2;
    float *pobj;
    float (*wall)[4];
    float *pos;
    struct FOG_WALL_DATA *pw;
    struct FOG_COLIS_HEAD *ch;

    wp = (float *)0x70003FA0;
    wall = (float (*)[4])0x70003FC0;
    pos = (float *)0x70003FB0;
    ch = fwork.ColisHead;
    if (ch == NULL) {
        return;
    }
    vcopy(fwork.WorldPosV, wp);
    vcopy(wp, fwork.MapPosV);
    wxf = wp[0];
    wzf = wp[2];
    wxf += 100000.0f;
    wzf -= 100000.0f;
    wx = ftoi(wxf / 50.0f);
    wz = ftoi(wzf / 50.0f);
    fogResetWall();
    od = fwork.Obj;
    i = 0;
    max = fwork.ObjMax;
    od2 = &fwork.Obj[max];
    for (; i < max;) {
        if (od->id >= 100) {
            od2--;
            max--;
            if (od2 != od) {
                fogCopyObj(od2, od);
            }
        } else {
            od++;
            i++;
        }
    }
    fwork.ObjMax = max;

    max = ch->wall1;
    pobj = (float *)(ch + 1);
    lim = 100;
    i = 0;
    while (i < max) {
        w1 = (struct FOG_COLIS_WALL *)pobj;
        if ((iabs(w1->x0 - wx) <= lim && iabs(w1->z0 - wz) <= lim) ||
            (iabs(w1->x1 - wx) <= lim && iabs(w1->z1 - wz) <= lim)) {
            wall[0][0] = wall[2][0] = w1->x0 - 2000;
            wall[0][2] = wall[2][2] = w1->z0 + 2000;
            wall[1][0] = wall[3][0] = w1->x1 - 2000;
            wall[1][2] = wall[3][2] = w1->z1 + 2000;
            wall[0][1] = wall[1][1] = w1->y0;
            wall[2][1] = wall[3][1] = w1->y1;
            _shScaleVector(wall[0], wall[0], 50.0f);
            _shScaleVector(wall[1], wall[1], 50.0f);
            _shScaleVector(wall[2], wall[2], 50.0f);
            _shScaleVector(wall[3], wall[3], 50.0f);
            pw = &fwork.Wall[fwork.WallNum];
            fogSetWall(wall);
            ((int *)pw->normal)[3] = 0;
            ((float **)pw->v0)[3] = pobj;
        }
        i++;
        pobj += 3;
    }

    max = ch->wall2;
    i = 0;
    while (i < max) {
        w2 = (struct FOG_COLIS_WALL2 *)pobj;
        if ((iabs(w2->x0 - wx) <= lim && iabs(w2->z0 - wz) <= lim) ||
            (iabs(w2->x1 - wx) <= lim && iabs(w2->z1 - wz) <= lim) ||
            (iabs(w2->x2 - wx) <= lim && iabs(w2->z2 - wz) <= lim) ||
            (iabs(w2->x3 - wx) <= lim && iabs(w2->z3 - wz) <= lim)) {
            wall[0][0] = w2->x0 - 2000;
            wall[0][1] = w2->y0;
            wall[0][2] = w2->z0 + 2000;
            wall[1][0] = w2->x1 - 2000;
            wall[1][1] = w2->y1;
            wall[1][2] = w2->z1 + 2000;
            wall[2][0] = w2->x2 - 2000;
            wall[2][1] = w2->y2;
            wall[2][2] = w2->z2 + 2000;
            wall[3][0] = w2->x3 - 2000;
            wall[3][1] = w2->y3;
            wall[3][2] = w2->z3 + 2000;
            _shScaleVector(wall[0], wall[0], 50.0f);
            _shScaleVector(wall[1], wall[1], 50.0f);
            _shScaleVector(wall[2], wall[2], 50.0f);
            _shScaleVector(wall[3], wall[3], 50.0f);
            pw = &fwork.Wall[fwork.WallNum];
            fogSetWall(wall);
            ((int *)pw->normal)[3] = 1;
            ((float **)pw->v0)[3] = pobj;
        }
        i++;
        pobj += 6;
    }

    wxf = wp[0];
    wzf = wp[2];
    max = ch->obj1;
    limf = 3000.0f;
    i = 0;
    while (i < max) {
        if (fabsf(pobj[0] - wxf) <= limf && fabsf(pobj[2] - wzf) <= limf) {
            pos[0] = pobj[0] - 100000.0f;
            pos[1] = pobj[1];
            pos[2] = 100000.0f + pobj[2];
            fogSetObj(i + 100, pos, pobj[3]);
        }
        i++;
        pobj += 4;
    }
    max = ch->obj2;
    i = 0;
    while (i < max) {
        if (fabsf(pobj[0] - wxf) <= limf && fabsf(pobj[2] - wzf) <= limf) {
            pos[0] = pobj[0] - 100000.0f;
            pos[1] = pobj[1];
            pos[2] = 100000.0f + pobj[2];
            fogSetObj2(i + 500, pos, pobj[3]);
        }
        i++;
        pobj += 4;
    }
    if (fwork.AreaTop == NULL) {
        fwork.AreaTop = (struct FOG_AREA_DATA *)pobj;
        fwork.AreaMax = ch->area;
        fwork.EnvTop = (struct FOG_ENV_DATA *)(((int)(fwork.AreaTop + ch->area) + 3) & ~3);
    }
}


/** Chooses the fog environment for the current area and position (fogSetEnvironment()). */
void fogSetAreaEnvironment(void) {
    int i;
    int max;
    short wx;
    short wz;
    struct FOG_AREA_DATA *pa;

    if ((Sh2sys.main_status >> 6) & 1) {
        switch (fwork.Global) {
        case 1:
            if (fwork.WorldPosV[0] > -20000.0f && fwork.WorldPosV[2] > -20000.0f) {
                fogSetEnvironment(&fog_demoenv_grave);
                return;
            }
            break;
        case 0x69:
            fogSetEnvironment(fwork.EnvTop);
            fwork.WorldPosV[0] = fwork.LocalPosV[0] = 140000.0f;
            fwork.WorldPosV[1] = fwork.LocalPosV[1] = 0.0f;
            fwork.WorldPosV[2] = fwork.LocalPosV[2] = -101500.0f;
            return;
        }
    }
    if (fwork.ColisHead != NULL) {
        pa = fwork.AreaTop;
        if (fwork.Global == 101) {
            wx = ftoi((100000.0f + fwork.WorldPosV[0]) / 50.0f);
            wz = ftoi((fwork.WorldPosV[2] - 100000.0f) / 50.0f);
        } else {
            wx = ftoi(fwork.WorldPosV[0] / 50.0f);
            wz = ftoi(fwork.WorldPosV[2] / 50.0f);
        }
        max = fwork.ColisHead->area;
        i = 0;
        while (i < max) {
            if (wx >= pa->x0 && wx <= pa->x1 && wz >= pa->z0 && wz <= pa->z1) {
                fogSetEnvironment(&fwork.EnvTop[pa->env]);
                return;
            }
            i++;
            pa++;
        }
        fogSetEnvironment(fwork.EnvTop);
    } else {
        fogSetEnvironment(&fog_defset_data);
    }
}
