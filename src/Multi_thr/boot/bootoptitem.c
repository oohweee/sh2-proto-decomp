/*
 * bootoptitem.c: the boot option table (command-line switches of the
 * executable) and the parse/print helpers for each option.
 */

#include "sh2.h"
#include "libc/stdio.h"

static int getopt_h(char *arg);
static int getopt_str(char *arg);
static void putopt_str(char *arg, int var);
static int getopt_I(char *arg);
static int getopt_B(char *arg);
static int getopt_D(char *arg);
static int getopt_H(char *arg);
static int getopt_v(char *arg);
static void putopt_v(char *arg, int var);
static int getopt_S(char *arg);
static void putopt_S(char *arg, int var);
static int getopt_d(char *arg);
static void putopt_d(char *arg, int var);
static int getopt_X(char *arg);
static void putopt_X(char *arg, int var);
static int getopt_V(char *arg);
static void putopt_V(char *arg, int var);
static int getopt_F(char *arg);
static void putopt_F(char *arg, int var);
static int getopt_x(char *arg);
static void putopt_x(char *arg, int var);

static int opt_h_dummy;
static char optI_strbuf[256];
static char optH_strbuf[256];
static char optB_strbuf[256];
static char optD_strbuf[256];

struct BootOptItem BootOptItemList[] = {
    {&opt_h_dummy, "h", getopt_h, NULL, "\tprint this Help.\n"},
    {&execEnv_quick_boot, "q", NULL, NULL, "\tQuick boot(skip title)\n\t(not supported yet)\n"},
    {&execEnv_skip_load_iop_mod, "i", NULL, NULL, "\tskip load Iop modules\n"},
    {&execEnv_skip_cd_check, "c", NULL, NULL, "\tskip Cd check(for use hd only)\n"},
    {&execEnv_hd_merge_file, "m", NULL, NULL, "\tno-use Merge files\n"},
    {&execEnv_self_reboot_mode, "r", NULL, NULL, "\tself-Reboot flag(reserved)\n\t(not supported yet)\n"},
    {&execEnv_verbose_level, "v:", getopt_v, putopt_v, " {0-9}\n\tVerbose level for verbose(level,format,...);\n"},
    {&execEnv_cdvd_media_type, "S:", getopt_S, putopt_S, "\tcd/dvd media Select\n"},
    {&execEnv_sound_data_from_hd, "s", NULL, NULL, "\tload 'sound.dat' from hd\n"},
    {&execEnv_auto_exit_time, "X:", getopt_X, putopt_X, "<timeout>\n\teXit if sleeping while <timeout> sec.\n"},
    {&execEnv_debug_flag, "d:", getopt_d, putopt_d, " {0x00000000-0xffffffff}\n\truntime Debug flag\n"},
    {&execEnv_video_mode, "V:", getopt_V, putopt_V, " {0|1}\n\tVideo mode\n\t0: NTSC (frame rate 60,disp height 448)\n\t1: PAL  (frame rate 50,disp height 512)\n\t(not supported yet)\n"},
    {&execEnv_file_load_mode, "F:", getopt_F, putopt_F, " {0|1|2}\n\tFile load mode\n\t0:use CD always\n\t1:use CD, but use HD if not exist CD\n\t2:use HD, but use CD if not exist HD\n"},
    {&execEnv_exec_path_mode, "x:", getopt_x, putopt_x, " {0|1|2}\n\teXec path mode(\".bin\",\".cnf\")\n\t0:same as File load mode(-F)\n\t1:use CD always\n\t2:use HD always\n"},
    {&execEnv_host_path, "H:", getopt_H, putopt_str, " <directory>\n\tHost directory (prefix for all files)\n"},
    {&execEnv_database_path, "B:", getopt_B, putopt_str, " <directory>\n\tdataBase(merge file) directory (prefix for all merge files)\n"},
    {&execEnv_data_path, "D:", getopt_D, putopt_str, " <directory>\n\tData files directory (prefix for all data files)\n"},
    {&execEnv_iop_path_hd, "I:", getopt_I, putopt_str, " <directory>\n\tIop modules directory for hd(prefix for all iop modules)\n"},
    {&execEnv_reboot_file, "R:", getopt_str, putopt_str, " <elf-file>\n\tReboot by elf-file\n\t(not supported yet)\n"},
    {NULL},
};

/* -h: prints every option with its help text. */
static int getopt_h(char *arg) {
    struct BootOptItem *item;
    char *help;

    for (item = BootOptItemList; item->var; item++) {
        if ((help = item->help)) {
            printf("-%c%s", item->key[0], help);
        }
    }
    return 0;
}

static int getopt_str(char *arg) {
    return (int)arg;
}

/* Copies a directory option to buf; a trailing '-' is replaced by the default data path. */
static int getopt_str2(char *arg, char *buf) {
    int len;
    char *str;

    str = arg;
    len = 0;
    len += UtilStrCpyL(buf, str, 255);
    if (buf[len - 1] == '-') {
        len--;
        len += UtilStrCpyL(buf + len, "daily.thu/", 255 - len);
        buf[len] = 0;

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 158
        printf(__FILE__ ":" SH_STRINGIFY(__LINE__) "> set default data path: %s\n", buf);
    } else {
        buf[len] = 0;
    }
    return (int)buf;
}

static void putopt_str(char *arg, int var) {
    int len;

    len = UtilStrCpyL(arg, (char *)var, 255);
    arg[len] = 0;
}

static int getopt_I(char *arg) {
    return getopt_str2(arg, optI_strbuf);
}

static int getopt_B(char *arg) {
    return getopt_str2(arg, optB_strbuf);
}

static int getopt_D(char *arg) {
    return getopt_str2(arg, optD_strbuf);
}

static int getopt_H(char *arg) {
    return getopt_str2(arg, optH_strbuf);
}

static int getopt_v(char *arg) {
    int var;

    var = arg[0] - '0';
    if (var < 0) {
        var = 0;
    }
    if (var > 9) {
        var = 9;
    }
    execEnv_verbose_level = var;
    return var;
}

static void putopt_v(char *arg, int var) {
    if (var < 0) {
        var = 0;
    }
    if (var > 9) {
        var = 9;
    }
    arg[0] = var + '0';
    arg[1] = 0;
}

static int getopt_S(char *arg) {
    int var;

    var = arg[0] - '0';
    if (var < 0) {
        var = 0;
    }
    if (var > 1) {
        var = 1;
    }
    execEnv_cdvd_media_type = var;
    return var;
}

static void putopt_S(char *arg, int var) {
    if (var < 0) {
        var = 0;
    }
    if (var > 1) {
        var = 1;
    }
    arg[0] = var + '0';
    arg[1] = 0;
}

/* Integer option: "<n>" sets, "<op><n>" combines with the current value. */
static int getopt_int(char *arg, int var0) {
    int var;
    int op;

    op = *arg;
    if (op >= '0' && op <= '9') {
        op = '=';
    } else {
        arg++;
    }
    sscanf(arg, "%i", &var);
    switch (op) {
    default:
    case '=':
        var0 = var;
        break;
    case '#':
        break;
    case '!':
        var0 = !var0;
        break;
    case '~':
        var0 = ~var0;
        break;
    case '+':
        var0 += var;
        break;
    case '-':
        var0 -= var;
        break;
    case '*':
        var0 *= var;
        break;
    case '/':
        var0 /= var;
        break;
    case '%':
        var0 %= var;
        break;
    case '|':
        var0 |= var;
        break;
    case '^':
        var0 ^= var;
        break;
    case '&':
        var0 &= var;
        break;
    case '<':
        var0 <<= var;
        break;
    case '>':
        var0 >>= var;
        break;
    }
    return var0;
}

static void putopt_int(char *arg, int var) {
    sprintf(arg, "=0x%08x", var);
}

static int getopt_d(char *arg) {
    int var;

    var = getopt_int(arg, execEnv_debug_flag);
    execEnv_debug_flag = var;
    if (var & 0x20) {
        Env_ctl.stat_ctl_2.uc8[0] = (4 - (var & 0xE)) >> 1;
    }
    return var;
}

static void putopt_d(char *arg, int var) {
    putopt_int(arg, var);
}

static int getopt_X(char *arg) {
    int var;

    var = getopt_int(arg, execEnv_auto_exit_time);
    return var;
}

static void putopt_X(char *arg, int var) {
    putopt_int(arg, var);
}

static int getopt_V(char *arg) {
    int var;

    var = arg[0] - '0';
    if (var) {
        var = 3;
        execEnv_frame_rate = 50;
        execEnv_disp_height = 512;
    } else {
        var = 2;
        execEnv_frame_rate = 60;
        execEnv_disp_height = 448;
    }
    return var;
}

static void putopt_V(char *arg, int var) {
    arg[0] = (var == 3 ? 1 : 0) + '0';
    arg[1] = 0;
}

static int getopt_F(char *arg) {
    int var;
    int mode;

    var = arg[0] - '0';
    mode = var;
    if (mode > 2) {
        mode = 2;
    } else if (mode < 0) {
        mode = 0;
    }
    return mode;
}

static void putopt_F(char *arg, int var) {
    int mode;
    int i;

    mode = var & 0xF;
    if (mode > 2) {
        mode = 2;
    } else if (mode < 0) {
        mode = 0;
    }
    i = mode;
    arg[0] = i + '0';
    arg[1] = 0;
}

static int getopt_x(char *arg) {
    int var;
    int mode;

    var = arg[0] - '0';
    mode = var;
    if (mode > 2) {
        mode = 2;
    } else if (mode < 0) {
        mode = 0;
    }
    return mode;
}

static void putopt_x(char *arg, int var) {
    int mode;
    int i;

    mode = var & 0xF;
    if (mode > 2) {
        mode = 2;
    } else if (mode < 0) {
        mode = 0;
    }
    i = mode;
    arg[0] = i + '0';
    arg[1] = 0;
}
