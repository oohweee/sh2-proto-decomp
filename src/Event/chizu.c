/*
 * chizu.c: the map screen. Shows the map of the area James is in (if he has
 * it), the markers drawn on it, the arrows to connected maps and his current
 * position.
 */
#include "sh2.h"
#include "asm_helpers.h"

/* Matching: fitted stand-in for double code (docs/stand-ins.md): later functions use a2 for temporaries. */
STRIPPED_DOUBLE_CODE()

/* Game flag n: bit n % 32 of game_flag.flag[n / 32]. */
#define GAME_FLAG(n) ((game_flag.flag[(n) >> 5] >> ((n) & 31)) & 1)

static int ChizuSelect(void);
static int ChizuPossessionCheck(int chizu);
static void ChizuFileLoad(int load_chizu);
static void ChizuDisplay(void);
static void ChizuMarkerDraw(void);
static void ChizuControl(void);
static void ChizuConnectArrowDraw(void);
static int ChizuConnectCheck(int chizu, int connect);
static void ChizuCurrentPositionDraw(void);
static void ChizuCurrentPositionCheck(float *px, float *py);

static struct Chizu_CurrentBlock chz_crt_block[190] = {
    { 0, 0.0f, 0.0f },
    { 1, 218.40036f, 170.84044f },
    { 1, 209.9651f, 174.0604f },
    { 1, 191.69885f, 90.898254f },
    { 2, -12.699994f, 66.0f },
    { 2, -193.8294f, 187.43202f },
    { 2, -141.44449f, 123.035995f },
    { 4, -754.10046f, -739.0f },
    { 3, 167.99997f, 67.70005f },
    { 3, 33.678513f, -63.149544f },
    { 3, 33.678513f, -63.149544f },
    { 3, 33.678513f, -63.149544f },
    { 3, 23.795807f, 2.529663f },
    { 3, 23.795807f, 2.529663f },
    { 1, 0.0f, 0.0f },
    { 0, 574.2999f, -332.69965f },
    { 0, 872.0f, -37.0f },
    { 0, 888.7005f, -420.00085f },
    { 4, 453.0f, -573.0f },
    { 6, 249.0f, -940.1f },
    { 4, 256.19995f, -694.0f },
    { 5, 846.70044f, 246.19823f },
    { 5, 141.5988f, 738.199f },
    { 5, 331.09888f, 346.69986f },
    { 5, 596.59875f, 142.39897f },
    { 5, 598.9f, 458.40033f },
    { 5, 353.59875f, -98.60103f },
    { 6, -264.50006f, 352.49985f },
    { 6, -117.2995f, 735.3991f },
    { 4, 255.0f, 1057.0f },
    { 4, -98.0f, 1056.0f },
    { 6, -433.6998f, 249.29956f },
    { 0, -1398.6998f, -178.9f },
    { 0, -1013.3f, -1086.0f },
    { 7, -658.3992f, -182.59967f },
    { 7, -705.5996f, -632.69965f },
    { 7, -717.9f, -987.70013f },
    { 7, -961.90076f, -617.8996f },
    { 8, -961.90076f, -191.8996f },
    { 8, -312.30237f, -79.7994f },
    { 8, -357.8025f, -1017.6994f },
    { 0, 0.0f, 0.0f },
    { 0, 0.0f, 0.0f },
    { 13, -418.8996f, -455.19922f },
    { 12, 411.09976f, -1295.7003f },
    { 12, 1030.4994f, -506.59985f },
    { 12, 1011.00024f, -1272.9f },
    { 12, 415.5999f, -2055.7998f },
    { 12, 0.0f, 0.0f },
    { 12, 2065.7986f, -1929.2997f },
    { 12, 1429.4974f, -1179.6001f },
    { 12, 419.39966f, -483.19962f },
    { 12, 2049.1982f, -1185.9995f },
    { 13, 389.59918f, 1202.7987f },
    { 13, 440.59918f, 431.29865f },
    { 13, 951.19727f, 1284.4004f },
    { 13, 2082.5977f, 1320.0002f },
    { 13, 859.59766f, 534.2009f },
    { 13, 1918.101f, 536.59973f },
    { 13, 1636.101f, 447.09985f },
    { 13, 856.09924f, 2110.799f },
    { 13, -783.4008f, 1788.999f },
    { 13, -192.59944f, 1789.7008f },
    { 13, -201.5008f, 980.09924f },
    { 13, -2082.3992f, 1030.8997f },
    { 13, -1444.2026f, 1082.5004f },
    { 13, -1585.401f, 298.29916f },
    { 13, -2228.9998f, 1870.2991f },
    { 13, -725.4008f, 205.6987f },
    { 13, -915.6007f, 1084.9993f },
    { 0, 0.0f, 0.0f },
    { 0, 0.0f, 0.0f },
    { 0, 0.0f, 0.0f },
    { 14, 892.0973f, -5953.2793f },
    { 14, 482.2997f, -5248.3984f },
    { 14, 2091.2996f, -5110.1904f },
    { 14, 2056.1982f, -4323.282f },
    { 14, 4.2e+02f, -4415.0f },
    { 14, 1452.2996f, -5116.691f },
    { 14, 1367.0973f, -5946.2812f },
    { 15, 834.3972f, -2614.2793f },
    { 15, 818.29987f, -3396.6948f },
    { 15, 145.29987f, -2609.3984f },
    { 15, 438.19968f, -3423.6948f },
    { 15, -156.30026f, -2883.3984f },
    { 15, -852.19995f, -2850.1992f },
    { 15, -1584.9f, -3636.0f },
    { 15, -939.9f, -3643.0f },
    { 15, -136.09987f, -3666.6985f },
    { 14, -196.19997f, -4214.199f },
    { 0, 0.0f, 0.0f },
    { 0, 0.0f, 0.0f },
    { 1, 0.0f, 0.0f },
    { 0, 0.0f, 0.0f },
    { 0, 0.0f, 0.0f },
    { 0, 0.0f, 0.0f },
    { 0, 0.0f, 0.0f },
    { 0, 0.0f, 0.0f },
    { 0, 0.0f, 0.0f },
    { 0, 0.0f, 0.0f },
    { 0, 0.0f, 0.0f },
    { 0, 0.0f, 0.0f },
    { 16, 1440.7001f, 759.1f },
    { 16, 859.6998f, 1013.00024f },
    { 16, 1339.6998f, 550.19995f },
    { 16, 1480.0995f, 131.69965f },
    { 16, 1272.899f, -214.30042f },
    { 16, 856.89905f, 916.69965f },
    { 16, 762.6001f, 487.90048f },
    { 16, 2127.699f, 751.7f },
    { 16, 2017.399f, 958.0002f },
    { 16, 381.59958f, -878.7998f },
    { 16, 947.3999f, -671.40015f },
    { 16, 996.89905f, -342.3004f },
    { 16, 1961.6997f, 306.9984f },
    { 16, 1999.1002f, -358.79987f },
    { 17, 1223.0f, 976.0f },
    { 0, 0.0f, 0.0f },
    { 0, 0.0f, 0.0f },
    { 0, 0.0f, 0.0f },
    { 18, 77.19977f, -723.2909f },
    { 18, -85.89999f, 833.2002f },
    { 19, -1252.3999f, 621.1f },
    { 18, -826.6006f, 592.09973f },
    { 19, -1324.0f, 261.0f },
    { 18, -474.99948f, 644.39954f },
    { 19, -560.9f, -32.20008f },
    { 18, -1377.199f, 1039.0992f },
    { 19, -633.5001f, 355.1996f },
    { 18, -997.6001f, 965.1996f },
    { 19, -280.8f, -449.39984f },
    { 19, -746.0f, -8e+02f },
    { 18, -697.20044f, 1051.6001f },
    { 19, -1031.0f, 82.400024f },
    { 18, -553.7001f, -650.1007f },
    { 18, -249.20044f, -254.3999f },
    { 19, -342.69995f, -803.6001f },
    { 19, -283.0f, 94.0f },
    { 18, -69.79998f, 457.4f },
    { 18, -1316.5001f, -292.9997f },
    { 18, -1365.4001f, -629.8997f },
    { 19, -1079.0f, -291.59998f },
    { 0, 0.0f, 0.0f },
    { 0, 0.0f, 0.0f },
    { 0, 0.0f, 0.0f },
    { 0, 1783.3007f, -372.2999f },
    { 21, 325.69995f, 450.59357f },
    { 21, 388.69992f, 1154.8993f },
    { 21, 872.19995f, 2007.0999f },
    { 29, 421.3999f, 1823.0f },
    { 29, 2325.5f, 1184.1005f },
    { 29, 1733.3999f, 1133.6006f },
    { 21, 1043.0997f, 1255.2f },
    { 21, 969.7018f, 494.49493f },
    { 29, 1680.9f, 487.0f },
    { 22, -445.2987f, -972.6998f },
    { 22, -1706.9988f, -992.6998f },
    { 30, -9e+02f, -1074.5999f },
    { 22, -400.2998f, -1927.1002f },
    { 22, -453.89825f, -438.60477f },
    { 22, -1044.6989f, -335.60477f },
    { 22, -1526.8998f, -336.29993f },
    { 23, -1042.3998f, 553.59937f },
    { 23, -341.5996f, 479.2f },
    { 23, 0.0f, 0.0f },
    { 20, 353.69995f, -276.89954f },
    { 28, 1042.5f, -276.89954f },
    { 28, 1809.2006f, -1045.2999f },
    { 20, 320.00168f, -1160.2048f },
    { 28, 1082.8009f, -1243.7998f },
    { 0, 0.0f, 0.0f },
    { 0, 1783.3007f, -372.2999f },
    { 25, 325.69995f, 450.59357f },
    { 25, 969.7018f, 494.49493f },
    { 33, 1755.5004f, 394.89987f },
    { 25, 424.0f, 1119.0f },
    { 26, -445.19977f, -971.59985f },
    { 26, -455.0f, -438.9f },
    { 26, -1043.0f, -335.9f },
    { 26, 0.0f, 0.0f },
    { 26, 0.0f, 0.0f },
    { 27, -342.0f, 481.0f },
    { 24, 353.8f, -277.0f },
    { 32, 1042.0f, -277.0f },
    { 24, 320.49988f, -1118.7003f },
    { 32, 1082.7003f, -1242.0002f },
    { 0, 0.0f, 0.0f },
    { 0, 0.0f, 0.0f },
    { 0, 0.0f, 0.0f },
    { 0, 0.0f, 0.0f },
};

static struct Chizu_ConnectInfo chz_connect[36] = {
    { 0, 0, 0, 0 },
    { 2, 3, 9, 29 },
    { 3, 1, 9, 29 },
    { 1, 2, 9, 29 },
    { 5, 0, 12, 1 },
    { 6, 4, 12, 1 },
    { 0, 5, 12, 1 },
    { 0, 0, 12, 1 },
    { 0, 0, 12, 1 },
    { 10, 0, 12, 1 },
    { 11, 9, 12, 1 },
    { 0, 10, 12, 1 },
    { 13, 0, 16, 9 },
    { 14, 12, 16, 9 },
    { 15, 13, 16, 9 },
    { 0, 14, 16, 9 },
    { 0, 18, 29, 12 },
    { 16, 18, 29, 12 },
    { 16, 19, 29, 12 },
    { 18, 0, 29, 12 },
    { 21, 0, 24, 16 },
    { 22, 20, 25, 16 },
    { 23, 21, 26, 16 },
    { 0, 22, 27, 16 },
    { 25, 0, 1, 20 },
    { 26, 24, 1, 21 },
    { 27, 25, 1, 22 },
    { 0, 26, 1, 23 },
    { 29, 0, 32, 16 },
    { 30, 28, 33, 16 },
    { 31, 29, 34, 16 },
    { 0, 30, 35, 16 },
    { 33, 0, 1, 28 },
    { 34, 32, 1, 29 },
    { 35, 33, 1, 30 },
    { 0, 34, 1, 31 },
};

static struct Chizu_MarkerTex chz_mrk_tex_hsp[13] = {
    { 0x40, 0x20, 0x6F, 0x3F },
    { 0x0, 0x1, 0xF, 0x3F },
    { 0x10, 0x1, 0x1F, 0x1F },
    { 0x30, 0x40, 0x4F, 0x4F },
    { 0x20, 0x10, 0x2F, 0x3F },
    { 0x0, 0x40, 0x2F, 0x4F },
    { 0x10, 0x20, 0x1F, 0x3F },
    { 0x20, 0x1, 0x3F, 0xF },
    { 0x30, 0x10, 0x3F, 0x3F },
    { 0x40, 0x1, 0x6F, 0x1F },
    { 0x60, 0x40, 0x7F, 0x5F },
    { 0x0, 0x50, 0x1F, 0x7F },
    { 0x20, 0x50, 0x3F, 0x7F },
};

static unsigned int chz_mrk_dat_hospitalb1f[45] = {
    0xBA015107, 0xCD0D0047, 0xB5F9BE01, 0xD3D1B046, 0xD3D22F06, 0xD3AA8286,
    0xD3C2B846, 0xD3A2EE06, 0xC692A880, 0xCABAAF42, 0xCAE2A806, 0xC2332C86,
    0xD3A38586, 0xD39BD886, 0xD3B4BE86, 0xCAD4C206, 0xC58BC6C7, 0x987B9781,
    0x9663C403, 0x9653C787, 0xAFA367C7, 0x92EB8586, 0xA9028900, 0xAD229082,
    0xAD5A8C86, 0xA492A606, 0xA4925546, 0xA479D906, 0xA16B3A41, 0x87D42507,
    0x77F3F481, 0x664BF681, 0x52EC2607, 0x4C23E4C6, 0x50535581, 0x54E39446,
    0x7C13C707, 0x879BC8C3, 0x9EB3E246, 0x96644607, 0xB8F29881, 0xB682D6C6,
    0xBDA23A40, 0x96833646, 0xB851F9CA,
};

static unsigned int chz_mrk_dat_hospital23rf[71] = {
    0xB614E106, 0xB1DC3B80, 0xBA7C7A80, 0xBED4EC86, 0xD3855B46, 0xDC56F446,
    0xDC4608C6, 0xBFA59787, 0xA9B5C6C1, 0xB2CD6801, 0xB00616C6, 0xA7FE7507,
    0x97C65407, 0x853E2441, 0x830E5487, 0x70A62401, 0x6E8E5487, 0x59EE54C7,
    0x456E5407, 0x329629C1, 0x6695C786, 0x907DF647, 0x9E4DC781, 0x9C5DF647,
    0x619D8A81, 0x5FBDB443, 0xB2E1AC41, 0xBA812DC1, 0xB8595F83, 0xC119DC47,
    0xD3F9A046, 0xDCB33346, 0xDCDA4C46, 0xAA620B01, 0xA84A3BC3, 0xB0525B06,
    0xA86AB947, 0x9F6A9A87, 0x95E29B47, 0x8FF26781, 0x8DEA9907, 0x846A9A87,
    0x7BE29987, 0x72F29B07, 0x6A5298C7, 0x61729A47, 0x58329A87, 0x4FDA9B87,
    0x49026B81, 0x3E129A47, 0x35229B87, 0x2EBA6B81, 0x66E20806, 0x91723B47,
    0x76D20AC1, 0x9CF23B47, 0x61F9CBC1, 0x6021FC43, 0xB758FF87, 0xB1090007,
    0xAD20D041, 0xA598FFC7, 0x2B02D00B, 0x2B02D00C, 0x7481D00B, 0x7481D00C,
    0x256BAFC1, 0x230BE0C3, 0x230BE047, 0x34336F86, 0x482AC80A,
};

static unsigned int chz_mrk_dat_hospitalb1b[46] = {
    0xA385D400, 0xA805E002, 0xAD9D9181, 0x9F060009, 0xBBC12401, 0xB8014C85,
    0xCD0D0047, 0xB3B9ED87, 0xCF51AC00, 0xD3F22AC6, 0xD3BA8106, 0xD3BAB5C6,
    0xD3BAF246, 0xCAAAAA06, 0xBDCB2500, 0xD3A38586, 0xD39BD886, 0xD3B4BE86,
    0xC9E4C206, 0xC58BC6C7, 0x9653C787, 0x9FBB6607, 0xAFA367C7, 0x92EB8586,
    0xAD028906, 0xA492A606, 0xA4925546, 0xA479D906, 0x897BF6C1, 0x781BF6C1,
    0x644C2307, 0x52EC2607, 0x4C23E4C0, 0x50535581, 0x54E39446, 0x7C13C707,
    0x879BC8C7, 0x9EB3E246, 0x96644607, 0x459AF981, 0x46FC2387, 0x80040008,
    0x9F836807, 0x54BB9406, 0x4003E40A, 0xBD81680A,
};

static unsigned int chz_mrk_dat_hospital23rb[50] = {
    0x619D8A81, 0x6695C786, 0x907DF647, 0x9C85F5C7, 0xA855F5C7, 0xAC061480,
    0xA7FE7507, 0x97C65407, 0x830E5487, 0x6E8E5487, 0x5BBE2601, 0x456E5407,
    0x329629C1, 0xDC66F246, 0xD3ADFB86, 0xB18D9847, 0x6685C806, 0xB8595F87,
    0xC349B101, 0xD3F9A046, 0xDCB33346, 0xDCDA4C46, 0xAA620B01, 0xB381B041,
    0xB0525B06, 0xA86AB947, 0x9F6A9A87, 0x95E29B47, 0x8FF26781, 0x846A9A87,
    0x7BE29987, 0x72F29B07, 0x6A5298C7, 0x61729A47, 0x58329A87, 0x4FDA9B87,
    0x49026B81, 0x3E129A47, 0x35229B87, 0x2C529AC7, 0x61F9CBC1, 0x62820C00,
    0x66DA12C2, 0x74EA3C47, 0x91723B47, 0x9CF23B47, 0x80040001, 0x80040000,
    0x2373E907, 0xB0018BCA,
};

static struct Chizu_MarkerTex chz_mrk_tex_apt[39] = {
    { 0x80, 0x10, 0x9F, 0x2F },
    { 0x0, 0x1, 0xF, 0x2F },
    { 0xB0, 0x1, 0xBF, 0x10 },
    { 0xA0, 0x10, 0xAF, 0x2F },
    { 0x50, 0x20, 0x6F, 0x2F },
    { 0xA0, 0x10, 0xAF, 0x2F },
    { 0x90, 0x1, 0xAF, 0x10 },
    { 0x70, 0x10, 0x7F, 0x2F },
    { 0x70, 0x1, 0x8F, 0x10 },
    { 0x40, 0x1, 0x6F, 0x1F },
    { 0x0, 0x60, 0x3F, 0x8F },
    { 0x40, 0x30, 0x8F, 0x4F },
    { 0x90, 0x30, 0xDF, 0x4F },
    { 0x40, 0x50, 0x7F, 0x7F },
    { 0x0, 0x90, 0x2F, 0xFF },
    { 0x30, 0xA0, 0x4F, 0xFF },
    { 0xE0, 0xB0, 0xFF, 0x11F },
    { 0xE0, 0x120, 0xFF, 0x16F },
    { 0x0, 0x100, 0x3F, 0x12F },
    { 0x40, 0x100, 0x8F, 0x11F },
    { 0xA0, 0x80, 0xFF, 0xAF },
    { 0xA0, 0xB0, 0xDF, 0xDF },
    { 0x40, 0x120, 0x8F, 0x14F },
    { 0x60, 0x150, 0x9F, 0x17F },
    { 0xC0, 0xE0, 0xDF, 0x13F },
    { 0xA0, 0x100, 0xBF, 0x16F },
    { 0x0, 0x130, 0x2F, 0x19F },
    { 0x30, 0x150, 0x5F, 0x1AF },
    { 0x60, 0x180, 0x9F, 0x1AF },
    { 0xA0, 0x180, 0xEF, 0x19F },
    { 0x50, 0x80, 0x9F, 0xBF },
    { 0x0, 0x30, 0x3F, 0x5F },
    { 0xC0, 0x1, 0xFF, 0x30 },
    { 0x50, 0xC0, 0x9F, 0xFF },
    { 0x80, 0x50, 0xBF, 0x7F },
    { 0xC0, 0x50, 0xFF, 0x7F },
    { 0x10, 0x1, 0x2F, 0x30 },
    { 0x0, 0x1A0, 0x1F, 0x1BF },
    { 0x30, 0x1, 0x4F, 0x30 },
};

static unsigned int chz_mrk_dat_apart_e1[22] = {
    0xDEAED681, 0xDC8EF284, 0x7ED6D681, 0x989DACC0, 0x9A9C6D87, 0x9A9B5F87,
    0x98824F40, 0xE1BDCA07, 0xE1BCAC07, 0xE133EF06, 0x9801EDC1, 0x97D20C42,
    0x939CCC40, 0x9574CA43, 0xAD84CE07, 0x83DDC201, 0x839DE282, 0x9F94F59C,
    0x9BEDA91D, 0x9CAA480C, 0xA161924D, 0x8206AC25,
};

static unsigned int chz_mrk_dat_apart_e2[35] = {
    0xA8627B01, 0x48A61686, 0x654DF781, 0x654E1782, 0x910616C6, 0xAA8E1586,
    0xC655F741, 0x99058807, 0x990472C7, 0x976B6040, 0x976A4E40, 0xDF6D97C0,
    0xE1A47487, 0xE1F3B886, 0x9789ECC1, 0x83D5F801, 0xE1861306, 0x42F5EA40,
    0x44F5E803, 0x602E1A8E, 0x72863C8F, 0xC3861810, 0xD30E5411, 0x9EAAA7D2,
    0x9A8B5A13, 0x99D24B54, 0xA0299615, 0xCFC590D6, 0xD094E357, 0x9D81EC25,
    0xA6829824, 0x86863C24, 0x9602F806, 0xA6829826, 0x86863C26,
};

static unsigned int chz_mrk_dat_apart_e3[20] = {
    0x4515EC07, 0x49D5F341, 0x64161206, 0x926DF4C1, 0xAA561546, 0xC42E1546,
    0x98FD8887, 0x97647540, 0x990B5E87, 0x990A5007, 0x97A9EC01, 0x8405F481,
    0xE375F501, 0x861DEA47, 0x57B637D8, 0x47261CD9, 0x8D361B5A, 0x9DBE369B,
    0x9E2BC10A, 0x9A9C74CB,
};

static unsigned int chz_mrk_dat_apart_w1[17] = {
    0x351CD4C7, 0x351DF387, 0x3A3565C7, 0x3A349987, 0x3843CA40, 0x3A32FD47,
    0x3A325047, 0x3519C847, 0x334ADC80, 0x3A11FCC7, 0x334C1EC0, 0x3BB41DDE,
    0x231357E0, 0x287C2107, 0x431C5C25, 0x414BA824, 0x414BA826,
};

static unsigned int chz_mrk_dat_apart_w2[20] = {
    0x7BC4DD07, 0x7BC5F207, 0x7EFD6580, 0x80EC9547, 0x80EBCB47, 0x80EAFD47,
    0x80EA5207, 0x7BC1C6C7, 0x7A12DAC0, 0x7BF2DDC3, 0x7F01C2C0, 0x80C1C583,
    0x7A5BDA80, 0x6A0AB701, 0x771BD9C7, 0x80040007, 0x82D5B9E1, 0x6A128662,
    0x69FB5223, 0x6C828425,
};

static unsigned int chz_mrk_dat_apart_w1_offsetx[17] = {
    0x2ADCD4C7, 0x2ADDF387, 0x2FF565C7, 0x2FF49987, 0x2E03CA40, 0x2FF2FD47,
    0x2FF25047, 0x2AD9C847, 0x290ADC80, 0x2FD1FCC7, 0x290C1EC0, 0x31741DDE,
    0x18D357E0, 0x1E3C2107, 0x38DC5C25, 0x370BA824, 0x370BA826,
};

static unsigned int chz_mrk_dat_apart_w2_offsetx[20] = {
    0x2B0CDD07, 0x2B0DF207, 0x2E456580, 0x30349547, 0x3033CB47, 0x3032FD47,
    0x30325207, 0x2B09C6C7, 0x295ADAC0, 0x2B3ADDC3, 0x2E49C2C0, 0x3009C583,
    0x29A3DA80, 0x1952B701, 0x2663D9C7, 0x2F4C0007, 0x321DB9E1, 0x195A8662,
    0x19435223, 0x1BCA8425,
};

static struct Chizu_MarkerTex chz_mrk_tex_htl[17] = {
    { 0x40, 0x1, 0x5F, 0x20 },
    { 0x30, 0x30, 0x3F, 0x5F },
    { 0x20, 0x1, 0x2F, 0x10 },
    { 0x10, 0x1, 0x1F, 0x20 },
    { 0x0, 0x30, 0x1F, 0x3F },
    { 0x0, 0x50, 0x2F, 0x5F },
    { 0x30, 0x1, 0x3F, 0x30 },
    { 0x0, 0x20, 0x1F, 0x2F },
    { 0x0, 0x1, 0xF, 0x20 },
    { 0x0, 0x40, 0x2F, 0x4F },
    { 0x20, 0x10, 0x2F, 0x3F },
    { 0x0, 0x60, 0x2F, 0x6F },
    { 0x30, 0x60, 0x4F, 0x7F },
    { 0x40, 0x20, 0x5F, 0x4F },
    { 0x60, 0x1, 0x7F, 0x30 },
    { 0x50, 0x50, 0x6F, 0x6F },
    { 0x70, 0x30, 0x7F, 0x7F },
};

static unsigned int chz_mrk_dat_hotel_gbf[4] = {
    0x8D8CD581, 0x8BC4F344, 0x70F3CA47, 0x67B3CA47,
};

static unsigned int chz_mrk_dat_hotel_ebf[11] = {
    0x9E054CC0, 0xA40D7947, 0xB4457947, 0xC09D1647, 0xC69CE9C0, 0xBC1467CA,
    0xB9FBD840, 0xAC8C9E88, 0x91BD70C0, 0x988D088D, 0x988D088E,
};

static unsigned int chz_mrk_dat_hotel_g1f[20] = {
    0x8DACD641, 0x6EA4D581, 0xA024B888, 0xA025220A, 0x9B1D5781, 0x7E06A201,
    0x71957807, 0x62157A07, 0x4C155AC1, 0x3CBD1BCA, 0x4384F587, 0x4A4CF407,
    0x7A12A28B, 0x96ABA08A, 0x94AC0380, 0x7B32FD0D, 0x9CC3F848, 0xA2E431C8,
    0x6583100F, 0x7B32FD0E,
};

static unsigned int chz_mrk_dat_hotel_e1f[27] = {
    0xB50D7909, 0xD2FD7A07, 0xE45D2A8A, 0xDE3CF801, 0xCC9D1607, 0xC06D1607,
    0xD803CDC8, 0xD8043208, 0x4A157947, 0xBA0407C0, 0xBC0B5808, 0xD1DB3488,
    0xD1DAF508, 0xE49276C8, 0xD65A6047, 0xCBB272C8, 0xBC22C488, 0xB6A2A247,
    0xB3D33B00, 0xB5E38A08, 0xB3A3DE00, 0xAC8CA748, 0x4F8DBA08, 0x4935BD48,
    0x4F7E6188, 0x46F66500, 0xBD131B48,
};

static unsigned int chz_mrk_dat_hotel_g2f[31] = {
    0x79DD584B, 0x40FD18C0, 0x33A25348, 0x3AD25280, 0x3CBA52C3, 0x33AAF548,
    0x3ACAF780, 0x3CBAF943, 0x33A39A48, 0x3CF39A48, 0x33A44048, 0x3CF44048,
    0x33A4E348, 0x33A529C8, 0x63051940, 0x4A1CF647, 0x4424F4C7, 0x4A4E4080,
    0x4C4D7DC8, 0x50956640, 0x949D1940, 0x9F055807, 0xAB85DC07, 0xB2BD8E48,
    0xB8AD0A8A, 0x9FFCB888, 0xAF7CD741, 0xAF44F4C2, 0xB404CA4F, 0x4D02100D,
    0x4D02100E,
};

static unsigned int chz_mrk_dat_hotel_g3f[10] = {
    0x79B55709, 0x80040009, 0x9FFCB808, 0x58BCD601, 0x58DCFA02, 0x61ED3701,
    0x621D5B02, 0x97D55707, 0xA60D080A, 0x555D080A,
};

static unsigned int chz_mrk_dat_hotel_gbb[2] = {
    0x8D8CD581, 0x87B4854B,
};

static unsigned int chz_mrk_dat_hotel_ebb[9] = {
    0x91BD70C0, 0x9E054CC0, 0xAC8C9E88, 0xA40D7947, 0xB4457947, 0xC894E888,
    0xBC1467CA, 0xB9FBD840, 0xC0851687,
};

static unsigned int chz_mrk_dat_hotel_g1b[9] = {
    0x7DE281C1, 0x8DACD501, 0x89D4F405, 0x6EB4D501, 0x6AC4F405, 0x71857A07,
    0x96851BCA, 0x67851C0A, 0x6D4A59CF,
};

static unsigned int chz_mrk_dat_hotel_e1b[16] = {
    0xBA1C0880, 0xB4747789, 0xD7E3D188, 0xD7E42C88, 0x8903900C, 0xBC0B56C8,
    0xD1EB3AC8, 0xD1EAF408, 0xE27A72C0, 0xCB7A7308, 0xB9D2C680, 0xB6D2A247,
    0xB5C33788, 0xB5C38A08, 0xB5C3DC48, 0xD5EA5F47,
};

static unsigned int chz_mrk_dat_hotel_g2b[32] = {
    0x40BD1880, 0x33A24FC8, 0x3AC25340, 0x33A2F3C8, 0x3AC2F780, 0x33A39A08,
    0x3CF39A08, 0x315C4240, 0x3CF44248, 0x33A4E308, 0x33A527C8, 0x5D34338A,
    0x64FD0DCA, 0x52A566C8, 0x49E642C0, 0x4C2581C8, 0x9695090A, 0xAD74F407,
    0x80040008, 0xB2A579C8, 0xABD5DC87, 0xB6C51A80, 0xC86D2688, 0xC674E800,
    0xBF0C40C8, 0xC87C40C8, 0xBF0B9E88, 0xC87B9E88, 0xBF0AF808, 0xC87AF808,
    0xBCE25080, 0xC67A43C0,
};

static unsigned int chz_mrk_dat_hotel_g3b[7] = {
    0x61ED3801, 0xA6450D0A, 0x55550D0A, 0xA00CB8C8, 0x79BD5789, 0x97D55807,
    0x9865338F,
};

static struct Chizu_MarkerTex chz_mrk_tex_lab_up[23] = {
    { 0x0, 0x1, 0x2F, 0x60 },
    { 0x30, 0x1, 0xCF, 0x60 },
    { 0x180, 0x1, 0x1FF, 0x80 },
    { 0x1C0, 0x80, 0x1DF, 0xAF },
    { 0xD0, 0x1, 0x11F, 0x40 },
    { 0x120, 0x1, 0x17F, 0x50 },
    { 0x0, 0x60, 0x50, 0x100 },
    { 0x50, 0x60, 0x80, 0xC0 },
    { 0x80, 0x60, 0xC0, 0x120 },
    { 0xC0, 0x60, 0x120, 0xC0 },
    { 0x180, 0x80, 0x1B0, 0xD0 },
    { 0x120, 0x70, 0x180, 0xD0 },
    { 0x1B0, 0xB0, 0x1E0, 0xD0 },
    { 0x0, 0x100, 0x80, 0x130 },
    { 0xC0, 0xD0, 0x160, 0x130 },
    { 0xC0, 0x130, 0xE0, 0x160 },
    { 0x130, 0x130, 0x1D0, 0x160 },
    { 0x160, 0xD0, 0x1A0, 0x110 },
    { 0x0, 0x130, 0x40, 0x180 },
    { 0xE0, 0x130, 0x130, 0x1D0 },
    { 0x40, 0x130, 0x80, 0x180 },
    { 0x80, 0x120, 0xC0, 0x180 },
    { 0x0, 0x180, 0x20, 0x1A0 },
};

static unsigned int chz_mrk_dat_lab_up[24] = {
    0x26A1E8C0, 0x2C129801, 0x4C81E302, 0x2B5B2DC3, 0x3BE30F84, 0x4C7B1845,
    0x2983EC06, 0x3BD3ED47, 0x5883E008, 0x37854809, 0x3956198A, 0x42DDF30B,
    0x3286C04C, 0x5A86A74D, 0x8C82E00E, 0x83059C0F, 0x89359C10, 0x9D25D5D1,
    0xAECD2712, 0xAA1C7913, 0xA99C72D4, 0xADBBDB95, 0x29A25956, 0x8385AC56,
};

static struct Chizu_MarkerTex chz_mrk_tex_lab_down[23] = {
    { 0x60, 0x30, 0xE0, 0x60 },
    { 0x0, 0x1, 0x40, 0xA0 },
    { 0x60, 0x1, 0xE0, 0x2F },
    { 0x40, 0x1, 0x60, 0xB0 },
    { 0x0, 0xA0, 0x40, 0xF0 },
    { 0xD0, 0x90, 0x120, 0xF0 },
    { 0x120, 0x90, 0x170, 0xC0 },
    { 0xE0, 0x1, 0x1C0, 0x90 },
    { 0x60, 0x60, 0xD0, 0x80 },
    { 0xA0, 0x80, 0xD0, 0xF0 },
    { 0x120, 0xE0, 0x1AE, 0x170 },
    { 0x0, 0xF0, 0x70, 0x150 },
    { 0x70, 0xC0, 0xA0, 0x140 },
    { 0x170, 0x90, 0x1D0, 0xF0 },
    { 0x170, 0x170, 0x1A0, 0x1FF },
    { 0xA0, 0xF0, 0x100, 0x160 },
    { 0x0, 0x150, 0x80, 0x1C0 },
    { 0x50, 0x1D0, 0x80, 0x200 },
    { 0x80, 0x180, 0xE0, 0x1FF },
    { 0xE0, 0x170, 0x170, 0x1FF },
    { 0x1B0, 0x140, 0x1FF, 0x1FF },
    { 0x1D0, 0x1, 0x1FF, 0x121 },
    { 0x0, 0x1C0, 0x50, 0x1FF },
};

static unsigned int chz_mrk_dat_lab_down[23] = {
    0x4378F840, 0x3F992781, 0x43723402, 0x5E191AC3, 0x4B917B84, 0x4B029C05,
    0x5EC29C86, 0x1B32E807, 0x28640188, 0x38544309, 0x204C1BCA, 0x270D3CCB,
    0x4304DCCC, 0x4734CB4D, 0x4C7D06CE, 0x38DDD00F, 0x3CB63350, 0x5C26B091,
    0x68461092, 0xA7194FD3, 0xB30A5FD4, 0xAB02FC15, 0x9D828256,
};

static struct Chizu_MarkerTex chz_mrk_tex_dl[15] = {
    { 0x24, 0x18, 0x48, 0x30 },
    { 0x48, 0x1, 0x60, 0x24 },
    { 0xC, 0x18, 0x18, 0x24 },
    { 0xC, 0x1, 0x18, 0x18 },
    { 0x0, 0x24, 0x18, 0x30 },
    { 0x0, 0x1, 0xC, 0x24 },
    { 0x0, 0x30, 0x18, 0x48 },
    { 0x18, 0x1, 0x24, 0x18 },
    { 0x24, 0x1, 0x48, 0x19 },
    { 0x18, 0x18, 0x24, 0x3C },
    { 0x24, 0x30, 0x3C, 0x54 },
    { 0x24, 0x30, 0x3C, 0x54 },
    { 0x60, 0x1, 0x77, 0x24 },
    { 0x54, 0x30, 0x6C, 0x54 },
    { 0x0, 0x70, 0x2, 0x72 },
};

static unsigned int chz_mrk_dat_dl[72] = {
    0x4378F841, 0x80040001, 0x26DB8F87, 0x1CCB5947, 0x23D323C0, 0x26DA3E47,
    0x26F9F807, 0x26D9A1C7, 0x29C9FFC0, 0x29D24080, 0x2D427907, 0x298AC340,
    0x298AF880, 0x2D1B5A47, 0x2D43A747, 0x284BCD06, 0x3931A909, 0x35F20080,
    0x35F24340, 0x391B6607, 0x35D4E940, 0x396D5548, 0x3F553407, 0x3C1C5C80,
    0x3BD36400, 0x3F82A5C7, 0x3BF26C80, 0x3F6A6E43, 0x3F619747, 0x40B47E06,
    0x45D47CC6, 0x4AB47DC6, 0x4FA47CC6, 0x55547401, 0x59BC7CC6, 0x5ED47C06,
    0x63C47C86, 0x695C72C1, 0x6E3C7D46, 0x40024C06, 0x452A4C06, 0x4A724C06,
    0x505A4081, 0x548A4C86, 0x59724C06, 0x5ED24C86, 0x64124C06, 0x68EA4C06,
    0x6EDA4201, 0x43A3CEC1, 0x47A3D9C6, 0x4BD3D9C6, 0x4392F706, 0x47B2F706,
    0x4BCAEC41, 0x72152A87, 0x6E645C00, 0x6E836600, 0x6EBA6C00, 0x71F9A087,
    0x74F36640, 0x6BA358C9, 0x20847F48, 0x1CD29801, 0x22EB2307, 0x300A0000,
    0x2584B80C, 0x2CCB180C, 0xB2835C0B, 0xB2834C0D, 0x286B8FCB, 0x28A37DCD,
};

static unsigned int chz_mrk_dat_dl_b[7] = {
    0x2CE60AC7, 0x2CE66707, 0x26E60B07, 0x26E66707, 0x274E9B88, 0x2895D281,
    0x28958A8E,
};

static struct Chizu_MarkerTex chz_mrk_tex_outdoor[13] = {
    { 0x60, 0x50, 0x90, 0x80 },
    { 0x90, 0x40, 0xB0, 0x80 },
    { 0x0, 0xA0, 0x50, 0xD0 },
    { 0x60, 0x80, 0x90, 0xF0 },
    { 0x90, 0x1, 0xB0, 0x41 },
    { 0x60, 0x1, 0x90, 0x51 },
    { 0x0, 0x1, 0x60, 0xA1 },
    { 0xB0, 0x1, 0xC0, 0x31 },
    { 0xB0, 0x40, 0xCF, 0x6F },
    { 0xC0, 0x1, 0xDF, 0x20 },
    { 0xC0, 0x20, 0xF0, 0x40 },
    { 0xD0, 0x40, 0xF0, 0x80 },
    { 0x90, 0x80, 0xFF, 0xB0 },
};

static unsigned int chz_mrk_dat_town_silent[3] = {
    0x6B84F405, 0x6D050C08, 0x4E819C05,
};

static unsigned int chz_mrk_dat_town_e[24] = {
    0x94AB3C0A, 0x29FE9C8B, 0x7293D40A, 0x7296C80A, 0x951DB98A, 0x951E558A,
    0x480B5ECA, 0x477EF08A, 0x24C4C00B, 0x323D24CA, 0x2EAEB007, 0x2A96A880,
    0x32344805, 0x4D159C05, 0x3396D409, 0x7DFC67C9, 0x32CE740A, 0x3BDCA800,
    0x1382500C, 0x1D820C08, 0x5D84B80A, 0x5D25100A, 0x5D25F40A, 0x8805F80A,
};

static unsigned int chz_mrk_dat_town_w[21] = {
    0x8383C000, 0x8383C007, 0x9003C800, 0xC824CEC2, 0xBE01AA46, 0x54895C43,
    0xDE04C80B, 0xDA03D3CB, 0xD882D80B, 0x2480D00B, 0xCD856BCA, 0xCD8727CA,
    0x9293878A, 0xE786AC07, 0xE386AC00, 0x8B03B005, 0x8982BC05, 0x86849405,
    0xC6832409, 0x86E600CB, 0xDC04B800,
};

static struct Chizu_MarkerList chizu_marker_list[36] = {
    { NULL, NULL, 0x0, 0x0 },
    { chz_mrk_tex_outdoor, (int *)chz_mrk_dat_town_silent, 0x529, 0x52D },
    { chz_mrk_tex_outdoor, (int *)chz_mrk_dat_town_e, 0x4E9, 0x502 },
    { chz_mrk_tex_outdoor, (int *)chz_mrk_dat_town_w, 0x508, 0x51E },
    { chz_mrk_tex_apt, (int *)chz_mrk_dat_apart_e1, 0x533, 0x54A },
    { chz_mrk_tex_apt, (int *)chz_mrk_dat_apart_e2, 0x550, 0x574 },
    { chz_mrk_tex_apt, (int *)chz_mrk_dat_apart_e3, 0x576, 0x58B },
    { chz_mrk_tex_apt, (int *)chz_mrk_dat_apart_w1, 0x591, 0x5A3 },
    { chz_mrk_tex_apt, (int *)chz_mrk_dat_apart_w2, 0x5A7, 0x5BC },
    { chz_mrk_tex_apt, (int *)chz_mrk_dat_apart_w1_offsetx, 0x591, 0x5A3 },
    { chz_mrk_tex_apt, (int *)chz_mrk_dat_apart_w2_offsetx, 0x5A7, 0x5BC },
    { chz_mrk_tex_apt, (int *)chz_mrk_dat_apart_e3, 0x576, 0x58B },
    { chz_mrk_tex_hsp, (int *)chz_mrk_dat_hospitalb1f, 0x5C2, 0x5F0 },
    { chz_mrk_tex_hsp, (int *)chz_mrk_dat_hospital23rf, 0x5F6, 0x63E },
    { chz_mrk_tex_hsp, (int *)chz_mrk_dat_hospitalb1b, 0x644, 0x673 },
    { chz_mrk_tex_hsp, (int *)chz_mrk_dat_hospital23rb, 0x677, 0x6AA },
    { chz_mrk_tex_dl, (int *)chz_mrk_dat_dl, 0x6B0, 0x6F9 },
    { chz_mrk_tex_dl, (int *)chz_mrk_dat_dl_b, 0x6FB, 0x703 },
    { chz_mrk_tex_lab_up, (int *)chz_mrk_dat_lab_up, 0x7FB, 0x814 },
    { chz_mrk_tex_lab_down, (int *)chz_mrk_dat_lab_down, 0x815, 0x82D },
    { chz_mrk_tex_htl, (int *)chz_mrk_dat_hotel_gbf, 0x709, 0x70E },
    { chz_mrk_tex_htl, (int *)chz_mrk_dat_hotel_g1f, 0x721, 0x736 },
    { chz_mrk_tex_htl, (int *)chz_mrk_dat_hotel_g2f, 0x75D, 0x77D },
    { chz_mrk_tex_htl, (int *)chz_mrk_dat_hotel_g3f, 0x781, 0x78C },
    { chz_mrk_tex_htl, (int *)chz_mrk_dat_hotel_gbb, 0x792, 0x795 },
    { chz_mrk_tex_htl, (int *)chz_mrk_dat_hotel_g1b, 0x7A7, 0x7B1 },
    { chz_mrk_tex_htl, (int *)chz_mrk_dat_hotel_g2b, 0x7CA, 0x7EB },
    { chz_mrk_tex_htl, (int *)chz_mrk_dat_hotel_g3b, 0x7EF, 0x7F7 },
    { chz_mrk_tex_htl, (int *)chz_mrk_dat_hotel_ebf, 0x712, 0x71E },
    { chz_mrk_tex_htl, (int *)chz_mrk_dat_hotel_e1f, 0x73B, 0x757 },
    { chz_mrk_tex_htl, (int *)chz_mrk_dat_hotel_g2f, 0x75D, 0x77D },
    { chz_mrk_tex_htl, (int *)chz_mrk_dat_hotel_g3f, 0x781, 0x78C },
    { chz_mrk_tex_htl, (int *)chz_mrk_dat_hotel_ebb, 0x799, 0x7A3 },
    { chz_mrk_tex_htl, (int *)chz_mrk_dat_hotel_e1b, 0x7B5, 0x7C6 },
    { chz_mrk_tex_htl, (int *)chz_mrk_dat_hotel_g2b, 0x7CA, 0x7EB },
    { chz_mrk_tex_htl, (int *)chz_mrk_dat_hotel_g3b, 0x7EF, 0x7F7 },
};

static float chizu_center_x;
static float chizu_center_y;
static int chizu_disp_step;
static union fsFileIndex *base_file;
static union fsFileIndex *marker_file;
static union fsFileIndex *cursor_file;
static float disp_rate;
static int chizu_next;
static int chizu_crnt;
static int chizu_from;

/** Map screen main loop, one step per call (Sh2sys.step[3]): picks and loads the map of the
 * current area (a message if James doesn't have it), fades in, runs the display and controls, and
 * fades back out. */
void ChizuMain(void) {
    while (1) {
        switch (Sh2sys.step[3]) {
        default:
        case 0:
            chizu_from = 1;
            Sh2sys.step[3] = 2;
            break;
        case 1:
            chizu_from = 0;
            Sh2sys.step[3] = 2;
            break;
        case 2:
            disp_rate = 0.0f;
            chizu_crnt = 0;
            chizu_next = ChizuSelect();
            chizu_disp_step = 0;
            base_file = NULL;
            marker_file = NULL;
            cursor_file = NULL;
            if (ChizuPossessionCheck(chizu_next)) {
                ChizuFileLoad(chizu_next);
            } else {
                chizu_next = 0;
                if (GAME_FLAG(24)) {
                    ChizuFileLoad(1);
                }
            }
            ScreenEffectFadeStart(1, 0.0f);
            Sh2sys.step[3] = 3;
            break;
        case 3:
            if (!ScreenEffectFadeCheck()) {
                return;
            }
            if (chizu_next == 0) {
                ScreenEffectFadeStart(5, 0.0f);
                fontMessageNum(msg_station, 3);
                Sh2sys.step[3] = 4;
            } else {
                Sh2sys.step[3] = 5;
            }
            break;
        case 4:
            if (fontGetStatus() != -2) {
                return;
            }
            fontClear();
            ScreenEffectFadeStart(3, 0.0f);
            if (chizu_next == 0) {
                if (!GAME_FLAG(24)) {
                    Sh2sys.step[3] = 9;
                } else {
                    chizu_next = 1;
                    Sh2sys.step[3] = 5;
                }
            } else {
                Sh2sys.step[3] = 5;
            }
            break;
        case 5:
            if (fsSync(1, -1) < 0) {
                return;
            }
            if (!ScreenEffectFadeCheck()) {
                return;
            }
            chizu_crnt = chizu_next;
            chizu_next = 0;
            SeCall(0x2B17, 1.0f, 0);
            ScreenEffectFadeStart(4, 0.0f);
            if (chizu_crnt == ChizuSelect()) {
                ChizuCurrentPositionCheck(&chizu_center_x, &chizu_center_y);
                chizu_center_x *= 2.0f;
                chizu_center_y *= 2.0f;
            } else {
                chizu_center_x = 0.0f;
                chizu_center_y = 0.0f;
            }
            Sh2sys.step[3] = 6;
            break;
        case 6:
            ChizuDisplay();
            if (!ScreenEffectFadeCheck()) {
                return;
            }
            Sh2sys.step[3] = 7;
            return;
        case 7:
            ChizuDisplay();
            ChizuControl();
            if (chizu_next) {
                ScreenEffectFadeStart(1, 0.0f);
                Sh2sys.step[3] = 8;
            }
            return;
        case 8:
            if (chizu_next == -1) {
                Sh2sys.step[3] = 9;
            } else {
                ChizuFileLoad(chizu_next);
                Sh2sys.step[3] = 5;
            }
            return;
        case 9:
            if (!ScreenEffectFadeCheck()) {
                return;
            }
            if (chizu_from) {
                Sh2sys.step[2] = 4;
                Sh2sys.step[3] = 0;
                Sh2sys.step[4] = 0;
                Sh2sys.step[5] = 0;
                Sh2sys.step[6] = 0;
                Sh2sys.step[7] = 0;
            } else {
                Sh2sys.step[2] = 6;
                Sh2sys.step[3] = 0;
                Sh2sys.step[4] = 0;
                Sh2sys.step[5] = 0;
                Sh2sys.step[6] = 0;
                Sh2sys.step[7] = 0;
            }
            ScreenEffectFadeStart(4, 0.0f);
            return;
        }
    }
}

/*
 * Matching: the original keeps the room-switch compare constants in a2 (case 0xF included);
 * that comes from the software-double register mode set up by STRIPPED_DOUBLE_CODE() at the top
 * of the file (without it, case 0xF uses a0).
 */
static int ChizuSelect(void) {
    struct SubCharacter *jms;
    int room;
    int work;

    jms = sh2jms.player;
    room = RoomName(0, jms->pos.x, jms->pos.z);
    work = chz_crt_block[room].chizu;
    switch (room) {
    case 0xF:
        if (jms->pos.y > -750.0 || (jms->pos.x < -60000.0f && jms->pos.y > -1000.0f)) {
            work = 4;
        } else if (jms->pos.y > -2350.0 || (jms->pos.x < -60000.0f && jms->pos.y > -2600.0f)) {
            work = 5;
        } else {
            work = 6;
        }
        break;
    case 0x10:
        if (jms->pos.y > -900.0 || (jms->pos.z > -19300.0f && jms->pos.y > -1150.0f)) {
            work = 4;
        } else if (jms->pos.y > -2700.0 || (jms->pos.z > -19300.0f && jms->pos.y > -3450.0f)) {
            work = 5;
        } else {
            work = 6;
        }
        break;
    case 0x11:
        if (jms->pos.y > -900.0 || (jms->pos.z > -59300.0f && jms->pos.y > -1150.0f)) {
            work = 4;
        } else if (jms->pos.y > -2700.0 || (jms->pos.z > -59300.0f && jms->pos.y > -3450.0f)) {
            work = 5;
        } else {
            work = 6;
        }
        break;
    case 0x20:
        if (jms->pos.z < -19200.0f && jms->pos.y > -1050.0f) {
            work = 7;
        } else {
            work = 8;
        }
        break;
    case 0x92:
        if (jms->pos.y > -1400.0f) {
            work = 0x15;
        } else {
            work = 0x16;
        }
        break;
    case 0x21:
        if (jms->pos.y < -855.0f || (jms->pos.z > -99431.0f && jms->pos.y < -854.0f)) {
            work = 8;
        } else {
            work = 7;
        }
        break;
    case 0x29:
        if (jms->pos.y > -950.0f || (jms->pos.z > -100000.0f && jms->pos.y > -1200.0f)) {
            work = 0xC;
        } else {
            work = 0xD;
        }
        break;
    case 0x2A:
        if (GAME_FLAG(17)) {
            work = 0xC;
        }
        if (GAME_FLAG(18) || GAME_FLAG(19)) {
            work = 0xD;
        }
        break;
    case 0x46:
        if (GAME_FLAG(17)) {
            work = 0xE;
        }
        if (GAME_FLAG(18) || GAME_FLAG(19)) {
            work = 0xF;
        }
        break;
    case 0x91:
    case 0xAB:
        if (jms->pos.y < -920.0f || (jms->pos.z < -20020.0f && jms->pos.y < -874.0f)) {
            work = 0x1D;
        } else {
            work = 0x1C;
        }
        break;
    case 0x47:
        if (jms->pos.y >= -1000.0f || (jms->pos.z > -219978.0f && jms->pos.y > -1001.0f)) {
            work = 0xE;
        } else {
            work = 0xF;
        }
        break;
    case 0x48:
        if (jms->pos.y > -1000.0f ||
            (jms->pos.y < -1001.0f && jms->pos.z > -99992.0f && jms->pos.y >= -1000.0f)) {
            work = 0xC;
        } else {
            work = 0xF;
        }
        break;
    }
    if (GAME_FLAG(25) && GAME_FLAG(26)) {
        if (4 <= work && work < 7) {
            work += 5;
        } else if (7 <= work && work < 9) {
            work += 2;
        }
    }
    if (GAME_FLAG(31)) {
        if (20 <= work && work < 28) {
            work += 8;
        }
    }
    if (work == 17) {
        work = 16;
    }
    return work;
}

static int ChizuPossessionCheck(int chizu) {
    switch (chizu) {
    default:
        return 1;
    case 1:
    case 2:
    case 3:
        if (GAME_FLAG(24)) {
            return 1;
        }
        break;
    case 4:
    case 5:
    case 6:
        if (GAME_FLAG(25)) {
            return 1;
        }
        break;
    case 9:
    case 10:
    case 11:
        if (GAME_FLAG(26) && GAME_FLAG(25)) {
            return 1;
        }
        break;
    case 7:
    case 8:
        if (GAME_FLAG(26)) {
            return 1;
        }
        break;
    case 14:
    case 15:
        if (!GAME_FLAG(215)) {
            break;
        }
    case 12:
    case 13:
        if (GAME_FLAG(27)) {
            return 1;
        }
        break;
    case 16:
    case 17:
        if (GAME_FLAG(28)) {
            return 1;
        }
        break;
    case 18:
    case 19:
        if (GAME_FLAG(29)) {
            return 1;
        }
        break;
    case 24:
    case 25:
    case 26:
    case 27:
        if (!GAME_FLAG(476)) {
            break;
        }
        if (GAME_FLAG(30)) {
            return 1;
        }
    case 20:
    case 21:
    case 22:
    case 23:
        if (GAME_FLAG(30)) {
            return 1;
        }
    case 28:
    case 29:
    case 30:
    case 31:
        if (GAME_FLAG(31)) {
            return 1;
        }
        break;
    case 32:
    case 33:
    case 34:
    case 35:
        if (GAME_FLAG(476)) {
            if (GAME_FLAG(31)) {
                return 1;
            }
        }
        break;
    case 0:
        break;
    }
    return 0;
}

static void ChizuFileLoad(int load_chizu) {
    static union fsFileIndex *base_file_list[36] = {
    NULL,
    data_pic_map_outmap_tex,
    data_pic_map_outmape_tex,
    data_pic_map_outmapw_tex,
    data_pic_map_apartmape1f_tex,
    data_pic_map_apartmape2f_tex,
    data_pic_map_apartmape3f_tex,
    data_pic_map_apartmapw_tex,
    data_pic_map_apartmapw_tex,
    data_pic_map_apartmapew1f_tex,
    data_pic_map_apartmapew2f_tex,
    data_pic_map_apartmapew3f_tex,
    data_pic_map_hospitalmap01_tex,
    data_pic_map_hospitalmap02_tex,
    data_pic_map_hospitalmap01_tex,
    data_pic_map_hospitalmap02_tex,
    data_pic_map_prisonmap03_tex,
    data_pic_map_prisonmap03_tex,
    data_pic_map_prisonmap_tex,
    data_pic_map_prisonmap_tex,
    data_pic_map_hotelmapbf01_tex,
    data_pic_map_hotelmap1f01_tex,
    data_pic_map_hotelmap2f01_tex,
    data_pic_map_hotelmap3f01_tex,
    data_pic_map_hotelmapbf01_tex,
    data_pic_map_hotelmap1f01_tex,
    data_pic_map_hotelmap2f01_tex,
    data_pic_map_hotelmap3f01_tex,
    data_pic_map_hotelmapbf02_tex,
    data_pic_map_hotelmap1f02_tex,
    data_pic_map_hotelmap2f02_tex,
    data_pic_map_hotelmap3f02_tex,
    data_pic_map_hotelmapbf02_tex,
    data_pic_map_hotelmap1f02_tex,
    data_pic_map_hotelmap2f02_tex,
    data_pic_map_hotelmap3f02_tex,
};

    static union fsFileIndex *marker_file_list[36] = {
    NULL,
    data_pic_map_outmapmark_tex,
    data_pic_map_outmapmark_tex,
    data_pic_map_outmapmark_tex,
    data_pic_map_apartmapmark_tex,
    data_pic_map_apartmapmark_tex,
    data_pic_map_apartmapmark_tex,
    data_pic_map_apartmapmark_tex,
    data_pic_map_apartmapmark_tex,
    data_pic_map_apartmapmark_tex,
    data_pic_map_apartmapmark_tex,
    data_pic_map_apartmapmark_tex,
    data_pic_map_hospitalmapmk_tex,
    data_pic_map_hospitalmapmk_tex,
    data_pic_map_hospitalmapmk_tex,
    data_pic_map_hospitalmapmk_tex,
    data_pic_map_prisonmap03mark_tex,
    data_pic_map_prisonmap03mark_tex,
    data_pic_map_prisonmap01mark_tex,
    data_pic_map_prisonmap02mark_tex,
    data_pic_map_hotelmapmark_tex,
    data_pic_map_hotelmapmark_tex,
    data_pic_map_hotelmapmark_tex,
    data_pic_map_hotelmapmark_tex,
    data_pic_map_hotelmapmark_tex,
    data_pic_map_hotelmapmark_tex,
    data_pic_map_hotelmapmark_tex,
    data_pic_map_hotelmapmark_tex,
    data_pic_map_hotelmapmark_tex,
    data_pic_map_hotelmapmark_tex,
    data_pic_map_hotelmapmark_tex,
    data_pic_map_hotelmapmark_tex,
    data_pic_map_hotelmapmark_tex,
    data_pic_map_hotelmapmark_tex,
    data_pic_map_hotelmapmark_tex,
    data_pic_map_hotelmapmark_tex,
};

    if (base_file_list[load_chizu] != NULL && base_file != base_file_list[load_chizu]) {
        FcRead(base_file_list[load_chizu], get_gp_data_buf_addr());
        base_file = base_file_list[load_chizu];
    }
    if (marker_file_list[load_chizu] != NULL && marker_file != marker_file_list[load_chizu]) {
        layer_adr = get_gp_data_buf_addr() + 0x104800;
        FcRead(marker_file_list[load_chizu], layer_adr);
        marker_file = marker_file_list[load_chizu];
    }
}

static void ChizuDisplay(void) {
    struct PicDraw_Data pic0;
    float u0;
    float v0;
    float u1;
    float v1;

    if (chizu_disp_step == 1) {
        disp_rate += shGetDT();
        if (disp_rate > 1.0f) {
            disp_rate = 1.0f;
            chizu_disp_step = 2;
        }
    } else if (chizu_disp_step == 3) {
        disp_rate -= shGetDT();
        if (disp_rate < 0.0f) {
            disp_rate = 0.0f;
            chizu_disp_step = 0;
        }
    }
    u0 = (256.0f + chizu_center_x) * disp_rate;
    v0 = (256.0f + chizu_center_y) * disp_rate;
    u1 = u0 + 512.0f * (2.0f - disp_rate);
    v1 = v0 + 512.0f * (2.0f - disp_rate);
    if (u0 < 0.0f) {
        u0 = 0.0f;
    }
    if (v0 < 0.0f) {
        v0 = 0.0f;
    }
    if (u1 > 1023.0f) {
        u1 = 1023.0f;
    }
    if (v1 > 1023.0f) {
        v1 = 1023.0f;
    }
    spkResetOT();
    PictureLoadImage((struct sh2gfw_AREA_HEAD *)get_gp_data_buf_addr(), 0, -1, -1);
    shQzero(&pic0, sizeof(pic0));
    pic0.ap = (struct sh2gfw_AREA_HEAD *)get_gp_data_buf_addr();
    pic0.tex = -1;
    pic0.clut = -1;
    pic0.status |= 1;
    pic0.otp = 1;
    pic0.us0 = ftoi4(u0);
    pic0.vt0 = ftoi4(v0);
    pic0.us1 = ftoi4(u1);
    pic0.vt1 = ftoi4(v1);
    pic0.status |= 4;
    PictureDraw(&pic0);
    ChizuMarkerDraw();
    ChizuConnectArrowDraw();
    if (chizu_crnt == ChizuSelect()) {
        ChizuCurrentPositionDraw();
    }
    d1cSend(spkDmaKick());
}

static void ChizuMarkerDraw(void) {
    struct Chizu_MarkerList *list[2];
    struct Chizu_MarkerList *lp;
    struct Chizu_MarkerTex *tp;
    int *dp;
    struct PicDraw_Data pic1;
    float x0;
    float y0;
    float x1;
    float y1;
    float rate;
    int i;
    int j;

    if (chizu_crnt == 0) {
        return;
    }
    PictureLoadImage((struct sh2gfw_AREA_HEAD *)layer_adr, 2, -1, -1);
    shQzero(&pic1, sizeof(pic1));
    pic1.ap = (struct sh2gfw_AREA_HEAD *)layer_adr;
    pic1.tex = -1;
    pic1.clut = -1;
    pic1.status |= 1;
    pic1.otp = 3;
    pic1.a = 0x80;
    pic1.alpha_a = 0;
    pic1.alpha_b = 1;
    pic1.alpha_c = 0;
    pic1.alpha_d = 1;
    pic1.alpha_fix = 0x80;
    pic1.status |= 0x20;
    switch (chizu_crnt) {
    case 7:
    case 8:
        list[0] = &chizu_marker_list[7];
        list[1] = &chizu_marker_list[8];
        break;
    case 9:
        list[0] = &chizu_marker_list[4];
        list[1] = &chizu_marker_list[9];
        break;
    case 0xA:
        list[0] = &chizu_marker_list[5];
        list[1] = &chizu_marker_list[0xA];
        break;
    case 0x1C:
        list[0] = &chizu_marker_list[0x1C];
        list[1] = &chizu_marker_list[0x14];
        break;
    case 0x1D:
        list[0] = &chizu_marker_list[0x1D];
        list[1] = &chizu_marker_list[0x15];
        break;
    case 0x20:
        list[0] = &chizu_marker_list[0x20];
        list[1] = &chizu_marker_list[0x18];
        break;
    case 0x21:
        list[0] = &chizu_marker_list[0x21];
        list[1] = &chizu_marker_list[0x19];
        break;
    case 0x10:
    case 0x11:
        list[0] = &chizu_marker_list[0x10];
        list[1] = &chizu_marker_list[0x11];
        break;
    case 1:
        list[0] = &chizu_marker_list[1];
        list[1] = NULL;
    default:
        list[0] = &chizu_marker_list[chizu_crnt];
        list[1] = NULL;
        break;
    }
    for (i = 0; i < 2; i++) {
        lp = list[i];
        if (lp == NULL || lp->head == 0) {
            continue;
        }
        for (j = 1; j < lp->tail - lp->head; j++) {
            if (!GAME_FLAG(j + lp->head)) {
                continue;
            }
            dp = lp->data + j - 1;
            tp = &lp->tex[*dp & 0x3F];
            x0 = itof4(((*dp >> 19) & 0x1FFF) - 0x1000);
            y0 = itof4(((*dp >> 6) & 0x1FFF) - 0x1000);
            rate = disp_rate / (2.0f - disp_rate);
            x0 = x0 + rate * (x0 - chizu_center_x);
            y0 = y0 + rate * (y0 - chizu_center_y);
            x1 = x0 + (1.0f + rate) * itof(tp->u1 - tp->u0) / 2.0f;
            y1 = y0 + (1.0f + rate) * itof(tp->v1 - tp->v0) / 2.0f;
            pic1.x0 = ftoi4(x0);
            pic1.y0 = ftoi4(y0);
            pic1.x1 = ftoi4(x1);
            pic1.y1 = ftoi4(y1);
            pic1.status |= 2;
            pic1.us0 = tp->u0 << 4;
            pic1.vt0 = tp->v0 << 4;
            pic1.us1 = tp->u1 << 4;
            pic1.vt1 = tp->v1 << 4;
            pic1.status |= 4;
            PictureDraw(&pic1);
        }
    }
}

static void ChizuControl(void) {
    float anx;
    float any;
    unsigned char lsx;
    unsigned char lsy;

    if (chizu_disp_step == 0) {
        if (shPadTrigger(0, key_config.enter)) {
            chizu_disp_step = 1;
        } else if (shPadTrigger(0, 0x400)) {
            chizu_next = ChizuConnectCheck(chizu_crnt, 0);
        } else if (shPadTrigger(0, 0x800)) {
            chizu_next = ChizuConnectCheck(chizu_crnt, 1);
        } else if (shPadTrigger(0, 0x100)) {
            chizu_next = ChizuConnectCheck(chizu_crnt, 2);
        } else if (shPadTrigger(0, 0x200)) {
            chizu_next = ChizuConnectCheck(chizu_crnt, 3);
        }
    } else if (shPadTrigger(0, key_config.enter)) {
        if (chizu_disp_step == 3) {
            chizu_disp_step = 1;
        } else {
            chizu_disp_step = 3;
        }
    } else {
        lsx = shPadPress(0, 0x40);
        if (lsx >= 0x9B) {
            anx = (lsx - 0x9B) / 100.0f;
        } else if (lsx <= 0x64) {
            anx = (lsx - 0x64) / 100.0f;
        } else {
            anx = 0.0f;
        }
        lsy = shPadPress(0, 0x80);
        if (lsy >= 0x9B) {
            any = (lsy - 0x9B) / 100.0f;
        } else if (lsy <= 0x64) {
            any = (lsy - 0x64) / 100.0f;
        } else {
            any = 0.0f;
        }
        if (anx != 0.0f || any != 0.0f) {
            chizu_center_x += 307.2f * anx * shGetDT();
            chizu_center_y += 384.0f * any * shGetDT();
        } else {
            if (shPadPress(0, 0x100)) {
                chizu_center_x += 307.2f * shGetDT();
            } else if (shPadPress(0, 0x200)) {
                chizu_center_x -= 307.2f * shGetDT();
            }
            if (shPadPress(0, 0x400)) {
                chizu_center_y -= 384.0f * shGetDT();
            } else if (shPadPress(0, 0x800)) {
                chizu_center_y += 384.0f * shGetDT();
            }
        }
        if (chizu_center_x > 256.0f) {
            chizu_center_x = 256.0f;
        }
        if (chizu_center_x < -256.0f) {
            chizu_center_x = -256.0f;
        }
        if (chizu_center_y > 256.0f) {
            chizu_center_y = 256.0f;
        }
        if (chizu_center_y < -256.0f) {
            chizu_center_y = -256.0f;
        }
    }
    if (shPadTrigger(0, key_config.cancel) || shPadTrigger(0, key_config.map)) {
        chizu_next = -1;
    }
}

static void ChizuConnectArrowDraw(void) {
    static unsigned char tex[4][4] = {
    { 0x60, 0x0, 0x9F, 0x3F },
    { 0xA0, 0x0, 0xDF, 0x3F },
    { 0x30, 0x0, 0x5F, 0x5F },
    { 0x0, 0x0, 0x2F, 0x5F },
};

    static float def_pos[4][2] = {
    { -36.0f, -224.0f },
    { -36.0f, 176.0f },
    { 212.0f, 0.0f },
    { -244.0f, 0.0f },
};

    struct PicDraw_Data pic2;
    float rate;
    float x0;
    float y0;
    float x1;
    float y1;
    unsigned char alpha;
    int work;
    int i;

    PictureLoadImage((struct sh2gfw_AREA_HEAD *)cursor_adr, 4, -1, -1);
    shQzero(&pic2, sizeof(pic2));
    pic2.ap = (struct sh2gfw_AREA_HEAD *)cursor_adr;
    pic2.tex = -1;
    pic2.clut = -1;
    pic2.status |= 1;
    pic2.otp = 5;
    if (disp_rate > 0.5f) {
        alpha = 0;
    } else {
        alpha = 0x80 - ftoi(256.0f * disp_rate);
    }
    pic2.a = alpha;
    pic2.alpha_a = 0;
    pic2.alpha_b = 1;
    pic2.alpha_c = 0;
    pic2.alpha_d = 1;
    pic2.alpha_fix = 0x80;
    pic2.status |= 0x20;
    for (i = 0; i < 4; i++) {
        switch (i) {
        default:
        case 0:
            work = ChizuConnectCheck(chizu_crnt, 0);
            break;
        case 1:
            work = ChizuConnectCheck(chizu_crnt, 1);
            break;
        case 2:
            work = ChizuConnectCheck(chizu_crnt, 2);
            break;
        case 3:
            work = ChizuConnectCheck(chizu_crnt, 3);
            break;
        }
        if (work == 0) {
            continue;
        }
        switch (chizu_crnt * 4 + i) {
        case 0x35:
            x0 = -240.0f;
            y0 = 172.0f;
            break;
        case 0x29:
        case 0x2D:
            x0 = -216.0f;
            y0 = 151.0f;
            break;
        default:
            x0 = def_pos[i][0];
            y0 = def_pos[i][1];
            break;
        }
        rate = disp_rate / (2.0f - disp_rate);
        x0 = x0 + rate * (x0 - chizu_center_x);
        y0 = y0 + rate * (y0 - chizu_center_y);
        x1 = x0 + (1.0f + rate) * itof(tex[i][2] - tex[i][0]);
        y1 = y0 + (1.0f + rate) * (tex[i][3] - tex[i][1]);
        pic2.x0 = ftoi4(x0);
        pic2.y0 = ftoi4(y0);
        pic2.x1 = ftoi4(x1);
        pic2.y1 = ftoi4(y1);
        pic2.status |= 2;
        pic2.us0 = tex[i][0] << 4;
        pic2.vt0 = tex[i][1] << 4;
        pic2.us1 = tex[i][2] << 4;
        pic2.vt1 = tex[i][3] << 4;
        pic2.status |= 4;
        PictureDraw(&pic2);
    }
}

static int ChizuConnectCheck(int chizu, int connect) {
    int work;

    switch (connect) {
    default:
    case 0:
        work = chz_connect[chizu].up;
        break;
    case 1:
        work = chz_connect[chizu].down;
        break;
    case 2:
        work = chz_connect[chizu].right;
        break;
    case 3:
        work = chz_connect[chizu].left;
        break;
    }
    if (work == 0) {
        return 0;
    }
    if (connect == 2 || connect == 3) {
        switch (work) {
        case 1:
            if (chizu_crnt > 0 && chizu_crnt < 4) {
                return 0;
            }
            break;
        case 9:
            if (4 <= chizu_crnt && chizu_crnt < 12) {
                return 0;
            }
            break;
        case 12:
            if (12 <= chizu_crnt && chizu_crnt < 16) {
                return 0;
            }
            break;
        case 16:
            if (16 <= chizu_crnt && chizu_crnt < 20) {
                return 0;
            }
            break;
        case 21:
            if (20 <= chizu_crnt && chizu_crnt < 24) {
                return 0;
            }
            break;
        case 29:
            if (28 <= chizu_crnt && chizu_crnt < 32) {
                return 0;
            }
            break;
        }
        if (ChizuPossessionCheck(work)) {
            return work;
        }
        if (work == 9) {
            if (ChizuPossessionCheck(4)) {
                return 4;
            }
            if (ChizuPossessionCheck(7)) {
                return 7;
            }
        } else if (work == 16) {
            if (ChizuPossessionCheck(18)) {
                return 18;
            }
        } else if (work == 29) {
            if (ChizuPossessionCheck(21)) {
                return 21;
            }
        }
        return ChizuConnectCheck(work, connect);
    }
    return ChizuPossessionCheck(work) ? work : 0;
}

static void ChizuCurrentPositionDraw(void) {
    static short uv[2][4] = {
        { 0x900, 0x400, 0xAF0, 0x7F0 },
        { 0xB00, 0x400, 0xCF0, 0x7F0 },
    };
    struct PicDraw_Data pic3;
    float cosrot;
    float sinrot;
    float pos[2][4][2];
    float rate;
    float px;
    float py;
    int i;
    int j;

    if (chizu_crnt == 1) {
        switch (RoomNameJms()) {
        case 1:
        case 2:
        case 3:
            break;
        default:
            return;
        }
    }
    ChizuCurrentPositionCheck(&px, &py);
    if (chizu_crnt == 0x10 || chizu_crnt == 0x11) {
        sinrot = shSinF(sh2jms.player->rot.y - 1.5707964f);
        cosrot = shCosF(sh2jms.player->rot.y - 1.5707964f);
    } else if (chizu_crnt == 0x12 || chizu_crnt == 0x13) {
        sinrot = shSinF(1.5707964f + sh2jms.player->rot.y);
        cosrot = shCosF(1.5707964f + sh2jms.player->rot.y);
    } else {
        sinrot = shSinF(sh2jms.player->rot.y);
        cosrot = shCosF(sh2jms.player->rot.y);
    }
    pos[0][0][0] = -10.0f * cosrot - -16.0f * sinrot;
    pos[0][0][1] = -10.0f * sinrot + -16.0f * cosrot;
    pos[0][1][0] = 10.0f * cosrot - -16.0f * sinrot;
    pos[0][1][1] = 10.0f * sinrot + -16.0f * cosrot;
    pos[0][2][0] = -10.0f * cosrot - 16.0f * sinrot;
    pos[0][2][1] = -10.0f * sinrot + 16.0f * cosrot;
    pos[0][3][0] = 10.0f * cosrot - 16.0f * sinrot;
    pos[0][3][1] = 10.0f * sinrot + 16.0f * cosrot;
    for (i = 0; i < 4; i++) {
        pos[1][i][0] = 2.0f + pos[0][i][0];
        pos[1][i][1] = 0.5f + pos[0][i][1];
    }
    rate = disp_rate / (2.0f - disp_rate);
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 4; j++) {
            pos[i][j][0] *= 0.8f;
            pos[i][j][0] += px;
            pos[i][j][1] += py;
            pos[i][j][0] = pos[i][j][0] + rate * (pos[i][j][0] - chizu_center_x);
            pos[i][j][1] = pos[i][j][1] + rate * (pos[i][j][1] - chizu_center_y);
        }
    }
    PictureLoadImage((struct sh2gfw_AREA_HEAD *)cursor_adr, 6, -1, -1);
    shQzero(&pic3, sizeof(pic3));
    pic3.ap = (struct sh2gfw_AREA_HEAD *)cursor_adr;
    pic3.tex = -1;
    pic3.clut = -1;
    pic3.status |= 1;
    pic3.a = 0x80;
    pic3.alpha_a = 0;
    pic3.alpha_b = 1;
    pic3.alpha_c = 0;
    pic3.alpha_d = 1;
    pic3.alpha_fix = 0x80;
    pic3.status |= 0x20;
    for (i = 0; i < 2; i++) {
        pic3.otp = 8 - i;
        pic3.x0 = ftoi4(pos[i][0][0]);
        pic3.y0 = ftoi4(pos[i][0][1]);
        pic3.x1 = ftoi4(pos[i][1][0]);
        pic3.y1 = ftoi4(pos[i][1][1]);
        pic3.status |= 2;
        pic3.x2 = ftoi4(pos[i][2][0]);
        pic3.y2 = ftoi4(pos[i][2][1]);
        pic3.x3 = ftoi4(pos[i][3][0]);
        pic3.y3 = ftoi4(pos[i][3][1]);
        pic3.status |= 0x80;
        pic3.us0 = uv[i][0];
        pic3.vt0 = uv[i][1];
        pic3.us1 = uv[i][2];
        pic3.vt1 = uv[i][3];
        pic3.status |= 4;
        PictureDraw(&pic3);
    }
}

static void ChizuCurrentPositionCheck(float *px, float *py) {
    struct SubCharacter *jms;
    int room;
    float tmp;

    room = RoomNameJms();
    jms = sh2jms.player;
    switch (RoomNameJms()) {
    default:
        *px = 0.00142f * jms->pos.x + chz_crt_block[room].cp_x;
        *py = chz_crt_block[room].cp_y - 0.00196f * jms->pos.z;
        break;
    case 0x3:
        *px = 0.00053 * jms->pos.x + chz_crt_block[room].cp_x;
        *py = chz_crt_block[room].cp_y - 0.00079 * jms->pos.z;
        break;
    case 0x1:
    case 0x2:
    case 0x5:
    case 0x6:
    case 0x9:
    case 0xA:
    case 0xB:
    case 0xC:
    case 0xD:
        *px = chz_crt_block[room].cp_x;
        *py = chz_crt_block[room].cp_y;
        break;
    case 0x8:
        if (jms->pos.x > 30453.578906 && jms->pos.x < 35960.0f && jms->pos.z >= -60828.847656 &&
            jms->pos.z <= -59016.0f) {
            *px = 212.4619f;
            *py = 183.76071f;
        } else {
            *px = 0.00146f * jms->pos.x + chz_crt_block[room].cp_x;
            *py = chz_crt_block[room].cp_y - 0.00196f * jms->pos.z;
        }
        break;
    case 0x78:
    case 0x79:
    case 0x7A:
    case 0x7B:
    case 0x7C:
    case 0x7D:
    case 0x7E:
    case 0x7F:
    case 0x80:
    case 0x81:
    case 0x82:
    case 0x83:
    case 0x84:
    case 0x85:
    case 0x86:
    case 0x87:
    case 0x88:
    case 0x89:
    case 0x8A:
    case 0x8B:
    case 0x8C:
    case 0x8D:
        *px = 0.01007f * jms->pos.x + chz_crt_block[room].cp_x;
        *py = chz_crt_block[room].cp_y - 0.0089f * jms->pos.z;
        tmp = *px;
        *px = -*py;
        *py = tmp;
        break;
    case 0x66:
    case 0x67:
    case 0x68:
    case 0x69:
    case 0x6A:
    case 0x6B:
    case 0x6C:
    case 0x6D:
    case 0x6E:
    case 0x6F:
    case 0x70:
    case 0x71:
    case 0x73:
    case 0x74:
        *px = 0.01377f * jms->pos.x + chz_crt_block[room].cp_x;
        *py = chz_crt_block[room].cp_y - 0.010408f * jms->pos.z;
        tmp = *px;
        *px = *py;
        *py = -tmp;
        break;
    case 0x72:
        *px = 0.01377f * jms->pos.x + chz_crt_block[room].cp_x;
        *py = chz_crt_block[room].cp_y - 0.009988f * jms->pos.z;
        tmp = *px;
        *px = *py;
        *py = -tmp;
        break;
    case 0x20:
    case 0x21:
    case 0x22:
    case 0x23:
    case 0x24:
    case 0x27:
    case 0x28:
        *px = 0.0087f * jms->pos.x + chz_crt_block[room].cp_x;
        *py = chz_crt_block[room].cp_y - 0.0096f * jms->pos.z;
        if (!GAME_FLAG(25) && GAME_FLAG(26) == 1) {
            switch (chizu_crnt) {
            case 7:
                *px += 24.965286f;
                break;
            case 8:
                *px += 164.96529f;
                break;
            }
        }
        break;
    case 0x25:
    case 0x26:
        *px = 0.00801f * jms->pos.x + chz_crt_block[room].cp_x;
        *py = chz_crt_block[room].cp_y - 0.01037f * jms->pos.z;
        if (!GAME_FLAG(25) && GAME_FLAG(26) == 1) {
            switch (chizu_crnt) {
            case 7:
                *px += 16.965286f;
                break;
            case 8:
                *px += 158.96529f;
                break;
            }
        }
        break;
    case 0xF:
    case 0x10:
    case 0x11:
    case 0x13:
    case 0x14:
    case 0x16:
    case 0x17:
    case 0x18:
    case 0x19:
    case 0x1A:
    case 0x1B:
    case 0x1C:
    case 0x1D:
    case 0x1E:
        *px = 0.0087f * jms->pos.x + chz_crt_block[room].cp_x;
        *py = chz_crt_block[room].cp_y - 0.0096f * jms->pos.z;
        break;
    case 0x7:
    case 0x12:
    case 0x15:
    case 0x1F:
        *px = 0.00801f * jms->pos.x + chz_crt_block[room].cp_x;
        *py = chz_crt_block[room].cp_y - 0.01037f * jms->pos.z;
        break;
    case 0x92:
    case 0xA2:
    case 0xAC:
        *px = 0.0164f * jms->pos.x + chz_crt_block[room].cp_x;
        *py = chz_crt_block[room].cp_y - 0.0209f * jms->pos.z;
        break;
    case 0x99:
    case 0x9A:
    case 0x9F:
    case 0xA0:
    case 0xA1:
    case 0xA3:
    case 0xA8:
    case 0xA9:
    case 0xAD:
    case 0xAE:
    case 0xAF:
    case 0xB1:
    case 0xB2:
    case 0xB5:
    case 0xB8:
    case 0xB9:
        *px = 0.01607f * jms->pos.x + chz_crt_block[room].cp_x;
        *py = chz_crt_block[room].cp_y - 0.0207f * jms->pos.z;
        break;
    case 0x91:
    case 0x93:
    case 0x94:
    case 0x95:
    case 0x96:
    case 0x97:
    case 0x98:
    case 0x9B:
    case 0x9C:
    case 0x9D:
    case 0x9E:
    case 0xA5:
    case 0xA6:
    case 0xA7:
    case 0xAB:
    case 0xB0:
    case 0xB6:
    case 0xB7:
        *px = 0.0164f * jms->pos.x + chz_crt_block[room].cp_x;
        *py = chz_crt_block[room].cp_y - 0.01875f * jms->pos.z;
        break;
    case 0x2B:
    case 0x2C:
    case 0x2D:
    case 0x2E:
    case 0x2F:
    case 0x31:
    case 0x32:
    case 0x33:
    case 0x34:
    case 0x35:
    case 0x36:
    case 0x37:
    case 0x38:
    case 0x39:
    case 0x3A:
    case 0x3C:
    case 0x3D:
    case 0x3E:
    case 0x3F:
    case 0x40:
    case 0x41:
    case 0x42:
    case 0x43:
    case 0x44:
    case 0x45:
    case 0x49:
    case 0x4A:
    case 0x4B:
    case 0x4C:
    case 0x4D:
    case 0x4E:
    case 0x4F:
    case 0x50:
    case 0x51:
    case 0x52:
    case 0x53:
    case 0x54:
    case 0x55:
    case 0x56:
    case 0x57:
    case 0x58:
    case 0x59:
        *px = 0.01477f * jms->pos.x + chz_crt_block[room].cp_x;
        *py = chz_crt_block[room].cp_y - 0.019658f * jms->pos.z;
        break;
    case 0x3B:
        *px = 0.014716f * jms->pos.x + chz_crt_block[room].cp_x;
        *py = chz_crt_block[room].cp_y - 0.019583f * jms->pos.z;
        break;
    case 0x29:
        if (jms->pos.y > -950.0f || (jms->pos.z > -100000.0f && jms->pos.y > -1200.0f)) {
            *px = 879.80005f + 0.013396f * jms->pos.x;
            *py = -1988.8f - 0.019375f * jms->pos.z;
        } else if (jms->pos.y > -2950.0f || (jms->pos.z > -100000.0f && jms->pos.y > -3200.0f)) {
            *px = 916.2002f + 0.013396f * jms->pos.x;
            *py = -1847.7001f - 0.019375f * jms->pos.z;
        } else if (jms->pos.y > -4950.0f || (jms->pos.z > -100000.0f && jms->pos.y > -5200.0f)) {
            *px = 916.40015f + 0.013396f * jms->pos.x;
            *py = -2086.5996f - 0.019375f * jms->pos.z;
        } else {
            *px = 631.7003f + 0.013396f * jms->pos.x;
            *py = -1956.7996f - 0.019375f * jms->pos.z;
        }
        break;
    case 0x2A:
        if (GAME_FLAG(19)) {
            *px = 1421.101f + 0.01477f * sh2jms.player->pos.x;
            *py = 0.019658f * -sh2jms.player->pos.z - 531.00037f;
        } else if (GAME_FLAG(18)) {
            *px = 1421.101f + 0.01477f * sh2jms.player->pos.x;
            *py = 0.019658f * -sh2jms.player->pos.z - 289.90015f;
        } else if (GAME_FLAG(17)) {
            *px = 1385.2999f + 0.01477f * sh2jms.player->pos.x;
            *py = 0.019658f * -sh2jms.player->pos.z - 426.99988f;
        }
        break;
    case 0x46:
        if (GAME_FLAG(19)) {
            *px = 240.9f + 0.01477f * sh2jms.player->pos.x;
            *py = 0.019658f * -sh2jms.player->pos.z - 6029.8965f;
        } else if (GAME_FLAG(18)) {
            *px = 238.69986f + 0.01477f * sh2jms.player->pos.x;
            *py = 0.019658f * -sh2jms.player->pos.z - 5789.196f;
        } else if (GAME_FLAG(17)) {
            *px = 203.69986f + 0.01477f * sh2jms.player->pos.x;
            *py = 0.019658f * -sh2jms.player->pos.z - 5931.196f;
        }
        break;
    case 0x48:
        if (jms->pos.y >= 1700.0f) {
            *px = 0.0f;
            *py = -1000.0f;
        } else if (jms->pos.y > -1000.0f ||
                   (jms->pos.y < -1001.0f && jms->pos.z > -99992.0f && jms->pos.y >= -1000.0f)) {
            *px = 1404.0f + 0.01477f * sh2jms.player->pos.x;
            *py = 0.019658f * -sh2jms.player->pos.z - 4336.0f;
        } else if (jms->pos.y > -3000.0f ||
                   (jms->pos.y < -3001.0f && jms->pos.z > -100009.0f && jms->pos.y >= -3000.0f)) {
            *px = 1438.0f + 0.01477f * sh2jms.player->pos.x;
            *py = 0.019658f * -sh2jms.player->pos.z - 4191.0f;
        } else {
            *px = 1438.0f + 0.01477f * sh2jms.player->pos.x;
            *py = 6.0f + (0.019658f * -sh2jms.player->pos.z - 4432.0f);
        }
        break;
    case 0x47:
        if (jms->pos.y >= 1001.0f || (jms->pos.y >= 1000.0f && jms->pos.z > -220067.0f)) {
            *px = 878.0f + 0.013396f * jms->pos.x;
            *py = -31.899994f - 0.019375f * jms->pos.z;
        } else if (jms->pos.y >= -1000.0f || (jms->pos.z > -219978.0f && jms->pos.y > -1001.0f)) {
            *px = 878.80005f + 0.013396f * jms->pos.x;
            *py = -211.9f - 0.019375f * jms->pos.z;
        } else if (jms->pos.y >= -2998.0f || (jms->pos.z > -220003.0f && jms->pos.y > -3000.0f)) {
            *px = 915.0f + 0.013396f * jms->pos.x;
            *py = -73.899994f - 0.019375f * jms->pos.z;
        } else if (jms->pos.y > -5000.0f || (jms->pos.z > -220011.0f && jms->pos.y >= -5000.0f)) {
            *px = 915.0f + 0.013396f * jms->pos.x;
            *py = -313.9f - 0.019375f * jms->pos.z;
        } else {
            *px = 630.0f + 0.013396f * jms->pos.x;
            *py = -183.9f - 0.019375f * jms->pos.z;
        }
        break;
    }
}
