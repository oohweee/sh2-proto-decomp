/*
 * RPJMS: James's mirror image. It follows the player reflected across a wall plane chosen by
 * room, and the player's light position/direction are reflected the same way.
 */

#include "sh2.h"
#include "asm_helpers.h"

static float wall_pos;
static int mirror_mode;

static int HumanRPJMSInit(struct SubCharacter *scp) {
    SCAnimeTypeSwitch(scp, 1);
    scp->model_type = 1;
    return 0;
}

static void HumanRPJMSFunction(struct SubCharacter *this) {
    switch (this->step) {
    case 0:
        HumanRPJMSInit(this);
        this->step++;
        break;
    case 1:
        sh2gfw_Set_JMSequip(this, sh2jms.parts_lhand, sh2jms.parts_rhand, sh2jms.parts_light);
        switch (mirror_mode) {
        case 0:
            this->pos.x = wall_pos + (wall_pos - sh2jms.player->pos.x);
            this->pos.y = sh2jms.player->pos.y;
            this->pos.z = sh2jms.player->pos.z;
            this->rot.y = -sh2jms.player->rot.y;
            break;
        case 1:
            this->pos.x = sh2jms.player->pos.x;
            this->pos.y = wall_pos + (wall_pos - sh2jms.player->pos.y);
            this->pos.z = sh2jms.player->pos.z;
            this->rot.y = sh2jms.player->rot.y;
            this->rot.z = 3.1415927f;
            break;
        case 2:
            this->pos.x = sh2jms.player->pos.x;
            this->pos.y = sh2jms.player->pos.y;
            this->pos.z = wall_pos + (wall_pos - sh2jms.player->pos.z);
            if (sh2jms.player->rot.y >= 0.0f) {
                this->rot.y = 3.1415927f - sh2jms.player->rot.y;
            } else {
                this->rot.y = -3.1415927f - sh2jms.player->rot.y;
            }
            break;
        }
        break;
    }
}

/** Installs the RPJMS (mirrored James) update function. @param scp the character. */
void shCharacterSetHumanRPJMSLow(struct SubCharacter *scp) {
    shCharacterSetFunction(scp, HumanRPJMSFunction);
}

/** Reflects sh2jms's light positions and directions across the mirror plane into the *_reverse fields. */
void shGetJamesLightPos_Calc_Reverse(void) {
    switch (mirror_mode) {
    case 0:
        sh2jms.light_pos_revise_reverse[0] = wall_pos + (wall_pos - sh2jms.light_pos_revise[0]);
        sh2jms.light_pos_revise_reverse[1] = sh2jms.light_pos_revise[1];
        sh2jms.light_pos_revise_reverse[2] = sh2jms.light_pos_revise[2];
        sh2jms.light_pos_revise_reverse[3] = 1.0f;
        sh2jms.light_pos_reverse[0] = wall_pos + (wall_pos - sh2jms.light_pos[0]);
        sh2jms.light_pos_reverse[1] = sh2jms.light_pos[1];
        sh2jms.light_pos_reverse[2] = sh2jms.light_pos[2];
        sh2jms.light_pos_reverse[3] = 1.0f;
        sh2jms.light_vec_revise_reverse[0] = -sh2jms.light_vec_revise[0];
        sh2jms.light_vec_revise_reverse[1] = sh2jms.light_vec_revise[1];
        sh2jms.light_vec_revise_reverse[2] = sh2jms.light_vec_revise[2];
        sh2jms.light_vec_reverse[0] = -sh2jms.light_vec[0];
        sh2jms.light_vec_reverse[1] = sh2jms.light_vec[1];
        sh2jms.light_vec_reverse[2] = sh2jms.light_vec[2];
        break;
    case 1:
        sh2jms.light_pos_revise_reverse[0] = sh2jms.light_pos_revise[0];
        sh2jms.light_pos_revise_reverse[1] = wall_pos + (wall_pos - sh2jms.light_pos_revise[1]);
        sh2jms.light_pos_revise_reverse[2] = sh2jms.light_pos_revise[2];
        sh2jms.light_pos_revise_reverse[3] = 1.0f;
        sh2jms.light_pos_reverse[0] = sh2jms.light_pos[0];
        sh2jms.light_pos_reverse[1] = wall_pos + (wall_pos - sh2jms.light_pos[1]);
        sh2jms.light_pos_reverse[2] = sh2jms.light_pos[2];
        sh2jms.light_pos_reverse[3] = 1.0f;
        sh2jms.light_vec_revise_reverse[0] = sh2jms.light_vec_revise[0];
        sh2jms.light_vec_revise_reverse[1] = -sh2jms.light_vec_revise[1];
        sh2jms.light_vec_revise_reverse[2] = sh2jms.light_vec_revise[2];
        sh2jms.light_vec_reverse[0] = sh2jms.light_vec[0];
        sh2jms.light_vec_reverse[1] = -sh2jms.light_vec[1];
        sh2jms.light_vec_reverse[2] = sh2jms.light_vec[2];
        break;
    case 2:
        sh2jms.light_pos_revise_reverse[0] = sh2jms.light_pos_revise[0];
        sh2jms.light_pos_revise_reverse[1] = sh2jms.light_pos_revise[1];
        sh2jms.light_pos_revise_reverse[2] = wall_pos + (wall_pos - sh2jms.light_pos_revise[2]);
        sh2jms.light_pos_revise_reverse[3] = 1.0f;
        sh2jms.light_pos_reverse[0] = sh2jms.light_pos[0];
        sh2jms.light_pos_reverse[1] = sh2jms.light_pos[1];
        sh2jms.light_pos_reverse[2] = wall_pos + (wall_pos - sh2jms.light_pos[2]);
        sh2jms.light_pos_reverse[3] = 1.0f;
        sh2jms.light_vec_revise_reverse[0] = sh2jms.light_vec_revise[0];
        sh2jms.light_vec_revise_reverse[1] = sh2jms.light_vec_revise[1];
        sh2jms.light_vec_revise_reverse[2] = -sh2jms.light_vec_revise[2];
        sh2jms.light_vec_reverse[0] = sh2jms.light_vec[0];
        sh2jms.light_vec_reverse[1] = sh2jms.light_vec[1];
        sh2jms.light_vec_reverse[2] = -sh2jms.light_vec[2];
        break;
    }
}

/** Gets the reflected light. @param pos out: light position. @param vec out: light direction. */
void shGetJamesLightPosOriginal_Reverse(float *pos, float *vec) {
    vcopy_dst_first(pos, sh2jms.light_pos_reverse);
    vcopy_dst_first(vec, sh2jms.light_vec_reverse);
}

/** Chooses the mirror plane (mirror_mode, wall_pos) for the current room, or none; also sets
 * water_road and light_reverse. */
void PlayerSetReverseMode(void) {
    switch (RoomNameJms()) {
    case 1:
        mirror_mode = 0;
        wall_pos = -20000.0f;
        sh2jms.water_road = 0;
        break;
    case 0x24:
        mirror_mode = 2;
        wall_pos = -99995.0f;
        sh2jms.water_road = 0;
        break;
    case 0x25:
    case 0x26:
    case 0x62:
        mirror_mode = 1;
        wall_pos = 0.0f;
        sh2jms.water_road = 0;
        break;
    case 0x63:
    case 0x80:
    case 0x85:
    case 0x7A:
    case 0x7C:
    case 0x7E:
    case 0xB6:
    case 0xB7:
    case 0xAB:
    case 0xB8:
    case 0xB9:
        mirror_mode = 1;
        wall_pos = -100.0f;
        sh2jms.water_road = 1;
        break;
    default:
        mirror_mode = -1;
        sh2jms.water_road = 0;
        break;
    }
    sh2jms.light_reverse = (mirror_mode == -1) ? 0 : 1;
}
