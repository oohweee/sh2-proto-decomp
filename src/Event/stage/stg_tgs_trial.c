/*
 * stg_tgs_trial.c: stage overlay for the TGS trial (demo) version of the hospital: maps, the
 * needle, the number and box puzzles, the shower drain, the doctor's memos, the elevator and the
 * end of the trial.
 * The stage's event programs (EvProg*), room set-up and Stage_Data (stage_tgs_trial).
 */
#include "sh2.h"
#include "asm_helpers.h"
#include "sh_vu0.h"

/* Next program step. Matching: the do/while leaves the original's nop before a case label it
 * falls into. */
#define EV_STEP(n) do { ev_p_step = (n); ev_s_step = 0; } while (0)
#define EV_FLAG(n) ((game_flag.flag[(n) >> 5] >> ((n) & 0x1F)) & 1)

/* PicDraw_Data alpha blend setup. Matching: the do/while leaves a nop before a join. */
#define PIC_ALPHA(p, alp)            \
    do {                             \
        (p).a = (alp);               \
        (p).alpha_a = 0;             \
        (p).alpha_b = 1;             \
        (p).alpha_c = 0;             \
        (p).alpha_d = 1;             \
        (p).alpha_fix = 0x80;        \
        (p).status |= 0x20;          \
    } while (0)

static int EvProgTrialStartSet(void);
static int EvProgGetHospitalMap(void);
static int EvProgGetNeedle(void);
static int EvProgGuruguruNumber(void);
static int EvProgLouiseTakecare(void);
static int EvProgBoxWithKey(void);
static int EvProgBoxWithKeyLayer(void);
static int EvProgBoxWithKeyCursor(void);
static int EvProgBoxWithKeyOpen(int alp);
static int EvProgEmptyBox(void);
static int EvProgOnlyNeedle(void);
static int EvProgOnlyHair(void);
static int EvProgInShowerDrain(void);
static int EvProgFishKey(void);
static int EvProgDoctorMemo1st(void);
static int EvProgDoctorMemo2nd(void);
static int EvProgUseElevatorKey(void);
static int EvProgElevatorButton(void);
static int EvProgElevatorButtonCheck(void);
static int EvProgElevatorButtonLight(void);
static int EvProgTrialEnd(void);
static void EvRoomInit(void);
static void EvAllTimeFunc(void);

int s11_ton;
float office_3d_timer;
float office_3d_rot;
float s11_timer;
static float cyl_alp;

static struct _AnimeInfo pjames_stage_anim[20] = {
    { 0x4E21, 0x14, 1024, 0x400, 0x413, 0, 10 },
    { 0x4E27, 0x1E, 1024, 0x414, 0x431, 0, 0 },
    { 0x4E28, 0xF, 768, 0x432, 0x440, 0, 0 },
    { 0x4E29, 0xF, 768, 0x441, 0x44F, 0, 0 },
    { 0x4E2A, 0x1E, 768, 0x450, 0x46D, 0, 0 },
    { 0x4E2B, 0x1E, 768, 0x46E, 0x48B, 0, 0 },
    { 0x4E2C, 0xF, 1024, 0x490, 0x49A, 0, 0 },
    { 0x4E2D, 0xF, 1024, 0x49F, 0x4A9, 0, 0 },
    { 0x4E2E, 0x1E, 640, 0x4AA, 0x4C7, 0, 0 },
    { 0x4E2F, 0x1E, 640, 0x4C8, 0x4E5, 0, 0 },
    { 0x4E30, 0xF, 1024, 0x4EA, 0x4F4, 0, 0 },
    { 0x4E31, 0xF, 1024, 0x4F9, 0x503, 0, 0 },
    { 0x4E32, 0x1E, 1024, 0x504, 0x521, 0, 0 },
    { 0x4E33, 0x1E, 1024, 0x522, 0x53F, 0, 0 },
    { 0x4E34, 0x1E, 1024, 0x540, 0x55D, 0, 0 },
    { 0x4E39, 0xF, 1024, 0x562, 0x56C, 0, 0 },
    { 0x4E3A, 0xF, 1024, 0x571, 0x57B, 0, 0 },
    { 0x4E3B, 0x1E, 1024, 0x57C, 0x599, 0, 0 },
    { 0x4E3C, 0x1E, 1024, 0x59A, 0x5B7, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0 },
};

static unsigned char ev_pos[1240] = {
    0x6E, 3, 0x96, 0xC6, 0, 0x80, 0x25, 0x4C, 0x82, 0xC6, 5, 0x5F, 0, 0xB1, 0x69, 0xC7,
    0, 0x80, 0, 0x78, 0x9B, 0xC6, 0x14, 0x5D, 0, 0xBC, 0x98, 0xC6, 0, 0x80, 0, 0x4C,
    0x9A, 0x46, 0x40, 0x5A, 0, 0x38, 0x95, 0xC6, 0, 0x80, 0, 0xBE, 0x6B, 0x47, 0x1A, 0x68,
    0, 0x84, 0x99, 0x46, 0, 0x80, 0x8D, 0x9B, 0x69, 0x47, 0x87, 0x5D, 0x80, 0x2E, 8, 0x48,
    0, 0x80, 0, 0xC0, 0xC1, 0x47, 0x40, 0x5E, 0x40, 0x5A, 8, 0x48, 0, 0x80, 0, 0xDF,
    0xC0, 0x47, 0xE1, 0x5D, 0xA8, 0x5C, 0xD, 0xA6, 8, 0x48, 0, 0x80, 0x81, 0x10, 0x66, 0x47,
    0, 0xEA, 8, 0x48, 0, 0x80, 0, 0x9E, 0x68, 0x47, 0x40, 0x5A, 0, 0x7C, 0xC4, 0xC7,
    0, 0x80, 0, 0x28, 0xA0, 0xC6, 0xB0, 0x64, 0, 0x10, 0x6F, 0xC7, 0, 0x80, 0, 0x70,
    0xC6, 0x47, 0xD0, 0x63, 0, 0xB0, 0x65, 0x47, 0, 0x80, 0, 0xA0, 0x70, 0x47, 0xD0, 0x63,
    0, 0x7C, 0xC4, 0xC7, 0, 0x80, 0xE6, 0x27, 0xA0, 0xC6, 0x40, 0x5A, 0, 0xDC, 0x91, 0xC6,
    0, 0x80, 0, 0xB8, 0x6C, 0xC7, 0x40, 0x5E, 0xFF, 0x82, 0x6C, 0xC7, 0, 0x80, 1, 0xF8,
    0x9D, 0xC6, 0x9F, 0x5D, 0, 7, 0x99, 0xC6, 0, 0x80, 0, 0xC8, 0x7A, 0xC6, 0x72, 0x62,
    0, 0x42, 0xA4, 0xC6, 0, 0x80, 0, 0x6E, 0x8C, 0xC6, 8, 0x5F, 0xFE, 0x67, 0x9C, 0xC6,
    0, 0x80, 0xBE, 0x76, 0x67, 0xC7, 0xA0, 0x5D, 0, 0x72, 0x83, 0xC6, 0, 0x80, 0, 0xB2,
    0x89, 0xC6, 0xD6, 0x5E, 0, 0x72, 0x83, 0xC6, 0, 0x80, 0, 0x32, 0x96, 0xC6, 8, 0x5F,
    0, 0xE, 0x83, 0xC6, 0, 0x80, 0, 0x2E, 0x9F, 0xC6, 0x89, 0x5D, 0, 0xF0, 0x82, 0xC6,
    0, 0x80, 0, 0x74, 0xA4, 0xC6, 0xD0, 0x5F, 0, 0xE, 0x83, 0xC6, 0, 0x80, 0x37, 0x90,
    0xA8, 0xC6, 0x89, 0x5D, 0, 0x6E, 0x8C, 0xC6, 0, 0x80, 1, 0x2E, 0x9F, 0xC6, 8, 0x5F,
    0xFE, 0xF5, 0x95, 0xC6, 0, 0x80, 0, 0xCA, 0xA8, 0xC6, 0xC5, 0x63, 0x9C, 0x4E, 0x83, 0xC6,
    0, 0x80, 0, 0xF0, 0xB9, 0xC6, 0x40, 0x62, 0, 0x72, 0x83, 0xC6, 0, 0x80, 0, 0x62,
    0xC0, 0xC6, 8, 0x5F, 0, 0x82, 0x91, 0xC6, 0, 0x80, 0, 0x4E, 0xBB, 0xC6, 0xD6, 0x5E,
    0, 0xE0, 0xC4, 0xC6, 0, 0x80, 0, 0x86, 0xBA, 0xC6, 0x40, 0x62, 0, 0x10, 0xA4, 0xC6,
    0, 0x80, 0, 0x20, 0xB2, 0xC6, 0x40, 0x62, 0, 0xDE, 0xC7, 0xC6, 0, 0x80, 0, 0xE6,
    0xB2, 0xC6, 8, 0x63, 0, 0x12, 0xAC, 0xC6, 0, 0x80, 0, 0x92, 0x9F, 0xC6, 8, 0x5F,
    0, 0xE, 0xB5, 0xC6, 0, 0x80, 1, 0x2E, 0x9F, 0xC6, 8, 0x5F, 0, 0xE, 0xB5, 0xC6,
    0, 0x80, 0, 0x5E, 0x97, 0xC6, 8, 0x5F, 0, 0xE, 0xB5, 0xC6, 0, 0x80, 1, 0xDE,
    0x8A, 0xC6, 8, 0x5F, 2, 0x9E, 0xB6, 0xC6, 0, 0x80, 0, 0x39, 0xB2, 0xC6, 8, 0x5F,
    0, 0xD1, 0x6C, 0xC7, 0, 0x80, 0, 0xE0, 0xC4, 0xC7, 8, 0x5F, 0, 0x90, 0xC9, 0xC7,
    0, 0x80, 0x71, 0x1E, 0x99, 0x46, 0xB0, 0x60, 0, 0x90, 0xC9, 0xC7, 0, 0x80, 0, 0xC0,
    0xA8, 0x46, 0x40, 0x5E, 0, 0xE0, 0x92, 0xC6, 0, 0x80, 0, 0x98, 0x69, 0x47, 0x40, 0x5E,
    0, 0x38, 0xC7, 0xC7, 0, 0x80, 0, 0x60, 0x9F, 0x46, 0x40, 0x5E, 0, 0xA0, 0xA5, 0xC6,
    0, 0x80, 0, 0xD0, 0x9D, 0x46, 0x40, 0x5E, 0, 0x38, 0xC7, 0xC7, 0, 0x80, 0, 0x70,
    0x94, 0x46, 0x40, 0x5E, 0, 0xC0, 0xC1, 0xC7, 0, 0x80, 0, 0xC0, 0x8F, 0x46, 0xD0, 0x63,
    0, 0x68, 0xBF, 0xC7, 0, 0x80, 0, 0x60, 0x51, 0x46, 0x40, 0x62, 0, 0x68, 0xBF, 0xC7,
    0, 0x80, 0, 0xC0, 0x73, 0x46, 0x40, 0x5E, 0, 0xA8, 0xC5, 0xC7, 0, 0x80, 0, 0x60,
    0x86, 0x46, 0x40, 0x5E, 0, 0x78, 0xCD, 0xC7, 0, 0x80, 0, 0x90, 0x7B, 0x46, 0x40, 0x62,
    0, 0x90, 0x49, 0xC7, 0, 0x80, 0, 0xE0, 0xC4, 0x47, 0x40, 0x62, 0xA7, 0xE7, 0xC9, 0xC7,
    0, 0x80, 0, 0x60, 0x86, 0x46, 8, 0x5F, 0, 0xD1, 0x6C, 0xC7, 0xD0, 0xE7, 0, 0xE0,
    0xC4, 0xC7, 8, 0x5F, 0xFF, 9, 0x48, 0xC7, 0, 0x80, 0, 0xEC, 0xC2, 0x47, 0x40, 0x62,
    0, 0x40, 0x4E, 0xC7, 0, 0x80, 0, 0xC0, 0xC1, 0x47, 0x40, 0x66, 0xB0, 0x64, 0, 0x48,
    0x55, 0xC7, 0, 0x80, 0, 0x88, 0xC2, 0x47, 0x40, 0x62, 0, 0x38, 0x60, 0xC7, 0, 0x80,
    0, 0x88, 0xC2, 0x47, 0x40, 0x62, 0, 0x28, 0x6B, 0xC7, 0, 0x80, 0, 0x88, 0xC2, 0x47,
    0x40, 0x62, 0, 0x18, 0x76, 0xC7, 0, 0x80, 0, 0x88, 0xC2, 0x47, 0x40, 0x62, 0, 0x84,
    0x80, 0xC7, 0, 0x80, 0, 0x88, 0xC2, 0x47, 0x40, 0x62, 0, 0xFC, 0x85, 0xC7, 0, 0x80,
    0, 0x88, 0xC2, 0x47, 0x40, 0x62, 0, 0x54, 8, 0xC8, 0, 0x80, 0xB, 0xF6, 0x98, 0x46,
    0x40, 0x62, 0, 0x10, 0x6F, 0xC7, 0, 0x80, 0, 0x44, 0xC5, 0x47, 0x40, 0x5E, 0, 0xA0,
    0x57, 0xC7, 0, 0x80, 0, 0xE0, 0xC4, 0x47, 0x40, 0x5E, 0, 0x73, 0x50, 0xC7, 0, 0x80,
    0, 0xE0, 0xC4, 0x47, 8, 0x63, 0, 0x31, 0x5D, 0x47, 0, 0x80, 0x43, 0x58, 0x86, 0x46,
    8, 0x5F, 0, 0xD1, 0x6C, 0xC7, 0xD0, 0xEB, 0, 0xE0, 0xC4, 0xC7, 8, 0x5F, 0, 0x19,
    0x61, 0x47, 0, 0x80, 0, 0xE0, 0x92, 0x46, 8, 0x5F, 0, 0x48, 0x6E, 0x47, 0, 0x80,
    0x80, 0x49, 0xC2, 0x47, 0x40, 0x5E, 0, 0xB0, 0x65, 0x47, 0, 0x80, 0, 0x60, 0x86, 0x46,
    0x40, 0x5E, 0, 0x80, 0x6D, 0x47, 0, 0x80, 0, 0xC0, 0x8F, 0x46, 0xD0, 0x63, 0, 0x30,
    0x72, 0x47, 0, 0x80, 0, 0xC0, 0x73, 0x46, 0x40, 0x5E, 0, 0x30, 0x72, 0x47, 0, 0x80,
    0, 0x80, 0x54, 0x46, 0xB0, 0x64, 0x40, 0x62, 0, 0x10, 0x56, 0x47, 0, 0x80, 0, 0x90,
    0x7B, 0x46, 0x40, 0x62, 0, 0x98, 0x85, 0x47, 0, 0x80, 0, 0x80, 0x6D, 0x47, 0x40, 0x62,
    0, 0x60, 0x86, 0x47, 0, 0x80, 0, 0x98, 0x69, 0x47, 0x40, 0x62, 0, 8, 0x84, 0x47,
    0, 0x80, 0, 0x40, 0x67, 0x47, 0x40, 0x62, 0, 0xB0, 0x65, 0x47, 0, 0x80, 0xFD, 0x33,
    0x6E, 0x47, 0xE0, 0x5E, 0, 0x20, 0x7D, 0x47, 0, 0x80, 0, 0x80, 0x6D, 0x47, 0x40, 0x5E,
    0, 0x10, 0x6F, 0x47, 0, 0x80, 0, 0x80, 0x6D, 0x47, 0x40, 0x5E, 0x9E, 0x85, 8, 0x48,
    0, 0x80, 0xC0, 0xC9, 0x62, 0x47, 0x3F, 0x5E, 0x82, 0x26, 0x82, 0x47, 0, 0x80, 0, 0x80,
    0x6D, 0x47, 8, 0x63, 0, 0xE2, 0x81, 0x47, 0, 0x80, 0, 0xD0, 0x68, 0x47, 0xD0, 0x5F,
    0, 0x14, 0x7F, 0x47, 0, 0x80, 0, 0xD0, 0x68, 0x47, 0xD0, 0x5F, 0, 0x64, 0x7A, 0x47,
    0, 0x80, 0, 0xD0, 0x68, 0x47, 0xD0, 0x5F, 0, 0xB4, 0x75, 0x47, 0, 0x80, 0, 0xD0,
    0x68, 0x47, 0xD0, 0x5F, 0, 4, 0x71, 0x47, 0, 0x80, 0, 0xD0, 0x68, 0x47, 0xD0, 0x5F,
    0, 0x54, 0x6C, 0x47, 0, 0x80, 0, 0xD0, 0x68, 0x47, 0xD0, 0x5F, 0, 0xA4, 0x67, 0x47,
    0, 0x80, 0, 0xD0, 0x68, 0x47, 0xD0, 0x5F, 0, 0xF4, 0x62, 0x47, 0, 0x80, 0, 0xD0,
    0x68, 0x47, 0xD0, 0x5F, 0, 0x44, 0x5E, 0x47, 0, 0x80, 0, 0xD0, 0x68, 0x47, 0xD0, 0x5F,
    0, 0x94, 0x59, 0x47, 0, 0x80, 0, 0xD0, 0x68, 0x47, 0xD0, 0x5F, 0, 0xE4, 0x54, 0x47,
    0, 0x80, 0, 0xD0, 0x68, 0x47, 0xD0, 0x5F, 0, 0x18, 0xC4, 0x47, 0, 0x80, 0xB, 0x92,
    0x98, 0x46, 0xD0, 0x5F, 0, 0x34, 0x50, 0x47, 0, 0x80, 0, 0xD0, 0x68, 0x47, 0xD0, 0x5F,
    0, 0x84, 0x4B, 0x47, 0, 0x80, 0, 0xD0, 0x68, 0x47, 0xD0, 0x5F, 0, 0xD4, 0x46, 0x47,
    0, 0x80, 0, 0xD0, 0x68, 0x47, 0xD0, 0x5F, 0, 0x1C, 9, 0x48, 0, 0x80, 0x80, 0x64,
    0xC2, 0x47, 0xD0, 0x5F, 0x7E, 0x27, 0x6F, 0x47, 0, 0x80, 0x80, 0x56, 0xC4, 0x47, 0xFB, 0x5E,
    0x7E, 7, 0x6C, 0x47, 0, 0x80, 0x80, 0x56, 0xC4, 0x47, 0xFB, 0x5E, 0x7E, 0xE7, 0x68, 0x47,
    0, 0x80, 0x80, 0x56, 0xC4, 0x47, 0xFB, 0x5E, 0, 0xB0, 0x9A, 0x46, 0, 0x80, 0x66, 0x31,
    0x68, 0x47, 0x40, 0x5E, 0x7E, 0xC7, 0x65, 0x47, 0, 0x80, 0x80, 0x56, 0xC4, 0x47, 0xFB, 0x5E,
    0, 0xD1, 0x6C, 0xC7, 0xDC, 0xED, 0, 0xE0, 0xC4, 0xC7, 8, 0x5F, 0, 0, 0, 0,
    0, 0x80, 0, 0, 0, 0x80, 0x40, 0x52,
};

static struct Event_List ev_list[119] = {
    { 0x40020000, 0x10000000, 0x30000000, 0x4000 },
    { 0x1B0000, 0x20004004, 0x30000000, 0x801B },
    { 0xA90000, 0x200C4004, 0x30000000, 0x300A9 },
    { 0, 0x200C4004, 0x30000000, 0x34000 },
    { 0xAA0000, 0x20181004, 0x30000000, 0xC0AA },
    { 0, 0x20242004, 0x60000000, 0x48000 },
    { 0, 0x20303004, 0x30000000, 0x100C0 },
    { 0, 0x203C3004, 0x30000000, 0x140BF },
    { 0x80AB0000, 0x20485004, 0x30000000, 0x1C000 },
    { 0, 0x20485004, 0x30000000, 0x18000 },
    { 0x40C20000, 0x5048520C, 0x30000000, 0x180C1 },
    { 0, 0x505604CC, 0x30000000, 0x2C0AC },
    { 0, 0x50560334, 0x30000000, 0x20000 },
    { 0, 0x50560324, 0x30000000, 0x24000 },
    { 0, 0x20560004, 0x30000000, 0x28000 },
    { 0, 0x20602004, 0x60000000, 0x34000 },
    { 0xC0AE4012, 0x10000000, 0x406C1000, 0x60F },
    { 0x80AD4012, 0x20782000, 0x406C1000, 0x60F },
    { 0x40AE0000, 0xA078222C, 0x30000000, 0x380AD },
    { 0, 0x20782004, 0x60000000, 0x40610 },
    { 0xC0AF4013, 0x10000000, 0x406C1000, 0x62F },
    { 0x80AD4013, 0x20842000, 0x406C1000, 0x62F },
    { 0x40AF0000, 0xA084222C, 0x30000000, 0x380AD },
    { 0, 0x20842004, 0x60000000, 0x40630 },
    { 0, 0x20901004, 0x30000000, 0x3C000 },
    { 0xC0150000, 0x10000000, 0x30000000, 0x40000 },
    { 0xC0160000, 0x10000000, 0x40782010, 0x60F },
    { 0xC0170000, 0x10000000, 0x40842010, 0x62F },
    { 0, 0x209C4000, 0x40A83000, 0x4005ED },
    { 0, 0x20A83000, 0x409C4000, 0x4005ED },
    { 0, 0x20B42000, 0x90000000, 0x7F85C3 },
    { 0, 0x20C01000, 0x40CC2000, 0x4005C5 },
    { 0, 0x20CC2000, 0x40C01010, 0x4005C5 },
    { 0, 0x20D84000, 0x90000000, 0x7FC5C6 },
    { 0, 0x20E44000, 0x90000000, 0x7FC5C7 },
    { 0, 0x20F04000, 0x90000000, 0x7FC5C8 },
    { 0, 0x20FC4000, 0x90000000, 0x7FC5C9 },
    { 0, 0x21084000, 0x90000000, 0x7FC5CA },
    { 0, 0x21143000, 0x90000000, 0x7FC5CD },
    { 0, 0x21203004, 0x60000000, 0x445CE },
    { 0, 0x212C4000, 0x90000000, 0x7FC5CF },
    { 0, 0x21384000, 0x90000000, 0x7FC5D0 },
    { 0, 0x21441000, 0x90000000, 0x7FC5D3 },
    { 0, 0x21501000, 0x90000000, 0x13FC5D5 },
    { 0, 0x215C2000, 0x90000000, 0x7FC5D7 },
    { 0, 0x21683000, 0x90000000, 0x7F85D8 },
    { 0, 0x21744000, 0x90000000, 0x7FC5DB },
    { 0, 0x21803000, 0x90000000, 0x7FC5DC },
    { 0, 0x218C3000, 0x90000000, 0x7FC5DD },
    { 0, 0x21983000, 0x90000000, 0x7FC5DE },
    { 0, 0x21A42000, 0x41B01000, 0x4005DF },
    { 0, 0x21B01000, 0x41A42010, 0x4005DF },
    { 0, 0x21BC3000, 0x90000000, 0x7FC5F7 },
    { 0, 0x21C83000, 0x41D44000, 0x4005F8 },
    { 0, 0x21D44000, 0x41C83010, 0x4005F8 },
    { 0, 0x21E04000, 0x41EC3000, 0x4005F9 },
    { 0, 0x21EC3000, 0x41E04010, 0x4005F9 },
    { 0, 0x21F84000, 0x90000000, 0x7FC5FA },
    { 0, 0x22043004, 0x60000000, 0x445FB },
    { 0, 0x22103000, 0x90000000, 0x7FC5FC },
    { 0, 0x221C3000, 0x90000000, 0x7FC5FD },
    { 0, 0x22282000, 0x90000000, 0x7FC5FE },
    { 0, 0x22341000, 0x42402010, 0x10005FF },
    { 0, 0x22402000, 0x42341010, 0x10005FF },
    { 0, 0x224C2000, 0x42581000, 0x400600 },
    { 0, 0x22581000, 0x424C2010, 0x400600 },
    { 0, 0x22644000, 0x90000000, 0x7FC601 },
    { 0, 0x2270B000, 0x90000000, 0x7FC602 },
    { 0, 0x227E1000, 0x90000000, 0x7FC603 },
    { 0, 0x228A1000, 0x90000000, 0x7FC605 },
    { 0, 0x22961000, 0x90000000, 0x7FC607 },
    { 0, 0x22A21000, 0x90000000, 0x7FC608 },
    { 0, 0x22AE1000, 0x90000000, 0x7FC609 },
    { 0, 0x22BA1000, 0x42C62000, 0x40060A },
    { 0, 0x22C62000, 0x42BA1010, 0x40060A },
    { 0, 0x22D24000, 0x90000000, 0x7FC60B },
    { 0, 0x22DE2000, 0x90000000, 0x7FC60C },
    { 0, 0x22EA2000, 0x90000000, 0x7FC60E },
    { 0, 0x22F62000, 0x43021000, 0x400611 },
    { 0, 0x23021000, 0x42F62010, 0x400611 },
    { 0, 0x230E2000, 0x431A1000, 0x400612 },
    { 0, 0x231A1000, 0x430E2010, 0x400612 },
    { 0, 0x23262000, 0x90000000, 0x7FC614 },
    { 0, 0x23323004, 0x60000000, 0x44615 },
    { 0, 0x233E3000, 0x90000000, 0x7FC617 },
    { 0, 0x234AB000, 0x90000000, 0x7FC616 },
    { 0, 0x23581000, 0x43642010, 0x1000618 },
    { 0, 0x23642000, 0x43581010, 0x1000618 },
    { 0, 0x23704000, 0x90000000, 0x7FC61A },
    { 0, 0x237C1000, 0x90000000, 0x7FC61B },
    { 0, 0x23884000, 0x90000000, 0x7FC62B },
    { 0, 0x23942000, 0x90000000, 0x7FC62C },
    { 0, 0x23A02000, 0x43AC1000, 0x40062D },
    { 0, 0x23AC1000, 0x43A02010, 0x40062D },
    { 0, 0x23B82000, 0x90000000, 0x7FC62E },
    { 0, 0x23C41000, 0x90000000, 0x7FC61C },
    { 0, 0x23D01000, 0x90000000, 0x7FC61D },
    { 0, 0x23DC1000, 0x90000000, 0x7FC61F },
    { 0, 0x23E81000, 0x90000000, 0x7FC620 },
    { 0, 0x23F41000, 0x90000000, 0x7FC621 },
    { 0, 0x24001000, 0x90000000, 0x7FC622 },
    { 0, 0x240C1000, 0x90000000, 0x7FC623 },
    { 0, 0x24181000, 0x90000000, 0x7FC624 },
    { 0, 0x24241000, 0x90000000, 0x7FC625 },
    { 0, 0x24301000, 0x90000000, 0x7FC626 },
    { 0, 0x243C1000, 0x44482000, 0x400627 },
    { 0, 0x24482000, 0x443C1010, 0x400627 },
    { 0, 0x24541000, 0x90000000, 0x7FC628 },
    { 0, 0x24601000, 0x90000000, 0x7FC629 },
    { 0, 0x246C1000, 0x44782000, 0x40062A },
    { 0, 0x24782000, 0x446C1010, 0x40062A },
    { 0, 0x24842000, 0x90000000, 0x7FC631 },
    { 0, 0x24902000, 0x90000000, 0x7FC632 },
    { 0, 0x249C2000, 0x44A81000, 0x400633 },
    { 0, 0x24A81000, 0x449C2000, 0x400633 },
    { 0, 0x24B42000, 0x90000000, 0x7FC634 },
    { 0, 0x24C01000, 0x90000000, 0x7FC63B },
    { 0, 0x24CC1004, 0x60000000, 0x4C5EE },
    { 0, 0, 0, 0 },
};

/* The drama-demo script of the key-fishing scene (EvProgFishKey): a "dds" file the original
 * linked in as data. It is the game's data, so it isn't committed: tools/extract.py writes it from
 * the user's disc at build time. */
static char dds_tsuri[24091] = {
#include "assets/Event/stage/stg_tgs_trial/dds_tsuri.inc"
};

static int (*ev_prog[17])(void) = {
    NULL,
    EvProgTrialStartSet,
    EvProgGetHospitalMap,
    EvProgGetNeedle,
    EvProgGuruguruNumber,
    EvProgLouiseTakecare,
    EvProgBoxWithKey,
    EvProgEmptyBox,
    EvProgOnlyNeedle,
    EvProgOnlyHair,
    EvProgInShowerDrain,
    EvProgFishKey,
    EvProgDoctorMemo1st,
    EvProgDoctorMemo2nd,
    EvProgUseElevatorKey,
    EvProgElevatorButton,
    EvProgTrialEnd,
};

static struct Model_List mdl_list[5] = {
    { 1799, 0, 169, 0, { -59650.03f, -453.03882f, -19790.521f, 1.0f }, { -1.517296f, 0.053424f, 2.354779f, 1.0f } },
    { 1801, 0, 170, 0, { -19405.768f, -444.6808f, 19563.783f, 1.0f }, { -1.570802f, -2e-06f, -1.190766f, 1.0f } },
    { 1802, 0, 172, 0, { 139875.48f, 56.0629f, 58876.75f, 1.0f }, { -2.263852f, -0.409495f, -1.204362f, 1.0f } },
    { 1300, 0, 0, 0, { 139998.47f, -463.00568f, 98616.99f, 1.0f }, { 0.0f, 0.0f, 0.0f, 1.0f } },
    { 0, 0, 0, 0, { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f, 1.0f } },
};

static struct Enemy_List en_list[1] = {
    { 0, 0, 0, 0, 0, 0, 0, 0 },
};

struct Stage_Data stage_tgs_trial = {
    ev_list,
    ev_pos,
    ev_prog,
    NULL,
    mdl_list,
    en_list,
    NULL,
    EvRoomInit,
    EvAllTimeFunc,
    10,
    1,
    pjames_stage_anim,
    NULL,
    NULL,
    NULL,
    NULL,
    0,
};

static float fly_pos[4] = { -140000.0f, -800.0f, 17500.0f, 1.0f };

static int EvProgTrialStartSet(void) {
    float dummy[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

    Sh2sys.main_status |= 1;
    switch (ev_p_step) {
    case 0:
        ItemGet(4);
        ItemGet(5);
        ItemGet(5);
        ItemGet(5);
        ItemGet(5);
        ItemGet(11);
        ItemGet(1);
        ItemGet(1);
        ItemGet(1);
        ItemGet(1);
        ItemGet(1);
        ItemGet(15);
        ItemGet(16);
        game_flag.flag[0] |= 0x8000000;
        item.radio_switch = item.light_switch = 1;
        item.radio_volume = 4;
        ev_timer = 0.0f;
        game_flag.flag[46] |= 0x8;
        game_flag.flag[46] |= 0x10;
        game_flag.flag[46] |= 0x40;
        game_flag.flag[46] |= 0x80;
        game_flag.flag[46] |= 0x100;
        game_flag.flag[46] |= 0x200;
        game_flag.flag[46] |= 0x400;
        game_flag.flag[46] |= 0x8000;
        game_flag.flag[46] |= 0x20000;
        game_flag.flag[46] |= 0x40000;
        game_flag.flag[46] |= 0x80000;
        game_flag.flag[46] |= 0x400000;
        game_flag.flag[46] |= 0x800000;
        game_flag.flag[46] |= 0x1000000;
        game_flag.flag[46] |= 0x8000000;
        game_flag.flag[46] |= 0x10000000;
        game_flag.flag[46] |= 0x20000000;
        game_flag.flag[46] |= 0x40000000;
        game_flag.flag[47] |= 0x4000;
        game_flag.flag[47] |= 0x4000000;
        game_flag.flag[47] |= 0x800000;
        game_flag.flag[47] |= 0x40000000;
        game_flag.flag[48] |= 0x2;
        game_flag.flag[48] |= 0x4;
        game_flag.flag[48] |= 0x8;
        game_flag.flag[48] |= 0x20;
        game_flag.flag[48] |= 0x80;
        game_flag.flag[48] |= 0x100;
        game_flag.flag[48] |= 0x1000;
        game_flag.flag[48] |= 0x4000;
        game_flag.flag[48] |= 0x400000;
        game_flag.flag[48] |= 0x800000;
        game_flag.flag[48] |= 0x4000000;
        game_flag.flag[48] |= 0x8000000;
        game_flag.flag[48] |= 0x20000000;
        game_flag.flag[48] |= 0x80000000;
        game_flag.flag[49] |= 0x1;
        game_flag.flag[49] |= 0x2;
        game_flag.flag[49] |= 0x4;
        game_flag.flag[49] |= 0x8;
        game_flag.flag[49] |= 0x10;
        game_flag.flag[49] |= 0x20;
        game_flag.flag[49] |= 0x100;
        game_flag.flag[49] |= 0x200;
        game_flag.flag[49] |= 0x1000;
        game_flag.flag[49] |= 0x20000;
        game_flag.flag[49] |= 0x100000;
        s11_ton = (shRandI() & 3) + 4;
        EV_STEP(0xD);
        break;
    case 2:
        if (EvSubFileLoadAndFadeOut(0, data_pic_etc_cesa_tex, NULL)) {
            EV_STEP(7);
        }
        break;
    case 7:
        ev_timer += shGetDT();
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureEnd();
        if (!shPadTrigger(0, key_config.enter) && !shPadTrigger(0, 4) && ev_timer < 3.0f) {
            break;
        }
    case 0xD:
        return 1;
    }
    return 0;
}

static int EvProgGetHospitalMap(void) {
    return 0;
}

static int EvProgGetNeedle(void) {
    return EvSubItemGetAndAnim(0x33, 0x19);
}

static int EvProgGuruguruNumber(void) {
    static short tex[9][4][4] = {
        { 0, 0, 32, 64, 160, 0, 192, 64, 320, 0, 352, 64, 0, 128, 16, 176 },
        { 32, 0, 64, 64, 192, 0, 224, 48, 352, 0, 400, 64, 16, 128, 48, 176 },
        { 64, 0, 96, 64, 224, 0, 256, 48, 400, 0, 432, 64, 48, 128, 80, 192 },
        { 96, 0, 128, 64, 256, 0, 288, 48, 432, 0, 464, 80, 80, 128, 112, 192 },
        { 128, 0, 160, 64, 288, 0, 320, 64, 464, 0, 496, 64, 112, 128, 144, 192 },
        { 0, 64, 32, 128, 160, 64, 192, 128, 320, 64, 368, 112, 144, 128, 176, 192 },
        { 32, 64, 64, 128, 192, 48, 240, 112, 368, 64, 400, 112, 176, 128, 208, 224 },
        { 64, 64, 96, 128, 240, 48, 272, 112, 400, 80, 448, 144, 208, 128, 240, 192 },
        { 96, 64, 128, 128, 272, 64, 304, 128, 464, 64, 496, 128, 240, 128, 272, 192 },
    };
    static short pos[9][4][2] = {
        { -768, -1120, -384, -832, 64, -1152, 656, -608 },
        { -896, -1072, -384, -736, 0, -1168, 577, -736 },
        { -896, -1056, -432, -672, 80, -1088, 560, -784 },
        { -880, -1088, -368, -672, 112, -1200, 560, -736 },
        { -800, -1168, -480, -944, 0, -1072, 576, -784 },
        { -800, -1184, -480, -880, 0, -864, 544, -624 },
        { -768, -1152, -464, -928, 128, -928, 609, -800 },
        { -864, -1120, -416, -944, -16, -1216, 560, -879 },
        { -816, -1216, -416, -976, 80, -1136, 528, -720 },
    };
    struct PicDraw_Data pic;
    int no;
    int i;

    switch (ev_p_step) {
    case 0:
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        if (EV_FLAG(0xC0)) {
            EV_STEP(2);
        } else {
            EV_STEP(0xA);
        }
        break;
    case 0xA:
        if (!EvSubMessage(3)) {
            break;
        }
        if (ev_cancel) {
            ev_prog_flag_set = 0;
            EV_STEP(0xD);
        } else {
            EV_STEP(2);
        }
        break;
    case 2:
        if (EvSubFileLoadAndFadeOut(0, data_pic_hsp_p_boxnumber_tex, data_pic_hsp_p_boxnumber_2_tex)) {
            EV_STEP(8);
        }
        break;
    case 8:
    case 7:
    case 4:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        PictureLoadImage((struct sh2gfw_AREA_HEAD *)layer_adr, 2, -1, -1);
        shQzero(&pic, sizeof(pic));
        pic.ap = (struct sh2gfw_AREA_HEAD *)layer_adr;
        pic.tex = -1;
        pic.clut = -1;
        pic.status |= 1;
        pic.otp = 3;
        PIC_ALPHA(pic, 0x80);
        for (i = 0; i < 4; i++) {
            no = game_flag.guruguru[i];
            pic.x0 = pos[no][i][0];
            pic.y0 = pos[no][i][1];
            pic.x1 = pos[no][i][0] + (tex[no][i][2] - tex[no][i][0]) * 16;
            pic.y1 = pos[no][i][1] + (tex[no][i][3] - tex[no][i][1]) * 16;
            pic.status |= 2;
            pic.us0 = tex[no][i][0] * 16;
            pic.vt0 = tex[no][i][1] * 16;
            pic.us1 = (tex[no][i][2] - 1) * 16;
            pic.vt1 = (tex[no][i][3] - 1) * 16;
            pic.status |= 4;
            PictureDraw(&pic);
        }
        if (ev_p_step == 7) {
            EvSubPictureFilter();
        }
        EvSubPictureEnd();
        if (ev_p_step == 8) {
            if (shPadTrigger(0, key_config.enter)) {
                EV_STEP(7);
            }
        } else if (ev_p_step == 7) {
            if (EvSubMessage(4)) {
                EV_STEP(4);
            }
        } else if (ScreenEffectFadeCheck()) {
            EV_STEP(0xD);
        }
        break;
    case 0xD:
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        return 1;
    }
    return 0;
}

static int EvProgLouiseTakecare(void) {
    switch (ev_p_step) {
    case 0:
        if (EV_FLAG(0xBF)) {
            EV_STEP(0xC);
        } else {
            EV_STEP(0xB);
        }
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        break;
    case 0xB:
        if (!EvSubMessage(3)) {
            break;
        }
        if (ev_cancel) {
            ev_prog_flag_set = 0;
            EV_STEP(0xD);
        } else {
            EV_STEP(0xC);
        }
        break;
    case 0xC:
        if (EvSubMessage(9)) {
            EV_STEP(0xD);
        }
        break;
    case 0xD:
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        return 1;
    }
    return 0;
}

static int EvProgBoxWithKey(void) {
    switch (ev_p_step) {
    case 0:
        if (EV_FLAG(0xC5)) {
            cyl_alp = 0.0f;
        } else {
            cyl_alp = 1.0f;
        }
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        ev_cursor_y = 0.0f;
        ev_cursor_x = 0.0f;
        EV_STEP(2);
        break;
    case 2:
        if (!EvSubFileLoadAndFadeOut(0, data_pic_hsp_p_box_tex, data_pic_hsp_pboxkey01_tex)) {
            break;
        }
        if (EV_FLAG(0xC2)) {
            EV_STEP(9);
            SeCall(0x4DBA, 1.0f, 0);
        } else {
            EV_STEP(0xA);
        }
        break;
    case 9:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureFilter();
        EvProgBoxWithKeyLayer();
        EvSubPictureEnd();
        if (!EvSubItemUse0(0x20, 0x17, 0, 0, NULL, 0)) {
            break;
        }
        game_flag.flag[6] |= 2;
        if (EV_FLAG(0xC5)) {
            EV_STEP(0xC);
        } else {
            EV_STEP(0xA);
        }
        break;
    case 0xA:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureFilter();
        EvProgBoxWithKeyLayer();
        EvSubPictureEnd();
        if (EvSubMessage(10)) {
            ev_cursor_x = 0.0f;
            ev_cursor_y = 0.0f;
            EV_STEP(7);
        }
        break;
    case 7:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvProgBoxWithKeyLayer();
        EvProgBoxWithKeyCursor();
        EvSubPictureCursor(0);
        EvSubPictureEnd();
        if (game_flag.guruguru[0] == game_flag.cylinder[0] && game_flag.guruguru[1] == game_flag.cylinder[1] &&
            game_flag.guruguru[2] == game_flag.cylinder[2] && game_flag.guruguru[3] == game_flag.cylinder[3]) {
            game_flag.flag[6] |= 0x20;
            SeCall(0x4A46, 1.0f, 0);
            EV_STEP(0xE);
        } else if (shPadTrigger(0, key_config.cancel)) {
            EV_STEP(4);
        }
        break;
    case 0xE:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvProgBoxWithKeyLayer();
        EvSubPictureCursor(0);
        EvSubPictureEnd();
        cyl_alp -= 0.5f * shGetDT();
        if (cyl_alp <= 0.0f) {
            if (EV_FLAG(0xC1)) {
                EV_STEP(0xC);
            } else {
                EV_STEP(0x10);
            }
        }
        break;
    case 0x10:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureEnd();
        if (shPadTrigger(0, key_config.enter) || shPadTrigger(0, key_config.cancel)) {
            EV_STEP(4);
        }
        break;
    case 0xC:
        if (EvSubFileLoadAndFadeOut(0, data_pic_hsp_p_hair_tex, data_pic_hsp_p_hair_hair_tex)) {
            ev_timer = 0.0f;
            EV_STEP(0xF);
        }
        break;
    case 0xF:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvProgBoxWithKeyOpen(0x80);
        EvSubPictureEnd();
        if (shPadTrigger(0, key_config.enter) || shPadTrigger(0, key_config.cancel)) {
            EV_STEP(0x11);
        }
        break;
    case 0x11:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvProgBoxWithKeyOpen(0x80);
        EvSubPictureFilter();
        EvSubPictureEnd();
        if (EvSubMessage(7)) {
            EV_STEP(8);
        }
        break;
    case 8:
        ev_timer += shGetDT();
        if (ev_timer > 2.0f) {
            ev_timer = 2.0f;
            ev_p_step = 0x12;
            ev_s_step = 0;
        }
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvProgBoxWithKeyOpen(0x80 - ftoi(128.0f * ev_timer / 2.0f));
        EvSubPictureEnd();
        break;
    case 0x12:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureFilter();
        EvSubPictureEnd();
        if (EvSubItemGet(0x32, 0x18)) {
            EV_STEP(4);
            game_flag.flag[5] |= 0x800;
        }
        break;
    case 4:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        if (!EV_FLAG(0xAB) && !EV_FLAG(0xC5)) {
            EvProgBoxWithKeyLayer();
        }
        EvSubPictureEnd();
        if (ScreenEffectFadeCheck()) {
            EV_STEP(0xD);
        }
        break;
    case 0xD:
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        game_flag.flag[6] &= ~4;
        return 1;
    }
    return 0;
}

static int EvProgBoxWithKeyLayer(void) {
    static unsigned char cyl_tex[10][4] = {
        { 0x00, 0x00, 0x50, 0xA0 },
        { 0x60, 0x00, 0xA0, 0x30 },
        { 0x60, 0x30, 0xA0, 0x60 },
        { 0x60, 0x60, 0xA0, 0x90 },
        { 0x60, 0x90, 0xA0, 0xC0 },
        { 0xA0, 0x00, 0xE0, 0x30 },
        { 0xA0, 0x30, 0xE0, 0x60 },
        { 0xA0, 0x60, 0xE0, 0x90 },
        { 0xA0, 0x90, 0xE0, 0xC0 },
        { 0x00, 0xA0, 0x40, 0xD0 },
    };
    static int cyl_pos[10][4][2] = {
        { -2618, -107, 0, 0, 0, 0, 0, 0 },
        { -2383, 329, -2452, 672, -2509, 994, -2575, 1337 },
        { -2364, 337, -2440, 670, -2501, 998, -2564, 1342 },
        { -2389, 335, -2460, 670, -2514, 1011, -2578, 1334 },
        { -2382, 330, -2461, 667, -2513, 995, -2577, 1328 },
        { -2358, 330, -2433, 661, -2492, 984, -2561, 1328 },
        { -2382, 310, -2451, 641, -2512, 974, -2580, 1316 },
        { -2373, 299, -2443, 630, -2506, 957, -2576, 1298 },
        { -2377, 359, -2442, 688, -2508, 1022, -2577, 1360 },
        { -2377, 303, -2442, 626, -2512, 960, -2577, 1296 },
    };
    struct PicDraw_Data pic;
    int no;
    int ring;
    int otp;
    int i;

    PictureLoadImage((struct sh2gfw_AREA_HEAD *)layer_adr, 2, -1, -1);
    shQzero(&pic, sizeof(pic));
    pic.ap = (struct sh2gfw_AREA_HEAD *)layer_adr;
    pic.tex = -1;
    pic.clut = -1;
    pic.status |= 1;
    if (cyl_tex != NULL) {
        for (i = -1; i < 4; i++) {
            if (i == -1) {
                no = 0;
                ring = 0;
                otp = 3;
            } else {
                no = game_flag.cylinder[i] + 1;
                ring = i;
                otp = 4;
            }
            pic.x0 = cyl_pos[no][ring][0];
            pic.y0 = cyl_pos[no][ring][1];
            pic.x1 = cyl_pos[no][ring][0] + (cyl_tex[no][2] - cyl_tex[no][0]) * 16;
            pic.y1 = cyl_pos[no][ring][1] + (cyl_tex[no][3] - cyl_tex[no][1]) * 16;
            pic.status |= 2;
            pic.us0 = cyl_tex[no][0] * 16;
            pic.vt0 = cyl_tex[no][1] * 16;
            pic.us1 = (cyl_tex[no][2] - 1) * 16;
            pic.vt1 = (cyl_tex[no][3] - 1) * 16;
            pic.status |= 4;
            pic.otp = otp;
            PIC_ALPHA(pic, ftoi(128.0f * cyl_alp));
            PictureDraw(&pic);
        }
    }
    return 1;
}

#define CYL_EDGE(ax, ay, bx, by) ((ev_cursor_x - (ax)) * ((by) - (ay)) - (ev_cursor_y - (ay)) * ((bx) - (ax)))

static int EvProgBoxWithKeyCursor(void) {
    static unsigned char cyl_touch_c[5][2][2] = {
        { { 0x88, 0x1C }, { 0x5C, 0x2C } },
        { { 0x8C, 0x30 }, { 0x60, 0x40 } },
        { { 0x90, 0x44 }, { 0x64, 0x54 } },
        { { 0x94, 0x58 }, { 0x68, 0x68 } },
        { { 0x98, 0x6C }, { 0x6C, 0x7C } },
    };
    float cyl_touch[5][3][2];
    int i;

    for (i = 0; i < 5; i++) {
        cyl_touch[i][0][0] = itof(-cyl_touch_c[i][0][0]);
        cyl_touch[i][1][0] = itof(-(cyl_touch_c[i][0][0] + cyl_touch_c[i][1][0]) >> 1);
        cyl_touch[i][2][0] = itof(-cyl_touch_c[i][1][0]);
        cyl_touch[i][0][1] = itof(cyl_touch_c[i][0][1]);
        cyl_touch[i][1][1] = itof((cyl_touch_c[i][0][1] + cyl_touch_c[i][1][1]) >> 1);
        cyl_touch[i][2][1] = itof(cyl_touch_c[i][1][1]);
    }
    if (!shPadTrigger(0, key_config.enter)) {
        return 1;
    }
    if (!EV_FLAG(0xC5)) {
        for (i = 0; i < 4; i++) {
            if (CYL_EDGE(cyl_touch[i][0][0], cyl_touch[i][0][1], cyl_touch[i][1][0], cyl_touch[i][1][1]) <= 0.0f &&
                CYL_EDGE(cyl_touch[i][1][0], cyl_touch[i][1][1], cyl_touch[i + 1][1][0], cyl_touch[i + 1][1][1]) <= 0.0f &&
                CYL_EDGE(cyl_touch[i + 1][1][0], cyl_touch[i + 1][1][1], cyl_touch[i + 1][0][0], cyl_touch[i + 1][0][1]) <= 0.0f &&
                CYL_EDGE(cyl_touch[i + 1][0][0], cyl_touch[i + 1][0][1], cyl_touch[i][0][0], cyl_touch[i][0][1]) <= 0.0f) {
                game_flag.cylinder[i]++;
                if (game_flag.cylinder[i] > 8) {
                    game_flag.cylinder[i] = 0;
                }
                SeCall(0x4A45, 1.0f, 0);
            }
            if (CYL_EDGE(cyl_touch[i][1][0], cyl_touch[i][1][1], cyl_touch[i][2][0], cyl_touch[i][2][1]) <= 0.0f &&
                CYL_EDGE(cyl_touch[i][2][0], cyl_touch[i][2][1], cyl_touch[i + 1][2][0], cyl_touch[i + 1][2][1]) <= 0.0f &&
                CYL_EDGE(cyl_touch[i + 1][2][0], cyl_touch[i + 1][2][1], cyl_touch[i + 1][1][0], cyl_touch[i + 1][1][1]) <= 0.0f &&
                CYL_EDGE(cyl_touch[i + 1][1][0], cyl_touch[i + 1][1][1], cyl_touch[i][1][0], cyl_touch[i][1][1]) <= 0.0f) {
                game_flag.cylinder[i]--;
                if (game_flag.cylinder[i] < 0) {
                    game_flag.cylinder[i] = 8;
                }
                SeCall(0x4A45, 1.0f, 0);
            }
        }
    }
    return 1;
}

static int EvProgBoxWithKeyOpen(int alp) {
    struct PicDraw_Data pic;

    PictureLoadImage((struct sh2gfw_AREA_HEAD *)layer_adr, 2, -1, -1);
    shQzero(&pic, sizeof(pic));
    pic.ap = (struct sh2gfw_AREA_HEAD *)layer_adr;
    pic.tex = -1;
    pic.clut = -1;
    pic.status |= 1;
    pic.otp = 3;
    PIC_ALPHA(pic, alp);
    pic.x0 = -0x470;
    pic.y0 = -0x340;
    pic.x1 = 0xB90;
    pic.y1 = 0xC0;
    pic.status |= 2;
    PictureDraw(&pic);
    return 1;
}

static int EvProgEmptyBox(void) {
    switch (ev_p_step) {
    case 0:
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        EV_STEP(2);
        break;
    case 2:
        if (EvSubFileLoadAndFadeOut(0, data_pic_hsp_p_hair_tex, NULL)) {
            EV_STEP(0xF);
        }
        break;
    case 0xF:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureEnd();
        if (EvSubMessage(8)) {
            EV_STEP(4);
        }
        break;
    case 4:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureEnd();
        if (ScreenEffectFadeCheck()) {
            EV_STEP(0xD);
        }
        break;
    case 0xD:
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        return 1;
    }
    return 0;
}

static int EvProgOnlyNeedle(void) {
    switch (ev_p_step) {
    case 0:
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        PlayerEventAnimeSet(0x4E21);
        EV_STEP(0x10);
        break;
    case 0x10:
        if (PlayerEventAnimeSuccessFrame()) {
            shCharacterAnimePause(sh2jms.player);
            EV_STEP(0xF);
        }
        break;
    case 0xF:
        if (EvSubMessage(0xF)) {
            shCharacterAnimeRestart(sh2jms.player);
            EV_STEP(1);
        }
        break;
    case 1:
        if (shCharacterAnimeIsEnd(sh2jms.player)) {
            EV_STEP(0xD);
        }
        break;
    case 0xD:
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        return 1;
    }
    return 0;
}

static int EvProgOnlyHair(void) {
    switch (ev_p_step) {
    case 0:
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        PlayerEventAnimeSet(0x4E21);
        EV_STEP(0x10);
        break;
    case 0x10:
        if (PlayerEventAnimeSuccessFrame()) {
            shCharacterAnimePause(sh2jms.player);
            EV_STEP(0xF);
        }
        break;
    case 0xF:
        if (EvSubMessage(0xE)) {
            shCharacterAnimeRestart(sh2jms.player);
            EV_STEP(1);
        }
        break;
    case 1:
        if (shCharacterAnimeIsEnd(sh2jms.player)) {
            EV_STEP(0xD);
        }
        break;
    case 0xD:
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        return 1;
    }
    return 0;
}

static int EvProgInShowerDrain(void) {
    struct PicDraw_Data pic;
    int msg;

    switch (ev_p_step) {
    case 0:
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        PlayerEventAnimeSet(0x4E21);
        EV_STEP(0x10);
        break;
    case 0x10:
        if (PlayerEventAnimeSuccessFrame()) {
            shCharacterAnimePause(sh2jms.player);
            EV_STEP(2);
        }
        break;
    case 2:
        if (EvSubFileLoadAndFadeOut(0, data_pic_hsp_p_drainage_tex, data_pic_hsp_p_drainage_key_tex)) {
            EV_STEP(0xF);
        }
        break;
    case 0xF:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        if (!EV_FLAG(0xAC)) {
            PictureLoadImage((struct sh2gfw_AREA_HEAD *)layer_adr, 2, -1, -1);
            shQzero(&pic, sizeof(pic));
            pic.ap = (struct sh2gfw_AREA_HEAD *)layer_adr;
            pic.tex = -1;
            pic.clut = -1;
            pic.status |= 1;
            pic.otp = 3;
            PIC_ALPHA(pic, 0x80);
            pic.x0 = -0x2C0;
            pic.y0 = 0x1F0;
            pic.x1 = 0x140;
            pic.y1 = 0x5F0;
            pic.status |= 2;
            PictureDraw(&pic);
            msg = 0xB;
        } else {
            msg = 0xC;
        }
        EvSubPictureEnd();
        if (EvSubMessage(msg)) {
            shCharacterAnimeRestart(sh2jms.player);
            EV_STEP(1);
        }
        break;
    case 1:
        if (shCharacterAnimeIsEnd(sh2jms.player)) {
            EV_STEP(0xD);
        }
        break;
    case 0xD:
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        return 1;
    }
    return 0;
}

static int EvProgFishKey(void) {
    static short fish_anim[3] = { 1073, 10126, 10127 };
    static struct DramaDemo_PlayInfo tsuri = { 0, dds_tsuri, fish_anim, NULL, 0, 0, NULL, 0, 0.0f, 0.0f, 0.0f };
    static struct CharaData_DemoList chara_data[4] = {
        { 0x103, data_chr_jms_hhh_jms_mdl, data_demo_tsuri_hhh_jms_anm, data_chr_jms_hhh_jms_kg1, NULL },
        { 0x41D, data_chr_item_i_needle_mdl, data_demo_tsuri_i_needle_anm, NULL, NULL },
        { 0x41E, data_chr_item_i_keyelevator_mdl, data_demo_tsuri_i_keyelevator_anm, NULL, NULL },
        { 0, NULL, NULL, NULL, NULL },
    };

    switch (ev_p_step) {
    case 0:
        sh2jms.player->status &= ~0x10;
        SCNowDemoEventSwitch(sh2jms.player, 1);
        CharaAdminPlayableDisplay(0);
        CharaDataLoadDemo(chara_data, 0);
        ev_p_step = 0x16;
        ev_s_step = 0;
    case 0x16:
        if (DramaDemoMain(&tsuri)) {
            EV_STEP(0xA);
        }
        break;
    case 0xA:
        DramaDemoSkipLast(&tsuri);
        if (EvSubItemGet(0x22, 0x1A)) {
            EV_STEP(0xD);
        }
        break;
    case 0xD:
        CharaDataDeleteOne(0x103);
        CharaDataDeleteOne(0x41D);
        CharaDataDeleteOne(0x41E);
        sh2jms.player->status |= 0x10;
        CharaAdminPlayableDisplay(1);
        SCNowDemoEventSwitch(sh2jms.player, 0);
        vcReturnPreAutoCamWork(1);
        return 1;
    }
    return 0;
}

static int EvProgDoctorMemo1st(void) {
    struct PicDraw_Data pic;
    int alp;

    switch (ev_p_step) {
    case 0:
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        PlayerEventAnimeSet(0x65);
        if (!EvSubMessage(0)) {
            break;
        }
        if (ev_cancel) {
            ev_prog_flag_set = 0;
            EV_STEP(0xD);
        } else {
            EV_STEP(2);
        }
        break;
    case 2:
        if (EvSubFileLoadAndFadeOut(0, data_pic_hsp_p_doctormemo_tex, data_pic_hsp_p_doctormemo_key_tex)) {
            EV_STEP(8);
        }
        break;
    case 8:
    case 7:
    case 9:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        PictureLoadImage((struct sh2gfw_AREA_HEAD *)layer_adr, 2, -1, -1);
        shQzero(&pic, sizeof(pic));
        pic.ap = (struct sh2gfw_AREA_HEAD *)layer_adr;
        pic.tex = -1;
        pic.clut = -1;
        pic.status |= 1;
        pic.otp = 3;
        if (ev_p_step == 9) {
            alp = 0x80 - ftoi(128.0f * ev_timer / 2.0f);
            PIC_ALPHA(pic, alp);
        } else {
            PIC_ALPHA(pic, 0x80);
        }
        pic.x0 = 0x6C0;
        pic.y0 = 0x500;
        pic.x1 = 0xEC0;
        pic.y1 = 0xD00;
        pic.status |= 2;
        PictureDraw(&pic);
        if (ev_p_step == 7) {
            EvSubPictureFilter();
        }
        EvSubPictureEnd();
        if (ev_p_step == 8) {
            if (shPadTrigger(0, key_config.enter)) {
                EV_STEP(7);
            }
        } else if (ev_p_step == 7) {
            if (EvSubMessage(1)) {
                EV_STEP(9);
                ev_timer = 0.0f;
            }
        } else {
            ev_timer += shGetDT();
            if (ev_timer > 2.0f) {
                EV_STEP(0xA);
            }
        }
        break;
    case 0xA:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureEnd();
        if (EvSubItemGet(0x20, 0x16)) {
            EV_STEP(4);
        }
        break;
    case 4:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureEnd();
        if (ScreenEffectFadeCheck()) {
            EV_STEP(0xD);
        }
        break;
    case 0xD:
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        return 1;
    }
    return 0;
}

static int EvProgDoctorMemo2nd(void) {
    switch (ev_p_step) {
    case 0:
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        PlayerEventAnimeSet(0x65);
        EV_STEP(2);
        break;
    case 2:
        if (EvSubFileLoadAndFadeOut(0, data_pic_hsp_p_doctormemo_tex, NULL)) {
            EV_STEP(8);
        }
        break;
    case 8:
    case 7:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        if (ev_p_step == 7) {
            EvSubPictureFilter();
        }
        EvSubPictureEnd();
        if (ev_p_step == 8) {
            if (shPadTrigger(0, key_config.enter)) {
                EV_STEP(7);
            }
        } else if (ev_p_step == 7) {
            if (EvSubMessage(1)) {
                EV_STEP(4);
            }
        }
        break;
    case 4:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureEnd();
        if (ScreenEffectFadeCheck()) {
            EV_STEP(0xD);
        }
        break;
    case 0xD:
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        return 1;
    }
    return 0;
}

static int EvProgUseElevatorKey(void) {
    return EvSubItemUse0(0x22, 0x1B, 0, 0, NULL, 0);
}

static int EvProgElevatorButton(void) {
    switch (ev_p_step) {
    case 0:
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        PlayerEventAnimeSet(0x65);
        ev_timer = 0.0f;
        ev_cursor_y = 0.0f;
        ev_cursor_x = 0.0f;
        EV_STEP(2);
        break;
    case 2:
        if (EvSubFileLoadAndFadeOut(0, data_pic_hsp_p_h_elevator_tex, data_pic_hsp_p_h_elevator_botan_tex)) {
            EV_STEP(8);
        }
        break;
    case 8:
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvSubPictureCursor(0);
        EvSubPictureEnd();
        EvProgElevatorButtonCheck();
        if (EV_FLAG(23) || EV_FLAG(22) || EV_FLAG(21)) {
            EV_STEP(7);
        } else if (shPadTrigger(0, key_config.cancel)) {
            EV_STEP(0xD);
        }
        break;
    case 7:
        ev_timer += 1.5f * shGetDT();
        if (ev_timer > 1.0f) {
            ev_timer = 1.0f;
        }
        EvSubPictureStart();
        EvSubPictureDisplayOnly();
        EvProgElevatorButtonLight();
        EvSubPictureEnd();
        if (ev_timer == 1.0f) {
            ev_timer = 0.0f;
            if ((EV_FLAG(19) && EV_FLAG(23)) || (EV_FLAG(18) && EV_FLAG(22))) {
                EV_STEP(0xD);
            } else {
                EV_STEP(0x10);
                ev_timer = 0.0f;
                SeCall(0x4A43, 1.0f, 0);
                PlayerEventAnimeSet(0x65);
            }
        }
        break;
    case 0x10:
        ev_timer += shGetDT();
        if (ev_timer > 3.0f) {
            SeStop(0x4A43);
            SeCall(0x4A44, 1.0f, 0);
            EV_STEP(0xD);
        }
        break;
    case 0xD:
        SCNowPlayableEventSwitch(sh2jms.player, 0);
        game_flag.flag[0] &= ~0x80000;
        game_flag.flag[0] &= ~0x40000;
        return 1;
    }
    return 0;
}

/** On enter, finds the elevator button under the cursor and lights it (sets its flags). Returns 1
 *  when a button was pressed, else 0. */
static int EvProgElevatorButtonCheck(void) {
    static short btn_ctr[5][2] = {
        { 57, -128 },
        { 57, -75 },
        { 57, -22 },
        { 29, 128 },
        { 85, 128 },
    };
    float px;
    float py;
    int i;

    if (!shPadTrigger(0, key_config.enter)) {
        return 0;
    }
    for (i = 0; i < 5; i++) {
        px = 1.25f * (ev_cursor_x - itof(btn_ctr[i][0]));
        py = ev_cursor_y - itof(btn_ctr[i][1]);
        px = sqr(px);
        py = sqr(py);
        if (px + py < 400.0f) {
            break;
        }
    }
    if (i == 5) {
        return 0;
    }
    SeCall(0x4A49, 1.0f, 0);
    switch (i) {
    case 0:
        if (!EV_FLAG(19)) {
            game_flag.flag[0] |= 0x800000;
        }
        break;
    case 1:
        if (!EV_FLAG(18)) {
            game_flag.flag[0] |= 0x400000;
        }
        break;
    case 2:
        game_flag.flag[0] |= 0x200000;
        break;
    case 3:
        if (EV_FLAG(19)) {
            game_flag.flag[0] |= 0x800000;
        }
        if (EV_FLAG(18)) {
            game_flag.flag[0] |= 0x400000;
        }
        break;
    }
    return 1;
}

static int EvProgElevatorButtonLight(void) {
    static int btn_tex[4][4] = {
        { 0, 160, 64, 240 },
        { 0, 80, 64, 160 },
        { 0, 0, 64, 80 },
        { 64, 0, 128, 80 },
    };
    static int btn_pos[4][2] = {
        { 416, -2704 },
        { 400, -1840 },
        { 400, -976 },
        { -48, 1456 },
    };
    struct PicDraw_Data pic;
    int no;
    int alp;

    if (EV_FLAG(23)) {
        if (EV_FLAG(19)) {
            no = 3;
        } else {
            no = 0;
        }
    } else if (EV_FLAG(22)) {
        if (EV_FLAG(18)) {
            no = 3;
        } else {
            no = 1;
        }
    } else {
        no = 2;
    }
    if (ev_timer < 0.5f) {
        alp = ftoi(256.0f * ev_timer);
    } else {
        alp = ftoi(256.0f - 256.0f * ev_timer);
    }
    PictureLoadImage((struct sh2gfw_AREA_HEAD *)layer_adr, 2, -1, -1);
    shQzero(&pic, sizeof(pic));
    pic.ap = (struct sh2gfw_AREA_HEAD *)layer_adr;
    pic.tex = -1;
    pic.clut = -1;
    pic.status |= 1;
    pic.x0 = btn_pos[no][0];
    pic.y0 = btn_pos[no][1];
    pic.x1 = btn_pos[no][0] + (btn_tex[no][2] - btn_tex[no][0]) * 16;
    pic.y1 = btn_pos[no][1] + (btn_tex[no][3] - btn_tex[no][1]) * 16;
    pic.status |= 2;
    pic.us0 = btn_tex[no][0] * 16;
    pic.vt0 = btn_tex[no][1] * 16;
    pic.us1 = (btn_tex[no][2] - 1) * 16;
    pic.vt1 = (btn_tex[no][3] - 1) * 16;
    pic.status |= 4;
    pic.otp = 3;
    PIC_ALPHA(pic, alp);
    PictureDraw(&pic);
    return 1;
}

static int EvProgTrialEnd(void) {
    struct PicDraw_Data pic;

    switch (ev_p_step) {
    case 0:
        SCNowPlayableEventSwitch(sh2jms.player, 1);
        PlayerEventAnimeSet(0x65);
        ev_timer = 0.0f;
        EV_STEP(0x10);
        break;
    case 0x10:
        ev_timer += shGetDT();
        if (ev_timer > 4.0f) {
            ev_timer = 4.0f;
            ev_p_step = 0xD;
            ev_s_step = 0;
        }
        shQzero(&pic, sizeof(pic));
        pic.r = 0;
        pic.g = 0;
        pic.b = 0;
        pic.status |= 0x10;
        PIC_ALPHA(pic, ftoi(32.0f * ev_timer));
        PictureDraw(&pic);
        EvSubPictureEnd();
        break;
    case 0xD:
        shQzero(&pic, sizeof(pic));
        pic.r = 0;
        pic.g = 0;
        pic.b = 0;
        pic.status |= 0x10;
        PIC_ALPHA(pic, 0x80);
        PictureDraw(&pic);
        Sh2sys.step[2] = 0xB;
        Sh2sys.step[3] = 0;
        Sh2sys.step[4] = 0;
        Sh2sys.step[5] = 0;
        Sh2sys.step[6] = 0;
        Sh2sys.step[7] = 0;
        EvSubPictureEnd();
        return 1;
    }
    return 0;
}

static void EvRoomInit(void) {
    float pos[4];
    int room;

    room = RoomNameJms();
    if (room == 0x3A) {
        flyInit(fly_pos);
        SeCallPos(0, 0.1f, fly_pos, 2);
    } else {
        SeStop(0);
    }
    if (room == 0x2C) {
        office_3d_timer = 0.0f;
        office_3d_rot = 0.0f;
        if (!Se3dPlayCheck(0)) {
            _shUnitVector(pos);
            SeCallPos(0, 1.0f, pos, 2);
        }
    } else if (room != 0x2D) {
        SeStop(0);
    }
    if (room == 0x42) {
        s11_timer = 3.0f + 2.0f * shRandF();
    }
}

static void EvAllTimeFunc(void) {
    float pos[4];
    float spd;

    switch (RoomNameJms()) {
    case 0x3A:
        flyMove();
        flyGetPos(pos);
        Se3dControl(0, 0.1f, pos);
        break;
    case 0x44:
    case 0x45:
        break;
    case 0x3C:
        if (!EV_FLAG(4)) {
            if (sh2jms.player->pos.x < -55600.0f && sh2jms.player->spd < 1.8f) {
                game_flag.flag[0] |= 0x10;
                pos[0] = sh2jms.player->pos.x - 2000.0f;
                pos[1] = 0.0f;
                pos[2] = sh2jms.player->pos.z;
                SeCallPos(0, 1.0f, pos, 2);
            }
        }
        break;
    case 0x33:
    case 0x2D:
        if (!Se3dPlayCheck(0)) {
            break;
        }
    case 0x2C:
        spd = 2.25f * (0.2f + (shSinF(office_3d_timer / 1.65f) / 3.1415927f) * (shSinF(office_3d_timer / 2.32f) / 3.1415927f));
        office_3d_rot += spd * shGetDT();
        pos[0] = vcWork.cam_pos[0] + 2000.0f * shSinF(office_3d_rot);
        pos[1] = vcWork.cam_pos[1];
        pos[2] = vcWork.cam_pos[2] + 2000.0f * -shCosF(office_3d_rot);
        Se3dControl(0, 1.0f, pos);
        office_3d_timer += shGetDT();
        break;
    case 0x42:
        if (s11_ton) {
            s11_timer -= shGetDT();
            if (s11_timer < 0.0f) {
                pos[0] = 99000.0f + 2000.0f * shRandF();
                pos[1] = -1.5f;
                pos[2] = 17500.0f + 2500.0f * shRandF();
                SeCallPos(0, 1.0f, pos, 2);
                s11_timer = 1.0f + 3.0f * shRandF();
                s11_ton--;
            }
        }
        break;
    }
}
