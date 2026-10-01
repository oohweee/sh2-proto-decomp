/*
 * demoview.c: the drama demo (cutscene) player: reads a demo script (.dds), creates and animates
 * its characters and camera, lights, subtitles and voices.
 */
#include "sh2.h"
#include "asm_helpers.h"
#include "sh_vu0.h"
#include "fi_libvu0_inline.h"
#include "libc/string.h"

static short DdsReadShort(void);
static float DdsReadFloat2(void);
static float DdsReadFloat4(void);
static int DramaDemoInit(struct DramaDemo_PlayInfo *info);
static void DramaDemoAnimationStart(short *adr_anim);
static int DdsPlay(struct DramaDemo_PlayInfo *info);
static void DdsPlayKey(void);
static void DdsPlayCamera(void);
static void DdsPlayLight(int no);
static void DdsPlayCharacter(int no);
static void RotationToInterest(float *position, float *rotation, float *interest, float *roll);

static struct DramaDemo_AnimInfo anim_info[79] = {
    { "hhh_jms_pos", 259, 0, 1000, 1165 },
    { "hhl_jms_pos", 258, 0, 1166, 1187 },
    { "hll_jms_pos", 257, 0, 952, 972 },
    { "lll_jms_pos", 256, 0, 973, 979 },
    { "hhh_mar_pos", 262, 0, 3000, 3029 },
    { "lll_mar_pos", 261, 0, 42, 46 },
    { "lau_pos", 260, 0, 2500, 2540 },
    { "hhh_edi_pos", 264, 0, 4503, 4527 },
    { "lll_edi_pos", 517, 0, 6801, 6801 },
    { "agl_pos", 263, 0, 3500, 3531 },
    { "mry_pos", 265, 0, 4002, 4020 },
    { "mxx_pos", 266, 0, 4102, 4110 },
    { "inu_pos", 269, 0, 4901, 4902 },
    { "boat_pos", 267, 0, 56, 58 },
    { "scu_pos", 512, 0, 6003, 6009 },
    { "pap_pos", 516, 0, 6602, 6607 },
    { "red_pos", 520, 0, 6300, 6349 },
    { "mkn_1_pos", 513, 0, 6102, 6104 },
    { "mkn_2_pos", 513, 1, 6105, 6107 },
    { "ike_1_pos", 515, 0, 6502, 6506 },
    { "ike_2_pos", 515, 1, 6507, 6511 },
    { "oni_pos", 521, 0, 6352, 6353 },
    { "oni_1_pos", 521, 0, 6354, 6356 },
    { "oni_2_pos", 521, 1, 6357, 6359 },
    { "bos_pos", 518, 0, 6701, 6720 },
    { "dmr_pos", 1061, 0, 10163, 10164 },
    { "dm2_pos", 1067, 0, 10165, 10166 },
    { "i_keycou_pos", 1026, 0, 10001, 10001 },
    { "i_radio_pos", 1027, 0, 10002, 10018 },
    { "i_kakuzai_pos", 1028, 0, 10019, 10024 },
    { "i_flight_pos", 1031, 0, 10035, 10040 },
    { "i_bear_pos", 1029, 0, 10025, 10030 },
    { "i_bear2_pos", 1030, 0, 10031, 10034 },
    { "i_letter_pos", 1032, 0, 10041, 10048 },
    { "i_handgun_pos", 1037, 0, 10064, 10066 },
    { "i_magazine_pos", 1038, 0, 10067, 10069 },
    { "i_lring_pos", 1039, 0, 10075, 10076 },
    { "i_purse_pos", 1046, 0, 10096, 10096 },
    { "i_j_light_pos", 1048, 0, 10100, 10112 },
    { "i_keyspe_pos", 1047, 0, 10097, 10099 },
    { "i_mtablet_1_pos", 1051, 0, 10122, 10123 },
    { "i_mtablet_2_pos", 1051, 1, 10124, 10125 },
    { "i_mdrug_pos", 1050, 0, 10120, 10121 },
    { "i_needle_pos", 1053, 0, 10126, 10126 },
    { "i_keyelevator_p", 1054, 0, 10127, 10127 },
    { "i_key_clock_pos", 1055, 0, 10129, 10131 },
    { "i_photo_pos", 1060, 0, 10150, 10159 },
    { "i_knife_pos", 1059, 0, 10140, 10149 },
    { "i_juice_pos", 1069, 0, 10188, 10188 },
    { "i_video_pos", 1074, 0, 10220, 10220 },
    { "i_colt_pos", 1066, 0, 10175, 10187 },
    { "i_hari_pos", 1072, 0, 10213, 10214 },
    { "i_letterm_pos", 1075, 0, 10228, 10240 },
    { "i_headphone_pos", 1076, 0, 10246, 10246 },
    { "b_tel_pos", 1033, 0, 10049, 10054 },
    { "b_d00_pos", 1034, 0, 10055, 10060 },
    { "b_d01_pos", 1049, 0, 10113, 10119 },
    { "b_d02_pos", 1042, 0, 10085, 10094 },
    { "b_d03_pos", 1040, 0, 10077, 10078 },
    { "b_d05_pos", 1056, 0, 10132, 10134 },
    { "b_rei_pos", 1035, 0, 10070, 10072 },
    { "b_doo_pos", 1036, 0, 10061, 10063 },
    { "b_pia_pos", 1041, 0, 10079, 10084 },
    { "b_ami_pos", 1044, 0, 10095, 10095 },
    { "b_rop_pos", 1052, 6, 10128, 10128 },
    { "b_clo_pos", 1058, 0, 10136, 10139 },
    { "b_tan_pos", 1063, 0, 10215, 10216 },
    { "b_dor_pos", 1068, 0, 10217, 10219 },
    { "b_d08_pos", 1073, 0, 10189, 10196 },
    { "b_piz_pos", 1070, 0, 10197, 10204 },
    { "b_bol_pos", 1071, 0, 10205, 10212 },
    { "b_ori_pos", 1307, 0, 10221, 10221 },
    { "b_d06_pos", 1065, 0, 10167, 10170 },
    { "b_sti_pos", 1079, 0, 10171, 10174 },
    { "b_d10_pos", 1080, 0, 10222, 10227 },
    { "b_hul_pos", 1077, 0, 10242, 10243 },
    { "b_cha_pos", 1078, 0, 10244, 10245 },
    { "mx2_pos", 1335, 0, 10241, 10241 },
    { "b_do4_pos", 1062, 0, 10160, 10162 },
};

int demo_status;
float demo_frame;
float total_demo_frame;
int demo_number;
int demo_counter;
int sbt_msg_no;
DramaDemo_KeyFrame last;
DramaDemo_KeyFrame next;
DramaDemo_KeyFrame base;
struct SubCharacter *chara_p[7];
void *adr_dds;
short total_light;
short point_light;
short spot_light;
short infinite_light;
int character_number;
unsigned short demo_anim_no;
unsigned short demo_msg_no;
unsigned short demo_voice_no;
float msg_frame;
struct DramaDemo_MessageTime *sbt_msg_time;
int sbt_str_no;

static short DdsReadShort(void) {
    unsigned char c_work[2];

    c_work[0] = *((char *)adr_dds)++;
    c_work[1] = *((char *)adr_dds)++;
    return *(short *)c_work;
}

/** Reads the next 16-bit float (1 sign, 5 exponent, 10 mantissa bits) of the demo data. */
/* Matching: compiled without the global optimizer (the pragma), as the same decoder in event.c; with it on,
 * sig and coe swap registers (docs/matching-notes.md#demoview-ddsreadfloat2). */
#pragma push
#pragma global_optimizer off
static float DdsReadFloat2(void) {
    int work;
    int coe;
    int exp;
    int sig;

    work = 0;
    ((char *)&work)[0] = *(char *)adr_dds;
    adr_dds = (char *)adr_dds + 1;
    ((char *)&work)[1] = *(char *)adr_dds;
    adr_dds = (char *)adr_dds + 1;
    sig = (work >> 15) & 1;
    exp = (work >> 10) & 0x1F;
    coe = work & 0x3FF;
    work = (sig << 31) | ((exp + 0x70) << 23) | (coe << 13);
    return *(float *)&work;
}
#pragma pop

static float DdsReadFloat4(void) {
    char c_work[4];

    c_work[0] = *((char *)adr_dds)++;
    c_work[1] = *((char *)adr_dds)++;
    c_work[2] = *((char *)adr_dds)++;
    c_work[3] = *((char *)adr_dds)++;
    return *(float *)c_work;
}

/** Plays a drama demo, one frame per call: sets it up on the first call, starts and syncs
 * its sound stream, plays the script; the skip button ends it.
 * @param info the demo to play
 * @return non-zero when the demo has ended (or was skipped) */
int DramaDemoMain(struct DramaDemo_PlayInfo *info) {
    static float stop_counter;
    int ret;

    if (ev_s_step == 0) {
        DramaDemoInit(info);
        ev_s_step = 1;
    }
    Sh2sys.main_status |= 0x40;
    demo_number = info->demo_no;
    if (info->stream_no) {
        if (!((demo_status >> 7) & 1)) {
            if (!(shSdStat() & 0xF0)) {
                shSdCall(info->stream_no, 0, 0, 0);
            }
            shResetDF();
            demo_status |= 0x80;
        } else if (!((demo_status >> 8) & 1) && !(demo_frame < info->stream_start)) {
            if ((shSdStat() & 0xF0) == 0x40) {
                stop_counter += shGetDTreal();
                printf("Stream read stop: %5.3f\n", stop_counter);
                shSdCall(0x3F7, 0, 0, 0);
                shResetDF();
                demo_status |= 0x100;
            } else {
                stop_counter += shGetDTreal();
                shSetDFZero();
            }
        }
    }
    ret = DdsPlay(info);
    if (!((demo_status >> 9) & 1) && shPadTrigger(0, key_config.skip)) {
        ret = 1;
        fontClear();
        shSdCall(0x3F4, 0, 0, 0);
    }
    return ret;
}

static int DramaDemoInit(struct DramaDemo_PlayInfo *info) {
    float dummy[2][4];
    char buf[32];
    unsigned short s_work;
    unsigned char c_work;
    int i;
    int j;

    dummy[0][0] = sh2jms.player->pos.x;
    dummy[0][1] = sh2jms.player->pos.y;
    dummy[0][2] = sh2jms.player->pos.z;
    dummy[0][3] = 1.0f;
    _sceVu0UnitVector(dummy[1]);
    adr_dds = info->adr_dds_top;
    /* Matching: the assert bakes its original line number into the object. */
#line 323
    assert(!strncmp( adr_dds ,"dds" , 3 ));
    adr_dds = (char *)adr_dds + 0x10;
    total_demo_frame = DdsReadShort();
    adr_dds = (char *)adr_dds + 2;
    point_light = *((char *)adr_dds)++;
    spot_light = *((char *)adr_dds)++;
    infinite_light = *((char *)adr_dds)++;
    adr_dds = (char *)adr_dds + 1;
    character_number = *((char *)adr_dds)++;
    for (i = 0; i < character_number; i++) {
        strncpy(buf, adr_dds, 0x10);
        adr_dds = (char *)adr_dds + 0x10;
        for (j = 0; ; j++) {
            if (strcmp(buf, anim_info[j].name) == 0) {
                chara_p[i] = shCharacterGetSubCharacter(anim_info[j].kind, anim_info[j].id);
                if (chara_p[i] == NULL) {
                    chara_p[i] = CharaWorkCreate(anim_info[j].kind, anim_info[j].id, dummy[0], dummy[1], 0);
                }
                /* Matching: the assert bakes its original line number into the object. */
#line 345
                assert(chara_p[i] != 0);
                break;
            }
        }
    }
    shQzero(&base, 0x360);
    shQzero(&next, 0x360);
    msg_frame = 0.0f;
    demo_frame = 0.0f;
    demo_counter = 0;
    total_light = point_light + spot_light + infinite_light;
    /* Matching: the assert bakes its original line number into the object. */
#line 358
    assert(total_light <= 6);
    demo_status = 0;
    demo_anim_no = 0;
    demo_msg_no = 0;
    demo_voice_no = 0;
    s_work = DdsReadShort();
    while (1) {
        c_work = *((unsigned char *)adr_dds)++;
        if (c_work == 0xFF) {
            break;
        }
        if (c_work == 0) {
            DdsPlayKey();
        } else if (c_work == 1) {
            DdsPlayCamera();
        } else if (c_work - 2 < total_light) {
            DdsPlayLight(c_work - 2);
        } else {
            DdsPlayCharacter(c_work - total_light - 2);
        }
    }
    if ((demo_status >> 2) & 1) {
        DramaDemoAnimationStart(info->adr_anim);
        demo_status &= ~4;
    }
    memcpy(&last, &next, 0x360);
    return 1;
}

static void DramaDemoAnimationStart(short *adr_anim) {
    short anim;
    int i;
    int j;
    int k;

    if (adr_anim == NULL) {
        return;
    }
    for (i = 0; i < character_number; i++) {
        anim = adr_anim[demo_anim_no];
        for (j = 0; ; j++) {
            if (anim_info[j].start <= anim && anim <= anim_info[j].end) {
                break;
            }
        }
        for (k = 0; ; k++) {
            if (chara_p[k]->kind == anim_info[j].kind && chara_p[k]->id == anim_info[j].id) {
                break;
            }
        }
        switch (anim_info[j].kind) {
        case 0x103:
        case 0x102:
            shCharacterHumanDJAMESAnimeSet(chara_p[k], anim);
            break;
        case 0x101:
        case 0x100:
            shCharacterHumanPJAMESAnimeSet(chara_p[k], anim);
            break;
        case 0x106:
            shCharacterHumanDMARAnimeSet(chara_p[k], anim);
            break;
        case 0x105:
            shCharacterHumanMARAnimeSet(chara_p[k], anim);
            break;
        case 0x104:
            shCharacterHumanLAUAnimeSet(chara_p[k], anim);
            break;
        case 0x107:
            shCharacterHumanAGLAnimeSet(chara_p[k], anim);
            break;
        case 0x108:
            shCharacterHumanEDIAnimeSet(chara_p[k], anim);
            break;
        case 0x109:
            shCharacterHumanMRYAnimeSet(chara_p[k], anim);
            break;
        case 0x10D:
            shCharacterHumanINUAnimeSet(chara_p[k], anim);
            break;
        case 0x10B:
            shCharacterHumanBOTAnimeSet(chara_p[k], anim);
            break;
        case 0x200:
            shCharacterEnemySCUAnimeSet(chara_p[k], anim);
            break;
        case 0x204:
            shCharacterEnemyPAPAnimeSet(chara_p[k], anim);
            break;
        case 0x208:
            shCharacterEnemyREDAnimeSet(chara_p[k], anim);
            break;
        case 0x201:
            shCharacterEnemyMKNAnimeSet(chara_p[k], anim);
            break;
        case 0x203:
            shCharacterEnemyIKEAnimeSet(chara_p[k], anim);
            break;
        case 0x209:
            shCharacterEnemyONIAnimeSet(chara_p[k], anim);
            break;
        case 0x205:
            shCharacterEnemyEDBAnimeSet(chara_p[k], anim);
            break;
        case 0x206:
            shCharacterEnemyBOSAnimeSet(chara_p[k], anim);
            break;
        default:
            shCharacterObjectAnimeSet(chara_p[k], anim);
            break;
        }
        demo_anim_no++;
    }
}

static int DdsPlay(struct DramaDemo_PlayInfo *info) {
    float position[4];
    float interest[4];
    float color[4];
    float normal[4];
    float falloff_s;
    float falloff_e;
    float cone;
    float roll;
    float plane;
    float frm_dcm;
    float next_data_frame;
    unsigned short s_work;
    unsigned char node_no;
    int i;
    int j;
    int k;

    DramaDemoFade();
    if ((int)last.frame < (int)demo_frame) {
        if ((demo_status >> 2) & 1) {
            DramaDemoAnimationStart(info->adr_anim);
            demo_status &= ~4;
        }
        demo_status &= ~1;
        while (1) {
            demo_status &= ~2;
            s_work = DdsReadShort();
            adr_dds = (char *)adr_dds - 2;
            next_data_frame = s_work;
            if (s_work == 0xFFFF || !(next_data_frame <= demo_frame + 1.0f)) {
                break;
            }
            memcpy(&last, &next, 0x360);
            adr_dds = (char *)adr_dds + 2;
            while (1) {
                node_no = *((unsigned char *)adr_dds)++;
                if (node_no == 0xFF) {
                    break;
                }
                if (node_no == 0) {
                    DdsPlayKey();
                } else if (node_no == 1) {
                    DdsPlayCamera();
                } else if (node_no - 2 < total_light) {
                    DdsPlayLight(node_no - 2);
                } else {
                    DdsPlayCharacter(node_no - total_light - 2);
                }
            }
        }
    }
    if (playing.subtitles == 1 && info->adr_msg_time) {
        if (info->adr_msg_time[demo_msg_no].end < ftoi(msg_frame)) {
            fontClear();
            demo_msg_no++;
            demo_status &= ~8;
        }
        if (info->adr_msg_time[demo_msg_no].start < ftoi(msg_frame) && !((demo_status >> 3) & 1)) {
            fontMessageNum(msg_buffer, info->msg_start + demo_msg_no);
            demo_status |= 8;
        }
    }
    if (demo_status & 1) {
        frm_dcm = 0.0f;
    } else {
        frm_dcm = demo_frame - (float)ftoi((float)ftoi(demo_frame));
    }
    for (i = 0; i < 3; i++) {
        position[i] = next.camera.position[i] * frm_dcm + last.camera.position[i] * (1.0f - frm_dcm);
        interest[i] = next.camera.interest[i] * frm_dcm + last.camera.interest[i] * (1.0f - frm_dcm);
    }
    position[0] += info->add_pos_x;
    position[2] += info->add_pos_z;
    interest[0] += info->add_pos_x;
    interest[2] += info->add_pos_z;
    position[3] = interest[3] = 1.0f;
    roll = next.camera.roll * frm_dcm + last.camera.roll * (1.0f - frm_dcm);
    plane = next.camera.plane * frm_dcm + last.camera.plane * (1.0f - frm_dcm);
    vcSetEventCamParamRefView(position, NULL, interest, NULL, roll, 1);
    VbScreenInfo.scr_z = 1.14702f * plane;
    vbCalcViewScreenMatrix();
    vcMoveAndSetCamera(0, 0, 0, 0, 0, 0, 0, 0);
    for (i = 0, j = 0; j < point_light; i++, j++) {
        if (next.light[i].visible) {
            for (k = 0; k < 3; k++) {
                position[k] = next.light[i].position[k] * frm_dcm + last.light[i].position[k] * (1.0f - frm_dcm);
                color[k] = next.light[i].color[k] * frm_dcm + last.light[i].color[k] * (1.0f - frm_dcm);
            }
            sh2gfw_Set_DemoPointLight(j, position, color,
                                      next.light[i].falloff[0] * frm_dcm + last.light[i].falloff[0] * (1.0f - frm_dcm),
                                      next.light[i].falloff[1] * frm_dcm + last.light[i].falloff[1] * (1.0f - frm_dcm));
        }
    }
    for (j = 0; j < spot_light; i++, j++) {
        if (next.light[i].visible) {
            for (k = 0; k < 3; k++) {
                position[k] = next.light[i].position[k] * frm_dcm + last.light[i].position[k] * (1.0f - frm_dcm);
                interest[k] = next.light[i].interest[k] * frm_dcm + last.light[i].interest[k] * (1.0f - frm_dcm);
                color[k] = next.light[i].color[k] * frm_dcm + last.light[i].color[k] * (1.0f - frm_dcm);
            }
            _shSubVector(normal, interest, position);
            _shNormalize(normal, normal);
            falloff_s = next.light[i].falloff[0] * frm_dcm + last.light[i].falloff[0] * (1.0f - frm_dcm);
            falloff_e = next.light[i].falloff[1] * frm_dcm + last.light[i].falloff[1] * (1.0f - frm_dcm);
            cone = next.light[i].cone[0] * frm_dcm + last.light[i].cone[0] * (1.0f - frm_dcm);
            position[3] = 1.0f;
            normal[3] = 0.0f;
            sh2gfw_Set_SpotLight(normal, position, color, 0, falloff_s, falloff_e, cone);
        }
    }
    for (j = 0; j < infinite_light; i++, j++) {
        if (next.light[i].visible) {
            for (k = 0; k < 3; k++) {
                position[k] = next.light[i].position[k] * frm_dcm + last.light[i].position[k] * (1.0f - frm_dcm);
                color[k] = next.light[i].color[k] * frm_dcm + last.light[i].color[k] * (1.0f - frm_dcm);
            }
            *(u_long128 *)normal = 0;
            _shSubVector(normal, normal, position);
            _shNormalize(normal, normal);
            normal[3] = 0.0f;
            sh2gfw_Set_PallarelLight(normal, color, j + 1);
        }
    }
    for (i = 0; i < character_number; i++) {
        chara_p[i]->pos.x = next.character[i].position[0] * frm_dcm + info->add_pos_x + last.character[i].position[0] * (1.0f - frm_dcm);
        chara_p[i]->pos.y = next.character[i].position[1] * frm_dcm + last.character[i].position[1] * (1.0f - frm_dcm);
        chara_p[i]->pos.z = next.character[i].position[2] * frm_dcm + info->add_pos_z + last.character[i].position[2] * (1.0f - frm_dcm);
        chara_p[i]->rot.y = 3.1415927f;
    }
    last.frame = demo_frame;
    if ((demo_status >> 5) & 1) {
        if ((shSdStat() & 0xF0) == 0x10 || (shSdStat() & 0xF0) == 0x50) {
            shResetDF();
            demo_status &= ~0x20;
        }
    }
    if (demo_frame < total_demo_frame) {
        demo_frame += 30.0f * shGetDT();
        demo_counter++;
    }
    msg_frame += 30.0f * shGetDT();
    if (!(demo_frame < total_demo_frame)) {
        return 1;
    }
    if (demo_frame - (float)ftoi((float)ftoi(demo_frame)) > 0.9999f) {
        demo_frame = 1.0f + (float)ftoi((float)ftoi(demo_frame));
    }
    if (msg_frame - (float)ftoi((float)ftoi(msg_frame)) > 0.9999f) {
        msg_frame = 1.0f + (float)ftoi((float)ftoi(msg_frame));
    }
    return 0;
}

static void DdsPlayKey(void) {
    unsigned char c_work;

    while (1) {
        c_work = *((unsigned char *)adr_dds)++;
        switch (c_work) {
        case 0x10:
            demo_status |= 1;
            demo_status |= 2;
            break;
        case 0x11:
            demo_status |= 4;
            break;
        case 0x14:
            demo_status |= 0x10;
            break;
        case 0x12:
            break;
        case 0x13:
            break;
        case 0xB:
            return;
        }
    }
}

static void DdsPlayCamera(void) {
    float camera_rotation[4];
    unsigned char c_work;
    int i;

    while (1) {
        c_work = *((unsigned char *)adr_dds)++;
        switch (c_work) {
        case 3:
            if ((demo_status >> 1) & 1) {
                for (i = 0; i < 3; i++) {
                    next.camera.position[i] = base.camera.position[i] = DdsReadFloat4();
                }
            } else {
                for (i = 0; i < 3; i++) {
                    next.camera.position[i] = base.camera.position[i] + DdsReadFloat2();
                }
            }
            break;
        case 4:
            if ((demo_status >> 1) & 1) {
                for (i = 0; i < 3; i++) {
                    next.camera.interest[i] = base.camera.interest[i] = DdsReadFloat4();
                }
            } else {
                for (i = 0; i < 3; i++) {
                    next.camera.interest[i] = base.camera.interest[i] + DdsReadFloat2();
                }
            }
            break;
        case 5:
            camera_rotation[0] = DdsReadFloat2();
            camera_rotation[1] = DdsReadFloat2();
            camera_rotation[2] = DdsReadFloat2();
            camera_rotation[3] = 0.0f;
            RotationToInterest(next.camera.position, camera_rotation, next.camera.interest, &next.camera.roll);
            break;
        case 6:
            next.camera.roll = DdsReadFloat2();
            break;
        case 7:
            next.camera.plane = DdsReadFloat4();
            break;
        case 11:
        default:
            return;
        }
    }
}

static void DdsPlayLight(int no) {
    float light_rotation[4];
    unsigned char c_work;
    int i;

    while (1) {
        c_work = *((unsigned char *)adr_dds)++;
        switch (c_work) {
        case 3:
            if ((demo_status >> 1) & 1) {
                for (i = 0; i < 3; i++) {
                    next.light[no].position[i] = base.light[no].position[i] = DdsReadFloat4();
                }
            } else {
                for (i = 0; i < 3; i++) {
                    next.light[no].position[i] = base.light[no].position[i] + DdsReadFloat2();
                }
            }
            break;
        case 4:
            if ((demo_status >> 1) & 1) {
                for (i = 0; i < 3; i++) {
                    next.light[no].interest[i] = base.light[no].interest[i] = DdsReadFloat4();
                }
            } else {
                for (i = 0; i < 3; i++) {
                    next.light[no].interest[i] = base.light[no].interest[i] + DdsReadFloat2();
                }
            }
            break;
        case 5:
            light_rotation[0] = DdsReadFloat2();
            light_rotation[1] = DdsReadFloat2();
            light_rotation[2] = DdsReadFloat2();
            light_rotation[3] = 0.0f;
            RotationToInterest(next.light[no].position, light_rotation, next.light[no].interest, NULL);
            break;
        case 8:
            next.light[no].color[0] = DdsReadFloat2();
            next.light[no].color[1] = DdsReadFloat2();
            next.light[no].color[2] = DdsReadFloat2();
            break;
        case 9:
            next.light[no].falloff[0] = DdsReadFloat2();
            next.light[no].falloff[1] = DdsReadFloat2();
            break;
        case 10:
            next.light[no].cone[0] = DdsReadFloat2();
            next.light[no].cone[1] = DdsReadFloat2();
            break;
        case 1:
            next.light[no].visible = 1;
            break;
        case 2:
            next.light[no].visible = 0;
            break;
        case 11:
        default:
            return;
        }
    }
}

static void DdsPlayCharacter(int no) {
    unsigned char c_work;
    int i;

    while (1) {
        c_work = *((unsigned char *)adr_dds)++;
        switch (c_work) {
        case 1:
            next.character[no].visible = 1;
            break;
        case 2:
            next.character[no].visible = 0;
            break;
        case 3:
            if ((demo_status >> 1) & 1) {
                for (i = 0; i < 3; i++) {
                    next.character[no].position[i] = base.character[no].position[i] = DdsReadFloat4();
                }
            } else {
                for (i = 0; i < 3; i++) {
                    next.character[no].position[i] = base.character[no].position[i] + DdsReadFloat2();
                }
            }
            break;
        case 11:
        default:
            return;
        }
    }
}

/** Skips a demo to its end: fades, then applies the rest of the script up to the last frame
 * so the characters end where the demo leaves them.
 * @param info the demo */
void DramaDemoSkipLast(struct DramaDemo_PlayInfo *info) {
    float position[4];
    float interest[4];
    float roll;
    float plane;
    unsigned short s_work;
    unsigned char node_no;
    int i;

    DramaDemoFade();
    demo_frame = total_demo_frame;
    if ((int)last.frame < (int)demo_frame) {
        while (1) {
            if ((demo_status >> 2) & 1) {
                DramaDemoAnimationStart(info->adr_anim);
                demo_status &= ~4;
            }
            demo_status &= ~1;
            s_work = DdsReadShort();
            adr_dds = (char *)adr_dds - 2;
            if (s_work == 0xFFFF) {
                break;
            }
            memcpy(&last, &next, 0x360);
            adr_dds = (char *)adr_dds + 2;
            while (1) {
                demo_status &= ~2;
                node_no = *((unsigned char *)adr_dds)++;
                if (node_no == 0xFF) {
                    break;
                }
                if (node_no == 0) {
                    DdsPlayKey();
                } else if (node_no == 1) {
                    DdsPlayCamera();
                } else if (node_no - 2 < total_light) {
                    DdsPlayLight(node_no - 2);
                } else {
                    DdsPlayCharacter(node_no - total_light - 2);
                }
            }
        }
    }
    demo_status |= 0x40;
    for (i = 0; i < 3; i++) {
        position[i] = last.camera.position[i];
        interest[i] = last.camera.interest[i];
    }
    position[3] = interest[3] = 0.0f;
    roll = last.camera.roll;
    plane = last.camera.plane;
    vcSetEventCamParamRefView(position, NULL, interest, NULL, roll, 1);
    VbScreenInfo.scr_z = 1.14702f * plane;
    vbCalcViewScreenMatrix();
    vcMoveAndSetCamera(0, 0, 0, 0, 0, 0, 0, 0);
    for (i = 0; i < character_number; i++) {
        chara_p[i]->pos.x = last.character[i].position[0];
        chara_p[i]->pos.y = last.character[i].position[1];
        chara_p[i]->pos.z = last.character[i].position[2];
        chara_p[i]->rot.y = 3.1415927f;
    }
    last.frame = demo_frame;
}

static void RotationToInterest(float *position, float *rotation, float *interest, float *roll) {
    float matrix[4][4];
    float vector[4];

    _sceVu0UnitMatrix(matrix);
    shRotMatrixZ(matrix, matrix, rotation[2]);
    shRotMatrixX(matrix, matrix, rotation[0]);
    shRotMatrixY(matrix, matrix, rotation[1]);
    _sceVu0UnitVector(vector);
    vector[2] = 1.0f;
    _sceVu0UnitVector(matrix[3]);
    _sceVu0ApplyMatrix(interest, matrix, vector);
    _shAddVector(interest, interest, position);
    if (roll) {
        *roll = rotation[2];
    }
}

/** Starts a fade to black (a soft filter picture) unless one is already running. */
void DramaDemoFade(void) {
    struct PicDraw_Data pic;

    if (!Check_Filter_Soft()) {
        shQzero(&pic, sizeof(pic));
        pic.r = 0;
        pic.g = 0;
        pic.b = 0;
        pic.status |= 0x10;
        pic.test_ate = 0;
        pic.test_atst = 0;
        pic.test_aref = 0;
        pic.test_afail = 0;
        pic.test_date = 0;
        pic.test_datm = 0;
        pic.test_zte = 1;
        pic.test_ztst = 1;
        pic.status |= 0x40;
        pic.x0 = -0x1000;
        pic.y0 = -0x1000;
        pic.x1 = 0x1000;
        pic.y1 = -0xC00;
        pic.status |= 2;
        PictureDraw(&pic);
        pic.x0 = -0x1000;
        pic.y0 = 0xC00;
        pic.x1 = 0x1000;
        pic.y1 = 0x1000;
        pic.status |= 2;
        PictureDraw(&pic);
    }
}

/** Returns the number of the demo playing (or last played). */
int DramaDemoNumber(void) {
    return demo_number;
}

static float sbt_timer;

/** Starts the subtitles of a demo or event.
 * @param msg_time start/end frames of each subtitle line
 * @param msg_no first message number
 * @param str_no voice stream to play with them, or 0
 * @param timer start time (0: wait for the stream to start) */
void SubtitlesExec(struct DramaDemo_MessageTime *msg_time, int msg_no, int str_no, float timer) {
    sbt_msg_time = msg_time;
    sbt_msg_no = msg_no;
    sbt_str_no = str_no;
    sbt_timer = timer;
}

/** Per-frame subtitle update: starts the voice stream and shows each line (when subtitles are
 * on) between its start and end frame. */
void SubtitlesManager(void) {
    if (sbt_msg_no) {
        if (sbt_str_no == 0 && !(shSdStat() & 0xF0)) {
            sbt_msg_no = 0;
            return;
        }
        if (sbt_str_no && (shSdStat() & 0xF0)) {
            return;
        }
        if (sbt_str_no) {
            shSdCall(sbt_str_no, 0, 0, 0);
            sbt_str_no = 0;
        }
        if (sbt_timer == 0.0f) {
            if ((shSdStat() & 0xF0) != 0x40) {
                return;
            }
            shSdCall(0x3F7, 0, 0, 0);
            demo_status &= ~8;
        }
        sbt_timer += 30.0f * shGetDT();
        if (playing.subtitles) {
            if (sbt_msg_time->end < ftoi(sbt_timer)) {
                fontClear();
                sbt_msg_time++;
                demo_status &= ~8;
            }
            if (sbt_msg_time->start < ftoi(sbt_timer) && !((demo_status >> 3) & 1)) {
                fontMessageNum(msg_buffer, sbt_msg_no);
                sbt_msg_no++;
                demo_status |= 8;
            }
        }
    }
}
