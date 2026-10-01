# What is notable about this prototype

Notes for researchers on the Silent Hill 2 prototype (SLUS_202.28, 2001-07-13). Every item cites
the source. Everything here comes from reading the code: none of it was tested on hardware or in an
emulator. Differences from the retail game are marked **possible difference, unverified against
retail** unless the repo itself proves them. This project doesn't compare against retail code.
Sources are cited by file and function (or table) name.

## Controller button masks

The game's pad queries (`shPadPress`, `shPadTrigger`, `shPadRepeat`, src/SH2_common/pad.c)
take a mask with one bit per entry of a 20-byte normalized pad array. The table below comes from
`shSysKeyNormalize` (src/Multi_thr/pad/keydata.c: pressure bytes 8-19 from the raw button bits,
bytes 20-23 from L3/R3/START/SELECT) and `shPadSet` (pad.c), which copies bytes 20-23 to 0-3. The
raw bit order is the PS2 controller's, from public hardware documentation. The field names of the
pressure table in keydata.c (`AN_6`/`AN_4`/`AN_8`/`AN_2` for the d-pad, `AN_O`, `AN_X`, `AN_L1`,
...) agree with it.

| Mask | Button | Mask | Button |
|---|---|---|---|
| 0x1 | L3 | 0x1000 | Triangle |
| 0x2 | R3 | 0x2000 | Circle |
| 0x4 | START | 0x4000 | Cross |
| 0x8 | SELECT | 0x8000 | Square |
| 0x10-0x80 | raw analog stick bytes (right X, right Y, left X, left Y, in the controller's data order), not buttons | 0x10000 | L1 |
| 0x100 | Right | 0x20000 | R1 |
| 0x200 | Left | 0x40000 | L2 |
| 0x400 | Up | 0x80000 | R2 |
| 0x800 | Down | | |

Check: by this table the soft reset in `GameKeyCheck` (src/main.c; masks 8, 4, 0x10000, 0x20000 on
port 0) is SELECT + START + L1 + R1, the usual PS2 reset combination.

"Controller 1" below is port 0 (the player's pad). "Controller 2" is port 1 (the debug pad).

## Debug features and how they are reached

### Boot options (`BootOptItemList`, src/Multi_thr/boot/bootoptitem.c)

The executable parses command-line switches with `getopt` (`BootOptGet`, bootopt.c). The table:
`-h` help, `-q` quick boot (marked "not supported yet"), `-i` skip IOP module loading, `-c` skip the
CD check, `-m` don't use merge files, `-r` self-reboot (reserved), `-v` verbose level 0-9, `-S`
media type, `-s` load sound.dat from the host drive, `-X` auto-exit after N seconds stalled (used
by src/DBG/dbflow.c), `-d` debug flags, `-V` video mode (PAL "not supported yet"), `-F`/`-x` file
and exec path mode, `-H`/`-B`/`-D`/`-I` directories (a trailing `-` puts in the default
`daily.thu/`, `getopt_str2` in bootoptitem.c), `-R` reboot ELF (not supported yet).

How to reach: only from a launcher that passes arguments (a dev-kit host). With no arguments, `main`
only turns `printf` off (`printf_skip(1)` in `main`, src/main.c). The disc's SYSTEM.CNF (`BOOT2 =
cdrom0:\SLUS_202.28;1`, docs/rom-map.md) passes none.

Debug flag bits (`-d`, stored in `execEnv_debug_flag`, tested with `dbFlag`, src/DBG/dbflag.c):

| Bit | Effect | Source |
|---|---|---|
| 0x1 | No sound: the sound IOP modules and sound.dat aren't loaded, and sound calls are skipped | `init_sh2_filesys` (src/Multi_thr/sys/init_mt_sys.c); src/sound/sh_sound.c; `shSdVSync` and `shSdCallCheck` (src/sound/sh_sd_call.c) |
| 0x2 | Enables sound calls that are otherwise dropped: ID 1500, 6000-6063 and 63000 and up | `shSdCallCheck` (sh_sd_call.c) |
| 0x4 | Hot init waits 100 frames per step instead of 2 | `systemHotInit` (src/sh2_init.c) |
| 0x8 | The pad thread skips vibration output | `ThreadPad` (src/Multi_thr/pad/th_pad.c) |
| 0x10 | Debug switch display on at boot; pad warning level 0 | `main` (src/main.c); `ThreadPad` (th_pad.c) |
| 0x20 | Also sets `Env_ctl.stat_ctl_2.uc8[0]` from bits 1-3 (purpose unknown) | `getopt_d` (bootoptitem.c) |
| 0x100 | Reserved "break at load module sequense for IOP memory check": reboots the IOP from `host0:` | `iop_mem_check` (init_mt_sys.c) |

`main` also calls `dbFlagSet(0x10)` unconditionally (src/main.c, as in the original; the
assembly passes the return value of the earlier `dbFlag(0x10)` on to `dbSwitchAllInit`). So
`execEnv_debug_flag` always has bit 0x10 after boot. `shPadGetPort` (pad.c) therefore never
returns 0, and the controller-2 debug features below are live on a normal disc boot. This is
read from the code, not tested.

### Debug switches (src/DBG/dbswitch.c, dbsw_all.c, dbsw_sys.c, dbsw_map.c)

Two pages of 32 on/off switches (`enum DBSW_ID`: `DBSW_SYS`, `DBSW_MAP`, include/sh2/types.h).
`dbSwitchAllPrint` (dbsw_all.c) runs every game frame (called from `main`) and reads controller 2:

- Cross toggles the switch display (`dbSwitchDispIndicator`, dbswitch.c).
- With the display on: the d-pad moves the cursor (left/right = page, up/down = switch) and Circle
  flips the switch under the cursor.
- Hold L1 and press Triangle to set every switch of the page, or Square to clear them. Hold L1 with
  Triangle and Square together to toggle the whole system, which reloads the defaults.
- Defaults: system switches 1, 2, 3, 5, 6, 30, 31 (dbsw_sys.c, `dbSwitchSysInit`); map switches 0
  and 1 (dbsw_map.c, `dbSwitchMapInit`).
- `dbSwitch` returns 0 for every switch while the display is off (dbswitch.c, `dbSwitch`).

System page help texts (dbsw_sys.c, `dbSwitchSysHelp`): 0 help line, 1 debug pad port, 2 show the
indicator on ports other than 1, 3 show all pages, 4 real-time clock, 5 file server status, 6
load/init server status, 7 verbose server status, 8 system language, 9-12 pad 0 info (step, raw
data, key data), 13-16 the same for pad 1, 17 semaphores (prints nothing), 18 threads (the printing
is empty in this build, dbkernel.c), 19 font test (characters 0x20-0x9F), 30 "debug menu from soft
reset", 31 "movie dummy step". No code outside src/DBG reads switches 30 and 31 (grep for
`dbSwitch`), so those two do nothing in this build.

Map page (dbsw_map.c): help texts for map info, global ID, reference position, room name, room
info, map block display (the drawing function `printR_mapblock` is empty), and map 0-3 clear-flag
init. Only the clear-flag switches do anything.

The key-data display (system switches 12 and 16, `printR_keydata`, dbsw_sys.c) prints
`DRINK` and `RADIO` among the keys (see DRINK below). It prints `CANCEL` in the `PAUSE:` slot.

### Debug pad port (src/SH2_common/pad.c)

`pad_x` (1-12) is the "port" the debug pad answers as. SELECT on controller 2 advances it and prints
"Contoroler 2 port change" (`shPadSet`). Ports other than 1 only work while system switch 1 is on
(which needs the switch display on). Features by port:

| Port | Feature | Source |
|---|---|---|
| 1 | Freeze: START on controller 2 freezes the game. While frozen, R1 advances one frame, holding R2 runs, START again resumes (`dbFreeze`, called every frame from `main`) | src/DBG/dbfreeze.c |
| 1 | The debug switch controls above | src/DBG/dbsw_all.c |
| 2, 3 | Camera and GS debug text (camera position, eye direction, view tiles); "No SemiTex!" warning | `kari_DBG_print_junbi` (src/GFW/gfw_test/kari_probe_draw.c); `sh2gfw_Draw_SemiTransBG` (src/GFW/sh2gfw_SemiTrans_FrameWork.c) |
| 3 | Fog toggle: START on controller 2 turns the fog particle pass (`sh2gfw_fogtest_calcmain`) on or off | `Spack_All_Draw` (src/sh2gfw_drawloop_main.c) |
| 6 | Packet debug print (`spkDebugPrint`) | `fjDrawExec` (src/Font/fj_man.c) |
| 6 | One-hit kills: while L2 on controller 1 is held, any damaging hit zeroes the enemy's HP | `enCheckDamage` (src/Enemy/en_common.c); `enBOSCheckDamage` (src/Enemy/en_bos.c) |
| 7 | Prints the current animation frame of each hanging meat (NIK) | `ObjectNIKFunction` (src/Chacter/m3_nik.c) |

"Fog test": `sh2gfw_fogtest_main.c` is the name of the fog module used in normal play. Only the
port-3 toggle above is a debug feature.

### Jump menu (src/jump_menu.c)

`CheckModeJumpDataSet` (jump_menu.c) gives the items and game flags that a chosen start point
expects. The start point is `jump_menu_select`, with cases 0x7-0x93. The same data carries seven
jump IDs in the title's start-point lists (`jump_nemu_id`, see below). There, 7 = apartment
stairwell, 8 = hospital 1F, 9 = Historical Society, 10 = hotel 1F, 11 = Wood Side 2F, 12 = prison
north, 16 = the restroom. Their item sets match: title mode-2 start point 1 and jump case 0xB give
the same items and flags (`titleSetDataStartPoint` in title.c against case 0xB of
`CheckModeJumpDataSet`).

How to reach: not possible in this build. Nothing in `src/` writes `jump_menu_select`, and the
original executable's relocations for `main` refer to it only from three reads (a HI16/LO16 pair
each; `config/relocs_main.txt`, generated from the disc by `configure.py`). No on-disc overlay
references it. The menu screen that set it isn't in the code. The relocations of the 39 missing
overlays aren't in the repo.

### Title test mode (src/Event/title.c)

`static int title_test_mode = 0;` (title.c) is never written, so its two test modes are dead code:

- Mode 1: a main menu with extra "new game" entries that start at the five `TitleJPStartPointList`
  points: stage 2 (restroom), 12 (apartment stairwell), 18 (hospital 1F), 28
  (Historical Society), 40 (hotel 1F).
- Mode 2: a short menu that skips the difficulty menus (battle level 2, riddle level 0) and starts
  at one of the three `TitleUSPStartPointList` points: stage 2 (restroom), 7 (Wood
  Side 2F), 31 (prison north).
- `titleSetDataStartPoint` (title.c) gives each start point its items. It is called on every
  stage load (`connectStageInit`, src/connect.c) but does nothing while the mode is 0.
- Normal New Game (mode 0) uses `TitleJPStartPointList[0]`, stage 2 at (-19606.6, 18.83, 20429.55)
  (`titleFadeOutNewGame`, title.c).

### Enemy radar (src/Enemy/en_draw.c, src/Font/fj_man.c)

A top-down overlay of enemies, collision walls and columns (`enDrawRadar`, en_draw.c).

How to reach: after at least one clear (`playing.clear_end_number` non-zero), hold L2 and press L3
on controller 1. Each press cycles `playing.radar` 0 → 4 → 0 (`fjDrawExec`, fj_man.c). Modes 1 and 3 use a
60 view scale, 2 and 4 use 90. Modes 1-2 rotate with the player, 3-4 are fixed per room
(`enDrawRadar`). Possible difference, unverified against retail.

### Free camera (src/view/vc_util.c)

`vcMoveAndSetCamera` (vc_util.c) has modes 1 and 3 (a free camera moved by forward/back,
turn and up/down flags) and mode 2 (a pad camera whose function only reads values,
`vcSetRefPosAndCamPosAngByPad`).

How to reach: not possible. `vcCameraInternalInfo.mode` is only ever set to 0 (`vcInitCamera`, `vcMoveAndSetCamera`),
and every caller passes 0 for all the movement flags (demoview.c, stage files, gfw_test/sh2gfw_shcamtest.c).

### "Coming soon" screen (src/Event/title.c)

`GameendMain` (title.c) shows `data_pic_etc_comingsoon_tex` for up to 5 seconds, then returns
to the title (`PlayableMain`, src/gamemain.c, state 11).

How to reach:
- `EvProgApartBoss` (src/Event/stage/stg_apart_stair.c), the apartment stairwell Pyramid Head
  encounter, sets `Sh2sys.step[2] = 0xB` (state 11) right after its intro demo. The playable game
  ends there.
- `EvProgTrialEnd` of the TGS trial stage (stg_tgs_trial.c) also enters it.

The memo list also uses the "coming soon" picture as a placeholder for five memos (the table `data` in src/Event/memo.c).

### TGS trial stage (src/Event/stage/stg_tgs_trial.c)

Stage 1 (`Stg_tgs_trial`) is a cut-down Brookhaven Hospital (map 10) built into main rather than an
overlay (`StgOverlayGetFileID`, `StgOverlayGetStageData`, stg_overlay.c). Its events: start kit, hospital map, needle, number and box
puzzles, shower drain, doctor's memos, elevator, trial end. `EvProgTrialStartSet`
gives the handgun, four boxes of handgun ammo, the wooden plank, five
health drinks, the flashlight and the radio. It has no enemies (`en_list` holds only the
terminator) but has a buzzing fly sound (en_fly.c). The item screen has a
matching temporary picture sheet (src/Item/item_tgs_tmp.c). "TGS" is read as Tokyo Game Show
(suspected: the repo never spells it out).

How to reach: unknown. No code in the repo sets `playing.stage` to 1 directly; stage changes come
from the stages' event lists (`EventExecMove`, src/Event/event.c), and those are only in the repo
for the 13 stages whose code is on the disc.

### Result-screen extra message (`ResultMain`, src/SH2_common/result.c)

Loads `data_menu_mc_result_msg_extra_mes` instead of the normal result text when all of these hold:
the Dog ending has been seen (bit 0x10), the language is 0 (Japanese, by the `_j`/`_e` message
file order in src/SH2_common/data_load.c), L3 and R3 are held, the sticks are pushed (left stick X
≤ 0x5F, left Y, right X and right Y > 0xA0, in the controller's data order), and nothing else is pressed.
The stick reading is derived from the masks; possible difference, unverified against retail.

## Content signs

### Only 12 of 51 stage overlays are on the disc

The ELF has symbols, relocations and DWARF for 51 stage overlays, but only 12 overlay binaries are on
the disc: toilet, observation, forest, town east, and the eight apartment overlays (docs/rom-map.md).
With the TGS trial in main, that is 13 of the 52 stages in `STG_NAME`. The playable route runs from
the restroom through the town and Wood Side/Blue Creek to the stairwell, where `EvProgApartBoss`
ends the game with the "coming soon" screen (above). The other 39 stages exist only as names
([missing-overlays.md](missing-overlays.md), endings-and-stages.md). Verified.

### No UFO ending, Blue Gem or Born from a Wish

Nothing in this build belongs to the UFO ending, the Blue Gem or the "Born from a Wish" scenario.
The file index (include/sh2/variables.h) has no name with `ufo` or `gem` in it: no stage, demo,
movie, picture or item model. Of the item models (`x_*`), none reads as the Blue Gem. The stages
are the 52 of `STG_NAME` (src/Event/stg_overlay.c), none of them a UFO or Born from a Wish stage;
the unused mansion map (below) is the only possible trace, and that link is speculation. Verified
for the names; the content of the 39 missing stages can't be checked.

### A mansion map (src/Event/stg_name.c)

`RoomName` has a map 16 handled by `RoomNameMansion` (stg_name.c), with 19 rooms (0xC0-0xD2).
Those rooms have BG blocks in `room_to_block`. The BG ID is `BG_ID_ma` (`enum STAGE_ID`,
include/sh2/types.h), and the file index has 454 `data_bg_ma_*` symbols (maps `ma01`, `ma02`, ...;
include/sh2/variables.h). No stage in `STG_NAME` uses map 16. What the mansion is, is unknown. The
original North American release (2001) has no mansion area in public descriptions. The later
releases (the Greatest Hits reissue, the European release, and Japan's "Saigo no Uta") add the
"Born from a Wish" scenario, which takes place in a mansion. Any link is unverified speculation.

### The DRINK key (src/Multi_thr/pad/keydata.c)

The key-assignment struct (`struct shGameKeyAssign`, include/sh2/types.h) has a `DRINK` key
(and `RADIO`). The default assigns DRINK to L3 and RADIO to R3 (`gkey_assign`, keydata.c).
`shGameKeyConvert` decodes it (keydata.c) and the debug key display prints it (`printR_keydata`).
No game code reads `f.DRINK` or `f.RADIO`: the only readers of `shGameKeyConvert` output are
`PlayerCheckKeyInput` (m3_play.c), `BoatCheckKeyInput` (m3_boat.c) and the debug page, and none
uses these fields (grep over src/). `shPadSetGameKeyAssign` (pad.c) rebuilds every key from the option screen's key
configuration except DRINK and RADIO. A quick-drink key looks planned and left unused. Possible
difference, unverified against retail.

### TYU crawls on walls and ceilings (src/Enemy/en_tyu.c)

The TYU AI has wall and ceiling states (`enTYUCtrlOnWall`, `enTYUCtrlOnCeiling`;
`enTY2CtrlWall`/`Ceiling`, `enTY3CtrlWall`/`Ceiling`). TYU is suspected to be the Creeper
(characters.md). Public descriptions of the retail Creeper mention only floor crawling. Possible
difference, unverified against retail.

### Enemy placements

- The town map has a second enemy set under condition 0x340 (game flag 251): XOO, MKN and RED (Pyramid
  Head) instead of SCU/TYU (src/Event/stage/stg_town_east.c, `en_list`, the entries with condition
  832; conditions in `CharaAdminEnemyEntryCondition`, src/Event/chara_admin.c). What flag 251
  marks is unknown. The jump-menu cases 0x13, 0x1F, 0x21, 0x23, 0x25 and 0x61 set it
  (`CheckModeJumpDataSet`). Possible difference, unverified against retail.
- XOO (kind 0x20B) is an enemy with its own model driven by the NSE AI (characters.md). Identity
  unknown.
- Kinds 0x20E, 0x20F, 0x210 have RED's animation frame size but no handler and no files
  (`shCharacterAnimeOneFrameSize`, m3_sc.c): unused slots.

### Ending hints in Wood Side

`EvProgEndHintRecoveryRead`/`MariaRead`/`SuicideRead` in the Wood Side 2F overlay
(stg_apart_e2f.c) show a hint picture (`data_pic_apt_p_endhint_tex`) for three of the endings
(recovery, Maria, suicide). Each is enabled by one of game flags 7, 8 and 9, which `FlagInit`
sets for endings not yet seen (endings-and-stages.md). Possible difference, unverified against
retail.

### Smaller observations

- `GameCalcRankEndingKind` tests the ending bits with `|` instead of `&`, so it always awards 20
  points (src/Chacter/player_result.c, marked `@bug`).
- The result screen picks the `n/4(+1)` ending-count format with `n & 0x10` after `n` has been
  replaced by a message number (7-11), so the "+1" form never shows (`ResultMain`,
  src/SH2_common/result.c, marked `@bug`).
- Item 0x1A comes from `EvProgGetYardKey` but its world model is `x_keyemerg`. Item 0x1B comes from
  `EvProgGetEmergencyKey` but its model is `x_keycourt` (stg_apart_e3fe.c; `item_to_chara`,
  src/Event/event_sub.c). The event names agree with the events that use the keys and with
  retail's doors, so the model pairing is probably the swapped part (suspected; game-ids.md).
- The DWARF path of the source tree is `E:\work\sh2(CVS全取得)\src\` (docs/rom-map.md).
