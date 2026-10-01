# Game ID spaces

IDs that modders and researchers meet in this prototype, with what the repo proves about them.
Confidence labels:
- **verified**: code on the disc ties the ID to the thing (an event program in the repo hands out
  that ID, or the code's behaviour shows it).
- **model**: only the model file's name suggests it. That is a reading, so treat it as suspected.
- **suspected**: some other inference, with the reason given.
- **unknown**: no evidence.

"Exists" after an event name means the function is in the symbol table of a stage whose code
isn't on the disc ([missing-overlays.md](missing-overlays.md) lists them): the name is the
developers', but no code on the disc shows what the event does. Retail names are from the
original 2001 release. Stage, map and room IDs are in endings-and-stages.md; character kinds are
in characters.md; pad button masks are in prototype.md.

## Character kind ranges

`shCharacterSetHandler` and `shCharacterGetSkeltonNum` (src/Chacter/m3_sc.c)
split kinds by their high byte:

| Range | What | Evidence |
|---|---|---|
| 0x1xx | Humans (James, drama copies, mirror copies at +0x20, Laura, Maria, Angela, Eddie, Mary, the boat, the dog) | `human_skelton[kind - 0x100]`; characters.md |
| 0x2xx | Enemies | `enemy_skelton[kind - 0x200]`; `enTransID` (src/Enemy/en_common.c) |
| 0x3xx | Outdoor objects: not created as characters, handed to the BG (`BgCharaRelocateSet`) | `CharaDataLoadRoom` (src/Event/chara_data_load.c) |
| 0x4xx | Animated objects (doors, props, the hanging meat 0x421; 0x443/0x444 are mirrored copies) | m3_sc.c; `item_list` |
| 0x5xx | Static ("stay") objects | `obj_stay_skelton` |
| 0x6xx | Items drawn on the item screen | `item_screen_obj_data` (src/Chacter/item_screen_obj.c) |
| 0x7xx | Items lying in the world (pick-ups) | `item_to_chara` (below); `item_list` |
| 0x8xx | James's weapons; bit 0x20 = mirrored copy | `weapon_file`; m3_sc.c `case 8` |

Demos also load 0x4xx kinds for props they animate. Examples: 1026 `i_keycou`, 1027 `i_radio`,
1028 `i_kakuzai`, 1036 `b_doo`, 1037 `i_handgun`, 1038 `i_magazine`, 0x41D `i_needle`, 0x41E
`i_keyelevator` (`EvProgLauraKickKey` in stg_apart_e3fw.c, `EvRoomInit` in stg_town_east.c,
`EvProgFirstMeetTrihead` in stg_apart_e3fe.c, `EvProgFishKey` in stg_tgs_trial.c; `anim_info` in
src/Event/demoview.c lists every demo prop's kind).

## Inventory items (`item.flag` bit / `item.number` index)

`struct Item` (include/sh2/types.h): `flag[2]` holds one bit per item kind, and `number[11]`
counts kinds 0-10. `ItemGet` (src/Event/item.c) sets the bit and adds a pick-up amount: medicine
+1, handgun ammo +10, shotgun ammo +6, rifle ammo +4, kind 10 +8. Ammo amounts are multiplied by
`playing.bullet_adjust`. `ItemUse` (item.c) treats kinds 75 and up as combinations.
`item_to_chara` (src/Event/event_sub.c) maps each kind to the world model (0x7xx) shown when it
lies in a stage. The model file comes from `item_list` / `bullet_and_drug_file`
(src/Event/chara_data_load.c).

| ID | World model kind | Model file | Reading | Confidence | Evidence |
|---|---|---|---|---|---|
| 0 | none | none | nothing / no weapon | verified | `item.equip == 0` has James's no-weapon animation (`weapon_file`, chara_data_load.c) |
| 1 | 0x700 | `x_drink` | Health drink (heals 1/4 of max HP) | verified | `ItemMedicineUse` (item.c) |
| 2 | 0x701 | `x_firstaid` | First aid kit (heals 1/2) | verified | `ItemMedicineUse` |
| 3 | 0x733 | `x_ample` | Ampoule (full heal, then a 600-unit effect) | verified | `ItemMedicineUse`; `ItemAmpolueEfficacy` |
| 4 | 0x702 | `x_handgun` | Handgun (magazine 10) | verified | `EvProgGetHandgun` gives 4 (stg_apart_e3fw.c); `ItemWeaponReload` (item.c) |
| 5 | 0x703 | `x_handbul` | Handgun bullets | verified | `ItemWeaponReload` pairs a weapon with kind + 1 |
| 6 | 0x743 | `x_wp_shotgun` | Shotgun (6) | verified (weapon logic) / model (name) | `ItemWeaponReload`; `weapon_file` equip 6 → 0x802 shotgun |
| 7 | 0x724 | `x_shotbul` | Shotgun shells | verified | reload pairing |
| 8 | 0x73B | `x_wp_riflgun` | Hunting rifle (4) | verified (weapon logic) / model (name) | `weapon_file` equip 8 → 0x803 |
| 9 | 0x723 | `x_riflebul` | Rifle bullets | verified | reload pairing |
| 10 | 0x744 | `x_wp_sp` | Spray weapon (reload sets 8; strength from `playing.spray_pow`). Probably the Hyper Spray | suspected | `ItemWeaponReload` case 10; `enGetSprayPower` (en_common.c); `weapon_file` equip 10 → 0x804 `wp_sp` |
| 11 | 0x704 | none listed | Wooden plank (`kakuzai`, 角材 "square timber") | verified | `EvProgFirstMonsterStart` gives 0xB and switches James to `wp_kakuzai` (stg_town_east.c) |
| 12 | 0x745 | `x_wp_pipe` | Steel pipe | model | `weapon_file` equip 12 → 0x806 `wp_pipe`; `EvProgGetSteelPipe` exists (town west) |
| 13 | 0x50C | `nat` | `nata` (鉈, a heavy blade). Probably the Great Knife | suspected | `weapon_file` equip 13 → 0x808 `wp_nata`; `EvProgGetGreatKnife` exists (labyrinth) |
| 14 | 0x746 | `x_wp_csaw` | Chainsaw | verified | `EvProgGetChainsaw` gives 0xE (stg_forest.c) |
| 15 | 0x705 | `x_jlight` | Flashlight | verified | `EvProgGetLight` gives 0xF (stg_apart_e2f.c) |
| 16 | 0x706 | none listed | Radio | verified | Given with the plank in `EvProgFirstMonsterStart` (stg_town_east.c). The same scene animates `i_radio` |
| 17, 18 | 0x70C, 0x70B | none listed | Two items every new game starts with | unknown | `ItemDataInit` (item.c) |
| 19 | none | none | unknown | unknown | |
| 20 | 0x70E | `x_video` | Video tape | model | `EvProgGetVideoTape` exists (hotel) |
| 21 | none | none | Angela's knife | suspected | `EvProgAngelaWithKnife` calls `ItemGet(0x15)` (stg_apart_w1f.c); the demo `knife_agl` has `i_knife_anm` |
| 22 | none | none | unknown | unknown | Given by jump-menu cases 0x84 and 0x8B (`CheckModeJumpDataSet`, src/jump_menu.c) |
| 23 | 0x722 | `x_keygate` | Apartment gate key | verified | `EvProgGetApartGateKey` gives 0x17 (stg_town_east.c) |
| 24 | 0x72F | `x_key202` | Room 202 key | verified | `EvProgGetApart202Key` gives 0x18 (stg_apart_e2f.c) |
| 25 | 0x41F | none (0x41F isn't in `item_list`) | Clock key | suspected | `EvProgAnyoneInHole` gives 0x19 (stg_apart_e2f.c). Demo `ana` ("hole") animates `i_key_clock`. `EvProgUseClockKey` is in the same overlay |
| 26 | 0x72C | `x_keyemerg` | Yard key: retail's Courtyard Key | verified (events); the retail match suspected | `EvProgGetYardKey` gives 0x1A (stg_apart_e3fe.c), and `EvProgUseYardKey` (stg_apart_stair.c) is the event that requires item 26 (see the note below the table). The model name says "emergency" (see prototype.md) |
| 27 | 0x732 | `x_keycourt` | Emergency key: retail's Fire Escape Key | verified (events); the retail match suspected | `EvProgGetEmergencyKey` gives 0x1B (stg_apart_e3fe.c), and `EvProgUseEmergencyKey` (stg_apart_e2f.c, Wood Side 2F) requires item 27. The model name says "court(yard)" |
| 28 | 0x731 | `x_keylyne` | Lyne house key | verified | `EvProgGetLyneKey` gives 0x1C (stg_apart_w1f.c) |
| 29 | 0x730 | `x_keynorth` | Apartment stairwell key | verified | `EvProgGetApartStairKey` gives 0x1D (stg_apart_w2f.c) |
| 30 | none | none | unknown | unknown | |
| 31 | 0x729 | `x_keyroof` | Roof key | model | `EvProgGetRoofKey` exists (hospital 3F) |
| 32 | 0x707 | `x_keypurple` | A key given in the TGS trial by `EvProgDoctorMemo1st` | verified (TGS event) | stg_tgs_trial.c `EvProgDoctorMemo1st` → `EvSubItemGet(0x20, ...)`; combines with 33 into 75 |
| 33 | 0x728 | `x_keyrapis` | Lapis eye key | model | `EvProgGetLapisEyeKey` exists (hospital 2F) |
| 34 | 0x70A | `x_keyelevator` | Elevator key | verified | TGS `EvProgFishKey` → `EvSubItemGet(0x22, ...)`, demo `tsuri` ("fishing") animates `i_keyelevator` |
| 35 | 0x72A | `x_keybase` | A key ("base") | model | |
| 36 | 0x71B | `x_keyhos` | A hospital key | model | |
| 37 | 0x726 | none listed | unknown | unknown | |
| 38 | 0x73F | `x_keyspiral` | Spiral key | model | `EvProgGetSpiralKey` exists (delusion_3) |
| 39 | 0x742 | `x_keyfalse` | False-charge key | model | `EvProgGetFalseChargeKey` exists (labyrinth north) |
| 40 | 0x73D | `x_key312` | Hotel room 312 key | model | `EvProgGetHotel312Key` exists |
| 41 | none | none | unknown | unknown | |
| 42 | 0x73E | `x_keyemploy` | Employee key | model | |
| 43 | 0x735 | `x_keybar` | Bar key | model | `EvProgGetBarKey` exists (hotel basement) |
| 44 | 0x736 | `x_keyfish` | Fish key | model | `EvProgGetFishKey`, `EvProgUseFishKey` exist (hotel) |
| 45 | 0x747 | `x_key3f` | A 3F key | model | |
| 46 | 0x72D | `x_juice` | Canned juice | verified | `EvProgGetCannedJuice` gives 0x2E (stg_apart_stair.c) |
| 47 | 0x71D | `x_coinsnake` | Snake coin | verified | `EvProgGetCoinOfSnake` gives 0x2F (stg_apart_out.c) |
| 48 | 0x71F | `x_coinelder` | Old man coin | verified | `EvProgGetCoinOfOldman` gives 0x30 (stg_apart_out.c) |
| 49 | 0x71C | `x_coinprisoner` | Prisoner coin | verified | `EvProgGetCoinOfPrisoner` gives 0x31 (stg_apart_w1f.c) |
| 50 | 0x708 | none listed | Given in the TGS trial's box puzzle. Probably the hair | suspected | `EvProgBoxWithKey` → `EvSubItemGet(0x32, ...)` (stg_tgs_trial.c). Combines with 51 into 76. The hospital 3F stage has `EvProgOnlyHair` |
| 51 | 0x709 | `x_needle` | Needle | verified | TGS `EvProgGetNeedle` gives 0x33 (stg_tgs_trial.c) |
| 52 | 0x72B | `x_battery` | Battery (dry cell) | model | `EvProgGetDryCell`, `EvProgUseDryCell` exist |
| 53 | 0x712 | `x_ringcopper` | Copper ring | model | `EvProgGetCopperRing` exists; combines with 54 into 77 |
| 54 | 0x711 | `x_ringlead` | Lead ring | model | |
| 55 | 0x710 | `x_spanner` | Wrench | model | `EvProgGetWrench` exists (town west) |
| 56, 57, 58 | 0x738, 0x737, 0x739 | `x_plate_kick`, `x_plate_pig`, `x_plate_female` | Prison tablets (reading: oppressor, gluttonous pig, seductress) | model | `EvProgGetTabletOppressor`/`Pig`/`Seduct` exist; the three combine into 78-81 |
| 59, 60, 61 | 0x73A, 0x73C, 0x70D | `x_horse`, `x_lighter`, `x_waxdoll` | Horseshoe, lighter, wax doll | model | `EvProgGetHorseshoe`/`GetLighter`/`GetWaxDoll` exist; combine into 82-85 |
| 62 | 0x714 | `x_plier` | Pliers | model | |
| 63 | 0x70F | `x_thinner` | Paint thinner | model | `EvProgGetThinner` exists (hotel) |
| 64 | 0x719 | `x_mermaid` | "Little Mermaid" Music Box | model | `EvProgGetLittleMermaid` exists (lake) |
| 65 | 0x71A | `x_cinderella` | "Cinderella" Music Box | model | `EvProgGetCinderella` exists (hotel 2F) |
| 66 | 0x718 | `x_snow` | "Snow White" Music Box | model | `EvProgGetSnowWhite` exists (hotel 1F) |
| 67 | 0x720 | `x_canopen` | Can opener | model | |
| 68 | 0x734 | `x_lightbulb` | Light bulb | model | `EvProgGetLightbulb` exists |
| 69, 70 | none | none | unknown | unknown | Given by jump-menu case 0x92 |
| 71 | 0x721 | `x_lostmemory` | "Lost Memories" (book) | model | `EvProgGetLostMemory` exists (town west) |
| 72 | 0x713 | `x_redrelig` | Book: "Crimson Ceremony" | suspected | `EvProgGetCrimsonCeremony` exists (hotel 2F otherworld); the model name suggests a red religious book |
| 73 | 0x717 | `x_oil` | White Chrism | verified | `EvProgGetWhiteChrism` gives 0x49 (stg_apart_w1f.c) |
| 74 | 0x71E | `x_cup` | Obsidian Goblet | suspected | `EvProgGetGoblet` exists (Historical Society) |

The music box names (64-66) come from the model and event names. In retail they are three small
music boxes that go into the big music box in the hotel lobby (the 2024 remake calls them
figurines). A music box ("orgel") is also in `EvProgOrgel` (hotel 1F, fog world).

Items 71-74 are the four items retail's Rebirth ending needs ("Lost Memories", "Crimson
Ceremony", the White Chrism, the Obsidian Goblet). Whether this build already checks for them is
unknown: the ending stages aren't on the disc.

Items 26 and 27: the model names look swapped against the event names, and the event names agree
with each other. Each stage's `ev_list` entry for a "use" event names the item it requires
(`cond` bits 4-11, `EventListElement` element 10, tested with `ITEM_FLAG` in `EventCheck`,
src/Event/event.c). Decoded that way, the entries for `EvProgUseCannedJuice`,
`EvProgUseApart202Key` and the clock-key event `EvProgClockNeedleMove` require items 46, 24 and
25, as their names say, and those for `EvProgUseYardKey` and `EvProgUseEmergencyKey` require 26
and 27. The doors fit retail's: the Courtyard Key opens the courtyard door in Wood Side's west
stairway hall (apart_stair), the Fire Escape Key the fire escape door at the west end of the 2F
corridor (apart_e2f). So `item_to_chara` probably pairs 26 and 27 with each other's model, or the
model files are named the other way round: suspected.

### Combinations (`cmb_check` in `ItemCombinationUseCheck`, src/Event/item.c)

| Result | Made from | `ItemUse` clears |
|---|---|---|
| 75 (0x4B) | 32 + 33 | 32, 33 |
| 76 (0x4C) | 50 + 51 | 50, 51 |
| 77 (0x4D) | 53 + 54 | 53, 54 |
| 78-81 (0x4E-0x51) | two or three of 56, 57, 58 | the parts used |
| 82-85 (0x52-0x55) | two or three of 59, 60, 61 | the parts used |

What each result is, is unknown.

## Weapons (`item.equip`, `weapon_file`, src/Event/chara_data_load.c)

| `item.equip` (item ID) | Weapon kind | James's animation | Weapon model | Reading |
|---|---|---|---|---|
| 0 | 0x800 | `jms_wpnone_anm` | `wp_kakuzai` | Bare-handed (loads the plank model) |
| 4 | 0x801 | `jms_wphand_anm` | `wp_handgun` | Handgun |
| 6 | 0x802 | `jms_wpshot_anm` | `wp_shotgun` | Shotgun |
| 8 | 0x803 | `jms_wprifl_anm` | `wp_riflgun` | Rifle |
| 10 | 0x804 | `jms_wpsp_anm` | `wp_sp` | Spray (suspected Hyper Spray) |
| 11 | 0x805 | `jms_wpkaku_anm` | `wp_kakuzai` | Wooden plank |
| 12 | 0x806 | `jms_wppipe_anm` | `wp_pipe` | Steel pipe |
| 14 | 0x807 | `jms_wpcsaw_anm` | `wp_csaw` | Chainsaw |
| 13 | 0x808 | `jms_wpnata_anm` | `wp_nata` | `nata` (suspected Great Knife) |

The weapon kind's low nibble goes to `JamesWeaponSet` (m3_sc.c, `case 8`). The mirrored copy is
kind + 0x20 with the `rwp_*` model (`CharaDataLoadWeapon`, chara_data_load.c).

## Enemy placement (`struct Enemy_List`, include/sh2/types.h)

Fields: `kind` (enemy kind), `id` (per-stage enemy number; also its bit in `game_flag.enemy`),
`pos_x`, `pos_z`, `pos_y`, `rot_y` (radians × 4096: `rot[1] = rot_y / 4096.0f`, src/Event/chara_admin.c),
`status` (the spawn state, `en_first_status`), `condition`.

- An enemy whose bit is set in `game_flag.enemy[id >> 5]` isn't spawned
  (`CharaAdminEnemyEntryCheck`, chara_admin.c). What sets the bit (death, or events)
  isn't documented here.
- Condition bits 0x1, 0x2, 0x4 remove the enemy at `playing.battle_level` 1, 2 and 3 (`CharaAdminEnemyEntryCondition`, chara_admin.c).
- `condition & 0xFFC0` selects a game-flag test (the same function):

| Code | Enemy present when | Code | Enemy present when |
|---|---|---|---|
| 0x040 | flag 43 and not flag 251 | 0x2C0 | flag 154 and not 251 |
| 0x080 | flag 67 | 0x300 | `item.flag[1]` bit 20 clear (item 52) |
| 0x0C0 | flag 483 | 0x340 | flag 251 |
| 0x100 | flag 91 | 0x380 | not flag 482 |
| 0x140 | flag 146 | 0x3C0 | flag 154, not (156 and not 157), not 162 |
| 0x180 | flag 68 and not 70 | 0x400 | flag 154 and not 162 |
| 0x1C0 | not (68 and not 70) | 0x440 | neither 101 nor 99 |
| 0x200 | flag 335 | 0x240 | flag 365 |
| 0x280 | not (329 and not 330) | | |

## Game flags with a known meaning

`game_flag.flag[n >> 5]` bit `n & 31`.

| Flag | Meaning | Confidence | Evidence |
|---|---|---|---|
| 7, 8, 9 | Hints for the Leave, Maria and In Water endings: each enables one of the three ending-hint readables in Wood Side 2F. Set at New Game for endings not yet seen (bits 0x01-0x04 of `clear_end_kind`), partly at random | verified (how set, and the events they enable); the hint reading strongly suspected | `FlagInit`, src/Event/event.c; `ev_list` of stg_apart_e2f.c (endings-and-stages.md, Notes) |
| 10 | At least one of the first three endings seen | verified (how set) | `FlagInit` |
| 11 | All three seen, or ending bit 0x08. Plausibly the condition for the Dog key | verified (how set); the meaning suspected | `FlagInit` |
| 15 | Maria is with James: she is loaded and created in every room | verified | `CharaDataLoadRoom` (chara_data_load.c); `ConnectCharaWorkAdminIn` (chara_admin.c) |
| 43 | The first-monster scene in the town is done | verified | stg_town_east.c `EvRoomInit` loads the scene only while it is clear; `EvCharaDataClear` returns `!flag 43` |
| 91 | Mannequins of the Pyramid Head scene become enemies (condition 0x100) | suspected | `en_list` of stg_apart_e3fe.c |
| 251 | Switches the town's enemy set (see prototype.md) | unknown meaning | stg_town_east.c `en_list` |

## `playing` fields

| Field | Meaning | Evidence |
|---|---|---|
| `battle_level` | Action difficulty (1-3 in enemy conditions) | `CharaAdminEnemyEntryCondition`; title.c |
| `riddle_level` | Riddle difficulty. `FlagInit` raises 2 to 3 when bits 0x20, 0x40, 0x80 of `clear_end_kind` are all set | `FlagInit` |
| `clear_end_kind` | Endings seen: 0x01-0x10 = endings (endings-and-stages.md); 0x20/0x40/0x80 = cleared on riddle level 0/1/2 | `EvProgLastScene` (stg_forest.c) |
| `clear_end_number` | Number of clears | `EvProgLastScene` (stg_forest.c); unlocks the radar combo (`fjDrawExec`, src/Font/fj_man.c) |
| `radar` | Enemy radar mode 0-4 | prototype.md |
| `spray_pow` | Spray weapon strength | `enGetSprayPower` (en_common.c) |
| `language` | 0 Japanese, 1 English (message files come in `_j`/`_e` pairs) | src/SH2_common/data_load.c |
| `stage` | Current `STG_NAME` | stg_overlay.c |

## States of the playable game (`Sh2sys.step[2]` in `PlayableMain`, src/gamemain.c)

`GameMain` runs the top-level state in `Sh2sys.step[1]`; while the game is playable,
`PlayableMain` switches on `Sh2sys.step[2]`. The states are named by the original flow-check
strings (`DB_FLOW_CHECK`): 0 start, 1 connect, 2 connect wait, 3 sound load, 4 playable, 5 map
(`chizu`), 6 item screen, 7 options, 8 memos, 9 memory-card save, 10 result, 11 end ("coming
soon"), 12 game over, 13 movie, 14 movie main. State 15 has no flow-check string: it is the pause
screen, which is also entered when the disc tray reports a problem (`fsGetTrayStat`).
