# Symbols of the overlays missing from the disc

The executable's symbol table has entries for all 51 stage overlays, but only 12 overlay binaries
are on this disc ([rom-map.md](rom-map.md)). For the other 39, the function names are all that is
left of their code. They are the developers' own names, and several docs use them as evidence
([characters.md](characters.md), [endings-and-stages.md](endings-and-stages.md),
[game-ids.md](game-ids.md)): this table lets a reader check those claims without the disc.

Generated once from the prototype's ELF symbol table (`SLUS_202.28`): every `FUNC` symbol of each
missing overlay's section (`gx_*`, `gy_*`, `gz_*`), in address order, and the stage from the
overlay's `stage_<name>` object symbol (the `Stage_Data` the game points `stage` at). The stage ID
is the `STG_NAME` value in src/Event/stg_overlay.c. `configure.py` writes the same symbols, with
addresses, to `config/symbols_extern.txt` (generated from the disc, so not in the repository). Only
names are listed here: no code or data of the game.

"Event programs" are the functions named `EvProg*`. In the stages on the disc
(src/Event/stage/*.c), those are the functions in the stage's `ev_prog` table, which its event list
(`ev_list`) starts, plus a few `EvProgSub*` helpers they call. Heaven's Night (17) has no functions
at all.

| ID | Stage | Overlay | Event programs | Other functions |
|---|---|---|---|---|
| 14 | town_west | gx_tww | `EvProgLauraOnWall`, `EvProgMariaMeetingStandby`, `EvProgMariaMeeting`, `EvProgMariaNotHotel`, `EvProgBowlDemoDataLoad`, `EvProgMariaFrontBowling1st`, `EvProgMariaFrontBowling2nd`, `EvProgLauraCantGo`, `EvProgEnemyLoadWestTownStart`, `EvProgEnemyLoadWestTownInit`, `EvProgLadyHoleMake`, `EvProgUseOldBronzeKey`, `EvProgGetWrench`, `EvProgUseWrench`, `EvProgSubCaseDraw`, `EvProgLookMetalBox`, `EvProgLauraGoHospital`, `EvProgGetSteelPipe`, `EvProgGetLostMemory`, `EvProgGetDogKey`, `EvProgEndMaria` | `EvStageInit`, `EvCharaDataClear`, `EvRoomInit`, `EvAllTimeFunc`, `EvBgmControl`, `EvMariaInParkSet`, `EvLadyBackDisplay`, `EvSignalControlCc10`, `EvSignalControlCc20`, `EvCharaDataLoadAfterWall`, `EvCamaraLostMemory`, `LinearTrim2`, `CC_Demo_Fog_Hosei` |
| 15 | bowling | gx_bow | `EvProgEddieAndLaura`, `EvProgEddieEatPizza`, `EvProgEddieEatPizzaAfter` | `EvStageInit`, `EvCharaDataClear`, `EvRoomInit`, `EvBgmControl`, `EvAllTimeFunc` |
| 16 | to_heaven | gx_thv | `EvProgLauraWentSideRoad`, `EvProgMariaUnlockHeaven`, `EvProgJamesCantGo` | `EvCharaDataClear`, `EvRoomInit` |
| 17 | heaven_night | gx_hvn | none | none |
| 18 | hospital_1f_f | gy_hxf | `EvProgGetHospitalMap`, `EvProgDoctorMemo1st`, `EvProgDoctorMemo2nd`, `EvProgUseExamKey`, `EvProgReadWhiteBoard`, `EvProgCardsOfPatient`, `EvProgHospitalExit`, `EvProgLauraDeceive1st` | `EvCharaDataClear`, `EvRoomInit`, `EvAllTimeFunc`, `EvBgmControl` |
| 19 | hospital_2f_f | gy_hyf | `EvProgGetLapisEyeKey`, `EvProgReadCarbonNumber`, `EvProgGetExamKey`, `EvProgNeedleInAnimal`, `EvProgUseElevatorKey`, `EvProgGetShotgun` | `EvCharaDataClear`, `EvRoomInit` |
| 20 | hospital_3f_f | gy_hzf | `EvProgLostMaria`, `EvProgLostMariaAfter`, `EvProgGetRoofKey`, `EvProgGuruguruNumber`, `EvProgSubGuruguruDraw`, `EvProgOnlyNeedle`, `EvProgOnlyHair`, `EvProgInShowerDrain`, `EvProgFishKey`, `EvProgBoxWithKey`, `EvProgSubKeyLayer`, `EvProgSubKeyCursor`, `EvProgSubHairInBox`, `EvProgEmptyBox`, `EvProgSetPassNumberT`, `EvProgSubSetNumberPicDisp`, `EvProgSubSetNumberButton`, `EvProgLouiseTakecare`, `EvProgUseElevatorKey`, `EvProgFromRoof` | `EvCharaDataClear`, `EvRoomInit`, `EvAllTimeFunc` |
| 21 | hospital_rf_f | gy_hrf | `EvProgElevatorButton`, `EvProgElevatorButtonCheck`, `EvProgElevatorButtonLight`, `EvProgUseRoofKey`, `EvProgTriheadOnRoof`, `EvProgTriheadEntry`, `EvProgReadDiary`, `EvProgMetalNetRusty` | `EvCharaDataClear`, `EvRoomInit`, `EvAllTimeFunc` |
| 22 | hospital_1fw_b | gy_hxw | `EvProgLauraDeceive2nd`, `EvProgLauraDeceive3rd`, `EvProgHospitalBack1st`, `EvProgHospitalBack2nd` | `EvRoomInit`, `EvSoundCallAfterLoad`, `EvAllTimeFunc`, `EvBgmControl` |
| 23 | hospital_1fe_b | gy_hxe | `EvProgMariaKilledAfter`, `EvProgUseHospitalEnterKey`, `EvProgGetHospitalEnterKey`, `EvProgLookDirectorMap` | none |
| 24 | hospital_2f_b | gy_hyb | `EvProgRefrigeratorOpen`, `EvProgRefrigeratorNotOpen`, `EvProgGetDryCell`, `EvProgGetStoreroomKey` | `EvRoomInit`, `EvAllTimeFunc` |
| 25 | hospital_3f_b | gy_hzb | `EvProgPictureOfAngel`, `EvProgAngelNoOneKnows`, `EvProgTryQuizAnswer`, `EvProgSubAnswer`, `EvProgIncorrectQuizAnswer` | `EvSubAngelRingDraw`, `EvStageInit`, `EvAllTimeFunc` |
| 26 | hospital_bf_b | gy_hbb | `EvProgMariaMeetAgain`, `EvProgMoveEmptyShelf`, `EvProgGetCopperRing`, `EvProgUseStoreroomKey`, `EvProgElevatorButton`, `EvProgElevatorButtonCheck`, `EvProgElevatorButtonLight`, `EvProgDirectorBronzeKey` | `EvCharaDataClear`, `EvRoomInit`, `EvAllTimeFunc` |
| 27 | hospital_pass | gy_hps | `EvProgChaseInHospital1st`, `EvProgChaseInHospital2nd`, `EvProgChaseInHospital3rd`, `EvProgChaseInHospital4th`, `EvProgChaseInHospital5th`, `EvProgMariaKilled` | `EvCharaDataClear`, `EvRoomInit`, `EvBgmControl`, `Maria_Killed_Hosei` |
| 28 | society | gy_soc | `EvProgTriheadPicture`, `EvProgGetGoblet` | none |
| 29 | delusion_2 | gy_dly | `EvProgHoleTo3rdLayer` | `EvBgmControl` |
| 30 | delusion_3 | gy_dlz | `EvProgStartBottomOfWell`, `EvProgBreakWellWall`, `EvProgGetSpiralKey`, `EvProgCocRoomLock`, `EvProgSubLockLayer`, `EvProgSubLockCheck`, `EvProgUseSpiralKey`, `EvProgHoleTo4thLayer`, `EvProgUseDryCell` | `EvRoomInit`, `EvAllTimeFunc` |
| 31 | prison_n | gy_psn | `EvProgEddieKillHuman`, `EvProgGetPrisonMap`, `EvProgGetTabletPig`, `EvProgGetTabletSeduct`, `EvProgGetRifle`, `EvProgGetLighter`, `EvProgReadLakeMagazine`, `EvProgTryOpenFloor`, `EvProgSuccessOpenFloor`, `EvProgSubPullDraw`, `EvProgHoleTo5thLayer` | `EvAllTimeFunc` |
| 32 | prison_s | gy_pss | `EvProgGetTabletOppressor`, `EvProgGetWaxDoll`, `EvProgGetHorseshoe`, `EvProgTabletSetScaffold`, `EvProgSubDrawTabletAndScaffold` | `EvRoomInit`, `EvAllTimeFunc` |
| 33 | prison_bf | gy_psb | `EvProg5thLayerStart`, `EvProgHoleTo6thLayer`, `EvProg6thLayerStart`, `EvProgDoorOpen6Layer`, `EvProgHoleTo7thLayer` | `EvAllTimeFunc` |
| 34 | labyrinth_w | gz_lrw | `EvProg7thLayerStart`, `EvProgGetGreatKnife`, `EvProgControlCube`, `EvProgMariaIsAlive`, `EvProgAfterMariaIsAlive`, `EvProgGetWireCutter`, `EvProgUseWireCutter` | `PS89_ParallelLightSet`, `EvRoomInit`, `EvSoundCallAfterLoad`, `EvAllTimeFunc`, `EvElevatorModelMove` |
| 35 | labyrinth_e | gz_lre | `EvProgScreamOfAngela`, `EvProgPapaAttackAngela`, `EvProgAngelaAttackPapa`, `EvProgNewsOfPapa` | `PSLE_ParallelLightSet`, `EvCharaDataClear`, `EvRoomInit`, `EvAllTimeFunc`, `EvBgmControl` |
| 36 | labyrinth_n | gz_lrn | `EvProgHangingCorpse`, `EvProgHangRope`, `EvProgReadHangHint`, `EvProgReadPullOnce`, `EvProgGetFalseChargeKey`, `EvProgCanNotRotate`, `EvProgUseFalseChargeKey`, `EvProgMariaRedeath`, `EvProgHoleTo9thLayer` | `PSLN_ParallelLightSet`, `EvRoomInit`, `EvAllTimeFunc`, `EvBgmControl` |
| 37 | eddie_boss | gz_edb | `EvProgEddieMurderer1st`, `EvProgEddieMurderer2nd`, `EvProgEddieMurderer3rd`, `EvProgEddieMurderer4th` | `EvCharaDataClear`, `EvRoomInit` |
| 38 | lake | gz_lak | `EvProgEmbarkBoat`, `EvProgDisembarkBoat`, `EvProgGetLittleMermaid`, `EvProgEndRebirth` | `EvCharaDataClear`, `EvRoomInit`, `EvAllTimeFunc` |
| 39 | hotel_bf_f | gz_rbf | `EvProgGetThinner`, `EvProgGetBarKey`, `EvProgNoLabelCan`, `EvProgGetLightbulb`, `EvProgUseLightbulb`, `EvProgUseBarKey` | `EvRoomInit`, `EvSoundCallAfterLoad`, `EvAllTimeFunc` |
| 40 | hotel_1f_f | gz_rxf | `EvProgGetHotelGuestMap`, `EvProgGetHotelEmployeeMap`, `EvProgGetHotel312Key`, `EvProgReadReceptionMemo`, `EvProgOrgel`, `EvProgSubOrgelPlay`, `EvProgGetHotelStairKey`, `EvProgGetVideoTape`, `EvProgSubVideoDraw`, `EvProgNothingInSafe`, `EvProgGetSnowWhite`, `EvProgGetFishKey`, `EvProgLauraPlayPiano` | `Draw_rr41Window`, `EvStageInit`, `EvCharaDataClear`, `EvRoomInit`, `EvAllTimeFunc`, `EvBgmControl` |
| 41 | hotel_2f_f | gz_ryf | `EvProgItemPutForShelf`, `EvProgUseHotel204Key`, `EvProgUseHotelElevatorKey`, `EvProgElvatorButton`, `EvProgSubElevatorButton`, `EvProgGetBaggageInShelf`, `EvProgPutBaggageForShelf`, `EvProgUseFishKey`, `EvProgSubCaseDraw`, `EvProgFishCaseClose`, `EvProgFishCaseOpen`, `EvProgTrunkUnlockChallenge`, `EvProgSubTrunkDrumDraw`, `EvProgSubTrunkDrumCheck`, `EvProgPictureWithMarker`, `EvProgSubDrawMarkerAndWork`, `EvProgGetElvHallKey`, `EvProgGetCinderella` | `EvAlphabetToNumber`, `EvStageInit`, `EvRoomInit`, `EvAllTimeFunc` |
| 42 | hotel_3f_f | gz_rzf | `EvProgUseHotelStairKey`, `EvProgUseHotel312Key`, `EvProgMemoryOfMurder`, `EvProgVideoReplay` | `Draw_rr91_rr94Window`, `EvRoomInit`, `EvAllTimeFunc`, `RR91_Filter` |
| 43 | hotel_bf_b | gz_rbb | `EvProgOpenElevatorWater`, `EvProgSubElevatorButton`, `EvProgElevatorOutCamChange` | `EvRoomInit`, `EvAllTimeFunc` |
| 44 | hotel_1f_b | gz_rxb | `EvProgTriheadBattle`, `EvProgTriheadDead`, `EvProgGetEggRustColored`, `EvProgGetEggScarlet`, `EvProgEggHoleOnDoor`, `EvProgSubDrawEgg` | `EvCharaDataClear` |
| 45 | hotel_2f_b | gz_ryb | `EvProgNotifyDeath`, `EvProgGetCrimsonCeremony` | `EvCharaDataClear`, `EvRoomInit`, `EvAllTimeFunc` |
| 46 | hotel_3f_b | gz_rzb | `EvProgUseDogKey` | none |
| 47 | hotel_fire | gz_rfr | `EvProgAngelaInFire`, `EvProgFireDamage` | `ru21_ru24_LightFlame` |
| 48 | end_recovery | gz_erc | `EvProgMaryBeforeBattle`, `EvProgMaryFall`, `EvProgChangeMaryRoom`, `EvProgMaryAfterBattle`, `EvProgKaidanFall` | `EvStageInit`, `EvCharaDataClear`, `EvRoomInit`, `EvAllTimeFunc` |
| 49 | end_maria | gz_emr | `EvProgMaryBeforeBattle`, `EvProgMaryFall`, `EvProgKaidanFall` | `EvCharaDataClear`, `EvRoomInit`, `EvAllTimeFunc` |
| 50 | end_suicide | gz_esu | `EvProgMaryBeforeBattle`, `EvProgMaryFall`, `EvProgChangeMaryRoom`, `EvProgMaryAfterBattle`, `EvProgKaidanFall` | `EvStageInit`, `EvCharaDataClear`, `EvRoomInit`, `EvAllTimeFunc` |
| 51 | end_rebirth | gz_erb | `EvProgMaryBeforeBattle`, `EvProgMaryFall`, `EvProgKaidanFall` | `EvCharaDataClear`, `EvRoomInit`, `EvAllTimeFunc` |
| 52 | end_dog | gz_edg | `EvProgEndingDog` | none |
