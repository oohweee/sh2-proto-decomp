/*
 * James in the rowing boat: his pose and matrix follow the boat (sh2bot) and the camera
 * control's player info.
 */

#include "sh2.h"
#include "sdk/libvu0.h"

struct FMAT bjms_localworld_matrix;

static void BoatPlayerCheckSetParameterPhase1(struct SubCharacter *this) {
    struct SubCharacterDisp *this_d;

    this_d = (struct SubCharacterDisp *)this;
    this_d->anime2.rot_body_neck.x = this_d->anime2.rot_body_neck.y = this_d->anime2.rot_body_neck.z = 0.0f;
    sh2jms.tgt_body_angle.x = 0.0f;
    sh2jms.tgt_body_angle.y = 0.0f;
    sh2jms.tgt_body_angle.z = 0.0f;
    sh2jms.tgt_neck_angle.x = 0.0f;
    sh2jms.tgt_neck_angle.y = 0.0f;
    sh2jms.tgt_neck_angle.z = 0.0f;
    sh2jms.tgt_arms_angle.x = 0.0f;
    sh2jms.tgt_arms_angle.y = 0.0f;
    sh2jms.tgt_arms_angle.z = 0.0f;
    sh2jms.look_tgt = NULL;
}

/*
 * Matching: an inline helper (the name is ours): the original's line table has the whole
 * rotation as one statement, and its locals (rot, rot_xz) aren't in BoatPlayerCheckControl's DWARF.
 */
static inline void BoatPlayerUpdateMatrixZYX(float *rot) {
    float rot_xz[4];

    rot_xz[0] = rot[0];
    rot_xz[1] = 0.0f;
    rot_xz[2] = rot[2];
    rot_xz[3] = 1.0f;
    sceVu0RotMatrix(bjms_localworld_matrix.d, kt_unit_matrix.d, rot_xz);
    sceVu0RotMatrixY(bjms_localworld_matrix.d, bjms_localworld_matrix.d, rot[1]);
}

/** Updates James while he rows the boat: animation synced to the boat, position, and local-world matrix. */
void BoatPlayerCheckControl(void) {
    struct shCharaInfo *james_info;
    struct SubCharacter *jms;
    struct SubCharacter *boat;
    struct SubCharacterDisp *this_d;
    int anim_counter;

    jms = sh2jms.player;
    this_d = (struct SubCharacterDisp *)jms;
    BoatPlayerCheckSetParameterPhase1(jms);
    BoatPlayerCheckAnime();
    anim_counter = shCharacterAnimeCounterGet(sh2bot.boat_p);
    shCharacterAnimeCounterSet_(sh2jms.player, 1, anim_counter);
    shCharacterAnimeCounterSet_(sh2jms.player, 2, anim_counter);
    shCharacterAnimeSpeedAdd_(sh2jms.player, 1, sh2bot.anime_speed);
    shCharacterAnimeSpeedAdd_(sh2jms.player, 2, sh2bot.anime_speed);
    SCRotZYXSwitch(sh2jms.player, 1);
    james_info = GetPlayerInfoForCameraCtrl();
    sh2jms.player->pos = james_info->pos;
    sh2jms.player->rot = james_info->rot;
    sh2jms.player->pos_spd = james_info->pos_spd;
    sh2jms.player->rot_spd = james_info->rot_spd;
    BoatPlayerUpdateMatrixZYX((float *)&sh2jms.player->rot);
    bjms_localworld_matrix.d[3][0] = sh2jms.player->pos.x;
    bjms_localworld_matrix.d[3][1] = sh2jms.player->pos.y;
    bjms_localworld_matrix.d[3][2] = sh2jms.player->pos.z;
    bjms_localworld_matrix.d[3][3] = 1.0f;
    this_d->anime2.rot_neck.x = -sh2jms.player->rot.x;
    this_d->anime2.rot_neck.z = -sh2jms.player->rot.z;
    boat = shCharacterGetSubCharacter(0x10B, -1);
}
