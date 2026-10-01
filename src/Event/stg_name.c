/*
 * stg_name.c: map positions to room names and to BG block numbers.
 *
 * A world position becomes a "tpwoin" cell index: each axis is cut into
 * 20000-unit cells, offset by 16, so tpwoin = (x + 16) * 32 + (z + 16).
 * Room tables work on 2x2 groups of cells (TPWOIN_ROOM clears bit 0 of both
 * axes).
 */
#include "sh2.h"
#include "asm_helpers.h"

#define Glb_crd_null 0
#define Glb_crd_num 17

/* the tpwoin of the even (lower-left) cell of the 2x2 group */
#define TPWOIN_ROOM(t) ((t) - (((t) & 0x20) ? 0x20 : 0) - (((t) & 1) ? 1 : 0))

struct BlockTable {
    short xz;
    short block;
};

/* room number -> BG block numbers of its 4 corners */
static unsigned char room_to_block[211][4] = {
    { 0, 0, 0, 0 }, { 1, 2, 3, 4 }, { 1, 2, 3, 4 }, { 0, 0, 0, 0 },
    { 0, 0, 0, 0 }, { 5, 6, 7, 8 }, { 9, 10, 11, 12 }, { 0, 0, 0, 0 },
    { 0, 0, 0, 0 }, { 9, 10, 11, 12 }, { 5, 6, 7, 8 }, { 1, 2, 3, 4 },
    { 5, 6, 7, 8 }, { 1, 2, 3, 4 }, { 0, 0, 0, 0 }, { 10, 11, 12, 13 },
    { 22, 23, 24, 25 }, { 26, 27, 28, 29 }, { 6, 7, 8, 9 }, { 18, 19, 20, 21 },
    { 14, 15, 16, 17 }, { 50, 51, 52, 53 }, { 34, 35, 36, 37 }, { 30, 31, 32, 33 },
    { 42, 43, 44, 45 }, { 46, 47, 48, 49 }, { 1, 2, 3, 4 }, { 54, 55, 56, 57 },
    { 58, 59, 60, 61 }, { 38, 39, 40, 41 }, { 100, 101, 102, 103 }, { 104, 105, 106, 107 },
    { 108, 109, 110, 111 }, { 88, 89, 90, 91 }, { 76, 77, 78, 79 }, { 80, 81, 82, 83 },
    { 84, 85, 86, 87 }, { 92, 93, 94, 95 }, { 96, 97, 98, 99 }, { 64, 65, 66, 67 },
    { 72, 73, 74, 75 }, { 21, 22, 23, 24 }, { 25, 26, 27, 28 }, { 137, 138, 139, 140 },
    { 5, 6, 7, 8 }, { 13, 14, 15, 16 }, { 17, 18, 19, 20 }, { 9, 10, 11, 12 },
    { 37, 38, 39, 40 }, { 45, 46, 47, 48 }, { 145, 146, 147, 148 }, { 1, 2, 3, 4 },
    { 41, 42, 43, 44 }, { 53, 54, 55, 56 }, { 49, 50, 51, 52 }, { 65, 66, 67, 68 },
    { 89, 90, 91, 92 }, { 61, 62, 63, 64 }, { 85, 86, 87, 88 }, { 73, 74, 75, 76 },
    { 69, 70, 71, 72 }, { 109, 110, 111, 112 }, { 105, 106, 107, 108 }, { 97, 98, 99, 100 },
    { 101, 102, 103, 104 }, { 121, 122, 123, 124 }, { 125, 126, 127, 128 }, { 77, 78, 79, 80 },
    { 113, 114, 115, 116 }, { 133, 134, 135, 136 }, { 158, 159, 160, 161 }, { 162, 163, 164, 165 },
    { 174, 175, 176, 177 }, { 170, 171, 172, 173 }, { 154, 155, 156, 157 }, { 234, 235, 236, 237 },
    { 186, 187, 188, 189 }, { 150, 151, 152, 153 }, { 178, 179, 180, 181 }, { 182, 183, 184, 185 },
    { 202, 203, 204, 205 }, { 198, 199, 200, 201 }, { 194, 195, 196, 197 }, { 190, 191, 192, 193 },
    { 210, 211, 212, 213 }, { 218, 219, 220, 221 }, { 222, 223, 224, 225 }, { 214, 215, 216, 217 },
    { 206, 207, 208, 209 }, { 226, 227, 228, 229 }, { 29, 30, 31, 32 }, { 230, 231, 232, 233 },
    { 1, 2, 3, 4 }, { 9, 10, 11, 12 }, { 13, 14, 0, 0 }, { 5, 6, 7, 8 },
    { 70, 71, 0, 0 }, { 15, 16, 0, 0 }, { 55, 0, 0, 0 }, { 37, 38, 0, 0 },
    { 53, 54, 0, 0 }, { 39, 0, 0, 0 }, { 45, 46, 0, 0 }, { 19, 20, 0, 0 },
    { 29, 30, 31, 32 }, { 25, 26, 27, 28 }, { 63, 64, 65, 0 }, { 17, 18, 0, 0 },
    { 21, 22, 23, 24 }, { 40, 41, 42, 0 }, { 43, 44, 0, 0 }, { 59, 60, 0, 0 },
    { 61, 62, 0, 0 }, { 56, 0, 0, 0 }, { 33, 34, 35, 36 }, { 72, 73, 74, 75 },
    { 47, 48, 0, 0 }, { 49, 50, 0, 0 }, { 51, 52, 0, 0 }, { 57, 58, 0, 0 },
    { 66, 67, 68, 69 }, { 101, 102, 103, 104 }, { 173, 174, 175, 176 }, { 169, 170, 171, 172 },
    { 177, 178, 179, 180 }, { 161, 162, 163, 164 }, { 181, 182, 183, 184 }, { 157, 158, 159, 160 },
    { 93, 94, 95, 96 }, { 153, 154, 155, 156 }, { 89, 90, 91, 92 }, { 141, 142, 143, 144 },
    { 125, 126, 127, 128 }, { 137, 138, 139, 140 }, { 105, 106, 107, 108 }, { 121, 122, 123, 124 },
    { 145, 146, 147, 148 }, { 85, 86, 87, 88 }, { 97, 98, 99, 100 }, { 109, 110, 111, 112 },
    { 113, 114, 115, 116 }, { 149, 150, 151, 152 }, { 185, 186, 187, 188 }, { 189, 190, 191, 192 },
    { 193, 194, 195, 196 }, { 9, 10, 11, 12 }, { 25, 26, 27, 28 }, { 37, 38, 39, 40 },
    { 55, 56, 57, 58 }, { 51, 52, 53, 54 }, { 49, 50, 0, 0 }, { 45, 46, 47, 48 },
    { 41, 42, 43, 44 }, { 29, 30, 31, 32 }, { 33, 34, 35, 36 }, { 71, 72, 73, 74 },
    { 79, 80, 81, 82 }, { 75, 76, 77, 78 }, { 83, 84, 85, 86 }, { 59, 60, 61, 62 },
    { 63, 64, 65, 66 }, { 67, 68, 69, 70 }, { 91, 92, 93, 94 }, { 87, 88, 89, 90 },
    { 95, 96, 97, 98 }, { 1, 2, 3, 4 }, { 5, 6, 7, 8 }, { 21, 22, 23, 24 },
    { 13, 14, 15, 16 }, { 17, 18, 19, 20 }, { 21, 22, 23, 24 }, { 9, 10, 11, 12 },
    { 25, 26, 27, 0 }, { 29, 30, 31, 32 }, { 33, 34, 35, 36 }, { 37, 38, 39, 40 },
    { 53, 54, 55, 56 }, { 41, 42, 43, 44 }, { 45, 46, 47, 48 }, { 49, 50, 51, 52 },
    { 57, 58, 59, 60 }, { 61, 62, 63, 64 }, { 1, 2, 3, 4 }, { 5, 6, 7, 8 },
    { 13, 14, 15, 16 }, { 17, 18, 19, 20 }, { 69, 70, 71, 72 }, { 73, 74, 75, 76 },
    { 1, 2, 3, 4 }, { 5, 6, 7, 8 }, { 13, 14, 15, 16 }, { 1, 2, 3, 4 },
    { 29, 30, 31, 32 }, { 41, 42, 43, 44 }, { 21, 22, 23, 24 }, { 57, 58, 59, 60 },
    { 53, 54, 55, 56 }, { 9, 10, 11, 12 }, { 1, 2, 3, 4 }, { 69, 70, 71, 72 },
    { 17, 18, 19, 20 }, { 61, 62, 63, 64 }, { 49, 50, 51, 52 }, { 37, 38, 39, 40 },
    { 73, 74, 75, 76 }, { 33, 34, 35, 36 }, { 25, 26, 27, 28 }, { 45, 46, 47, 48 },
    { 13, 14, 15, 16 }, { 77, 78, 79, 80 }, { 65, 66, 67, 68 },
};

/* sorted (descending xz) tpwoin -> block number, per outdoor stage */
static struct BlockTable block_a[23] = {
    { 556, 3 },
    { 555, 4 },
    { 528, 10 },
    { 527, 11 },
    { 526, 12 },
    { 525, 9 },
    { 524, 8 },
    { 496, 13 },
    { 495, 14 },
    { 464, 15 },
    { 463, 16 },
    { 432, 17 },
    { 431, 18 },
    { 400, 19 },
    { 399, 20 },
    { 368, 21 },
    { 367, 22 },
    { 336, 23 },
    { 335, 24 },
    { 303, 26 },
    { 302, 27 },
    { 271, 25 },
    { 270, 28 },
};

static struct BlockTable block_b[55] = {
    { 689, 64 },
    { 684, 69 },
    { 657, 62 },
    { 656, 63 },
    { 652, 67 },
    { 651, 68 },
    { 624, 60 },
    { 623, 61 },
    { 620, 65 },
    { 619, 66 },
    { 594, 54 },
    { 593, 55 },
    { 591, 57 },
    { 590, 59 },
    { 589, 58 },
    { 561, 49 },
    { 560, 50 },
    { 559, 51 },
    { 558, 52 },
    { 557, 53 },
    { 529, 2 },
    { 528, 3 },
    { 527, 4 },
    { 526, 5 },
    { 525, 7 },
    { 497, 10 },
    { 496, 11 },
    { 495, 12 },
    { 494, 13 },
    { 493, 14 },
    { 465, 18 },
    { 464, 19 },
    { 463, 20 },
    { 462, 21 },
    { 461, 22 },
    { 434, 25 },
    { 433, 26 },
    { 432, 27 },
    { 431, 28 },
    { 430, 29 },
    { 429, 30 },
    { 428, 31 },
    { 402, 33 },
    { 401, 34 },
    { 400, 35 },
    { 399, 36 },
    { 398, 37 },
    { 397, 38 },
    { 396, 39 },
    { 368, 41 },
    { 367, 42 },
    { 365, 44 },
    { 364, 47 },
    { 336, 46 },
    { 335, 45 },
};

static struct BlockTable block_c[76] = {
    { 621, 77 },
    { 620, 76 },
    { 589, 75 },
    { 588, 74 },
    { 565, 1 },
    { 564, 2 },
    { 563, 3 },
    { 562, 4 },
    { 561, 5 },
    { 557, 6 },
    { 556, 68 },
    { 533, 7 },
    { 532, 8 },
    { 531, 9 },
    { 530, 10 },
    { 529, 11 },
    { 528, 12 },
    { 527, 24 },
    { 526, 14 },
    { 525, 15 },
    { 524, 16 },
    { 501, 17 },
    { 500, 18 },
    { 499, 19 },
    { 498, 20 },
    { 497, 21 },
    { 496, 22 },
    { 495, 23 },
    { 494, 13 },
    { 493, 25 },
    { 492, 26 },
    { 469, 27 },
    { 468, 28 },
    { 467, 29 },
    { 466, 30 },
    { 464, 31 },
    { 463, 32 },
    { 462, 33 },
    { 461, 34 },
    { 435, 35 },
    { 434, 36 },
    { 432, 37 },
    { 431, 38 },
    { 430, 39 },
    { 429, 40 },
    { 403, 41 },
    { 402, 42 },
    { 401, 43 },
    { 400, 44 },
    { 399, 45 },
    { 398, 46 },
    { 397, 47 },
    { 372, 48 },
    { 371, 49 },
    { 370, 50 },
    { 369, 51 },
    { 368, 52 },
    { 367, 53 },
    { 366, 54 },
    { 365, 55 },
    { 340, 56 },
    { 339, 57 },
    { 309, 73 },
    { 308, 60 },
    { 307, 61 },
    { 277, 62 },
    { 276, 63 },
    { 245, 64 },
    { 244, 65 },
    { 238, 99 },
    { 214, 66 },
    { 213, 67 },
    { 182, 69 },
    { 181, 70 },
    { 150, 71 },
    { 149, 72 },
};

static struct BlockTable block_d[6] = {
    { 527, 4 },
    { 496, 2 },
    { 495, 5 },
    { 490, 1 },
    { 463, 6 },
    { 428, 11 },
};

static int RoomNameExtra(int tpwoin);
static int RoomNameTownEast(int tpwoin);
static int RoomNameBowling(int tpwoin);
static int RoomNameHeaven(int tpwoin);
static int RoomNameApart(int tpwoin);
static int RoomNameHospital(int tpwoin);
static int RoomNameDelusion(int tpwoin);
static int RoomNameHotelFace(int tpwoin);
static int RoomNameHotelBack(int tpwoin);
static int RoomNameLastStage(int tpwoin);
static int RoomNameMansion(int tpwoin);

/** Returns the room the player is in (0 without a player). */
int RoomNameJms(void) {
    if (sh2jms.player) {
        return RoomName(0, sh2jms.player->pos.x, sh2jms.player->pos.z);
    }
    return 0;
}

/** Returns the room number of a position in a stage.
 * @param glb_crd stage number, or 0 for the current stage
 * @param pos_x x position
 * @param pos_z z position */
int RoomName(int glb_crd, float pos_x, float pos_z) {
    int tpwoin;
    int x;
    int z;

    x = ftoi(16.0f + pos_x / 20000.0f);
    z = ftoi(16.0f + pos_z / 20000.0f);
    tpwoin = z + (x << 5);
    if (glb_crd == 0) {
        glb_crd = stage->glb_crd;
    }
    switch (glb_crd) {
    case 1:
        return 3;
    case 2:
        return RoomNameTownEast(tpwoin);
    case 3:
        return 8;
    case 4:
        return 0xE;
    case 5:
        return 2;
    case 6:
        return RoomNameExtra(tpwoin);
    case 7:
        return RoomNameBowling(tpwoin);
    case 8:
        return RoomNameHeaven(tpwoin);
    case 9:
        return RoomNameApart(tpwoin);
    case 10:
        return RoomNameHospital(tpwoin);
    case 11:
        return RoomNameDelusion(tpwoin);
    case 12:
        return RoomNameHotelFace(tpwoin);
    case 13:
        return RoomNameHotelBack(tpwoin);
    case 14:
        return RoomNameLastStage(tpwoin);
    case 15:
        return 0xBF;
    case 16:
        return RoomNameMansion(tpwoin);
    }
    return 0;
}

static int RoomNameExtra(int tpwoin) {
    switch (TPWOIN_ROOM(tpwoin)) {
    case 0x210:
        return 5;
    case 0x1D0:
        return 1;
    case 0x20E:
        return 6;
    case 0x1CE:
        return 0xBE;
    }
    return 0;
}

static int RoomNameTownEast(int tpwoin) {
    switch (tpwoin) {
    case 0x26C:
    case 0x28C:
    case 0x2AC:
    case 0x26B:
    case 0x28B:
    case 0x2AB:
        return 7;
    }
    return 4;
}

static int RoomNameBowling(int tpwoin) {
    switch (TPWOIN_ROOM(tpwoin)) {
    case 0x210:
        return 0xB;
    case 0x20E:
        return 9;
    case 0x1D0:
        return 0xA;
    }
    return 0;
}

static int RoomNameHeaven(int tpwoin) {
    switch (TPWOIN_ROOM(tpwoin)) {
    case 0x210:
        return 0xC;
    case 0x1D0:
        return 0xD;
    }
    return 0;
}

static int RoomNameApart(int tpwoin) {
    switch (tpwoin) {
    case 0x1AD:
    case 0x1AC:
    case 0x1AB:
    case 0x1AA:
        return 0x12;
    case 0x18D:
    case 0x18C:
    case 0x16D:
    case 0x16C:
        return 0x11;
    }
    switch (TPWOIN_ROOM(tpwoin)) {
    case 0x18E:
        return 0xF;
    case 0x14E:
        return 0x10;
    case 0x1CA:
        return 0x13;
    case 0x1CC:
        return 0x14;
    case 0x150:
        return 0x15;
    case 0x1D2:
        return 0x16;
    case 0x1D0:
        return 0x17;
    case 0x190:
        return 0x18;
    case 0x192:
        return 0x19;
    case 0x1CE:
        return 0x1A;
    case 0x210:
        return 0x1B;
    case 0x212:
        return 0x1C;
    case 0x1D4:
        return 0x1D;
    case 0x214:
        return 0x1E;
    case 0x250:
        return 0x1F;
    case 0x2CE:
        return 0x20;
    case 0x24E:
        return 0x22;
    case 0x24C:
        return 0x23;
    case 0x24A:
        return 0x24;
    case 0x28C:
        return 0x25;
    case 0x28E:
        return 0x26;
    case 0x20E:
        return 0x27;
    case 0x20A:
        return 0x28;
    case 0x28A:
        return 0x21;
    }
    return 0;
}

static int RoomNameHospital(int tpwoin) {
    switch (TPWOIN_ROOM(tpwoin)) {
    case 0x18A:
        return 0x29;
    case 0x14E:
        return 0x2A;
    case 0x20E:
        return 0x2B;
    case 0x1CC:
        return 0x2C;
    case 0x18E:
        return 0x2D;
    case 0x18C:
        return 0x2E;
    case 0x1CA:
        return 0x2F;
    case 0x10E:
        return 0x30;
    case 0x10A:
        return 0x31;
    case 0x14C:
        return 0x32;
    case 0x1CE:
        return 0x33;
    case 0x10C:
        return 0x34;
    case 0x1D2:
        return 0x35;
    case 0x1D0:
        return 0x36;
    case 0x192:
        return 0x37;
    case 0x112:
        return 0x38;
    case 0x190:
        return 0x39;
    case 0x110:
        return 0x3A;
    case 0x150:
        return 0x3B;
    case 0x194:
        return 0x3C;
    case 0x254:
        return 0x3D;
    case 0x214:
        return 0x3E;
    case 0x212:
        return 0x3F;
    case 0x2D2:
        return 0x40;
    case 0x292:
        return 0x41;
    case 0x290:
        return 0x42;
    case 0x2D4:
        return 0x43;
    case 0x250:
        return 0x44;
    case 0x252:
        return 0x45;
    case 0x1C0:
        return 0x46;
    case 0x184:
        return 0x47;
    case 0x144:
        return 0x48;
    case 0x180:
        return 0x49;
    case 0x1C2:
        return 0x4A;
    case 0x102:
        return 0x4B;
    case 0x104:
        return 0x4C;
    case 0x1C4:
        return 0x4D;
    case 0x142:
        return 0x4E;
    case 0x140:
        return 0x4F;
    case 0x188:
        return 0x50;
    case 0x186:
        return 0x51;
    case 0x1C8:
        return 0x52;
    case 0x1C6:
        return 0x53;
    case 0x208:
        return 0x54;
    case 0x248:
        return 0x55;
    case 0x286:
        return 0x56;
    case 0x246:
        return 0x57;
    case 0x206:
        return 0x58;
    case 0x204:
        return 0x59;
    case 0x200:
        return 0x5A;
    case 0x202:
        return 0x5B;
    }
    return 0;
}

static int RoomNameDelusion(int tpwoin) {
    switch (tpwoin) {
    case 0x1F2:
    case 0x1F3:
    case 0x1F4:
    case 0x1F5:
        return 0x5D;
    case 0x1D2:
    case 0x1D3:
        return 0x5E;
    case 0x1AC:
    case 0x18C:
        return 0x60;
    case 0x1D4:
    case 0x1D5:
        return 0x61;
    case 0x1AE:
        return 0x62;
    case 0x132:
    case 0x112:
        return 0x63;
    case 0x1AF:
    case 0x18F:
        return 0x64;
    case 0x133:
        return 0x65;
    case 0x174:
    case 0x154:
        return 0x66;
    case 0x1B5:
    case 0x195:
        return 0x67;
    case 0x16F:
    case 0x16E:
    case 0x14F:
        return 0x6A;
    case 0x1B4:
    case 0x194:
        return 0x6B;
    case 0x134:
    case 0x113:
    case 0x114:
        return 0x6D;
    case 0x135:
    case 0x115:
        return 0x6E;
    case 0x1EC:
    case 0x1CC:
        return 0x6F;
    case 0x1AD:
    case 0x18D:
        return 0x70;
    case 0x18E:
        return 0x71;
    case 0x175:
    case 0x155:
        return 0x74;
    case 0x1EF:
    case 0x1CF:
        return 0x75;
    case 0x1EE:
    case 0x1CE:
        return 0x76;
    case 0x1ED:
    case 0x1CD:
        return 0x77;
    }
    switch (TPWOIN_ROOM(tpwoin)) {
    case 0x1D0:
        return 0x5C;
    case 0x190:
        return 0x5F;
    case 0x152:
        return 0x68;
    case 0x150:
        return 0x69;
    case 0x192:
        return 0x6C;
    case 0x110:
        return 0x72;
    case 0x10E:
        return 0x73;
    case 0x1CA:
        return 0x78;
    case 0x214:
        return 0x79;
    case 0x2D2:
        return 0x7A;
    case 0x292:
        return 0x7B;
    case 0x2D0:
        return 0x7C;
    case 0x252:
        return 0x7D;
    case 0x24E:
        return 0x7E;
    case 0x2D4:
        return 0x7F;
    case 0x250:
        return 0x80;
    case 0x294:
        return 0x81;
    case 0x20C:
        return 0x82;
    case 0x24A:
        return 0x83;
    case 0x254:
        return 0x84;
    case 0x290:
        return 0x85;
    case 0x24C:
        return 0x86;
    case 0x20E:
        return 0x87;
    case 0x20A:
        return 0x88;
    case 0x210:
        return 0x89;
    case 0x212:
        return 0x8A;
    case 0x2CE:
        return 0x8B;
    case 0x2CC:
        return 0x8C;
    case 0x28E:
        return 0x8D;
    case 0x28C:
        return 0x8E;
    case 0x28A:
        return 0x8F;
    case 0x2CA:
        return 0x90;
    }
    return 0;
}

static int RoomNameHotelFace(int tpwoin) {
    switch (tpwoin) {
    case 0x132:
    case 0x133:
        return 0x96;
    }
    switch (TPWOIN_ROOM(tpwoin)) {
    case 0x14E:
        return 0x91;
    case 0x1D0:
        return 0x92;
    case 0x1D2:
        return 0x93;
    case 0x194:
        return 0x94;
    case 0x1D4:
        return 0x95;
    case 0x152:
        return 0x97;
    case 0x192:
        return 0x98;
    case 0x190:
        return 0x99;
    case 0x150:
        return 0x9A;
    case 0x20C:
        return 0x9B;
    case 0x28C:
        return 0x9C;
    case 0x24C:
        return 0x9D;
    case 0x20A:
        return 0x9E;
    case 0x20E:
        return 0x9F;
    case 0x24E:
        return 0xA0;
    case 0x28E:
        return 0xA1;
    case 0x250:
        return 0xA2;
    case 0x210:
        return 0xA3;
    case 0x290:
        return 0xA4;
    case 0x1CE:
        return 0xA5;
    case 0x18E:
        return 0xA6;
    case 0x14C:
        return 0xA7;
    case 0x1CC:
        return 0xA8;
    case 0x18C:
        return 0xA9;
    }
    return 0;
}

static int RoomNameHotelBack(int tpwoin) {
    switch (TPWOIN_ROOM(tpwoin)) {
    case 0x14C:
        return 0xAA;
    case 0x14E:
        return 0xAB;
    case 0x1D0:
        return 0xAC;
    case 0x190:
        return 0xAD;
    case 0x150:
        return 0xAE;
    case 0x1D2:
        return 0xAF;
    case 0x20C:
        return 0xB0;
    case 0x20E:
        return 0xB1;
    case 0x24E:
        return 0xB2;
    case 0x28E:
        return 0xB3;
    case 0x24C:
        return 0xB4;
    case 0x210:
        return 0xB5;
    case 0x1CE:
        return 0xB6;
    case 0x18E:
        return 0xB7;
    case 0x1CC:
        return 0xB8;
    case 0x18C:
        return 0xB9;
    case 0x250:
        return 0xBA;
    case 0x252:
        return 0xBB;
    }
    return 0;
}

static int RoomNameLastStage(int tpwoin) {
    switch (TPWOIN_ROOM(tpwoin)) {
    case 0x1D0:
        return 0xBC;
    case 0x210:
        return 0xBD;
    }
    return 0;
}

static int RoomNameMansion(int tpwoin) {
    switch (TPWOIN_ROOM(tpwoin)) {
    case 0x210:
        return 0xC0;
    case 0x250:
        return 0xC1;
    case 0x1CE:
        return 0xC2;
    case 0x20E:
        return 0xC3;
    case 0x290:
        return 0xC4;
    case 0x1D4:
        return 0xC5;
    case 0x1D0:
        return 0xC6;
    case 0x24E:
        return 0xC7;
    case 0x194:
        return 0xC8;
    case 0x20C:
        return 0xC9;
    case 0x254:
        return 0xCA;
    case 0x214:
        return 0xCB;
    case 0x24C:
        return 0xCC;
    case 0x212:
        return 0xCD;
    case 0x1CC:
        return 0xCE;
    case 0x252:
        return 0xCF;
    case 0x192:
        return 0xD0;
    case 0x24A:
        return 0xD1;
    case 0x20A:
        return 0xD2;
    }
    return 0;
}


/** Gets the BG blocks (glb_crd << 16 | block) at a position: the 4 blocks of the room indoors,
 * or the one block of its cell outdoors (a binary search of the stage's block table).
 * @param ret result, 4 entries (unused ones 0)
 * @param glb_crd stage number, or 0 for the current stage
 * @param pos_x x position
 * @param pos_z z position
 * @return 1 outdoors, 0 indoors */
int BlockNumber(int *ret, int glb_crd, float pos_x, float pos_z) {
    int i;
    int z;
    int x;
    int room;
    struct BlockTable *block;
    int result;
    int xz;
    int no;

    if (glb_crd == 0) {
        glb_crd = stage->glb_crd;
    }
    for (i = 0; i < 4; i++) {
        ret[i] = 0;
    }
    room = RoomName(glb_crd, pos_x, pos_z);
    if (!BgIsOut(glb_crd)) {
        for (i = 0; i < 4; i++) {
            ret[i] = glb_crd << 16 | room_to_block[room][i];
        }
        return 0;
    }
    x = (pos_x >= 0.0f ? 1 : -1) + ftoi(pos_x / 20000.0f);
    z = (pos_z >= 0.0f ? 1 : -1) + ftoi(pos_z / 20000.0f);
    if (x > 0) {
        x--;
    }
    if (z > 0) {
        z--;
    }
    xz = (z + 16) + ((x + 16) << 5);
    switch (glb_crd) {
    case 1:
    default:
        block = block_a;
        no = 23;
        break;
    case 2:
        block = block_b;
        no = 55;
        break;
    case 3:
        block = block_c;
        no = 76;
        break;
    case 4:
        block = block_d;
        no = 7; /* @bug block_d has 6 entries: finding the last one reads block_d[6] (the zero pad) */
        break;
    }
    result = 0;
    while (1) { /* @bug the search can read entries before or after the table (results unaffected) */
        if (no == 0) {
            break;
        }
        if (no < 2) {
            no = 0;
        } else {
            no = (no + 1) >> 1;
        }
        if (block->xz < xz) {
            block -= no;
        } else if (xz < block->xz) {
            block += no;
        } else if (block->xz == xz) {
            result = block->block;
            break;
        }
    }
    ret[0] = glb_crd << 16 | result;
    return 1;
}

/**
 * Returns non-zero if a stage is outdoors (the town: glb_crd 1-4); those use the xz block
 * tables above, indoor stages the per-room room_to_block table.
 * @param glb_crd stage number, or 0 for the current stage
 */
int BgIsOut(int glb_crd) {
    if (glb_crd == 0) {
        glb_crd = stage->glb_crd;
    }
    /* Matching: the assert bakes its original line number into the object. */
#line 963
    assert_dw(glb_crd > Glb_crd_null && glb_crd < Glb_crd_num);
    return glb_crd >= 1 && glb_crd < 5;
}
