/*
 * init_mt_sys.c: start-up of the system layer: file system, loader thread,
 * IOP modules, sound, CD, DMA servers, pad thread.
 */

#include "sh2.h"
#include "sdk/eekernel.h"
#include "sdk/libcdvd.h"
#include "lib/sh_kernel.h"

/* utilities without DWARF */

/* Linker-defined: start of the overlay area and the end of the program. */
extern char _ovl_start_addr[];
extern char _ovl_align_addr[];
extern char _end[];

#define BOOT_FILE_NAME "SLUS_202.28"
#define MC_FILE_NAME "BASLUS-20228"
#define PRODUCT_CODE 20228

/* printf/verbose text tagged with "<file>:<line>> " like the asserts. */
#define LOG_HEAD __FILE__ ":" SH_STRINGIFY(__LINE__) "> "

/* Thread stacks and message queues. */
u_long128 ADDR_STACK_FS_BG[512];
u_long128 ADDR_STACK_FS_CB[512];
u_long128 ADDR_STACK_LI[512];
u_long128 ADDR_STACK_PAD[512];
u_long128 ADDR_STACK_VB_S[256];
u_long128 ADDR_STACK_VB_E[512];
u_long128 ADDR_STACK_DMA_GIF[256];
u_long128 ADDR_STACK_DMA_VIF1[256];
u_long128 ADDR_QUEUE_FS_BG[512];
u_long128 ADDR_QUEUE_LI[128];
u_long128 ADDR_QUEUE_DMA_GIF[512];
u_long128 ADDR_QUEUE_DMA_VIF1[512];

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 127
/* Matches once linked: _ovl_start_addr/_ovl_align_addr/_end are absolute linker symbols. */

static void check_code_and_data_size(void) {
    assert_dw(("code+data size over",(_ovl_start_addr<=_end)));

    assert(("code+data size over",(_ovl_align_addr<=_end)));
}



/* The executable must be started under the name the disc's SYSTEM.CNF boots. */
static void check_boot_file_name_in_system_cnf(char *bootfilename) {
    char *dirchp;
    char *search;
    char *filename;

    if (!bootfilename) {
        return;
    }
    for (search = bootfilename, dirchp = NULL; *search; search++) {
        switch (*search) {
        case ':':
        case '/':
        case '\\':
            dirchp = search;
            break;
        }
    }
    filename = bootfilename;
    if (dirchp) {
        filename = dirchp + 1;
    }
    search = BOOT_FILE_NAME;
    while (*search == *filename) {
        if (*search == '\0') {
            break;
        }
        search++;
        filename++;
    }
    assert_dw(*search=='\0' && ( *filename=='\0' || *filename==';' ));
}
/**
 * Logs the product code, boot file and memory card file names, and checks the program size and that
 * `bootfilename` matches SYSTEM.CNF.
 */
void check_build_environment(char *bootfilename) {
    verbose(1, LOG_HEAD "CODE:%d\n", PRODUCT_CODE);
    verbose(1, LOG_HEAD "BOOT:%s\n", BOOT_FILE_NAME);
    verbose(1, LOG_HEAD "MC FILE:%s\n", MC_FILE_NAME);
    check_code_and_data_size();
    check_boot_file_name_in_system_cnf(bootfilename);
}

/* Queues each merge file without a buffer, which only locates it and seeks there (nothing is read). */
static int prepare_data_mgf(void) {
    int fid;

    fid = FcRead(data_bg_mgf, NULL);
    fid = FcRead(data_chr_mgf, NULL);
    fid = FcRead(data_etc_mgf, NULL);
    fid = FcRead(data_menu_mgf, NULL);
    fid = FcRead(data_movie_mgf, NULL);
    fid = FcRead(data_pic_mgf, NULL);
    fid = FcRead(data_sound_mgf, NULL);
    return fid;
}

/* Debug: reboots the IOP with a 2MB memory limit to check the modules fit. */
static void iop_mem_check(int mmode) {
    dbFlagReserve(0x100, "break at load module sequense for IOP memory check.");
    if (dbFlag(0x100)) {
        shSifRebootIopR("host0:ioprp.img");
        shSifInit();
        shSifLoadModuleR("host0:mem2MB.irx", 0, "");
        shSifRebootIopR("host0:ioprp.img");
        shCdInitR(0, mmode);
        execEnv_skip_load_iop_mod = 1;
    }
}

/* Matching: #line keeps the original line numbers (the original had more lines here). */
#line 232
/* Debug: waits for the developer to clear break_flag from the debugger. */
static void iop_mem_check_break(int fid) {
    volatile int break_flag;
    int count;

    if (dbFlag(0x100)) {
        fsSync(0, fid);
        break_flag = 1;
        count = 0;
        printf(LOG_HEAD "Please break and modify flag(@0x%08x)\n", &break_flag);
        while (break_flag) {
            count++;
        }
    }
}

/**
 * Starts the system layer: V-blank sync, CD, the IOP modules, the file and load-init servers, the
 * sound driver and the V-blank threads. Returns the command ID of the data file preparation.
 */
int init_sh2_filesys(void) {
    int fid;
    int mmode;

    ChangeThreadPriority(GetThreadId(), 32);
    utilExclInit();
    shSyncVEndInit();
    shSyncVStartInit();
    mmode = 2;
    if (execEnv_cdvd_media_type) {
        mmode = 1;
    }
    shCdInitR(0, mmode);
    iop_mem_check(mmode);
    if (!execEnv_skip_load_iop_mod) {
        shSifInit();
        shIopReplaceMod("iop/lib224/ioprp.img");
        execEnv_skip_load_iop_mod = 1;
        shCdInitR(0, mmode);
    }
    printf_enable();
    fsInit(28, ADDR_STACK_FS_BG, sizeof(ADDR_STACK_FS_BG), ADDR_QUEUE_FS_BG, sizeof(ADDR_QUEUE_FS_BG));
    lisInit(30, ADDR_STACK_LI, sizeof(ADDR_STACK_LI), ADDR_QUEUE_LI, sizeof(ADDR_QUEUE_LI));
    fcSifInit();
    fcIopLoadMod("iop/lib224/sio2man.irx");
    fcIopLoadMod("iop/lib224/padman.irx");
    fcIopLoadMod("iop/lib224/mcman.irx");
    fid = fcIopLoadMod("iop/lib224/mcserv.irx");
    iop_mem_check_break(fid);
    if (!dbFlag(1)) {
        fcIopLoadMod("iop/lib224/libsd.irx");
        fcIopLoadMod("iop/lib224/sdrdrv.irx");
    }
    if (!dbFlag(1)) {
        fcIopLoadMod(execEnv_sound_data_from_hd ? "iop/sd0712/sd_hd.irx" : "iop/sd0712/sd_cd.irx");
    }
    if (!dbFlag(1)) {
        fcIopLoadMod("iop/sd0712/sdstr.irx");
        fcIopLoadMod(execEnv_sound_data_from_hd ? "iop/sd0712/soundhd.irx" : "iop/sd0712/soundcd.irx");
    }
    fcHdInit(1);
    fcDiskSelect(execEnv_file_load_mode & 3);
    fcExecDevSelect(execEnv_exec_path_mode & 3);
    fcSetParamForSystemCnf(execEnv_skip_cd_check);
    fsSync(0, -1);
    if (!dbFlag(1)) {
        int sect;
        char buf[256] = "host0:./sound.dat";
        char *path;
        struct sceCdlFILE fp[1];

        shPathMakeIop(buf, "iop/sd0712/sound.dat");
        if (!execEnv_sound_data_from_hd) {
            shCdSearchFile(fp, buf);
            sect = fp[0].lsn;
            path = NULL;
        } else {
            sect = 0;
            path = buf;
        }
        sceCdSync(0);
        SeCallInit(sect, mmode, path);
        shSdSifInit();
        sceCdSync(0);
    }
    fcCdInitW(8, ADDR_STACK_FS_CB, sizeof(ADDR_STACK_FS_CB));
    fcCdCheckDisk(1);
    fid = prepare_data_mgf();
    ThreadVbeStart(ADDR_STACK_VB_E, sizeof(ADDR_STACK_VB_E), 4, 0, -1);
    ThreadVbsStart(ADDR_STACK_VB_S, sizeof(ADDR_STACK_VB_S), 2, 0, -1);
    return fid;
}

/** Sets up the texture slots and starts the DMA channel 1 (VIF1) and 2 (GIF) servers. */
void init_sh2_dmac(void) {
    texSlotInit();
    d1sInit(6, ADDR_STACK_DMA_VIF1, sizeof(ADDR_STACK_DMA_VIF1), ADDR_QUEUE_DMA_VIF1, sizeof(ADDR_QUEUE_DMA_VIF1));
    d1cInit();
    d2sInit(6, ADDR_STACK_DMA_GIF, sizeof(ADDR_STACK_DMA_GIF), ADDR_QUEUE_DMA_GIF, sizeof(ADDR_QUEUE_DMA_GIF));
    d2cInit();
}

/** Starts the pad thread and waits until it is up; returns its semaphore. */
int init_sh2_devsys(void) {
    int sid_pad;
    int sid_pad_fin;

    sid_pad = CreateSema2(0, 256, NULL);
    fsSync(0, -1);
    sid_pad_fin = ThreadPadStart(ADDR_STACK_PAD, sizeof(ADDR_STACK_PAD), 6, 0, sid_pad);
    SignalSemaMax(sid_pad);
    SignalSema(WaitSema(sid_pad_fin));
    return sid_pad_fin;
}
