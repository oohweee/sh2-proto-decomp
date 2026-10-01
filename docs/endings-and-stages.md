# Endings, stages and rooms

Maps the internal stage, map and room IDs, and the ending codes, to public Silent Hill 2 names.
Confidence labels work as in [characters.md](characters.md):
- **verified**: code on the disc identifies the place or ending unambiguously;
- **by name**: the only evidence is the developers' own symbol names (event programs, demo and
  picture file names) of stages whose code isn't on the disc;
- **suspected**: a name reading or a match by elimination, with the reason given.

Most area evidence for stages 14-52 comes from event-program names. Their code isn't on the disc
([rom-map.md](rom-map.md)), but the executable's symbol table still names their functions:
[missing-overlays.md](missing-overlays.md) lists them per stage.

## Endings

| Stage (`STG_NAME`) | Overlay | Demo prefix (Japanese reading) | Ending pictures | `clear_end_kind` bit | Public name | Confidence | Evidence |
|---|---|---|---|---|---|---|---|
| `Stg_end_recovery` (48) | gz_erc | `mry_yarinaoshi_a/e/i` (やり直し, "starting over") | none named `end_*`; `mry_yarinaoshi_*_dds` | 0x01 | Leave | verified | `EvProgLastScene` of the forest stage (stg_forest.c), the scene with the `mry_yarinaoshi_i` demo, loads Laura (kind 260, `data_chr_lau_lau_mdl`), plays the movie `ending.pss` and sets `clear_end_kind \|= 1`. Leaving with Laura is the Leave ending. The stage name "recovery" and the demo name "starting over" fit it too. |
| `Stg_end_maria` (49) | gz_emr | `mar_isho_a/e/i` (いっしょ "together", or 遺書 "farewell letter"; the reading is unclear) | `end_maria_a/e/i_dds` | 0x02 (suspected) | Maria | verified | Picture names `end_maria_*`. `EvProgLastScene` of the observation stage (stg_ovservation.c) loads Maria (kind 261, `data_chr_mar_lll_mar_mdl`) with the demo `mar_isho_i`. The town-west stage has `EvProgEndMaria`. |
| `Stg_end_suicide` (50) | gz_esu | `jisatsu_a/e` (自殺, "suicide") | `end_suicide_a/e_dds` | 0x04 (suspected) | In Water | by name | Picture names `end_suicide_*`. In Water is the ending in which James drives his car into Toluca Lake. |
| `Stg_end_rebirth` (51) | gz_erb | `fukatsu_a/d` (復活, "resurrection") | `end_rebirth_a/d_dds` | 0x08 (suspected) | Rebirth | by name | Picture names `end_rebirth_*`. `fukatsu_d` has `boat_anm` and `mx2_anm`. The lake stage (gz_lak) has `EvProgEndRebirth`. |
| `Stg_end_dog` (52) | gz_edg | `inu` (犬, "dog") | `end_inu_dds` | 0x10 (suspected) | Dog | by name | Picture `end_inu`, `inu_anm`. The stage has `EvProgEndingDog`. The dog key: `EvProgGetDogKey` (town west), `EvProgUseDogKey` (hotel 3F otherworld). |

This build has no UFO ending: no stage, demo, movie or picture in the file index
(include/sh2/variables.h) has a `ufo` name, and no item model is named for the Blue Gem (see
[prototype.md](prototype.md)).

Notes:
- The stage enum is in src/Event/stg_overlay.c (the DWARF leaves it anonymous; the name `STG_NAME`
  is ours). James's per-stage animation list in `CharaDataLoadExecJames`
  (src/Event/chara_data_load.c) follows the same order.
- Bit 0x01 is verified (`EvProgLastScene`, stg_forest.c). The other bits are assigned by order:
  `ResultMain` (src/SH2_common/result.c) tests bits 0x10, 0x08, 0x04, 0x02, 0x01 and prints
  messages 11, 10, 9, 8, 7. It then chooses the count format `n/4(+1)`, which treats one ending as
  an extra fifth, by `n & 0x10`, but by then `n` holds the message number (7-11), so that branch
  never runs (marked `@bug` in the source). The intent, a "+1" ending next to four others, matches
  the public status of the Dog ending.
- Game flags 7, 8 and 9 are the hints for the Leave, Maria and In Water endings (strongly
  suspected). `FlagInit` (src/Event/event.c) sets them at New Game from bits 0x01-0x04 of
  `clear_end_kind`: with two of those endings seen, the flag of the third (7 for bit 0x01, 8 for
  0x02, 9 for 0x04); with one seen, one or both flags of the other two, partly at random, never
  the seen one's. The Wood Side 2F stage's event list (`ev_list`, stg_apart_e2f.c) has three
  entries that require flag 7, 8 and 9 and start `EvProgEndHintRecoveryRead`,
  `EvProgEndHintMariaRead` and `EvProgEndHintSuicideRead`: decoded with `EventListElement`
  (src/Event/event.c), the flag number is `flag` bits 16-29, bit 31 set means the flag must be
  on (`EventCheck` skips the entry otherwise), and `rslt1` bits 14-20 index the stage's `ev_prog`
  table (`EventExecProgram`). The hint picture is `data_pic_apt_p_endhint_tex`. The readables themselves
  are the inference: the code shows only which event each flag enables.
- `FlagInit` sets flag 10 when at least one of those three endings has been seen, and flag 11 when
  all three have or bit 0x08 (Rebirth, by the order above) is set. Flag 11 is plausibly the
  condition for the Dog key (`EvProgGetDogKey` is in town west, not on the disc): suspected.
- Demo suffixes: every `_a` demo holds `bos_anm` + `mry_anm` (Mary and her boss form). The `_e`
  demos hold `i_letterm_anm` (reading: Mary's letter) and MXX or Maria. The `_i` demos are the final
  scenes, with Laura or Maria. `_d` is the Rebirth boat scene. What the letters stand for is
  unknown.
- Every ending stage has `EvProgMaryBeforeBattle`, `EvProgMaryFall` and `EvProgKaidanFall`
  ("kaidan" = stairs). Only recovery and suicide also have `EvProgChangeMaryRoom` and
  `EvProgMaryAfterBattle`. So in this build, Leave and In Water have a scene with Mary after the
  fight and Maria and Rebirth don't (a reading of the names only).

## Stages (`STG_NAME`, src/Event/stg_overlay.c)

52 stages: `tgs_trial` is built into main, the other 51 are overlays. "Disc": whether the stage's
code is on this disc. `glb_crd` is the stage's map ID (the `Stage_Data.glb_crd` field) where the
stage data is in the repo; where it isn't, the ID is inferred and marked.

| ID | Name | Overlay | Disc | `glb_crd` | Area (public name) | Confidence | Evidence |
|---|---|---|---|---|---|---|---|
| 1 | tgs_trial | none (in main) | yes | 10 | A trial version of Brookhaven Hospital, probably the Tokyo Game Show demo | verified (hospital); "TGS" = Tokyo Game Show suspected | src/Event/stage/stg_tgs_trial.c: `EvProgTrialStartSet`, `EvProgGetHospitalMap`, `EvProgTrialEnd`. `StgOverlayGetFileID` has no file for it and `StgOverlayGetStageData` defaults to it (stg_overlay.c). |
| 2 | toilet | gx_toi | yes | 6 | The restroom where the game starts | verified | `EvProgPrologueInToilet`, demo `first_toilet`. The normal New Game starts here (`TitleJPStartPointList[0]`, stage 2, src/Event/title.c). |
| 3 | observation | gx_obs | yes | 5 | Observation deck and parking lot at the start | verified | `EvProgLetterFromMary`, `EvProgItemInCar`, `EvProgHadBetterGetMap`; the Maria ending's last scene (`EvProgLastScene`). File name spelled stg_ovservation.c. |
| 4 | forest | gx_fst | yes | 1 | Forest trail and cemetery | verified | `EvProgAngelaInGrave`, `EvProgGraveLookingFor`, demo `haka_agl` ("haka" = grave); `EvProgGetChainsaw`; the Leave ending's last scene (`EvProgLastScene`). |
| 5 | town_east | gx_twe | yes | 2 | East South Vale (the streets up to Wood Side) | verified (area); the district name comes from public sources | `EvProgFirstMonsterStart`/`Corpse`/`End`, `EvProgGetApartGateKey`/`EvProgUseApartGateKey`. |
| 6 | apart_e1f | gx_aex | yes | 9 | Wood Side Apartments 1F | verified (building); the floor is read from the name | `EvProgVomitEddie`, `EvProgTouristGuideRead`. |
| 7 | apart_e2f | gx_aey | yes | 9 | Wood Side Apartments 2F | verified | Clock puzzle (`EvProgUseClockKey`, `EvProgTryMoveClock`), room-202 key, `EvProgLookDustChute`, `EvProgGetLight`, `EvProgUseEmergencyKey` (the fire escape door; retail puts it at the west end of the 2F corridor). |
| 8 | apart_e3fw | gx_aew | yes | 9 | Wood Side Apartments 3F, west part | verified (building); "west part" suspected, but it agrees with retail's placement of these events | `EvProgLauraKickKey`, `EvProgGetHandgun`. |
| 9 | apart_e3fe | gx_aee | yes | 9 | Wood Side Apartments 3F, east part | verified (building); "east part" suspected, but it agrees with retail's placement of these events | `EvProgFirstMeetTrihead` (Pyramid Head and the mannequins), `EvProgGetYardKey`, `EvProgGetEmergencyKey`. |
| 10 | apart_w1f | gx_awx | yes | 9 | Blue Creek Apartments 1F | verified | `EvProgAngelaWithKnife`, the three-coin puzzle, `EvProgGetLyneKey`. Public walkthroughs put Angela in Blue Creek room 109 and the coin puzzle with the Lyne House key in room 105. |
| 11 | apart_w2f | gx_awy | yes | 9 | Blue Creek Apartments 2F | verified (building); floor from the name | `EvProgReadDearTim`, `EvProgUseLyneKey`, `EvProgOpenSafe`, `EvProgGetApartStairKey`. |
| 12 | apart_stair | gx_ast | yes | 9 | The stairways and halls that join the apartment floors and buildings, including the first Pyramid Head fight | verified (the fight); the extent of the area is suspected | `EvProgApartBoss`, `EvProgApartBossSiren`, `EvProgApartBossEnd`; `EvProgUseYardKey` (retail's courtyard door is in Wood Side's west stairway hall); both apartment maps (`EvProgGetApartMap`, `EvProgGetApartWestMap`); `EvProgGetCannedJuice`. |
| 13 | apart_out | gx_aot | yes | 2 | The outdoor parts of the apartment block: the way between Wood Side and Blue Creek, and probably the courtyard | verified (outdoors, between the buildings); the rest suspected | `EvProgApartEastToWest`, `EvProgApartWestToEast`, `EvProgGetCoinOfSnake`, `EvProgGetCoinOfOldman`, `EvProgMurderNewsRead`. Uses the town map (`glb_crd` 2 in `stage_apart_out`, stg_apart_out.c). Its enemy positions fall in room 7 (see Rooms). |
| 14 | town_west | gx_tww | no | unknown | West South Vale (Rosewater Park, Jack's Inn area) | by name; the district name comes from public sources | `EvProgMariaMeeting`, `EvMariaInParkSet`, `EvProgMariaFrontBowling1st`, `EvProgGetDogKey`, `EvProgLauraGoHospital`, `EvProgEndMaria`. |
| 15 | bowling | gx_bow | no | 7 (suspected) | Pete's Bowl-O-Rama | by name | `EvProgEddieAndLaura`, `EvProgEddieEatPizza`. `glb_crd` 7 has `RoomNameBowling` (`RoomName`, src/Event/stg_name.c). |
| 16 | to_heaven | gx_thv | no | unknown | Street approach to Heaven's Night | suspected | `EvProgLauraWentSideRoad`, `EvProgMariaUnlockHeaven`, `EvProgJamesCantGo`. |
| 17 | heaven_night | gx_hvn | no | 8 (suspected) | Heaven's Night | by name | Stage name; no functions. `glb_crd` 8 has `RoomNameHeaven`. |
| 18-21 | hospital_1f_f, 2f_f, 3f_f, rf_f | gy_hxf, gy_hyf, gy_hzf, gy_hrf | no | 10 (suspected) | Brookhaven Hospital, fog world, 1F-3F and roof | by name | 1F: `EvProgLauraDeceive1st`, `EvProgReadWhiteBoard`; 2F: `EvProgGetShotgun`, `EvProgGetLapisEyeKey`; 3F: `EvProgInShowerDrain`, `EvProgLostMaria`; roof: `EvProgTriheadOnRoof`, `EvProgReadDiary`. |
| 22-26 | hospital_1fw_b, 1fe_b, 2f_b, 3f_b, bf_b | gy_hxw, gy_hxe, gy_hyb, gy_hzb, gy_hbb | no | 10 (suspected) | Brookhaven Hospital, otherworld (1F west/east, 2F, 3F, basement) | by name (see the _f/_b note) | 1FW: `EvProgLauraDeceive2nd`/`3rd`; 1FE: `EvProgMariaKilledAfter`; basement: `EvProgMariaMeetAgain`. |
| 27 | hospital_pass | gy_hps | no | unknown | The otherworld hospital corridor chase where Maria dies | by name | `EvProgChaseInHospital1st`-`5th`, `EvProgMariaKilled`. |
| 28 | society | gy_soc | no | unknown | Toluca Historical Society | by name | `EvProgTriheadPicture` (the Pyramid Head painting), `EvProgGetGoblet`. Entry 3 of the JP test start list (`TitleJPStartPointList`, title.c) is stage 28. |
| 29-30 | delusion_2, delusion_3 | gy_dly, gy_dlz | no | 11 (suspected) | The descent below the Historical Society (holes, the well) | suspected | `EvProgHoleTo3rdLayer`; `EvProgStartBottomOfWell`, `EvProgBreakWellWall`, `EvProgGetSpiralKey`, `EvProgHoleTo4thLayer`. |
| 31-33 | prison_n, prison_s, prison_bf | gy_psn, gy_pss, gy_psb | no | 11 (suspected) | Toluca Prison (north, south, basement) | by name | `EvProgGetTabletPig`, `EvProgGetTabletSeduct`, `EvProgGetTabletOppressor`, `EvProgGetRifle`, `EvProgGetWaxDoll`, `EvProgGetHorseshoe`, `EvProgEddieKillHuman`; `EvProgHoleTo5thLayer`-`7thLayer`. |
| 34-36 | labyrinth_w, labyrinth_e, labyrinth_n | gz_lrw, gz_lre, gz_lrn | no | unknown | The Labyrinth | by name | `EvProgGetGreatKnife`, `EvProgMariaIsAlive`; `EvProgPapaAttackAngela`; `EvProgMariaRedeath`, `EvProgHangingCorpse`, `EvProgHoleTo9thLayer`. |
| 37 | eddie_boss | gz_edb | no | unknown | The Eddie fight (a room of hanging meat) | by name | `EvProgEddieMurderer1st`-`4th`; the NIK meat positions (`nik_pos_data`, en_edb.c). |
| 38 | lake | gz_lak | no | unknown | Toluca Lake (by boat) | by name | `EvProgEmbarkBoat`, `EvProgDisembarkBoat`, `EvProgGetLittleMermaid`, `EvProgEndRebirth`. |
| 39-42 | hotel_bf_f, 1f_f, 2f_f, 3f_f | gz_rbf, gz_rxf, gz_ryf, gz_rzf | no | 12 (suspected) | Lakeview Hotel, fog world (basement, 1F-3F) | by name | `EvProgReadReceptionMemo`, `EvProgLauraPlayPiano`, `EvProgGetVideoTape`, `EvProgUseHotel312Key`, `EvProgVideoReplay`, `EvProgMemoryOfMurder`. |
| 43-46 | hotel_bf_b, 1f_b, 2f_b, 3f_b | gz_rbb, gz_rxb, gz_ryb, gz_rzb | no | 13 (suspected) | Lakeview Hotel, otherworld | by name | `EvProgTriheadBattle`, `EvProgTriheadDead` (1F); `EvProgNotifyDeath` (2F); `EvProgUseDogKey` (3F); `EvProgOpenElevatorWater` (basement). |
| 47 | hotel_fire | gz_rfr | no | unknown | The burning stairway with Angela | by name | `EvProgAngelaInFire`, `EvProgFireDamage`. |
| 48-52 | end_recovery, end_maria, end_suicide, end_rebirth, end_dog | gz_erc, gz_emr, gz_esu, gz_erb, gz_edg | no | 14 (suspected) | Final area (Mary's fight) and the endings | see Endings | See above. |

Fog world vs otherworld (`_f`/`_b`): the fog-world events (Laura's first trick, the reception
memo, the piano) are all in `_f` stages. The otherworld events (Maria's rescue in the basement, the
chase, the Pyramid Head fight, the fire) are all in `_b` stages. The room functions for the hotel
are `RoomNameHotelFace` and `RoomNameHotelBack` (src/Event/stg_name.c). So `_f`/`_b` probably mean
front/back (Japanese omote/ura), and the same idea appears in the demo `go_to_ura` and the models
`ura0`-`ura3`. By name for the placement of events, suspected for the meaning of the letters.

## Map IDs (`glb_crd` = `enum STAGE_ID`, include/sh2/types.h)

`RoomName` (src/Event/stg_name.c) turns a position into a room number per map. `BgIsOut`
(stg_name.c) treats maps 1-4 as outdoors. The BG directory prefix comes from `FilesBgStageList`
(src/FilesList/fileslist_bg.c).

| `glb_crd` | BG prefix | Room function | Rooms | Stages using it | Area | Confidence |
|---|---|---|---|---|---|---|
| 1 | ca | (constant) | 3 | forest | Forest trail and cemetery | verified (stage data) |
| 2 | cb | `RoomNameTownEast` | 4, 7 | town_east, apart_out | East town and the outside of the apartments | verified (stage data) |
| 3 | cc | (constant) | 8 | unknown | unknown. Outdoors, and the largest outdoor block table (`block_c`, 76 blocks). Probably West South Vale | suspected |
| 4 | cd | (constant) | 0x0E | unknown | unknown. Outdoors, 6 blocks (`block_d`; the lookup reads a 7th, `@bug` in stg_name.c) | unknown |
| 5 | ob | (constant) | 2 | observation | Observation deck | verified (stage data) |
| 6 | er | `RoomNameExtra` | 1, 5, 6, 0xBE | toilet | The restroom (room 1) and three other small rooms | verified for room 1; others unknown |
| 7 | bw | `RoomNameBowling` | 9-11 | bowling (suspected) | Bowling alley | by name |
| 8 | th | `RoomNameHeaven` | 12-13 | heaven_night (suspected) | Heaven's Night | by name |
| 9 | ap | `RoomNameApart` | 15-40 (0x0F-0x28) | all apart_* except apart_out | Wood Side and Blue Creek | verified (stage data) |
| 10 | hp | `RoomNameHospital` | 41-91 (0x29-0x5B) | tgs_trial; hospital_* (suspected) | Brookhaven Hospital | verified (tgs_trial) |
| 11 | ps | `RoomNameDelusion` | 92-144 (0x5C-0x90) | unknown | "ps" suggests the prison; "Delusion" suggests the delusion_* stages | suspected |
| 12 | rr | `RoomNameHotelFace` | 145-169 (0x91-0xA9) | hotel_*_f (suspected) | Lakeview Hotel, fog world | by name |
| 13 | ru | `RoomNameHotelBack` | 170-187 (0xAA-0xBB) | hotel_*_b (suspected) | Lakeview Hotel, otherworld | by name |
| 14 | qp | `RoomNameLastStage` | 188-189 (0xBC-0xBD) | end_* (suspected) | Final area | suspected |
| 15 | qt | (constant) | 191 (0xBF) | unknown | unknown | unknown |
| 16 | ma | `RoomNameMansion` | 192-210 (0xC0-0xD2) | none in `STG_NAME` | A mansion (see prototype.md) | unknown |

Room numbers are global (one number space across maps). `room_to_block` (stg_name.c) has 211
entries, rooms 0-210, and gives each indoor room its four BG blocks.

### Rooms with a known identity

| Room | Map | Place | Confidence | Evidence |
|---|---|---|---|---|
| 1 | 6 (toilet) | The prologue restroom (with the mirror) | verified | `RoomNameExtra` returns 1. James's mirror copy is loaded in rooms 1 and 0x24 (`CharaDataLoadRoom`, chara_data_load.c), and the prologue scene loads the mirrored drama James (`EvProgPrologueInToilet`, stg_toilet.c). |
| 2 | 5 | Observation deck (whole map) | verified | `RoomName` case 5. |
| 3 | 1 | Forest (whole map) | verified | `RoomName` case 1. |
| 4 | 2 | Town east (most of the map) | verified | `RoomNameTownEast` default. |
| 7 | 2 | Outside the apartments | verified | `RoomNameTownEast` returns 7 for cells 0x26B-0x2AC. The apart_out enemy (84000, -72500) falls in cell 0x28C. |
| 9-11 | 7 | Bowling alley rooms | by name | `RoomNameBowling`. |
| 12-13 | 8 | Heaven's Night rooms | by name | `RoomNameHeaven`. |
| 0x24 (36) | 9 | The apartment room with the mirror where Angela holds the knife (Blue Creek room 109 in public walkthroughs) | suspected | Mirror models are loaded here (`CharaDataLoadRoom`). The Angela knife scene uses mirrored James and Angela (`EvProgAngelaWithKnife`, stg_apart_w1f.c). |
| 0x3A (58) | 10 | Hospital room with the buzzing fly (TGS trial) | verified (content) | The TGS trial's `EvRoomInit` (stg_tgs_trial.c). |
