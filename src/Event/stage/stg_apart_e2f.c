/*
 * stg_apart_e2f.c: stage overlay for the apartments, east building 2F: the clock puzzle (hint,
 * time, key), the hole and the dust chute, the canned juice, the corpse, the light, room 202's key
 * and the ending hints.
 * The stage's event programs (EvProg*), room set-up and Stage_Data (stage_apart_e2f).
 */
#include "sh2.h"
#include "asm_helpers.h"

/* Next program step. Matching: the do/while leaves the original's nop before a case label it
 * falls into. */
#define EV_STEP(n) do { ev_p_step = (n); ev_s_step = 0; } while (0)

static int EvProgThreeNameOnWall(void);
static int EvProgHintOfClockSet(void);
static int EvProgClockTime(void);
static int EvProgClockNeedleMove(void);
static void EvProgSubClockNeedleDraw(int open_or_close);
static int EvProgUseClockKey(void);
static int EvProgTryMoveClock(void);
static int EvProgUseEmergencyKey(void);
static int EvProgAnyoneInHole(void);
static int EvProgNooneInHole(void);
static int EvProgLookDustChute(void);
static int EvProgUseCannedJuice(void);
static int EvProgNoFaceCorpse(void);
static int EvProgAnyoneCry(void);
static int EvProgGetLight(void);
static int EvProgGetApart202Key(void);
static int EvProgEndHintRecoveryRead(void);
static int EvProgEndHintMariaRead(void);
static int EvProgEndHintSuicideRead(void);
static int EvProgUseApart202Key(void);
static int EvProgMonkeyKick206(void);
static int EvCharaDataClear(int room);
static void EvRoomInit(void);
static void EvSoundCallAfterLoad(void);
static void EvAllTimeFunc(void);
static void Delete_RedPointLight(void);

static struct _AnimeInfo pjames_stage_anim[22] = {
    { 0x4E21, 20, 1024, 1024, 1043, 0, 10 },
    { 0x4E22, 68, 2048, 1044, 1111, 0, 0 },
    { 0x4E27, 30, 1024, 1112, 1141, 0, 0 },
    { 0x4E28, 15, 768, 1142, 1156, 0, 0 },
    { 0x4E29, 15, 768, 1157, 1171, 0, 0 },
    { 0x4E2A, 30, 768, 1172, 1201, 0, 0 },
    { 0x4E2B, 30, 768, 1202, 1231, 0, 0 },
    { 0x4E2C, 15, 1024, 1236, 1246, 0, 0 },
    { 0x4E2D, 15, 1024, 1251, 1261, 0, 0 },
    { 0x4E2E, 30, 640, 1262, 1291, 0, 0 },
    { 0x4E2F, 30, 640, 1292, 1321, 0, 0 },
    { 0x4E30, 15, 1024, 1326, 1336, 0, 0 },
    { 0x4E31, 15, 1024, 1341, 1351, 0, 0 },
    { 0x4E32, 30, 1024, 1352, 1381, 0, 0 },
    { 0x4E33, 30, 1024, 1382, 1411, 0, 0 },
    { 0x4E34, 30, 1024, 1412, 1441, 0, 0 },
    { 0x4E39, 15, 1024, 1446, 1456, 0, 0 },
    { 0x4E3A, 15, 1024, 1461, 1471, 0, 0 },
    { 0x4E3B, 30, 1024, 1472, 1501, 0, 0 },
    { 0x4E3C, 30, 1024, 1502, 1531, 0, 0 },
    { 0x4E45, 15, 1024, 1532, 1546, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0 },
};

static unsigned char ev_pos[778] = {
    0x73, 0x2F, 0x72, 0xC7, 0x00, 0x80, 0x73, 0xFC, 0x6C, 0x47, 0x6B, 0x5B, 0x00, 0x80, 0x6B, 0x5B,
    0x3E, 0xDD, 0x00, 0x00, 0x3E, 0xDD, 0x00, 0x6C, 0x9D, 0xC6, 0x00, 0x80, 0x00, 0x40, 0x9C, 0x46,
    0x00, 0x00, 0xB0, 0x58, 0x40, 0x52, 0x40, 0x5A, 0xD0, 0x5B, 0x40, 0x5A, 0xB0, 0x5C, 0xB0, 0x58,
    0xB0, 0x5C, 0x00, 0x80, 0x00, 0xF0, 0xA0, 0xC6, 0x00, 0x80, 0x00, 0x24, 0x90, 0x46, 0x1A, 0x64,
    0x00, 0x3C, 0xA5, 0xC6, 0x00, 0x80, 0x00, 0x00, 0x96, 0x46, 0xB0, 0x5C, 0x00, 0x80, 0xB0, 0x5C,
    0xD0, 0xDB, 0x00, 0xEC, 0xC2, 0xC7, 0x00, 0x80, 0x00, 0x80, 0x3B, 0x46, 0x40, 0x62, 0x00, 0x80,
    0x40, 0x5E, 0x40, 0xE2, 0x7F, 0xB9, 0x64, 0xC7, 0x00, 0x80, 0x88, 0x70, 0x95, 0x46, 0xCE, 0xDC,
    0xE7, 0x5E, 0x00, 0xC3, 0x66, 0xC7, 0x00, 0x80, 0x00, 0x74, 0xA4, 0x46, 0xB0, 0x58, 0x00, 0xDB,
    0xC9, 0xC7, 0x00, 0x80, 0x00, 0xE0, 0x12, 0x46, 0xB0, 0x5C, 0x40, 0xD2, 0xB0, 0x5C, 0xB0, 0xDC,
    0xFE, 0x3B, 0xB4, 0xC6, 0x00, 0x80, 0x00, 0x1B, 0x69, 0x47, 0x40, 0x5A, 0xF4, 0x08, 0x66, 0xC7,
    0x00, 0x80, 0xDA, 0x38, 0xA6, 0x46, 0xAC, 0x5C, 0x73, 0xB8, 0x64, 0xC7, 0x00, 0x00, 0x00, 0xCA,
    0xB2, 0x46, 0x3C, 0x5D, 0xF3, 0xE6, 0x65, 0xC7, 0x00, 0x00, 0x00, 0xCA, 0xB2, 0x46, 0x3C, 0x5D,
    0x73, 0xD4, 0x63, 0xC7, 0x00, 0x80, 0x2B, 0xAB, 0xB4, 0x46, 0x40, 0xDA, 0x00, 0x80, 0x40, 0xDA,
    0xD3, 0xDB, 0xF3, 0x17, 0x6C, 0xC7, 0x00, 0x80, 0x5A, 0xCD, 0x94, 0x46, 0x2F, 0x5B, 0x73, 0xBC,
    0x67, 0xC7, 0x00, 0x80, 0x3B, 0xB6, 0xB2, 0x46, 0x40, 0x5E, 0xD3, 0x5B, 0x73, 0x2C, 0x66, 0xC7,
    0x00, 0x80, 0x3B, 0xB6, 0xB2, 0x46, 0x40, 0xDE, 0x00, 0x80, 0x40, 0xDE, 0xD3, 0x5B, 0x73, 0x2C,
    0x66, 0xC7, 0x00, 0x80, 0x3B, 0xB6, 0xB2, 0x46, 0x00, 0x00, 0xD3, 0x5B, 0x40, 0xDE, 0xD3, 0x5B,
    0x73, 0x9C, 0x64, 0xC7, 0x00, 0x80, 0x78, 0xA1, 0xB4, 0x46, 0x40, 0x5E, 0x00, 0xA4, 0x67, 0xC7,
    0x00, 0x80, 0x00, 0x62, 0x5E, 0x47, 0x40, 0x5E, 0x8D, 0xFF, 0xE9, 0xC7, 0x00, 0x80, 0xFB, 0xDC,
    0x35, 0x46, 0x5D, 0x5F, 0x7A, 0xE5, 0x9C, 0x47, 0x84, 0xE7, 0x20, 0x67, 0xA3, 0xC7, 0x00, 0x34,
    0xE9, 0xC7, 0x00, 0x80, 0x00, 0xC0, 0x28, 0x46, 0x40, 0x66, 0xB0, 0x64, 0xC0, 0xD7, 0xDA, 0xC7,
    0x00, 0x80, 0x1F, 0x84, 0x28, 0x46, 0xD0, 0x5F, 0xC7, 0xEC, 0xA7, 0xC6, 0x00, 0x80, 0x00, 0xD8,
    0x73, 0x47, 0xF9, 0x5F, 0x00, 0x3D, 0xA5, 0xC6, 0x00, 0x80, 0x73, 0x57, 0x63, 0x47, 0xB0, 0x68,
    0xA4, 0x6A, 0xC0, 0xF7, 0xC4, 0xC7, 0x00, 0x80, 0x1F, 0x84, 0x28, 0x46, 0xD0, 0x5F, 0x00, 0x8C,
    0xB9, 0xC7, 0x00, 0x80, 0x00, 0xC0, 0x28, 0x46, 0x40, 0x66, 0xB0, 0x64, 0xC0, 0xF7, 0xAB, 0xC7,
    0x00, 0x80, 0x1F, 0x84, 0x28, 0x46, 0xD0, 0x5F, 0xCD, 0xEC, 0xA7, 0xC6, 0x00, 0x80, 0x00, 0x30,
    0xAF, 0x46, 0xF8, 0x5F, 0x00, 0x3D, 0xA5, 0xC6, 0x00, 0x80, 0x00, 0x2F, 0x8E, 0x46, 0xB0, 0x68,
    0xA4, 0x6A, 0x79, 0x54, 0xC1, 0xC7, 0x00, 0x80, 0x1F, 0xA2, 0x50, 0x46, 0xD0, 0x5F, 0x79, 0x54,
    0xC1, 0xC7, 0x00, 0x80, 0x0D, 0x51, 0x9A, 0x46, 0xD0, 0x5F, 0x79, 0x54, 0xC1, 0xC7, 0x00, 0x80,
    0x03, 0x51, 0xCC, 0x46, 0xD0, 0x5F, 0xF3, 0xD7, 0x73, 0xC7, 0x00, 0x80, 0x4F, 0x93, 0x90, 0x46,
    0xF8, 0x5F, 0x73, 0x9F, 0x70, 0xC7, 0x00, 0x80, 0xFD, 0x42, 0x93, 0x46, 0xA4, 0x6A, 0xB0, 0x68,
    0x79, 0x54, 0xC1, 0xC7, 0x00, 0x80, 0xEB, 0x50, 0xFE, 0x46, 0xD0, 0x5F, 0xF3, 0x67, 0x75, 0xC7,
    0x00, 0x80, 0xA7, 0x89, 0x64, 0x47, 0xF9, 0x5F, 0x00, 0x10, 0x6F, 0xC7, 0x00, 0x80, 0x00, 0xE2,
    0x65, 0x47, 0xDC, 0x69, 0xB0, 0x68, 0x80, 0x3B, 0x9E, 0xC7, 0x00, 0x80, 0x29, 0x1E, 0x52, 0x46,
    0xD0, 0x5F, 0xFB, 0x4F, 0x89, 0xC6, 0x00, 0x80, 0x4B, 0xE9, 0xAB, 0xC6, 0xF9, 0x5F, 0xFD, 0x50,
    0xAA, 0xC6, 0x00, 0x80, 0x00, 0x3D, 0xA5, 0xC6, 0xA4, 0x6A, 0xB0, 0x68, 0x80, 0x3B, 0x9E, 0xC7,
    0x00, 0x80, 0x0D, 0x2F, 0x9E, 0x46, 0xD0, 0x5F, 0x39, 0xF0, 0x9C, 0xC7, 0x00, 0x80, 0xFB, 0xDD,
    0xC1, 0x46, 0xD0, 0x5F, 0x66, 0xAC, 0xC1, 0xC7, 0x00, 0x80, 0x78, 0xF8, 0x07, 0x47, 0x5D, 0x5F,
    0xCD, 0x36, 0x6C, 0xC7, 0x3E, 0xE6, 0x81, 0xFD, 0xAB, 0xC6, 0x13, 0x60, 0x80, 0x0D, 0xCC, 0xC7,
    0x00, 0x80, 0x00, 0xE8, 0x28, 0x46, 0x4C, 0x60, 0x00, 0x7B, 0xC0, 0xC7, 0x08, 0xE7, 0x06, 0xB4,
    0x8C, 0xC6, 0xC5, 0x5F, 0x00, 0xE9, 0x9D, 0xC7, 0x00, 0x80, 0x00, 0xE8, 0x28, 0x46, 0xD0, 0x5F,
    0xFB, 0x31, 0x91, 0xC6, 0x00, 0x80, 0xFF, 0x88, 0x69, 0x47, 0x8F, 0x5E, 0xF8, 0x7E, 0xAE, 0xC6,
    0x00, 0x80, 0x00, 0xD8, 0x6A, 0x47, 0x90, 0x5E, 0xFB, 0xFF, 0xA9, 0xC6, 0x00, 0x80, 0x03, 0x93,
    0xA8, 0x46, 0x57, 0x5F, 0x8F, 0x60, 0xA5, 0xC6, 0x00, 0x80, 0xE4, 0xF5, 0x9A, 0x46, 0x90, 0x5E,
    0xFF, 0xBA, 0x69, 0xC7, 0x00, 0x80, 0x08, 0x1F, 0x93, 0x46, 0x8F, 0x5E, 0xF4, 0xA4, 0x6B, 0xC7,
    0x00, 0x80, 0x00, 0x96, 0xAA, 0x46, 0x90, 0x5E, 0x01, 0xA7, 0x69, 0xC7, 0x00, 0x80, 0x00, 0x56,
    0x6F, 0x47, 0x8F, 0x5E, 0x74, 0xDD, 0x6A, 0xC7, 0x00, 0x80, 0x01, 0xFD, 0x63, 0x47, 0x90, 0x5E,
    0x74, 0xDD, 0x6A, 0xC7, 0x00, 0x80, 0x80, 0x40, 0x61, 0x47, 0x90, 0x5E, 0x00, 0xEC, 0xC2, 0xC7,
    0x00, 0x80, 0x00, 0x80, 0x54, 0x46, 0x40, 0x62, 0x40, 0x5E,
};

static struct Event_List ev_list[73] = {
    { 0, 0x20007004, 0xA0000000, 0x00018549 },
    { 0x003E0000, 0x20169000, 0x30000000, 0x0003803E },
    { 0, 0x20341004, 0x60000000, 0x00048000 },
    { 0x80070000, 0x20406004, 0x30000000, 0x0004003F },
    { 0x80080000, 0x20406004, 0x30000000, 0x00044040 },
    { 0x80090000, 0x20406004, 0x30000000, 0x00048041 },
    { 0x80440045, 0x4052C000, 0x30000000, 0x00034045 },
    { 0x80450047, 0x20645004, 0x30000000, 0x00030047 },
    { 0x80470000, 0x20645004, 0x60000000, 0x00004000 },
    { 0x80450048, 0x20722004, 0x30000000, 0x0003C048 },
    { 0x005F0000, 0x207E6004, 0x30000000, 0x00028000 },
    { 0, 0x507E62EC, 0x30000000, 0x0002C05F },
    { 0x805F0000, 0x207E6004, 0x60000000, 0x00044000 },
    { 0x004F0000, 0x20903000, 0x30000000, 0x00020000 },
    { 0x804F0000, 0x20903000, 0x30000000, 0x00024000 },
    { 0, 0x209C1004, 0x30000000, 0x0000404A },
    { 0x80500052, 0x20A82004, 0x30000000, 0x00014000 },
    { 0x40510000, 0x50A8219C, 0x30000000, 0x00014050 },
    { 0x00500000, 0x20A82004, 0x30000000, 0x0000C000 },
    { 0x80520000, 0x20B42004, 0x30000000, 0x0000C000 },
    { 0x00520000, 0x20C06004, 0x30000000, 0x00018000 },
    { 0, 0x20D21004, 0x30000000, 0x0000804E },
    { 0x00520000, 0x20DEB004, 0x60000000, 0x00030000 },
    { 0x00520000, 0x20ECC004, 0x60000000, 0x00030000 },
    { 0x00520000, 0x20FEC004, 0x60000000, 0x00030000 },
    { 0x80520000, 0x21102000, 0x411C1000, 1361 },
    { 0x80520000, 0x211C1000, 0x41102000, 1361 },
    { 0x805D0000, 0x21283000, 0x413401A0, 0x01400063 },
    { 0, 0xA12831BC, 0x30000000, 0x0001C05D },
    { 0, 0x21283000, 0x80000000, 0x01000563 },
    { 0x05520000, 0x413EB004, 0x10000000, 1362 },
    { 0, 0xA14C118C, 0x30000000, 0x0004C049 },
    { 0x00490000, 0x214C1000, 0x80000000, 0x00400554 },
    { 0x80490000, 0x214C1000, 0x41582000, 0x00400553 },
    { 0x80490000, 0x21582000, 0x414C1000, 0x00400553 },
    { 0x05640000, 0x4164B004, 0x10000000, 1380 },
    { 0x05650000, 0x4164B004, 0x10000000, 1381 },
    { 0, 0x21721000, 0x90000000, 0x007FC555 },
    { 0x05560000, 0x417EB004, 0x10000000, 1366 },
    { 0, 0x218C1000, 0x41982000, 0x00400557 },
    { 0, 0x21982000, 0x418C1000, 0x00400557 },
    { 0x05660000, 0x41A4B004, 0x10000000, 1382 },
    { 0x05670000, 0x41A4B004, 0x10000000, 1383 },
    { 0, 0x21B24000, 0x90000000, 0x007FC558 },
    { 0, 0x21BE4000, 0x90000000, 0x007FC559 },
    { 0, 0x21CA4000, 0x41D63000, 0x0040055A },
    { 0, 0x21D63000, 0x41CA4000, 0x0040055A },
    { 0x05680000, 0x41E2B004, 0x10000000, 1384 },
    { 0x05690000, 0x41E2B004, 0x10000000, 1385 },
    { 0, 0x21F04000, 0x41FC3000, 0x0040055B },
    { 0, 0x21FC3000, 0x41F04000, 0x0040055B },
    { 0x056A0000, 0x4208B004, 0x10000000, 1386 },
    { 0x056B0000, 0x4208B004, 0x10000000, 1387 },
    { 0, 0x22163000, 0x42224000, 0x0040055C },
    { 0, 0x22224000, 0x42163000, 0x0040055C },
    { 0x056C0000, 0x422EB004, 0x10000000, 1388 },
    { 0x056D0000, 0x422EB004, 0x10000000, 1389 },
    { 0, 0x223C3000, 0x90000000, 0x007FC55D },
    { 0, 0x22482000, 0x90000000, 0x007FC55E },
    { 0, 0x22542000, 0x42601180, 0x0100055F },
    { 0, 0x226C1000, 0x42782180, 0x01000560 },
    { 0, 0x22841000, 0x90000000, 0x013F8561 },
    { 0, 0x22901000, 0x90000000, 0x007FC000 },
    { 0, 0x229C2000, 0x90000000, 0x007FC000 },
    { 0, 0x22A81000, 0x90000000, 0x007F8000 },
    { 0, 0x22B43000, 0x90000000, 0x007FC000 },
    { 0, 0x22C01000, 0x90000000, 0x007FC000 },
    { 0, 0x22CC3000, 0x90000000, 0x007FC000 },
    { 0, 0x22D82000, 0x90000000, 0x007FC000 },
    { 0, 0x22E43000, 0x90000000, 0x007FC000 },
    { 0, 0x22F03000, 0x90000000, 0x007FC000 },
    { 0x821F0220, 0x42FCB000, 0x30000000, 0x00050220 },
    { 0, 0, 0, 0 },
};

static struct Item_List gi_list[7] = {
    { -20975.0f, -20300.0f, 0xDC0F, 0x35C2, 0x200002F7 },
    { -21900.0f, -17100.0f, 0xDCD9, 0x3481, 0x200002F9 },
    { -19015.0f, 20504.0f, 0xDEB0, 0x8000, 0x800002FB },
    { -18977.0f, 20584.5f, 0xDEB0, 0x8000, 0x800002FD },
    { -17284.05f, 62070.62f, 0xDF24, 0x8000, 0x800002FF },
    { -61695.0f, 62785.0f, 0xDF24, 0x8000, 0x80000301 },
    { 0.0f, 0.0f, 0, 0, 0xE0000000 },
};

static struct _CL_HITPOLY_PLANE clActWallList_ap42[24] = {
    { 1, 1, 0, 4, 8, 0, {
            { -59245.0f, 0.0f, 23162.5f, 1.0f },
            { -59245.0f, -1250.0f, 23162.5f, 1.0f },
            { -59245.0f, -1250.0f, 22837.5f, 1.0f },
            { -59245.0f, 0.0f, 22837.5f, 1.0f },
        } },
    { 1, 1, 0, 4, 8, 0, {
            { -59245.0f, 0.0f, 22837.5f, 1.0f },
            { -59245.0f, -1250.0f, 22837.5f, 1.0f },
            { -58795.0f, -1250.0f, 22837.5f, 1.0f },
            { -58795.0f, 0.0f, 22837.5f, 1.0f },
        } },
    { 1, 1, 0, 4, 8, 0, {
            { -58795.0f, 0.0f, 22837.5f, 1.0f },
            { -58795.0f, -1250.0f, 22837.5f, 1.0f },
            { -58795.0f, -1250.0f, 23162.5f, 1.0f },
            { -58795.0f, 0.0f, 23162.5f, 1.0f },
        } },
    { 1, 1, 0, 4, 8, 0, {
            { -58795.0f, 0.0f, 23162.5f, 1.0f },
            { -58795.0f, -1250.0f, 23162.5f, 1.0f },
            { -59245.0f, -1250.0f, 23162.5f, 1.0f },
            { -59245.0f, 0.0f, 23162.5f, 1.0f },
        } },
    { 0, 0, 0, 0, 0, 0, {
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
        } },
    { 1, 1, 0, 4, 8, 0, {
            { -58945.0f, 0.0f, 23150.0f, 1.0f },
            { -58945.0f, -1250.0f, 23150.0f, 1.0f },
            { -58945.0f, -1250.0f, 22825.0f, 1.0f },
            { -58945.0f, 0.0f, 22825.0f, 1.0f },
        } },
    { 1, 1, 0, 4, 8, 0, {
            { -58945.0f, 0.0f, 22825.0f, 1.0f },
            { -58945.0f, -1250.0f, 22825.0f, 1.0f },
            { -58495.0f, -1250.0f, 22825.0f, 1.0f },
            { -58495.0f, 0.0f, 22825.0f, 1.0f },
        } },
    { 1, 1, 0, 4, 8, 0, {
            { -58495.0f, 0.0f, 22825.0f, 1.0f },
            { -58495.0f, -1250.0f, 22825.0f, 1.0f },
            { -58495.0f, -1250.0f, 23150.0f, 1.0f },
            { -58495.0f, 0.0f, 23150.0f, 1.0f },
        } },
    { 1, 1, 0, 4, 8, 0, {
            { -58495.0f, 0.0f, 23150.0f, 1.0f },
            { -58495.0f, -1250.0f, 23150.0f, 1.0f },
            { -58945.0f, -1250.0f, 23150.0f, 1.0f },
            { -58945.0f, 0.0f, 23150.0f, 1.0f },
        } },
    { 0, 0, 0, 0, 0, 0, {
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
        } },
    { 1, 1, 0, 4, 9, 0, {
            { -58778.0f, 0.0f, 19295.05f, 1.0f },
            { -58778.0f, -528.735f, 19295.05f, 1.0f },
            { -58778.0f, -528.735f, 18850.6f, 1.0f },
            { -58778.0f, 0.0f, 18850.6f, 1.0f },
        } },
    { 1, 1, 0, 4, 9, 0, {
            { -58594.5f, -528.735f, 19870.95f, 1.0f },
            { -58979.5f, -528.735f, 19496.85f, 1.0f },
            { -58979.5f, 0.0f, 19496.85f, 1.0f },
            { -58594.5f, 0.0f, 19870.95f, 1.0f },
        } },
    { 1, 1, 0, 4, 9, 0, {
            { -58274.0f, -528.735f, 19554.55f, 1.0f },
            { -58594.5f, -528.735f, 19870.95f, 1.0f },
            { -58594.5f, 0.0f, 19870.95f, 1.0f },
            { -58274.0f, 0.0f, 19554.55f, 1.0f },
        } },
    { 1, 1, 0, 4, 9, 0, {
            { -58778.0f, 0.0f, 19295.05f, 1.0f },
            { -58979.5f, 0.0f, 19496.85f, 1.0f },
            { -58979.5f, -528.735f, 19496.85f, 1.0f },
            { -58778.0f, -528.735f, 19295.05f, 1.0f },
        } },
    { 1, 1, 0, 4, 9, 0, {
            { -58274.0f, -528.735f, 19554.55f, 1.0f },
            { -58274.0f, 0.0f, 19554.55f, 1.0f },
            { -57703.0f, 0.0f, 19554.55f, 1.0f },
            { -57703.0f, -528.735f, 19554.55f, 1.0f },
        } },
    { 0, 0, 0, 0, 0, 0, {
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
        } },
    { 1, 1, 0, 4, 9, 0, {
            { -59059.5f, -0.684475f, 20554.3f, 1.0f },
            { -59059.5f, -534.58f, 20554.3f, 1.0f },
            { -59500.5f, -534.58f, 20020.9f, 1.0f },
            { -59500.5f, -0.684475f, 20020.9f, 1.0f },
        } },
    { 1, 1, 0, 4, 9, 0, {
            { -59500.5f, -0.684475f, 20020.9f, 1.0f },
            { -59500.5f, -534.58f, 20020.9f, 1.0f },
            { -59078.0f, -534.58f, 19592.25f, 1.0f },
            { -59078.0f, -0.684475f, 19592.25f, 1.0f },
        } },
    { 1, 1, 0, 4, 9, 0, {
            { -59078.0f, -0.684475f, 19592.25f, 1.0f },
            { -59078.0f, -534.58f, 19592.25f, 1.0f },
            { -58598.0f, -534.58f, 19710.0f, 1.0f },
            { -58598.0f, -0.684475f, 19710.0f, 1.0f },
        } },
    { 1, 1, 0, 4, 9, 0, {
            { -58598.0f, -0.684475f, 19710.0f, 1.0f },
            { -58598.0f, -534.58f, 19710.0f, 1.0f },
            { -59059.5f, -534.58f, 20554.3f, 1.0f },
            { -59059.5f, -0.684475f, 20554.3f, 1.0f },
        } },
    { 0, 0, 0, 0, 0, 0, {
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
        } },
    { 1, 1, 0, 4, 8, 0, {
            { -58072.0f, 3.54898f, 19396.55f, 1.0f },
            { -58072.0f, -497.221f, 19396.55f, 1.0f },
            { -58670.5f, -497.221f, 18850.4f, 1.0f },
            { -58670.5f, 3.54898f, 18850.4f, 1.0f },
        } },
    { 1, 1, 0, 4, 8, 0, {
            { -57680.0f, 3.54898f, 19396.55f, 1.0f },
            { -57680.0f, -497.221f, 19396.55f, 1.0f },
            { -58072.0f, -497.221f, 19396.55f, 1.0f },
            { -58072.0f, 3.54898f, 19396.55f, 1.0f },
        } },
    { 0, 0, 0, 0, 0, 0, {
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
        } },
};

static struct _CL_HITPOLY_PLANE clActFloorList_ap42[8] = {
    { 1, 0, 0, 4, 8, 0, {
            { -58274.0f, -528.735f, 19554.55f, 1.0f },
            { -58778.0f, -528.735f, 18850.6f, 1.0f },
            { -58778.0f, -528.735f, 19295.05f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
        } },
    { 1, 1, 0, 4, 8, 0, {
            { -58778.0f, -528.735f, 18850.6f, 1.0f },
            { -58274.0f, -528.735f, 19554.55f, 1.0f },
            { -57703.0f, -528.735f, 19554.55f, 1.0f },
            { -57703.0f, -528.735f, 18850.6f, 1.0f },
        } },
    { 1, 1, 0, 4, 8, 0, {
            { -58778.0f, -528.735f, 19295.05f, 1.0f },
            { -58979.5f, -528.735f, 19496.85f, 1.0f },
            { -58594.5f, -528.735f, 19870.95f, 1.0f },
            { -58274.0f, -528.735f, 19554.55f, 1.0f },
        } },
    { 0, 0, 0, 0, 0, 0, {
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
        } },
    { 1, 1, 0, 4, 8, 0, {
            { -59059.5f, -534.58f, 20554.3f, 1.0f },
            { -58598.0f, -534.58f, 19710.0f, 1.0f },
            { -59078.0f, -534.58f, 19592.25f, 1.0f },
            { -59500.5f, -534.58f, 20020.9f, 1.0f },
        } },
    { 0, 0, 0, 0, 0, 0, {
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
        } },
    { 1, 1, 0, 4, 7, 0, {
            { -58072.0f, -497.221f, 19396.55f, 1.0f },
            { -57680.0f, -497.221f, 19396.55f, 1.0f },
            { -57680.5f, -497.221f, 18850.4f, 1.0f },
            { -58670.5f, -497.221f, 18850.4f, 1.0f },
        } },
    { 0, 0, 0, 0, 0, 0, {
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
        } },
};

static int (*ev_prog[21])(void) = {
    NULL,
    EvProgThreeNameOnWall,
    EvProgHintOfClockSet,
    EvProgClockTime,
    EvProgUseClockKey,
    EvProgClockNeedleMove,
    EvProgTryMoveClock,
    EvProgUseEmergencyKey,
    EvProgAnyoneInHole,
    EvProgNooneInHole,
    EvProgLookDustChute,
    EvProgUseCannedJuice,
    EvProgNoFaceCorpse,
    EvProgAnyoneCry,
    EvProgGetLight,
    EvProgGetApart202Key,
    EvProgEndHintRecoveryRead,
    EvProgEndHintMariaRead,
    EvProgEndHintSuicideRead,
    EvProgUseApart202Key,
    EvProgMonkeyKick206,
};

static struct Model_List mdl_list[9] = {
    { 1302, 0, 0, 0, { -20000.0f, 0.0f, 20000.0f, 0.0f }, { -0.0f, -0.0f, 0.0f, 0.0f } },
    { 1367, 0, 0, 69, { -58262.5f, -360.005f, 19193.25f, 0.0f }, { -0.0f, -0.0f, 0.0f, 0.0f } },
    { 1294, 0, 0, 69, { -58554.5f, 0.0f, 19395.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f } },
    { 1797, 0, 62, 0, { -20000.0f, -688.3995f, 20026.2f, 0.0f }, { 0.179796f, -0.0f, 0.00001f, 0.0f } },
    { 1838, 0, 79, 0, { -23460.0f, -553.4105f, 59573.84f, 0.0f }, { 1.570796f, 0.0f, -1.745329f, 0.0f } },
    { 1058, 0, 0, 82, { -59020.0f, 0.0f, 23000.0f, 0.0f }, { -0.0f, -0.0f, 0.0f, 0.0f } },
    { 1058, 0, 82, 0, { -58720.0f, 0.0f, 23000.0f, 0.0f }, { -0.0f, -0.0f, 0.0f, 0.0f } },
    { 1839, 0, 72, 69, { -59157.453f, -572.6027f, 21120.393f, 0.0f }, { -1.570796f, 0.0f, 1.221729f, 0.0f } },
    { 0, 0, 0, 0, { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f, 1.0f } },
};

static struct Enemy_List en_list[15] = {
    { 512, 61, -99600, 15000, 0, 0, 0, 451 },
    { 512, 62, -93600, 11500, 0, 0, 0, 448 },
    { 512, 63, -110000, 11600, 0, 6433, 0, 451 },
    { 512, 64, -99600, 22200, 0, -12867, 0, 449 },
    { 514, 65, -82600, 11500, 0, -6433, 0, 449 },
    { 512, 66, -80800, 22000, 0, 6433, 0, 448 },
    { 513, 67, -80600, 19000, 0, -12867, 0, 449 },
    { 513, 68, -20300, 18900, 0, -8578, 9, 0 },
    { 513, 69, -17900, 21750, 0, -7290, 9, 3 },
    { 512, 70, -21200, -19200, 0, 6433, 0, 0 },
    { 512, 71, -18000, -18500, 0, 0, 0, 3 },
    { 513, 72, -19800, -21800, 0, -6433, 0, 1 },
    { 513, 73, -21900, -18200, 0, 8578, 0, 3 },
    { 520, 5, -99400, 32100, 0, 12867, 12, 384 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
};

static struct Stage_GfwFunc gfw_func = { NULL, Delete_RedPointLight, NULL, NULL };

struct Stage_Data stage_apart_e2f = {
    ev_list,
    ev_pos,
    ev_prog,
    gi_list,
    mdl_list,
    en_list,
    NULL,
    EvRoomInit,
    EvAllTimeFunc,
    9,
    1,
    pjames_stage_anim,
    NULL,
    &gfw_func,
    (int (*)(void))EvCharaDataClear,
    EvSoundCallAfterLoad,
    0,
};

static float tv_pos[4] = { -58082.375f, -356.5f, 19011.39f, 0.0f };
static float clock_vec_0[2][4] = {
    { -58720.0f, 0.0f, 23000.0f, 0.0f },
    { -0.0f, -0.0f, 0.0f, 0.0f },
};
static float clock_vec_1[2][4] = {
    { -59020.0f, 0.0f, 23000.0f, 0.0f },
    { -0.0f, -0.0f, 0.0f, 0.0f },
};

unsigned char *kao_dds;

static int EvProgThreeNameOnWall(void) {
    switch (ev_p_step) {
    case 0:
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        PlayerEventAnimeSet(0x65);
        EV_STEP(0xA);
    case 0xA:
        if (!EvSubMessage(7)) {
            break;
        }
        if (!ev_cancel) {
            EV_STEP(2);
        } else {
            ev_prog_flag_set = 0;
            EV_STEP(0xD);
        }
        break;
    case 2:
        if (!EvSubFileLoadAndFadeOut(0, data_pic_apt_clock_name_tex, NULL)) {
            break;
        }
        ScreenEffectFadeStart(4, 0.0f);
        EV_STEP(3);
    case 3:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureEnd();
        if (!ScreenEffectFadeCheck()) {
            break;
        }
        EV_STEP(8);
        break;
    case 8:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureEnd();
        if (shPadTrigger(0, key_config.enter) || shPadTrigger(0, key_config.cancel)) {
            EV_STEP(7);
        }
        break;
    case 7:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureFilter();
        EvSubPictureEnd();
        if (!EvSubMessage(8)) {
            break;
        }
        EV_STEP(4);
        ScreenEffectFadeStart(1, 0.0f);
        break;
    case 4:
        if (!ScreenEffectFadeCheck()) {
            break;
        }
        EV_STEP(0xD);
        ScreenEffectFadeStart(4, 0.0f);
        break;
    case 0xD:
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        return 1;
    }
    return 0;
}

static int EvProgHintOfClockSet(void) {
    switch (ev_p_step) {
    case 0:
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        PlayerEventAnimeSet(0x65);
        EV_STEP(0xA);
    case 0xA:
        if (!EvSubMessage(4)) {
            break;
        }
        if (!ev_cancel) {
            EV_STEP(2);
        } else {
            ev_prog_flag_set = 0;
            EV_STEP(0xD);
        }
        break;
    case 2:
        if (playing.riddle_level > 0) {
            if (!EvSubFileLoadAndFadeOut(0, data_pic_apt_clock_memo_tex, NULL)) {
                break;
            }
        } else {
            if (!EvSubFileLoadAndFadeOut(0, data_pic_apt_clock_memo_2_tex, NULL)) {
                break;
            }
        }
        ScreenEffectFadeStart(4, 0.0f);
        EV_STEP(3);
    case 3:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureEnd();
        if (!ScreenEffectFadeCheck()) {
            break;
        }
        EV_STEP(8);
        break;
    case 8:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureEnd();
        if (shPadTrigger(0, key_config.enter) || shPadTrigger(0, key_config.cancel)) {
            EV_STEP(7);
        }
        break;
    case 7:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureFilter();
        EvSubPictureEnd();
        if (playing.riddle_level > 0) {
            if (!EvSubMessage(5)) {
                break;
            }
        } else {
            if (!EvSubMessage(9)) {
                break;
            }
        }
        EV_STEP(4);
        ScreenEffectFadeStart(1, 0.0f);
        break;
    case 4:
        if (!ScreenEffectFadeCheck()) {
            break;
        }
        EV_STEP(0xD);
        ScreenEffectFadeStart(4, 0.0f);
    case 0xD:
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        return 1;
    }
    return 0;
}

static int EvProgClockTime(void) {
    char c_work[6];
    int work;
    int i;
    int j;

    switch (ev_p_step) {
    case 0:
        game_flag.flag[2] |= 0x2000;
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        EV_STEP(2);
    case 2:
        if (!EvSubFileLoadAndFadeOut(0, data_pic_apt_clock_close_tex, data_pic_apt_clock_hari_tex)) {
            break;
        }
        ScreenEffectFadeStart(4, 0.0f);
        EV_STEP(3);
        break;
    case 3:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvProgSubClockNeedleDraw(0);
        EvSubPictureEnd();
        if (!ScreenEffectFadeCheck()) {
            break;
        }
        EV_STEP(8);
        break;
    case 8:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvProgSubClockNeedleDraw(0);
        EvSubPictureEnd();
        if (!shPadTrigger(0, key_config.enter + key_config.cancel)) {
            break;
        }
        EV_STEP(0xA);
        break;
    case 0xA:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvProgSubClockNeedleDraw(0);
        EvSubPictureFilter();
        EvSubPictureEnd();
        if (playing.language == 0) {
            j = 0;
        } else {
            c_work[0] = 0x5C;
            c_work[1] = 0x68;
            j = 2;
        }
        for (i = 0; i < 2; i++) {
            if (i) {
                work = ((game_flag.clock + 0x20) >> 6) % 60;
            } else {
                work = ((game_flag.clock + 0x20) >> 6) / 60;
            }
            if (work / 10) {
                c_work[j] = work / 10 + '0';
                c_work[j + 1] = work % 10 + '0';
                c_work[j + 2] = 0;
            } else {
                c_work[j] = work % 10 + '0';
                c_work[j + 1] = 0;
            }
            fontSetMes(i, dicSetStr(c_work));
        }
        if ((game_flag.flag[2] >> 18) & 1) {
            if (!EvSubMessage(0xA)) {
                break;
            }
        } else {
            if (!EvSubMessage(0xB)) {
                break;
            }
        }
        ScreenEffectFadeStart(1, 0.0f);
        EV_STEP(4);
        break;
    case 4:
        if (!ScreenEffectFadeCheck()) {
            break;
        }
        EV_STEP(0xD);
        break;
    case 0xD:
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        ScreenEffectFadeStart(4, 0.0f);
        return 1;
    }
    return 0;
}

static int EvProgClockNeedleMove(void) {
    static short anim_1[2] = { 0x446, 0x2798 };
    static short anim_2[2] = { 0x447, 0x2799 };
    static struct CharaData_DemoList chara_1[3] = {
        { 259, data_chr_jms_hhh_jms_notex_mdl, data_demo_tokei_0a1_hhh_jms_anm, data_chr_jms_hhh_jms_kg1, data_demo_tokei_0a1_hhh_jms_cls },
        { 1058, data_chr_item_b_clo_mdl, data_demo_tokei_0a1_b_clo_anm, data_chr_item_b_clo_kg1, NULL },
        { 0, NULL, NULL, NULL, NULL },
    };
    static struct CharaData_DemoList chara_2[3] = {
        { 259, data_chr_jms_hhh_jms_notex_mdl, data_demo_tokei_0a2_hhh_jms_anm, data_chr_jms_hhh_jms_kg1, data_demo_tokei_0a2_hhh_jms_cls },
        { 1058, data_chr_item_b_clo_mdl, data_demo_tokei_0a2_b_clo_anm, data_chr_item_b_clo_kg1, NULL },
        { 0, NULL, NULL, NULL, NULL },
    };
    static struct DramaDemo_PlayInfo info_1 = { 12, MemShare_gp_data_buf, anim_1, NULL, 0, 0, NULL, 0, 0.0f, 0.0f, 0.0f };
    static struct DramaDemo_PlayInfo info_2 = { 12, MemShare_gp_data_buf, anim_2, NULL, 0, 0, NULL, 0, 0.0f, 0.0f, 0.0f };
    static int mv_se = 0;
    int work;

    switch (ev_p_step) {
    case 0:
        game_flag.flag[2] |= 0x2000;
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        if ((game_flag.flag[2] >> 17) & 1) {
            CharaDataLoadDemo(chara_1, 0);
            FcRead(data_demo_tokei_0a1_clock_open_dds, MemShare_gp_data_buf);
            fsSync(0, -1);
            mv_se = 0;
            EV_STEP(0xA);
        } else {
            SeCallPos(0x3E82, 1.0f, (float[4]){ -58808.85f, -606.0f, 22913.494f, 0.0f }, 0);
            EV_STEP(2);
        }
        break;
    case 0xA:
        if (!EvSubItemUse0(0x19, 0x16, 0x3E81, 0, (float[4]){ -58808.85f, -606.0f, 22913.494f, 0.0f }, 0)) {
            break;
        }
        CharaAdminPlayableDisplay(0);
        SCNowDemoEventSwitch(sh2jms.player, 1);
        EV_STEP(0x18);
    case 0x18:
        work = DramaDemoMain(&info_1);
        if (demo_frame > total_demo_frame - 15.0f) {
            ScreenEffectFadeStart(2, 0.5f);
        }
        if (demo_frame > 45.0f && !mv_se) {
            SeCallPos(0x3E82, 1.0f, (float[4]){ -58808.85f, -606.0f, 22913.494f, 0.0f }, 0);
            mv_se = 1;
        }
        if (!work) {
            break;
        }
        EV_STEP(2);
        ScreenEffectFadeStart(3, 0.0f);
        CharaAdminReCreate(0x422, 0, 0, clock_vec_0[0], clock_vec_0[1], 0);
        break;
    case 2:
        if (!EvSubFileLoadAndFadeOut(0, data_pic_apt_clock_open_tex, data_pic_apt_clock_hari_tex)) {
            break;
        }
        ScreenEffectFadeStart(4, 0.0f);
        mv_se = 0;
        EV_STEP(3);
        break;
    case 3:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvProgSubClockNeedleDraw(1);
        EvSubPictureEnd();
        if (!ScreenEffectFadeCheck()) {
            break;
        }
        EV_STEP(8);
        break;
    case 8:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvProgSubClockNeedleDraw(1);
        EvSubPictureEnd();
        if (!((game_flag.flag[2] >> 19) & 1)) {
            if (shPadPress(0, 0x500)) {
                game_flag.clock += ftoi(itof(1920) * shGetDT());
                if (game_flag.clock > 0xB400) {
                    game_flag.clock -= 0xB400;
                }
            } else if (shPadPress(0, 0xA00)) {
                work = game_flag.clock;
                work -= ftoi(itof(1920) * shGetDT());
                if (work < 0) {
                    work += 0xB400;
                }
                game_flag.clock = work;
            }
        }
        if (shPadPress(0, 0xF00)) {
            if (!mv_se) {
                SeCall(0x3E83, 1.0f, 0);
            }
            mv_se = 1;
        } else {
            if (mv_se) {
                SeStop(0x3E83);
                if (game_flag.clock > 0x88C0 && game_flag.clock < 0x8A40) {
                    SeCall(0x426C, 1.0f, 0);
                    if (playing.riddle_level == 0) {
                        game_flag.flag[2] |= 0x80000;
                    }
                }
            }
            mv_se = 0;
        }
        if (!shPadTrigger(0, key_config.cancel)) {
            break;
        }
        ScreenEffectFadeStart(1, 0.0f);
        SeStop(0x3E83);
        if ((game_flag.flag[2] >> 17) & 1) {
            CharaDataLoadDemo(chara_2, 1);
            FcRead(data_demo_tokei_0a2_clock_close_dds, MemShare_gp_data_buf);
        }
        EV_STEP(4);
        break;
    case 4:
        if (!ScreenEffectFadeCheck()) {
            break;
        }
        if ((game_flag.flag[2] >> 17) & 1) {
            CharaDataLoadDemo(chara_2, 0);
            ScreenEffectFadeStart(4, 0.5f);
            mv_se = 0;
            EV_STEP(0x19);
        } else {
            SeCallPos(0x3E8B, 1.0f, (float[4]){ -58808.85f, -606.0f, 22913.494f, 0.0f }, 0);
            ScreenEffectFadeStart(4, 0.0f);
            EV_STEP(0xD);
            break;
        }
    case 0x19:
        work = DramaDemoMain(&info_2);
        if (demo_frame > 50.0f && !mv_se) {
            SeCallPos(0x3E8B, 1.0f, (float[4]){ -58808.85f, -606.0f, 22913.494f, 0.0f }, 0);
            mv_se = 1;
        }
        if (!work) {
            break;
        }
        EV_STEP(6);
        break;
    case 6:
        CharaDataDeleteOne(0x103);
        CharaAdminPlayableDisplay(1);
        SCNowDemoEventSwitch(sh2jms.player, 0);
        CharaAdminReCreate(0x422, 0, 0, clock_vec_0[0], clock_vec_0[1], 0);
        vcReturnPreAutoCamWork(1);
        EV_STEP(0xD);
    case 0xD:
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        game_flag.flag[2] &= ~0x20000;
        return 1;
    }
    return 0;
}

static void EvProgSubClockNeedleDraw(int open_or_close) {
    static unsigned short tex[8][4] = {
        { 0, 0, 496, 3056 },
        { 512, 0, 1008, 2544 },
        { 1280, 0, 1776, 2032 },
        { 1280, 2048, 1776, 2544 },
        { 1792, 0, 2288, 3056 },
        { 2304, 0, 2800, 2544 },
        { 3072, 0, 3568, 2032 },
        { 3072, 2048, 3568, 2544 },
    };
    struct PicDraw_Data pic;
    float cosrot;
    float sinrot;
    float pos[4][2];
    float rot;
    float x;
    float y0;
    float y1;
    int otp;
    int no;
    int i;

    PictureLoadImage((struct sh2gfw_AREA_HEAD *)layer_adr, 2, -1, -1);
    shQzero(&pic, sizeof(pic));
    pic.ap = (struct sh2gfw_AREA_HEAD *)layer_adr;
    pic.tex = -1;
    pic.clut = -1;
    pic.status |= 1;
    pic.a = 0x80;
    pic.alpha_a = 0;
    pic.alpha_b = 1;
    pic.alpha_c = 0;
    pic.alpha_d = 1;
    pic.alpha_fix = 0x80;
    pic.status |= 0x20;
    for (i = 0; i < 4; i++) {
        switch (i) {
        case 0:
        default:
            x = 16.0f;
            y0 = 144.0f;
            y1 = 48.0f;
            rot = 1.5707964f;
            otp = 5;
            break;
        case 1:
            x = 16.0f;
            y0 = 144.0f;
            y1 = 16.0f;
            rot = 3.1415927f * (2.0f * itof(game_flag.clock % 0xF00)) / itof(0xF00);
            otp = 4;
            break;
        case 2:
            x = 16.0f;
            y0 = 112.0f;
            y1 = 16.0f;
            rot = 3.1415927f * (2.0f * itof(game_flag.clock)) / itof(0xB400);
            otp = 3;
            break;
        case 3:
            x = 16.0f;
            y0 = 16.0f;
            y1 = 16.0f;
            rot = 0.0f;
            otp = 6;
            break;
        }
        sinrot = shSinF(rot);
        cosrot = shCosF(rot);
        pos[0][0] = 0.8f * (cosrot * -x - sinrot * -y0);
        pos[0][1] = sinrot * -x + cosrot * -y0;
        pos[1][0] = 0.8f * (cosrot * x - sinrot * -y0);
        pos[1][1] = sinrot * x + cosrot * -y0;
        pos[2][0] = 0.8f * (cosrot * -x - sinrot * y1);
        pos[2][1] = sinrot * -x + cosrot * y1;
        pos[3][0] = 0.8f * (cosrot * x - sinrot * y1);
        pos[3][1] = sinrot * x + cosrot * y1;
        pic.x0 = ftoi4(pos[0][0]);
        pic.y0 = ftoi4(pos[0][1] - 24.0f);
        pic.x1 = ftoi4(pos[1][0]);
        pic.y1 = ftoi4(pos[1][1] - 24.0f);
        pic.status |= 2;
        pic.x2 = ftoi4(pos[2][0]);
        pic.y2 = ftoi4(pos[2][1] - 24.0f);
        pic.x3 = ftoi4(pos[3][0]);
        pic.y3 = ftoi4(pos[3][1] - 24.0f);
        pic.status |= 0x80;
        no = i + (open_or_close ? 4 : 0);
        pic.us0 = tex[no][0];
        pic.vt0 = tex[no][1];
        pic.us1 = tex[no][2];
        pic.vt1 = tex[no][3];
        pic.status |= 4;
        pic.otp = otp;
        PictureDraw(&pic);
    }
}

static int EvProgUseClockKey(void) {
    return 0;
}

static int EvProgTryMoveClock(void) {
    static short clock_o_anim[2] = { 0x448, 0x279A };
    static struct DramaDemo_PlayInfo clock_o = { 12, MemShare_gp_data_buf, clock_o_anim, NULL, 0, 0, NULL, 0, 0.0f, 0.0f, 0.0f };
    static struct CharaData_DemoList clock_o_chara[2] = {
        { 259, data_chr_jms_hhh_jms_notex_mdl, data_demo_tokei_0b_hhh_jms_anm, data_chr_jms_hhh_jms_kg1, data_demo_tokei_0b_hhh_jms_cls },
        { 0, NULL, NULL, NULL, NULL },
    };
    static short clock_x_anim[2] = { 0x449, 0x279B };
    static struct DramaDemo_PlayInfo clock_x = { 12, MemShare_gp_data_buf, clock_x_anim, NULL, 0, 0, NULL, 0, 0.0f, 0.0f, 0.0f };
    static struct CharaData_DemoList clock_x_chara[2] = {
        { 259, data_chr_jms_hhh_jms_notex_mdl, data_demo_tokei_0c_hhh_jms_anm, data_chr_jms_hhh_jms_kg1, data_demo_tokei_0c_hhh_jms_cls },
        { 0, NULL, NULL, NULL, NULL },
    };
    static int se = 0;
    static u_long128 *anim_adr;
    int ox;
    int ret;

    if (game_flag.clock > 0x88C0 && game_flag.clock < 0x8A40) {
        ox = 1;
    } else {
        ox = 0;
    }
    switch (ev_p_step) {
    case 0:
        game_flag.flag[2] |= 0x2000;
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        PlayerEventAnimeSet(0x65);
        if (ox) {
            FcRead(data_demo_tokei_0b_clock_push_dds, MemShare_gp_data_buf);
            CharaDataLoadDemo(clock_o_chara, 1);
            anim_adr = CharaDataLoadExtra(data_demo_tokei_0b_b_clo_anm, 0x100);
        } else {
            FcRead(data_demo_tokei_0c_clock_fail_dds, MemShare_gp_data_buf);
            CharaDataLoadDemo(clock_x_chara, 1);
            anim_adr = CharaDataLoadExtra(data_demo_tokei_0c_b_clo_anm, 0x100);
        }
        EV_STEP(0x1E);
    case 0x1E:
        if (!EvSubMessage(6)) {
            break;
        }
        EV_STEP(0x1F);
        break;
    case 0x1F:
        if (!EvSubQuestion(0xD)) {
            break;
        }
        if (fontGetStatus() == 0) {
            EV_STEP(2);
        } else {
            CharaDataLoadCancel(ox ? clock_o_chara : clock_x_chara);
            EV_STEP(0xD);
        }
        break;
    case 2:
        if (fsSync(1, -1) < 0) {
            break;
        }
        if (ox) {
            CharaDataLoadDemo(clock_o_chara, 0);
            CharaDataAnimSetExtra(0x422, data_demo_tokei_0b_b_clo_anm, anim_adr, 0);
        } else {
            CharaDataLoadDemo(clock_x_chara, 0);
            CharaDataAnimSetExtra(0x422, data_demo_tokei_0c_b_clo_anm, anim_adr, 0);
        }
        CharaAdminPlayableDisplay(0);
        SCNowDemoEventSwitch(sh2jms.player, 1);
        se = 0;
        shCharacter_Manage_Delete(NULL, 0x422, 0);
        EV_STEP(0x16);
    case 0x16:
        if (ox) {
            ret = DramaDemoMain(&clock_o);
            if (!se && demo_frame > 30.0f) {
                SeCallPos(0x3E84, 1.0f, (float[4]){ -58722.55f, 0.0f, 23009.684f, 0.0f }, 0);
                se = 1;
            }
            if (!ret) {
                break;
            }
            game_flag.flag[2] |= 0x40000;
            EV_STEP(6);
        } else {
            if (!DramaDemoMain(&clock_x)) {
                break;
            }
            EV_STEP(0x20);
        }
        break;
    case 0x20:
        DramaDemoSkipLast(&clock_x);
        if (!EvSubMessage(0xE)) {
            break;
        }
        EV_STEP(6);
        break;
    case 6:
        CharaDataDeleteOne(0x103);
        CharaAdminPlayableDisplay(1);
        vcReturnPreAutoCamWork(1);
        SCNowDemoEventSwitch(sh2jms.player, 0);
        shCharacterPlayerModelToPlayable();
        if (!ox) {
            CharaAdminReCreate(0x422, 0, 0, clock_vec_0[0], clock_vec_0[1], 0);
        } else {
            CharaAdminReCreate(0x422, 0, 0, clock_vec_1[0], clock_vec_1[1], 0);
        }
        EV_STEP(0x12);
    case 0xD:
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        return 1;
    }
    return 0;
}

static int EvProgUseEmergencyKey(void) {
    return EvSubItemUse0(0x1B, 0x18, 0, 0, NULL, 1);
}

static int EvProgAnyoneInHole(void) {
    static short hole_anim[6] = { 0x436, 0x277B, 0x2791, 0x437, 0x277C, 0x2792 };
    static struct DramaDemo_PlayInfo hole = { 11, MemShare_gp_data_buf, hole_anim, NULL, 0, 0, NULL, 60001, 108.0f, 0.0f, 0.0f };
    static struct CharaData_DemoList chara_data[4] = {
        { 259, data_chr_jms_hhh_jms_notex_mdl, data_demo_ana_hhh_jms_anm, data_chr_jms_hhh_jms_kg1, data_demo_ana_hhh_jms_cls },
        { 1048, data_chr_item_i_j_light_mdl, data_demo_ana_i_j_light_anm, NULL, NULL },
        { 1055, data_chr_item_i_key_clock_mdl, data_demo_ana_i_key_clock_anm, NULL, NULL },
        { 0, NULL, NULL, NULL, NULL },
    };
    int ret;
    void DSR_Entry0(); /* Matching: called without a prototype in the original (arguments passed unconverted). */

    switch (ev_p_step) {
    case 0:
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        PlayerEventAnimeSet(0x65);
        FcRead(data_demo_ana_ana_dds, MemShare_gp_data_buf);
        CharaDataLoadDemo(chara_data, 1);
        EV_STEP(0xA);
    case 0xA:
        if (!EvSubQuestion(2)) {
            break;
        }
        if (fontGetStatus() == 0) {
            EV_STEP(2);
        } else {
            CharaDataLoadCancel(chara_data);
            EV_STEP(0xD);
        }
        break;
    case 2:
        if (fsSync(1, -1) < 0) {
            break;
        }
        CharaDataLoadDemo(chara_data, 0);
        CharaAdminPlayableDisplay(0);
        sh2jms.player->status |= 0x8000;
        SCNowDemoEventSwitch(sh2jms.player, 1);
        if (!item.light_switch) {
            item.light_switch = 1;
            LightSpotOnOffSet();
        }
        EV_STEP(0x16);
    case 0x16:
        DramaDemoMain(&hole);
        if (shPadTrigger(0, key_config.skip)) {
            EV_STEP(0x12);
        }
        if (demo_frame > total_demo_frame - 60.0f) {
            EV_STEP(9);
        }
        if (demo_frame >= 298.0f && demo_frame < 303.0f) {
            DSR_Entry0(__otn_ana_00, 0, 1.0);
        }
        if (demo_frame >= 520.0f && demo_frame < 530.0f) {
            DSR_Entry0(__otn_ana_01, 0, 1.0);
        }
        break;
    case 9:
        ret = EvSubItemGetAndAnim(0x19, 0x13);
        if (DramaDemoMain(&hole) && ret) {
            EV_STEP(6);
        }
        break;
    case 0x12:
        DramaDemoSkipLast(&hole);
        if (!EvSubItemGetAndAnim(0x19, 0x13)) {
            break;
        }
        EV_STEP(6);
        break;
    case 6:
        CharaDataDeleteOne(0x103);
        CharaDataDeleteOne(0x418);
        CharaDataDeleteOne(0x41F);
        CharaAdminPlayableDisplay(1);
        sh2jms.player->status &= ~0x8000;
        vcReturnPreAutoCamWork(1);
        SCNowDemoEventSwitch(sh2jms.player, 0);
        shCharacterPlayerModelToPlayable();
        game_flag.flag[2] |= 0x8000;
        EV_STEP(0xD);
    case 0xD:
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        return 1;
    }
    return 0;
}

static int EvProgNooneInHole(void) {
    static short hole_anim[3] = { 0x438, 0x277D, 0x2793 };
    static struct DramaDemo_PlayInfo hole = { 11, MemShare_gp_data_buf, hole_anim, NULL, 0, 0, NULL, 0, 0.0f, 0.0f, 0.0f };
    static struct CharaData_DemoList chara_data[4] = {
        { 259, data_chr_jms_hhh_jms_notex_mdl, data_demo_ana_c_hhh_jms_anm, data_chr_jms_hhh_jms_kg1, data_demo_ana_c_hhh_jms_cls },
        { 1048, data_chr_item_i_j_light_mdl, data_demo_ana_c_i_j_light_anm, NULL, NULL },
        { 1055, data_chr_item_i_key_clock_mdl, data_demo_ana_c_i_key_clock_anm, NULL, NULL },
        { 0, NULL, NULL, NULL, NULL },
    };
    int ret;

    switch (ev_p_step) {
    case 0:
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        PlayerEventAnimeSet(0x65);
        FcRead(data_demo_ana_c_ana_c_dds, MemShare_gp_data_buf);
        CharaDataLoadDemo(chara_data, 1);
        EV_STEP(2);
    case 2:
        if (fsSync(1, -1) < 0) {
            break;
        }
        CharaDataLoadDemo(chara_data, 0);
        CharaAdminPlayableDisplay(0);
        sh2jms.player->status |= 0x8000;
        SCNowDemoEventSwitch(sh2jms.player, 1);
        if (!item.light_switch) {
            item.light_switch = 1;
            LightSpotOnOffSet();
        }
        EV_STEP(0x16);
    case 0x16:
        DramaDemoMain(&hole);
        if (shPadTrigger(0, key_config.skip)) {
            EV_STEP(0x12);
        }
        if (demo_frame > total_demo_frame - 90.0f) {
            EV_STEP(9);
        }
        break;
    case 9:
        ret = EvSubMessage(3);
        if (DramaDemoMain(&hole) && ret) {
            EV_STEP(0xD);
        }
        break;
    case 0x12:
        DramaDemoSkipLast(&hole);
        if (!EvSubMessage(3)) {
            break;
        }
        EV_STEP(0xD);
        break;
    case 0xD:
        CharaDataDeleteOne(0x103);
        CharaDataDeleteOne(0x418);
        CharaDataDeleteOne(0x41F);
        CharaAdminPlayableDisplay(1);
        sh2jms.player->status &= ~0x8000;
        vcReturnPreAutoCamWork(1);
        SCNowDemoEventSwitch(sh2jms.player, 0);
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        shCharacterPlayerModelToPlayable();
        return 1;
    }
    return 0;
}

static int EvProgLookDustChute(void) {
    switch (ev_p_step) {
    case 0:
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        EV_STEP(2);
    case 2:
        if (!EvSubFileLoadAndFadeOut(0, data_pic_apt_p_dust_in_tex, NULL)) {
            break;
        }
        ScreenEffectFadeStart(4, 0.0f);
        EV_STEP(3);
        break;
    case 3:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureEnd();
        if (!ScreenEffectFadeCheck()) {
            break;
        }
        EV_STEP(7);
        break;
    case 7:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureEnd();
        if (shPadTrigger(0, key_config.enter) || shPadTrigger(0, key_config.cancel)) {
            EV_STEP(0x1E);
        }
        break;
    case 0x1E:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureFilter();
        EvSubPictureEnd();
        if (!EvSubMessage(0xF)) {
            break;
        }
        if (playing.riddle_level > 0) {
            ScreenEffectFadeStart(1, 0.0f);
            EV_STEP(4);
        } else {
            EV_STEP(0x1F);
        }
        break;
    case 0x1F:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureFilter();
        EvSubPictureEnd();
        if (!EvSubMessage(0x10)) {
            break;
        }
        ScreenEffectFadeStart(1, 0.0f);
        EV_STEP(4);
        break;
    case 4:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureEnd();
        if (!ScreenEffectFadeCheck()) {
            break;
        }
        EV_STEP(0xD);
        break;
    case 0xD:
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        ScreenEffectFadeStart(4, 0.0f);
        return 1;
    }
    return 0;
}

static int EvProgUseCannedJuice(void) {
    static short juice_anim[2] = { 0x498, 0x27CC };
    static struct DramaDemo_PlayInfo juice = { 15, MemShare_gp_data_buf, juice_anim, NULL, 0, 0, NULL, 60011, 15.0f, 0.0f, 0.0f };
    static struct CharaData_DemoList chara_data[3] = {
        { 258, data_chr_jms_hhl_jms_notex_mdl, data_demo_dust_hhl_jms_anm, NULL, data_demo_dust_hhl_jms_cls },
        { 1069, data_chr_item_i_juice_mdl, data_demo_dust_i_juice_anm, NULL, NULL },
        { 0, NULL, NULL, NULL, NULL },
    };
    static int se_check = 0;
    int ret;

    switch (ev_p_step) {
    case 0:
        FcRead(data_demo_dust_dust_dds, MemShare_gp_data_buf);
        CharaDataLoadDemo(chara_data, 0);
        CharaAdminPlayableDisplay(0);
        SCNowDemoEventSwitch(sh2jms.player, 1);
        se_check = 0;
        EV_STEP(0x16);
    case 0x16:
        ret = DramaDemoMain(&juice);
        if (demo_frame >= 60.0f && !se_check) {
            SeCallPos(0x3E85, 1.0f, (float[4]){ -103202.05f, -97.754f, 8977.81f, 0.0f }, 0);
            se_check = 1;
        }
        if (!ret) {
            break;
        }
        EV_STEP(0xD);
        break;
    case 0xD:
        CharaDataDeleteOne(0x102);
        CharaDataDeleteOne(0x42D);
        CharaAdminPlayableDisplay(1);
        vcReturnPreAutoCamWork(1);
        SCNowDemoEventSwitch(sh2jms.player, 0);
        shCharacterPlayerModelToPlayable();
        return 1;
    }
    return 0;
}

static int EvProgNoFaceCorpse(void) {
    static short face_anim[1] = { 0x496 };
    static struct DramaDemo_MessageTime face_msg[2] = { { 0xA5, 0x129 }, { 0xFFFF, 0xFFFF } };
    static struct DramaDemo_PlayInfo face = { 10, NULL, face_anim, face_msg, 25, 0, NULL, 60037, 0.0f, 0.0f, 0.0f };

    switch (ev_p_step) {
    case 0:
        CharaAdminPlayableDisplay(0);
        SCNowDemoEventSwitch(sh2jms.player, 1);
        face.adr_dds_top = (char *)kao_dds;
        EV_STEP(0x16);
    case 0x16:
        if (!DramaDemoMain(&face)) {
            break;
        }
        EV_STEP(0xD);
        break;
    case 0xD:
        CharaDataDeleteOne(0x102);
        sh2jms.player->status |= 0x10;
        vcReturnPreAutoCamWork(1);
        CharaAdminPlayableDisplay(1);
        SCNowDemoEventSwitch(sh2jms.player, 0);
        shCharacterPlayerModelToPlayable();
        return 1;
    }
    return 0;
}

static int EvProgAnyoneCry(void) {
    static float cry_pos[4] = { -99400.0f, -500.0f, 26400.0f, 0.0f };
    int anm;

    switch (ev_p_step) {
    case 0:
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        PlayerEventAnimeSet(0x4E22);
        ev_timer = 0.0f;
        SeCallPos(0x9C49, 1.0f, cry_pos, 6);
        EV_STEP(0x1B);
    case 0x1B:
        Se3dControl(0x9C49, 1.0f, cry_pos);
        ev_timer += shGetDT();
        if (!shCharacterAnimeIsEnd(sh2jms.player)) {
            break;
        }
        anm = (ev_timer < 2.0f) ? 0 : EvSubMessage(0);
        if (!anm) {
            break;
        }
        EV_STEP(0xD);
        break;
    case 0xD:
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        return 1;
    }
    return 0;
}

static int EvProgGetLight(void) {
    int ret;

    ret = EvSubItemGetAndAnim(0xF, 0x14);
    if (ret) {
        item.light_switch = 1;
        LightSpotOnOffSet();
        if (((game_flag.flag[2] >> 2) & 1) && ((game_flag.flag[2] >> 3) & 1)) {
            game_flag.flag[2] |= 0x10;
        }
        enEventDriven(2, 0);
    }
    return ret;
}

static int EvProgGetApart202Key(void) {
    return EvSubItemGetAndAnim(0x18, 0x15);
}

static int EvProgEndHintRecoveryRead(void) {
    switch (ev_p_step) {
    case 0:
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        PlayerEventAnimeSet(0x65);
        FcRead(data_pic_apt_p_endhint_tex, get_gp_data_buf_addr());
        EV_STEP(0x1E);
    case 0x1E:
        if (!EvSubMessage(0x1A)) {
            break;
        }
        ScreenEffectFadeStart(1, 0.0f);
        EV_STEP(2);
    case 2:
        if (fsSync(1, -1) < 0) {
            break;
        }
        if (!ScreenEffectFadeCheck()) {
            break;
        }
        ScreenEffectFadeStart(4, 0.0f);
        EV_STEP(3);
    case 3:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureEnd();
        if (!ScreenEffectFadeCheck()) {
            break;
        }
        EV_STEP(8);
        break;
    case 8:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureEnd();
        if (shPadTrigger(0, key_config.enter) || shPadTrigger(0, key_config.cancel)) {
            EV_STEP(0xA);
        }
        break;
    case 0xA:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureFilter();
        EvSubPictureEnd();
        if (!EvSubMessage(0x1B)) {
            break;
        }
        ScreenEffectFadeStart(1, 0.0f);
        EV_STEP(4);
        break;
    case 4:
        if (!ScreenEffectFadeCheck()) {
            break;
        }
        EV_STEP(0xD);
        ScreenEffectFadeStart(4, 0.0f);
    case 0xD:
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        return 1;
    }
    return 0;
}

static int EvProgEndHintMariaRead(void) {
    switch (ev_p_step) {
    case 0:
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        PlayerEventAnimeSet(0x65);
        FcRead(data_pic_apt_p_endhint_tex, get_gp_data_buf_addr());
        EV_STEP(0x1E);
    case 0x1E:
        if (!EvSubMessage(0x1A)) {
            break;
        }
        ScreenEffectFadeStart(1, 0.0f);
        EV_STEP(2);
    case 2:
        if (fsSync(1, -1) < 0) {
            break;
        }
        if (!ScreenEffectFadeCheck()) {
            break;
        }
        ScreenEffectFadeStart(4, 0.0f);
        EV_STEP(3);
    case 3:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureEnd();
        if (!ScreenEffectFadeCheck()) {
            break;
        }
        EV_STEP(8);
        break;
    case 8:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureEnd();
        if (shPadTrigger(0, key_config.enter) || shPadTrigger(0, key_config.cancel)) {
            EV_STEP(0xA);
        }
        break;
    case 0xA:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureFilter();
        EvSubPictureEnd();
        if (!EvSubMessage(0x1C)) {
            break;
        }
        ScreenEffectFadeStart(1, 0.0f);
        EV_STEP(4);
        break;
    case 4:
        if (!ScreenEffectFadeCheck()) {
            break;
        }
        EV_STEP(0xD);
        ScreenEffectFadeStart(4, 0.0f);
    case 0xD:
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        return 1;
    }
    return 0;
}

static int EvProgEndHintSuicideRead(void) {
    switch (ev_p_step) {
    case 0:
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        PlayerEventAnimeSet(0x65);
        FcRead(data_pic_apt_p_endhint_tex, get_gp_data_buf_addr());
        EV_STEP(0x1E);
    case 0x1E:
        if (!EvSubMessage(0x1A)) {
            break;
        }
        ScreenEffectFadeStart(1, 0.0f);
        EV_STEP(2);
    case 2:
        if (fsSync(1, -1) < 0) {
            break;
        }
        if (!ScreenEffectFadeCheck()) {
            break;
        }
        ScreenEffectFadeStart(4, 0.0f);
        EV_STEP(3);
    case 3:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureEnd();
        if (!ScreenEffectFadeCheck()) {
            break;
        }
        EV_STEP(8);
        break;
    case 8:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureEnd();
        if (shPadTrigger(0, key_config.enter) || shPadTrigger(0, key_config.cancel)) {
            EV_STEP(0xA);
        }
        break;
    case 0xA:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureFilter();
        EvSubPictureEnd();
        if (!EvSubMessage(0x1D)) {
            break;
        }
        ScreenEffectFadeStart(1, 0.0f);
        EV_STEP(4);
        break;
    case 4:
        if (!ScreenEffectFadeCheck()) {
            break;
        }
        EV_STEP(0xD);
        ScreenEffectFadeStart(4, 0.0f);
    case 0xD:
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        return 1;
    }
    return 0;
}

static int EvProgUseApart202Key(void) {
    return EvSubItemUse0(0x18, 0x17, 0x4A3B, 0, (float[4]){ -100447.5f, -500.0f, -10785.03f, 0.0f }, 1);
}

static int EvProgMonkeyKick206(void) {
    if (!((game_flag.flag[2] >> 5) & 1)) {
        SeCallPos(0x9C47, 1.0f, (float[4]){ -99000.0f, -1500.0f, 13600.0f, 0.0f }, 2);
    }
    return 1;
}

static int EvCharaDataClear(int room) {
    if (room == 0x18 && ((game_flag.flag[2] >> 5) & 1) && !((game_flag.flag[2] >> 7) & 1)) {
        return 1;
    }
    return 0;
}

static void EvRoomInit(void) {
    static struct CharaData_DemoList chara_data[2] = {
        { 258, data_chr_jms_hhl_jms_notex_mdl, data_demo_kao_hhl_jms_anm, data_chr_jms_hhh_jms_kg1, data_demo_kao_hhl_jms_cls },
        { 0, NULL, NULL, NULL, NULL },
    };
    int room;

    room = RoomNameJms();
    if (room == 0x18 && ((game_flag.flag[2] >> 5) & 1) && !((game_flag.flag[2] >> 7) & 1)) {
        CharaDataLoadDemo(chara_data, 0);
        kao_dds = (unsigned char *)CharaDataLoadExtra(data_demo_kao_kao_dds, 0x200);
        fsSync(0, -1);
    }
    if (room == 0x16) {
        enSetInsect((float[4]){ -22800.0f, -700.0f, 59200.0f, 1.0f }, 5);
    } else {
        enKillAllInsect();
    }
}

static void EvSoundCallAfterLoad(void) {
    int room;

    room = RoomNameJms();
    if (room == 0x18) {
        if ((game_flag.flag[2] >> 5) & 1) {
            SeCallPos(0x3E80, 0.3f, tv_pos, 0xC);
            sh2gfw_SetNoise_CharaTexture(0x557);
            game_flag.flag[2] |= 0x40;
        } else {
            sh2gfw_RemoveNoise_CharaTexture(0x557);
        }
    }
}

static void EvAllTimeFunc(void) {
    static unsigned char corpse_on[8][2] = {
        {
            0x2A, 0x00,
        },
        {
            0x2B, 0x00,
        },
        {
            0x2B, 0x01,
        },
        {
            0x2B, 0x02,
        },
        {
            0x2B, 0x03,
        },
        {
            0x2D, 0x00,
        },
        {
            0x2D, 0x02,
        },
        {
            0x2D, 0x04,
        },
    };
    static unsigned char corpse_off[5][2] = {
        {
            0x2A, 0x01,
        },
        {
            0x2B, 0x04,
        },
        {
            0x2D, 0x01,
        },
        {
            0x2D, 0x03,
        },
        {
            0x2D, 0x05,
        },
    };
    struct SubCharacter *scp;
    float pos[4];
    float rot[4];
    float volume;
    int disp_ctrl_list[7];
    int room;
    int i;

    disp_ctrl_list[0] = 0;
    room = RoomNameJms();
    if (room == 0x17) {
        if (!((item.flag[0] >> 15) & 1)) {
            sh2gde_SetSpot_JmsOrBG(0);
        }
        if (((game_flag.flag[0] >> 7) & 1) || ((game_flag.flag[0] >> 8) & 1) || ((game_flag.flag[0] >> 9) & 1)) {
            EvDispControlModelEntry(disp_ctrl_list, 0x20, 0);
        } else {
            EvDispControlModelEntry(disp_ctrl_list, 0x20, -1);
        }
    } else if (room == 0x18) {
        sh2shd_add_map_to_shadow_off_work(0x2A);
        if ((game_flag.flag[2] >> 5) & 1) {
            for (i = 0; i < 8; i++) {
                EvDispControlModelEntry(disp_ctrl_list, corpse_on[i][0], corpse_on[i][1]);
            }
            clAddDynamicWall(&clActWallList_ap42[10]);
            clAddDynamicFloor(clActFloorList_ap42);
            sh2shd_off_obj(0x2A, 0x3F);
        } else {
            for (i = 0; i < 5; i++) {
                EvDispControlModelEntry(disp_ctrl_list, corpse_off[i][0], corpse_off[i][1]);
            }
            clAddDynamicWall(&clActWallList_ap42[16]);
            clAddDynamicFloor(&clActFloorList_ap42[4]);
            clAddDynamicWall(&clActWallList_ap42[21]);
            clAddDynamicFloor(&clActFloorList_ap42[6]);
            sh2shd_off_obj(0x2A, 0x3E);
        }
        if ((game_flag.flag[2] >> 18) & 1) {
            clAddDynamicWall(clActWallList_ap42);
        } else {
            clAddDynamicWall(&clActWallList_ap42[5]);
        }
        scp = shCharacterGetSubCharacter(0x422, 0);
        shCharacterStayObjectNthPartsGet1st(scp, 2, pos, rot);
        rot[2] += shAngleRegulate(6.2831855f * itof(game_flag.clock % 0xF00) / 3840.0f);
        shCharacterStayObjectNthPartsSet(scp, 2, pos, rot);
        shCharacterStayObjectNthPartsGet1st(scp, 3, pos, rot);
        rot[2] += shAngleRegulate(1.5707964f + 6.2831855f * itof(game_flag.clock) / 46080.0f);
        shCharacterStayObjectNthPartsSet(scp, 3, pos, rot);
        volume = SeCallPosDistanceF(tv_pos);
        if (sh2jms.player->pos.x > -59750.0f && sh2jms.player->pos.z > 21300.0f) {
            volume *= 1.0f - (59750.0f + sh2jms.player->pos.x) / 4000.0f;
        }
        Se2dManageDataVolumeChange(0x3E80, 0.4f * volume);
    }
    EvDispControlModelExec(disp_ctrl_list);
}

static void Delete_RedPointLight(void) {
    int *mp;
    struct DrawEnvData *ded;

    mp = Get_NowMapId();
    ded = Get_NowDrawEnvData();
    if (*mp == 0x90032) {
        if (EventProgressCheck() == 4) {
            if (ded->pointLNum < 5) {
                ded->pointLNum = 5;
            }
        } else {
            if (ded->pointLNum > 4) {
                ded->pointLNum = 4;
            }
        }
    }
}
