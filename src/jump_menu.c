/*
 * jump_menu.c: the debug "jump menu" -- start the game at a chosen point with the items and game
 * flags that point expects. Nothing in this build writes jump_menu_select or sets user_data.flg, so
 * both stay 0: CheckModeJumpDataSet gives items 0x11 and 0x12 and sets light_switch; the rest is dead.
 */

#include "sh2.h"
#define GAME_FLAG_ON(n) (game_flag.flag[(n) >> 5] |= 1 << ((n) & 31))
#define GAME_FLAG_OFF(n) (game_flag.flag[(n) >> 5] &= ~(1 << ((n) & 31)))

static struct JumpMenu_UserData user_data = {0};
static int jump_after_data_set;
int jump_menu_select;

/** Clears the jump menu's pending-position flag. */
void JumpMenuPosNormal(void) {
    if (user_data.flg) {
        user_data.flg = 0;
    }
}

/**
 * Once per jump: gives the player the items and sets the game flags that the chosen start point
 * (jump_menu_select) expects.
 */
void CheckModeJumpDataSet(void) {
    float dummy[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    int i;

    if (!jump_after_data_set) {
        jump_after_data_set = 1;
        ItemGet(0x11);
        ItemGet(0x12);
        item.light_switch = 1;
        if (jump_menu_select != 0xB && jump_menu_select != 0xC && jump_menu_select != 0x30 &&
            jump_menu_select != 0xA && jump_menu_select != 9 && jump_menu_select != 8 &&
            jump_menu_select != 7 && jump_menu_select != 0xD && jump_menu_select != 0) {
            ItemGet(4);
            for (i = 0; i < 9; i++) {
                ItemGet(5);
            }
            ItemGet(6);
            for (i = 0; i < 9; i++) {
                ItemGet(7);
            }
            ItemGet(8);
            for (i = 0; i < 9; i++) {
                ItemGet(9);
            }
            ItemGet(10);
            ItemGet(11);
            ItemGet(12);
            ItemGet(13);
            ItemGet(14);
            for (i = 0; i < 16; i++) {
                ItemGet(1);
            }
            for (i = 0; i < 16; i++) {
                ItemGet(2);
            }
            ItemGet(15);
            ItemGet(16);
            GAME_FLAG_ON(32);
            GAME_FLAG_ON(24);
            GAME_FLAG_ON(25);
            GAME_FLAG_ON(26);
            GAME_FLAG_ON(27);
            GAME_FLAG_ON(28);
            GAME_FLAG_ON(29);
            GAME_FLAG_ON(30);
            GAME_FLAG_ON(31);
        }
        switch (jump_menu_select) {
        case 0x10:
            GAME_FLAG_OFF(32);
            break;
        case 0x6:
            GAME_FLAG_ON(15);
            GAME_FLAG_ON(150);
            GAME_FLAG_ON(166);
            break;
        case 0x7:
            ItemGet(11);
            ItemGet(16);
            for (i = 0; i < 16; i++) {
                ItemGet(1);
            }
            item.equip = 11;
            break;
        case 0x8:
            GAME_FLAG_ON(15);
            ItemGet(11);
            ItemGet(4);
            for (i = 0; i < 9; i++) {
                ItemGet(5);
            }
            ItemGet(12);
            ItemGet(16);
            ItemGet(15);
            for (i = 0; i < 16; i++) {
                ItemGet(1);
            }
            item.equip = 4;
            break;
        case 0x62:
            GAME_FLAG_ON(10);
            break;
        case 0x9:
            ItemGet(11);
            ItemGet(4);
            for (i = 0; i < 9; i++) {
                ItemGet(5);
            }
            ItemGet(6);
            for (i = 0; i < 9; i++) {
                ItemGet(7);
            }
            ItemGet(12);
            ItemGet(16);
            ItemGet(15);
            for (i = 0; i < 16; i++) {
                ItemGet(1);
            }
            item.equip = 4;
            ItemGet(52);
            break;
        case 0xA:
            ItemGet(4);
            for (i = 0; i < 9; i++) {
                ItemGet(5);
            }
            ItemGet(6);
            for (i = 0; i < 9; i++) {
                ItemGet(7);
            }
            ItemGet(8);
            for (i = 0; i < 9; i++) {
                ItemGet(9);
            }
            ItemGet(10);
            ItemGet(11);
            ItemGet(12);
            ItemGet(13);
            for (i = 0; i < 16; i++) {
                ItemGet(1);
            }
            ItemGet(15);
            ItemGet(16);
            item.equip = 4;
            break;
        case 0xB:
            ItemGet(4);
            for (i = 0; i < 3; i++) {
                ItemGet(5);
            }
            ItemGet(11);
            for (i = 0; i < 5; i++) {
                ItemGet(1);
            }
            for (i = 0; i < 1; i++) {
                ItemGet(2);
            }
            ItemGet(16);
            item.equip = 4;
            GAME_FLAG_ON(25);
            GAME_FLAG_ON(66);
            GAME_FLAG_ON(35);
            GAME_FLAG_ON(40);
            GAME_FLAG_ON(41);
            GAME_FLAG_ON(43);
            GAME_FLAG_ON(49);
            GAME_FLAG_ON(67);
            playing.riddle_level = 0;
            break;
        case 0xC:
            ItemGet(4);
            for (i = 0; i < 3; i++) {
                ItemGet(5);
            }
            ItemGet(6);
            for (i = 0; i < 1; i++) {
                ItemGet(7);
            }
            ItemGet(11);
            ItemGet(12);
            for (i = 0; i < 8; i++) {
                ItemGet(1);
            }
            for (i = 0; i < 2; i++) {
                ItemGet(2);
            }
            ItemGet(15);
            ItemGet(16);
            item.equip = 4;
            GAME_FLAG_ON(28);
            GAME_FLAG_ON(35);
            playing.riddle_level = 0;
            break;
        case 0x13:
        case 0x1F:
        case 0x21:
        case 0x23:
        case 0x25:
            GAME_FLAG_ON(251);
            GAME_FLAG_ON(43);
            break;
        case 0x27:
            GAME_FLAG_ON(12);
            break;
        case 0x2B:
            GAME_FLAG_ON(95);
            break;
        case 0x2F:
            ItemGet(25);
            GAME_FLAG_ON(69);
            break;
        case 0x30:
            GAME_FLAG_ON(68);
            GAME_FLAG_ON(7);
            break;
        case 0x36:
            ItemGet(46);
            break;
        case 0x37:
            ItemGet(27);
            break;
        case 0x34:
            GAME_FLAG_ON(67);
            break;
        case 0x3F:
            GAME_FLAG_ON(150);
            break;
        case 0x40:
            GAME_FLAG_ON(150);
            GAME_FLAG_ON(152);
            GAME_FLAG_ON(15);
            break;
        case 0x45:
            GAME_FLAG_ON(150);
            GAME_FLAG_ON(15);
            GAME_FLAG_ON(162);
            GAME_FLAG_ON(164);
            GAME_FLAG_ON(166);
            break;
        case 0x46:
        case 0x49:
            GAME_FLAG_ON(15);
            break;
        case 0x4E:
            ItemGet(34);
            break;
        case 0x52:
            ItemGet(32);
            ItemGet(33);
            break;
        case 0x58:
            ItemGet(35);
            break;
        case 0x59:
            GAME_FLAG_ON(15);
            ItemGet(53);
            ItemGet(54);
            break;
        case 0x5F:
            GAME_FLAG_ON(15);
            break;
        case 0x3C:
            ItemGet(28);
            break;
        case 0x39:
            GAME_FLAG_ON(85);
            break;
        case 0x3A:
            GAME_FLAG_ON(109);
            break;
        case 0x3B:
            ItemGet(47);
            ItemGet(48);
            ItemGet(49);
            break;
        case 0x3D:
            ItemGet(29);
            break;
        case 0x3E:
            GAME_FLAG_ON(146);
            break;
        case 0x42:
            GAME_FLAG_ON(150);
            GAME_FLAG_ON(162);
            GAME_FLAG_ON(154);
            GAME_FLAG_ON(156);
            GAME_FLAG_ON(157);
            break;
        case 0x43:
            GAME_FLAG_ON(15);
            break;
        case 0x53:
            ItemGet(50);
            ItemGet(51);
            break;
        case 0x4F:
            playing.riddle_level = 0;
            ItemGet(34);
            break;
        case 0x50:
        case 0x5B:
            GAME_FLAG_ON(15);
            break;
        case 0x55:
            GAME_FLAG_ON(214);
            break;
        case 0x5D:
            GAME_FLAG_ON(232);
            break;
        case 0x56:
            GAME_FLAG_ON(222);
            break;
        case 0x61:
            GAME_FLAG_ON(251);
            GAME_FLAG_ON(150);
            ItemGet(55);
            break;
        case 0x63:
            GAME_FLAG_ON(10);
            break;
        case 0x65:
            GAME_FLAG_ON(265);
            break;
        case 0x66:
            ItemGet(52);
            break;
        case 0x67:
            ItemGet(38);
            break;
        case 0x68:
            GAME_FLAG_ON(274);
            break;
        case 0x6B:
            ItemGet(56);
            ItemGet(57);
            ItemGet(58);
            break;
        case 0x6C:
            ItemGet(60);
            ItemGet(61);
            ItemGet(59);
            break;
        case 0x72:
            ItemGet(62);
            break;
        case 0x79:
            ItemGet(39);
            break;
        case 0x81:
            ItemGet(66);
            ItemGet(64);
            ItemGet(65);
            break;
        case 0x84:
            ItemGet(17);
            ItemGet(18);
            ItemGet(19);
            ItemGet(21);
            ItemGet(22);
            ItemGet(30);
            ItemGet(40);
            ItemGet(63);
            ItemGet(64);
            ItemGet(65);
            ItemGet(71);
            ItemGet(73);
            ItemGet(74);
            GAME_FLAG_ON(415);
            break;
        case 0x82:
            ItemGet(44);
            break;
        case 0x83:
            ItemGet(63);
            break;
        case 0x88:
            ItemGet(45);
            break;
        case 0x89:
            ItemGet(20);
            break;
        case 0x86:
            ItemGet(67);
            ItemGet(43);
            break;
        case 0x92:
            GAME_FLAG_ON(483);
            GAME_FLAG_ON(484);
            ItemGet(70);
            ItemGet(69);
            break;
        case 0x93:
            GAME_FLAG_ON(10);
            item.flag[0] &= ~(1 << 15);
            break;
        case 0x8B:
            ItemGet(22);
            GAME_FLAG_ON(11);
        case 0x8C:
        case 0x8E:
        case 0x8F:
        case 0x91:
            item.flag[0] &= ~(1 << 15);
        }
    }
}
