/* Second compiler test: medium functions with float math, loops, a switch, and a local array. */

int printf(const char *fmt, ...);

struct shBattleInfo {
    unsigned char pad0[0x28];
    float damage;           /* 0x28 */
    unsigned char pad2C[0x54 - 0x2C];
    float hp;               /* 0x54 */
    float hp_max;           /* 0x58 */
    float hp_rate;          /* 0x5C */
    unsigned char pad60[0x80 - 0x60];
};

struct SubCharacter {
    unsigned char pad0[0x110];
    struct shBattleInfo battle; /* 0x110 */
    unsigned char pad190[0x1F0 - 0x190];
};

struct EnLOCAL_DATA {
    unsigned char pad0[0x1C];
    struct SubCharacter *scp; /* 0x1C */
    unsigned char pad20[0x40 - 0x20];
    float endurance;          /* 0x40 */
    unsigned char pad44[0xE0 - 0x44];
};

/* en_common.c */
float enReduceHP(struct EnLOCAL_DATA *dp) {
    struct shBattleInfo *bi = &dp->scp->battle;

    bi->hp -= bi->damage;
    if (bi->hp < 0.0f) {
        bi->hp = 0.0f;
    }
    dp->endurance -= bi->damage;
    if (dp->endurance < 0.0f) {
        dp->endurance = 0.0f;
    }
    bi->damage = 0.0f;
    bi->hp_rate = bi->hp / bi->hp_max * 100.0f;
    return dp->endurance;
}

/* utilstr.c */
int UtilStrConvertCdPath(char *path) {
    char ch;
    int len = 0;

    while ((ch = *path) != '\0') {
        if (ch >= 'a' && ch <= 'z') {
            *path = ch - ('a' - 'A');
        } else if (ch == '/') {
            *path = '\\';
        }
        path++;
        len++;
    }
    return len - 1;
}

/* title.c */
#define assert(x) if (!(x)) { printf("title.c:1452> assert:(%s)\n", #x); while (1) {} }

unsigned char titleGetBattleLevelFromCursor(int cur) {
    unsigned char ret;

    switch (cur) {
    case 1:
        ret = 3;
        break;
    case 2:
        ret = 2;
        break;
    case 3:
        ret = 1;
        break;
    default:
        assert(0);
    }
    return ret;
}

/* en_mkn.c */
int enGetMode(void);

float enMKNGetRotSpeed(void) {
    float rot_rate[5] = { 0.5f, 0.8f, 1.0f, 1.2f, 1.5f };

    return rot_rate[enGetMode()] * 0.05235988f;
}
