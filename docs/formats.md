# File formats

What the decompiled loaders show about the game's data files. Everything here comes from the code
in `src/`, the DWARF-derived structs in `include/sh2/types.h` and, for the file index, the tables
in the executable. Nothing is taken from the data files on the disc, and no game data is
reproduced; the only numbers from the executable's tables are counts.

Conventions:
- All values are little-endian.
- "Offset" means a byte offset from the start of the file unless a row says otherwise.
- Field names are the DWARF names from `include/sh2/types.h` (cited as `types.h`; look the struct
  up by name). Structs the DWARF leaves anonymous appear as `anon_<size>_<hash>`.
- Code is cited by file and by function, table or macro name (`` `fsSubCmdRealRead` in
  filecmd.c ``); a bare file name means the function or table named just before it.
- **Inferred** marks a conclusion the code supports but does not state. **[ext]** marks public PS2
  knowledge (GS register names, VIF codes, PSS) that is not in this repo.
- Code comments in `src/` are ours, not the developers', so they are not cited as evidence.

## Status

"Complete" below means complete as far as the loader code goes: everything here is read from the
code, and none of it has been tested against the real files.

| Format | Where | Understood (from the loader code; untested on real files) |
|---|---|---|
| File index + `.mgf` archives | executable tables; `DATA/*.MGF` | Complete for what the game does. Bytes the index never points at are unknown. |
| `.tex` / `.tbn2` textures | `data/bg`, `data/pic` | Headers, image and CLUT upload complete. About a dozen header fields are never read. |
| `.mes` messages | `data/etc/message`, `data/menu` | Container and control codes complete. The glyph table is in the executable. |
| `.map` background geometry | `data/bg/<stage>/` | Container, part order and packet walk complete. Vertex contents are decided by VU1 microcode (not decompiled). |
| `.cam` cameras | `data/bg/<stage>/` | Record layout complete. Some enum values have no known meaning. |
| `.cld` collision | `data/bg/<stage>/` | Header, sections and primitives known. Two of the four plane sets are unnamed. |
| `.kg2` / `.kg1` shadows | `data/bg`, `data/chr` | Stream structure known. Five quadwords per object and the vertex data are not. |
| `.mdl` `.anm` `.cls` characters | `data/chr`, `data/demo` | See [Characters](#characters-mdl-anm-kg1-cls). |
| `.dds` demo scripts | `data/demo` | Header partly known, record and command stream complete. |
| Save data | memory card | Layout, cipher and all checksums complete. |
| `.fcl` fog collision | `data/bg` | Layout known. Most fog parameters have no known meaning. |
| `.sdb` BGM areas | `data/sound/snd_data` | Layout known. |
| `.sfc`, `SOUND.DAT` | `data/sound/s_force`, `IOP/SD0712` | Opaque: read by the sound driver, whose code isn't decompiled. |
| `.pss` movies | `data/movie` | Sony's standard PSS [ext]. |
| `savebg.raw`, `icondata*` | `data/menu/mc` | Headerless image (inferred); PS2 icon files, never parsed (inferred). |
| `.bin` stage overlays | `GX/` | Header fields the loader uses; the rest is in [rom-map.md](rom-map.md). |

## Finding files: the file index and `.mgf` archives

The game never reads a directory from an archive. Every data file it can open has a record
compiled into the executable. The record says which archive the file is in, at which offset and
with which size. The archives (`DATA/*.MGF`, "merge files" in the boot option text) are therefore
plain concatenations: all that matters about a member is the offset and size stored in the
executable.

### Records

A file is named in code by a `union fsFileIndex *` (types.h, 8 bytes): a pointer to the file's
location record and its path.

| off | size | type | field |
|---|---|---|---|
| 0x0 | 4 | `union fsFile *` | `index.fp` (`anon_8_7a68f61e`, types.h) |
| 0x4 | 4 | `char *` | `index.name`, a relative path such as `data/bg/<stage>/<name>.cam` |

The location record is a `union fsFile` (types.h, 16 bytes). Byte 0 is a type/flag byte
(`check.type`, a bitfield of `anon_10_5ad76737`, types.h), and the other bytes depend on it:

| view | struct (types.h) | 0x0 | 0x4 | 0x8 | 0xC |
|---|---|---|---|---|---|
| `cd` | `fsCdFile` | type:8, number:24 | name | lsn | size |
| `hd` | `fsHdFile` | type:8 | name | offset | size |
| `mgf` | `fsMgfFile` | type:8 | parent (`fsFile *`) | offset | size |
| `mgc` | `fsMgcFile` | type:8 | parent | start (`char *`) | end (`char *`) |
| `mgp` | `fsMgpFile` | type:8 | file | start | end |

Type bits, from the tests in `src/Multi_thr/filesys/filecmd.c`:

| bit | meaning | evidence |
|---|---|---|
| 0x01 | look on the CD first | `___fsSubCmdCd1stFixFile`, filecmd.c |
| 0x02 | look on the host (HD) first | `___fsSubCmdHd1stFixFile`, filecmd.c |
| 0x04 / 0x08 | may be on the CD / host | tested as `0x5` / `0xA`, `___fsSubCmdCdFixFile`, `___fsSubCmdHdFixFile` in filecmd.c |
| 0x10 | member of the file `parent` | `___fsSubCmdCheckCdHdFile`, `___fsSubCmdCheckRootFile` in filecmd.c |
| 0x20 | path is relative to the "database" directory (`-B`), not the data directory (`-D`) | `___fsSubCmdCdCommonFixFile`, `fsSubCmdRealRead` in filecmd.c |
| 0x40 | location resolved | `___fsSubCmdCheckCdHdFixFile`, `___fsSubCmdCdCommonFixFile` in filecmd.c |
| 0x80 | found on the host | `___fsSubCmdCheckCdHdExistFile`, `___fsSubCmdHdCommonFixFile`, `fsSubCmdRealRead` in filecmd.c |

`check.number` (24 bits) is the disc number the CD location was resolved on (`___fsSubCmdCdCommonFixFile` in filecmd.c).

The records are data in the executable, with no DWARF. `configure.py` splits them out of the ELF
into the "codeless" rodata segment at `0x39C600`, as the symbols `data_<path>` / `root_<path>`
(the `fsFileIndex`), `z_<path>__info` (the `fsFile`) and `z_<path>__name` (the string). A census of
that table (counts only):

| records | type byte | what |
|---|---|---|
| 8 | `0x23` | the top-level archives `data/{bg,chr,demo,etc,menu,movie,pic,sound}.mgf` |
| 53 | `0x03` | loose files: 51 stage overlays (`./gx/*.bin`, `./gy/*.bin`, `./gz/*.bin`), `./system.cnf`, `./null.dev` |
| 3,502 | `0x50` | archive members, among them 146 sub-archives (`data/bg/ap.mgf` inside `data/bg.mgf`, etc.) and a 2048-byte slice of `system.cnf` |

Nesting is at most two levels (archive → sub-archive → file). There are 3,414 `fsFileIndex`
handles. Sub-archives have a location record but no handle and no name. By extension:
943 `.map`, 354 `.cld`, 352 `.cam`, 327 `.tex`, 311 `.kg2`, 307 `.anm`, 253 `.mdl`, 117 `.mes`,
116 `.dds`, 89 `.cls`, 54 `.kg1`, 53 `.sdb`, 51 `.bin`, 33 `.sfc`, 19 `.tbn2`, 15 `.pss`, 9 `.fcl`,
one `.raw`, and two extensionless icon files.

The index names all 51 stage overlays: 16 under `./gx/`, 16 under `./gy/`, 19 under `./gz/`. Only
12 are on this disc ([rom-map.md](rom-map.md)). They are loose files, not archive members, so the
other 39 are simply missing from the disc. Nothing in the index places them inside an `.mgf`.

### Resolving a file

1. `FcRead` / `FcReadPart` / `FcGetFileSize` (src/Multi_thr/filesys/fcread.c) take the
   `fsFileIndex` and queue a command on the file server thread (`fcRead` in fileserv.c).
2. `fsCmdFixFile` (filecmd.c) walks up from a member to its top-level file
   (`___fsSubCmdCheckRootFile`, filecmd.c). It then resolves that file once through the
   current list of fix functions (`___fsSubCmdFixFile` in filecmd.c): CD only, CD then host, or host then CD
   (the `___fsSubCmdFixFuncList*` tables and `fsCmdDiskSelectC` in filecmd.c; chosen by the `-F` boot option, `BootOptItemList` in bootoptitem.c, `init_sh2_filesys` in init_mt_sys.c):
   - **CD:** the name becomes `cdrom0:\PATH\NAME.EXT;1`: upper-cased, `/` turned into `\`, prefixed
     with the `-D` or `-B` directory (`shPathMakeCd`, filepath.c; `UtilStrConvertCdPath`,
     utilstr.c). It is then looked up in the ISO 9660 directory with `sceCdSearchFile`
     (`___shCdSearchFile`, sh_cdvd.c). The result fills `cd.lsn` and `cd.size`
     (`___fsSubCmdCdCommonFixFile` in filecmd.c).
   - **Host:** `host0:` + the `-H` directory + the `-D`/`-B` directory + the name; the size comes
     from the host file (`___fsSubCmdHdCommonFixFile` in filecmd.c).
   - Names that start with `./`, `../`, `/`, `\` or a drive letter get no directory prefix
     (`pathname_skipcheck`, filepath.c). The `-x` option picks their device separately
     (`___fsSubCmdFixFile` in filecmd.c; `BootOptItemList` in bootoptitem.c).
3. `___fsSubCmdSetRealFile0` (filecmd.c) turns a member into a plain range of its
   top-level file, recursively:
   - on the CD the parent's `lsn` advances by `offset / 2048`; on the host the parent's `offset`
     advances by `offset`;
   - the offset is rounded **down** to 2048 (`___fsSubCmdSetRealFile0` in filecmd.c);
   - `size = min(member size, parent size − offset)` (`___fsSubCmdSetRealFile0` in filecmd.c).
4. The read (`fsSubCmdRealRead`, filecmd.c) transfers `(size + 2047) / 2048` whole
   sectors from the CD (filecmd.c). From the host it reads `size` bytes in 256 KB chunks
   (`fsSubCmdHdRead0` in filecmd.c).

Consequences:
- **Members must start on a 2048-byte boundary.** All 3,502 in this build do.
- **Buffers need room for whole sectors.** A CD read writes up to 2047 bytes past the member's size.
- **Buffers must be 64-byte aligned.** `fcRead` halts on anything else (`checkReadAlign`,
  fileserv.c).
- **Partial reads use 2048-byte multiples.** `fsCmdReadPart` asserts that offset and size are
  multiples of 2048 (filecmd.c). The background streamer reads in 64 KB units
  (`_loadBgMem_LoadMemR2L` in loadbg_mem.c).
- **Top-level archives are located by name at run time**, so they may move or grow on the disc.
  **Member offsets and sizes are fixed in the executable.** Changing a member's size or position
  means changing its `fsFile` record (the ELF relocations cover the pointers, not these numbers).
- **Loose-file mode.** With the `-m` boot option ("no-use Merge files", `BootOptItemList` in bootoptitem.c),
  `file_trans_merge_to_direct` rewrites a member's record into a plain file (type `0x03`) named by
  its `fsFileIndex` path (fcread.c). The game then reads `data/bg/ap/ap01.cam` etc. as
  separate files from the CD or host. **Inferred:** that is how the developers ran from a PC
  (`host0:`). It is also the simplest way to test modified files without rebuilding the tables.

### What is known about the `.mgf` bytes themselves

- No code reads an `.mgf` header or directory. `prepare_data_mgf` (init_mt_sys.c) calls
  `FcRead` with a NULL buffer on seven of the eight archives (not `demo.mgf`). A NULL buffer only
  resolves the location and seeks: `shCdSeekW` on the CD, `lseek` on the host (`fsSubCmdCdRead` and `fsSubCmdHdRead0` in filecmd.c).
  Nothing is read.
- In most archives the first indexed member is at offset 0, so those archives have no header.
  In 20 archives (`bg.mgf`, `pic.mgf`, `etc.mgf`, `menu.mgf`, some sub-archives) the first indexed
  member starts later, and some archives have unindexed gaps between members. What those ranges
  hold is not known from the code. The game never reads them.

## Textures: `.tex` and `.tbn2`

One texture container is used by background area/room textures, textures embedded in `.map`
blocks, pictures (`data/pic`) and effect textures. It is not TIM2: no TIM2 code exists in the repo.
Uploads are built by `sh2gfw_init_SyncTexTag` (src/GFW/sh2gfw_Texpacket.c) and given a
GS slot by `sh2gfw_SetSlot2Tex` (sh2gfw_Texpacket.c).

Files come in two shapes, depending on the loader:
- **With an area header**: background `…GB.tex` / `…TR.tex` (`sh2gfw_Set_TrTex`, `sh2gfw_Process_AREAtoMAN` in sh2gfw_read_process.c) and
  `data/pic` pictures (`PictureLoadImage`, src/Event/picture.c).
- **Bare**: the file starts with `sh2gfw_TEX_HEAD`, and `allsize` leads to the CLUT header. Used for
  `data/pic/effect/*.tbn2` and `*.tex` (`TextureBinary_DesignateEntryLevel_Load` in src/Effect2/hh_effect_object_texture.c) and by the
  lens flare (`shLensFlareInit` in src/Lens/lens_flare.c).

`sh2gfw_AREA_HEAD` (types.h, 0x10):

| off | size | type | field | use |
|---|---|---|---|---|
| 0x0 | 4 | u32 | `area_id` | not read |
| 0x4 | 4 | u32 | `toGlobalTexHead` | offset of the `sh2gfw_TEX_HEAD` |
| 0x8 | 4 | u32 | `toGlobalClutsHead` | offset of the `sh2gfw_CLUTS_HEAD` |
| 0xC | 4 | u32 | `date` | not read |

`sh2gfw_TEX_HEAD` (types.h, 0x30); the image follows it:

| off | size | type | field | use |
|---|---|---|---|---|
| 0x00 | 4 | u32 | `texture_no` | not read by the upload code |
| 0x04 | 2+2 | u16 | `x`, `y` | not read |
| 0x08 | 2 | u16 | `w` | width. Upload width `w >> (bitshift ? 1 : 0)`; TEX0.TBW = `w >> 6` (`sh2gfw_init_SyncTexTag`, `sh2gfw_SetSlot2Tex` in sh2gfw_Texpacket.c) |
| 0x0A | 2 | u16 | `h` | height. Upload height `h >> bitshift` (`sh2gfw_init_SyncTexTag` in sh2gfw_Texpacket.c) |
| 0x0C | 1 | u8 | `color` | not read |
| 0x0D | 1 | u8 | `padbyte` | pixels start at TEX_HEAD + 0x30 + `padbyte` (`sh2gfw_init_SyncTexTag` in sh2gfw_Texpacket.c) |
| 0x0E | 2 | u16 | `importance` | not read |
| 0x10 | 4 | u32 | `datasize` | pixel bytes; GIF NLOOP and DMA QWC = `datasize >> 4` (`sh2gfw_init_SyncTexTag` in sh2gfw_Texpacket.c) |
| 0x14 | 4 | u32 | `allsize` | offset from TEX_HEAD to CLUTS_HEAD, used by the bare-file loaders (`TextureContext_DesignateEntryLevel_Entry` in hh_effect_object_texture.c) |
| 0x18 | 1 | u8 | `sendpsm` | pixel format of the transfer (BITBLTBUF, `sh2gfw_init_SyncTexTag` in sh2gfw_Texpacket.c) |
| 0x19 | 1 | u8 | `drawpsm` | TEX0.PSM. CLUTs are uploaded only if `(drawpsm & 0x13) == 0x13` or `drawpsm & 0x14` (`sh2gfw_init_SyncTexTag` in sh2gfw_Texpacket.c) [ext: PSMT8 / PSMT4] |
| 0x1A | 1 | u8 | `bitshift` | halves the upload width and shifts the height. **Inferred:** the image is sent in a wider format than it is drawn in. |
| 0x1B | 1 | u8 | `tagpoint` | not read |
| 0x1C | 1 | u8 | `bitw` | TEX0.TW (log2 width) |
| 0x1D | 1 | u8 | `bith` | TEX0.TH |
| 0x1E | 2 | u16 | `check` | not read |
| 0x20 | 16 | qword | `giftag` | not read (packets are built fresh) |

`sh2gfw_CLUTS_HEAD` (types.h, 0x30); the CLUT data follows at +0x30:

| off | size | type | field | use |
|---|---|---|---|---|
| 0x00 | 4 | u32 | `clutssize` | CLUT bytes, uploaded as one `clw × clh` PSMCT32 rectangle (`sh2gfw_init_SyncTexTag`, `sh2gfw_SetSlot2Tex` in sh2gfw_Texpacket.c) |
| 0x04 | 4 | u32 | `toGSREGS` | not read |
| 0x08 | 4 | u32 | `toRawClut` | GFW ignores it. `PictureLoadImage` (picture.c) uploads a CLUT whenever `drawpsm` is non-zero (not only for indexed formats), read at CLUTS_HEAD + `toRawClut` + 0x30 |
| 0x0C | 1 | u8 | `clutamount` | number of CLUTs, at most 16. CLUT *i* is at CBP + 4·*i* and gets its own TEX0 (`sh2gfw_SetSlot2Tex` in sh2gfw_Texpacket.c) |
| 0x0D | 1 | u8 | `transcluts` | not read |
| 0x0E | 1 | u8 | `clw` | CLUT upload width |
| 0x0F | 1 | u8 | `clh` | CLUT upload height |
| 0x10 | 16 | u8[16] | `fmt` | TEX0.TFX for each CLUT (`sh2gfw_SetSlot2Tex` and `sh2gfw_Get_TFX` in sh2gfw_Texpacket.c) |
| 0x20 | 16 | u8[16] | `transparency` | not read |

Notes:
- **CLUT selection.** Each CLUT gets two TEX0 values. The second forces TFX = 3 for the
  environment-map pass (`sh2gfw_SetSlot2Tex` in sh2gfw_Texpacket.c). Geometry chooses a CLUT by index (`id` in the `.map`
  GIF-group header).
- **Pools.** At most 96 textures can be registered at once (`ALL_TEXNUM`, sh2gfw_Texpacket.c; the
  assert in `sh2gfw_set_TexToTrasMan`). There are 5 GS texture slots.
- **Background buffers.** The area (`GB`) and room (`TR`) textures are loaded into
  `areabuf[2][17664]` quadwords, 0x45000 bytes each (sh2gfw_parse_and_packet.c).
- **Size floor.** Both must be larger than 0x40000 bytes or they are rejected
  (`loadBgTEX_AreaInit`, `loadBgTEX_RoomInit` in loadbg_map.c; `sh2gfw_LoadSet_SemiTransTEX` in sh2gfw_read_process.c). The area texture is registered with a made-up size
  of 0x40001 (`sh2gfw_process_AreaDATA` in sh2gfw_read_process.c).
- **"TR" means semi-transparent** in the code's naming (`sh2_TR_MAN`, `sh2gfw_LoadSet_SemiTransTEX`).
- Effect textures are limited to 0x44800 bytes each (`TextureBinary_DesignateEntryLevel_Load` in hh_effect_object_texture.c).

## Messages: `.mes`

Loaded by `DataLoadMessage` (src/SH2_common/data_load.c). The file is picked per language
(`playing.language` indexes six-entry tables; only Japanese and English entries are filled). The
common file goes into `msg_station` (u16[0x800]), every other file into `msg_buffer` (u16[0x8000]),
so a message file can be at most 64 KB.

The file is an array of u16 (`fontGetMesAdr`, src/Font/font.c):

| index | meaning |
|---|---|
| `[0]` | number of messages N |
| `[1 + n]` | start of message *n*, in u16 units from the start of the file |
| … | message streams |

Codes in a message (`fontPrintStrMain`, font.c; `fontGetCode`, font.c):

| code | meaning |
|---|---|
| < 0x7FFF | glyph index. 0 is a space; codes ≥ 0xE0 are full-width. Glyph widths and shapes come from font data compiled into the executable (`FontDataTable`, font.c). |
| 0xFFFF | end of page. The next u16 is the page code: bits 12-15 the wait type (bit 15 marks the last page), bits 0-11 a wait in frames. Wait types 0/1 wait for a button (`fontNextMessage`, `fontEachTurn` in font.c). |
| 0xFFFE | full-width space |
| 0xFFFD | new line |
| 0xFFFC / 0xFFFB / 0xFFFA | alignment modes |
| 0xFFF9 | new line and start of a choice item |
| 0xFFF8 | preselect the choice |
| 0xFFF7 | blank-box start/end |
| 0xFFF6 / 0xFFF5 | yes/no choice (0xFFF5 selects the second by default) |
| 0xFFE0-0xFFE9 | insert the string `font.mes_v[n]` (one level deep) |
| 0xFF00-0xFF63 | colour *n* |
| 0xFExx | underline of width xx |
| 0xFDxx / 0xFCxx | move x right / left by xx |
| 0xFA00 \| x | absolute x (9 bits) |
| 0xF800 \| y | absolute y |
| 0x8000 | switch to a byte stream until the end of the page: one byte per code, 0xE0 escapes a following 16-bit little-endian code, a byte above 0xE0 starts a 2-byte big-endian code; u16 alignment is restored at the page end (`fontPrintStrMain` in font.c) |

The glyph-index-to-character table is not in any file; it is in the executable's font data.
`dic.c` converts ASCII/Shift-JIS to these codes at run time, but the files already hold codes.

## Background blocks

### The per-block file set

A stage (`enum STAGE_ID`, types.h; 16 background sets, `ca`…`ma`) has a table of blocks
(`FilesBgBlockList_<stage>`, executable data). `FilesGetBgBlock(stage, block)` returns a
`struct FilesBgBlock` (src/FilesList/fileslist_bg.c):

`FilesBgBlock` (types.h, 0x20), every field a `union fsFileIndex *` or NULL:

| off | field | block 0 (the stage) | blocks 1… |
|---|---|---|---|
| 0x00 | `map` | the area texture `<stage>GB.tex` (`sh2gfw_LOAD_AREADATA_ID` in sh2gfw_read_process.c) | geometry `.map` |
| 0x04 | `cld` | – | collision `.cld` |
| 0x08 | `cam` | stage-wide camera `<stage>GB.cam`; only the four outdoor sets (`ca`-`cd`) have one (`step_init_STAGE` in sh2gfw_all_sysinit.c) | camera `.cam` |
| 0x0C | `kg2` | – | background shadows `.kg2` |
| 0x10 | `tex` | – | room texture `<name>TR.tex` (`loadBg1x1_GetTrTexFile` in loadbg_1x1.c) |
| 0x14 | `ex0` | – | background character, filled at run time; bit 31 is a flag (`BgCharaRelocateItemSet` in src/Chacter/bg_chara.c) |
| 0x18 | `ex1` | – | as `ex0` |
| 0x1C | `ex2` | – | not used by the code found |

`FilesBgStage.room_list` / `room_max` (types.h) are NULL for every stage
(`FilesBgStage_ap` in fileslist_bg.c).

**Map IDs.** A block is identified by `mid = (glb_crd << 16) | block` (`loadBgAll_Set2x2Block`, `loadBgAll_ExecLoadRequestIndoor2x2` in loadbg_all.c), where
`glb_crd` is the background set (`enum STAGE_ID`). Which blocks are around a position is decided by
code and executable tables, not by data files:
- the world is a grid of 20000-unit cells (`BLOCK_SIZE`, loadbg_common.c);
- outdoor stages (glb_crd 1-4, `BgIsOut`, src/Event/stg_name.c) look up a cell → block table;
- indoor stages map position → room → four blocks (`RoomName`, `BlockNumber`,
  stg_name.c).

Adding blocks or rooms therefore needs changes to the executable as well as new files.

### Streaming and memory limits

The 2x2 loader (`src/LoadBg/loadbg_2x2.c`, `loadbg_mem.c`, driven by `loadBgAll_PrepareAround`,
loadbg_all.c) loads each block's files in the order **cam, cld, kg2, map**
(`loadBg2x2_SetRequestOutdoor`, `loadBg2x2_SetRequestIndoor` in loadbg_2x2.c). It hands them to the per-type managers
(`loadBg2x2_ActivateRequestOutdoor`, `loadBg2x2_ActivateRequestIndoor` in loadbg_2x2.c).

| limit | value | evidence |
|---|---|---|
| read unit | 64 KB (0x10000) | `loadBg2x2_CheckCacheWork`, `loadBg2x2_CheckLoadWork` in loadbg_2x2.c |
| load area | 120 units = 7.5 MB | `loadBg2x2_CheckLoadWork` in loadbg_2x2.c |
| outdoor block (4 files, each rounded up to 64 KB) | 0x1E0000 (30 units) | `loadBg2x2_ResetSectionAll` in loadbg_2x2.c |
| indoor room (5 blocks × 4 files) | 0x5A0000 (90 units) | `loadBg2x2_ResetSectionAll` in loadbg_2x2.c |
| blocks with cam/cld/kg2 at once | 4 (slot 4 gets a map only) | `loadBg2x2_ActivateRequestIndoor` in loadbg_2x2.c |
| `.cld` slots / `.kg2` slots / `.cam` slots | 16 / 4 / 17 (0 = stage-wide) | `LBM_CLD_SLOTS` in loadbg_cld.c, `LBM_KG2_SLOTS` in loadbg_kg2.c, `LBM_CAM_SLOTS` in loadbg_cam.c |
| stage-wide `.cam` | 2048 bytes | `CAMbuf` in loadbg_camdata.c |

- **Distant outdoor blocks** get a "reduce rate". Only the first `(8 − rate)/8` of the section's
  units are requested (`loadBgMem_SetRequest` in loadbg_mem.c, rate set at `loadBgAll_SetLoadRequestOutdoor2x2` in loadbg_all.c). A file is
  registered only if all its units fit (`loadBgMem_SetRequest` in loadbg_mem.c). Because `map` comes last, far blocks
  stream their small files first. **Inferred:** a prefetch.
- **Indoors**, a fifth block `mid4[3] + 1` is loaded (map only) in background set 5 (`BG_ID_ob`)
  (`loadBg2x2_SetRequestIndoor` in loadbg_2x2.c).
- **Events and characters.** Event files use the same streamer with 8 KB units
  (`EVENT_UNIT_SIZE` in loadbg_event.c). Background characters use two 512 KB slots (`d` in loadbg_chara.c).

### Geometry: `.map`

Parsed by `sh2gfw_process_blockLOCAL_main` (src/GFW/sh2gfw_read_process.c). Everything is
quadword-aligned. Header offsets are byte offsets, used as `>> 4` quadword indices. Nothing is
patched to pointers. The loader writes into the file in two places only:
- `block_id` is replaced with the map ID (`sh2gfw_process_blockLOCAL_main` in sh2gfw_read_process.c);
- `vukind` may be rewritten (`sh2gfw_get_VUmode` in sh2gfw_packetVU.c).

There is no magic number or version.

`sh2gfw_BLOCK_HEAD` (types.h, 0x30), parsed by `sh2gfw_parse_HEAD`
(sh2gfw_parse_and_packet.c):

| off | size | type | field | use |
|---|---|---|---|---|
| 0x00 | 4 | u32 | `block_id` | overwritten with the map ID at load |
| 0x04 | 4 | u32 | `toGlobaldef` | offset of the global part; 0 = the block does not use the area texture |
| 0x08 | 4 | u32 | `toLocaldef` | offset of the local part when there is no global part |
| 0x0C | 4 | u32 | `toRawblockdataParms` | offset of a 4×4 float local-to-world matrix; row 3 is the block origin (`sh2gfw_parse_HEAD` in sh2gfw_parse_and_packet.c) |
| 0x10 | 12 | u32[3] | `toLocalTex` | offsets of up to 3 local `sh2gfw_TEX_HEAD`s |
| 0x1C | 12 | u32[3] | `toLocalcluts` | offsets of the matching `sh2gfw_CLUTS_HEAD`s |
| 0x28 | 4 | u32 | `texnum` | number of local textures (≤ 3) |
| 0x2C | 1 | u8 | `globaltexnum` | tested by view clipping (`sh2gfw_ClipDraw_BG` in sh2gfw_viewclip.c) |
| 0x2D | 1 | u8 | `transtexnum` | non-zero: the block has semi-transparent geometry |
| 0x2E | 1 | u8 | `divflg` | turns on per-tile clipping (`sh2gfw_parse_HEAD` in sh2gfw_parse_and_packet.c) |
| 0x2F | 1 | u8 | `padc` | – |

**Parts, in walk order** (`sh2gfw_process_blockLOCAL_main` in sh2gfw_read_process.c):

1. **Global part** at `toGlobaldef` (`sh2gfw_parse_global`, sh2gfw_parse_and_packet.c). It starts
   with `sh2gfw_BLOCKGLOBAL_HEAD` (types.h, 1 qword; only `gtexnum` is read). If `gtexnum`
   is non-zero, an `sh2gfw_GSREGS_HEAD` follows (types.h, 3 qwords; only `toNextDATA` is
   read), then a GIF-group chain drawn with the area texture.
2. **Semi-transparent part** (`sh2gfw_parse_SemiTransTex`, sh2gfw_parse_and_packet.c): a
   GSREGS_HEAD and a GIF-group chain. It is walked only while a `TR` texture is loaded
   (`sh2gfw_process_blockLOCAL_main` in sh2gfw_read_process.c).
3. **Local part** (`sh2gfw_parse_local`, sh2gfw_parse_and_packet.c). It directly follows the
   parts before it, or starts at `toLocaldef` when there are none. It begins with
   `sh2gfw_BLOCKLOCAL_HEAD` (types.h, not read), then one GSREGS_HEAD + GIF-group chain per
   local texture.

**GIF group:** `sh2gfw_GIFTAG_HEAD` (types.h, 1 qword):

| off | type | field | use |
|---|---|---|---|
| 0x0 | u16 | `gsregs_amount` | must be 0 (assert, `PacketGIF_Pre` in sh2gfw_parse_and_packet.c) |
| 0x2 | u8 | `trans_flg` | group is semi-transparent |
| 0x3 | u8 | `eop_flg` | last group of this texture |
| 0x4 | u32 | `toNextGIFHEAD` | not read (groups are contiguous) |
| 0x8 | u16 | `id` | CLUT index into the texture's TEX0 table |
| 0xA / 0xB | u8 | `tcc`, `tfx` | not read |
| 0xC | u8 | `abe` | alpha-blend mode: 0, 1, 3, 5 or 6, anything else asserts (`PacketGIF_Post` in sh2gfw_parse_and_packet.c) |

**VU part:** `sh2gfw_VU_HEAD` (types.h, 2 qwords), one or more after each group:

| off | type | field | use |
|---|---|---|---|
| 0x00 | u16[10] | `vuparm` | the first two words are read as floats (brightness, exponent) by the specular microprograms (`GetPhongParm` in sh2gfw_Vertexpacket.c) |
| 0x14 | u8 | `vukind` | shading kind (2 "M-Blinn", 3 "EnvMap"); forced to 4 when TFX is 2 and to 2 when `abe` is 5 |
| 0x15-0x17 | u8 | `vuamount`, `eop_flg`, `padc` | not read |
| 0x18 | u32 | `vupartsize` | not read |
| 0x1C | u32 | `toNextVUPART` | non-zero: another VU part follows |

**Geometry:** `sh2gfw_GEOM_HEAD` (types.h) for opaque geometry, `sh2gfw_TRANSGEOM_HEAD`
(types.h) for semi-transparent geometry. Both are 0x70 bytes with the same layout:

| off | type | field | use |
|---|---|---|---|
| 0x00 | u16 | `tsleng` / `vNum` | vertex count |
| 0x02 | u16 | `tileno` | clip tile or object group (below) |
| 0x04 | u32 | `datasize` | bytes to the next header (the walker adds `datasize >> 4` qwords) |
| 0x08 | u32 | `toNextGEOM` | non-zero: another header follows |
| 0x0C | u16 | `unpack_format` | not read |
| 0x0E | u16 | `prim` | GS PRIM, placed in the GIF tag with PRE = 1 (`sh2gfw_Geom_MakePacket` in sh2gfw_Vertexpacket.c) |
| 0x10-0x6F | 6 qwords | `calcparms0/1`, `work1-4` | not read |

**Vertices** follow at +0x70, 16 bytes each:
- They are sent in batches that overlap by 2 vertices (`sh2gfw_Geom_MakePacket` in sh2gfw_Vertexpacket.c). **Inferred:**
  triangle strips.
- The VIF code is `0x6D…` [ext: UNPACK V4-16], so each vertex is two 4×16-bit elements. The GIF
  tag is PACKED with registers 0x412 [ext: ST, RGBAQ, XYZF2] (`sh2gfw_Geom_MakePacket` in sh2gfw_Vertexpacket.c).
- What the 16-bit values mean is decided by the VU1 microcode, which is not decompiled. **Unknown.**

**`tileno`** (`TransFormTileNo`, sh2gfw_parse_and_packet.c):
- **Bit 15 clear:** x tile = low nibble, z tile = next nibble, with some out-of-range values
  clamped (sh2gfw_parse_and_packet.c). A block is 8 × 8 tiles of 2500 units
  (`sh2gfw_make_tagclipdata` in sh2gfw_viewclip.c).
- **Bit 15 set:** the low byte is an object group. Each frame, bit *group* of the block's
  `ObjCondition` sets that geometry's DMA tag byte to 0x21 or 0x11 (`sh2gfw_setVCTAG_DrawSys` in sh2gfw_viewclip.c).
  **Inferred:** geometry that the game shows or hides at run time; the exact effect of the two
  tag values was not traced.

**Not in `.map`:** lights, fog and per-room draw settings are compiled into the executable
(`struct DrawEnvData`, types.h; tables in src/GFW/sh2_DrawEnvData.c, chosen by map ID in
`Set_DrawEnvData`).

### Cameras: `.cam`

The file is a headerless array of `struct _VC_ROAD_DATA` (types.h, 0x80 bytes). It ends at
the first record with `flags & 1`; that record is a terminator. `vcConvertCamFile`
(src/view/vc_calc.c) edits the records in place when they are registered
(`_loadBgCAM_Regist` in loadbg_cam.c).

| off | size | type | field |
|---|---|---|---|
| 0x00 | 0x20 | `_VC_ROAD_AREA` | `lim_sw`: the area that selects this record |
| 0x20 | 0x20 | `_VC_ROAD_AREA` | `lim_rd`: the area the camera target is kept inside |
| 0x40 | 4 | int | `kind_id` (sign-extended from its low 16 bits at load) |
| 0x44 | 4 | int | `flags` (`enum _VC_ROAD_FLAGS`, types.h) |
| 0x48 | 4 | int | `area_size_type` (`enum _VC_AREA_SIZE_TYPE`, types.h) |
| 0x4C | 4 | int | `rd_type` (`enum _VC_ROAD_TYPE`, types.h), the priority between overlapping records |
| 0x50 | 4 | int | `mv_y_type` |
| 0x54 | 4 | float | `ofs_watch_hy` |
| 0x58 | 4 | float | `trace_btm_hy` |
| 0x5C | 4 | int | `rd_dir_type` |
| 0x60 | 4 | float | `projection` |
| 0x64 | 4 | float | `proj_volume` |
| 0x68 | 4 | float | `proj_sec`: projection change time in seconds |
| 0x6C | 4 | int | `cam_mv_type` (`enum _VC_CAM_MV_TYPE`, types.h; CHASE 0, SETTLE 1, FIX_ANG 2, SELF_VIEW 3, LOCUS_CIRCLE 4, THROUGH_DOOR 5) |
| 0x70 | 0x10 | union `anon_10_ff8a34f9` (types.h) | `tmp`: four floats, read according to `cam_mv_type` |

- **`_VC_ROAD_AREA`** (types.h) is eight floats: `x0 z0 x1 z1 x2 z2 min_hy max_hy`.
  `vcSetNearRoadAryByCharaPos` (src/view/vc_main.c) builds a frame rotated along p1→p0 and
  takes p0 and p2 as box corners in it. **Inferred:** an oriented rectangle with corners p0 and p2.
- **`tmp` variants:**
  - CHASE: `ofs_hy, ratio_r_xz, lr_lim_ang_y, rr_lim_ang_y`;
  - SETTLE: `sta_base_ang_y, end_base_ang_y, lr_lim_ang_y, rr_lim_ang_y`;
  - FIX_ANG: `ang_x, ang_y, ofs_hy, cam2wth_dist`;
  - LOCUS_CIRCLE: `origin_x, origin_z, ang_y, radius`

  (`struct _VC_CHASE_CAM_PARAM`, `struct _VC_SETTLE_CAM_PARAM`, `struct _VC_FIX_ANG_CAM_PARAM`, `struct _VC_LOCUS_CIRCLE_CAM_PARAM` in types.h).
- **Flag bits:**

  | bit | name |
  |---|---|
  | 0x1 | END_DATA |
  | 0x2 | WARP_IN |
  | 0x4 | WARP_OUT |
  | 0x8 | NO_FRONT_FLIP |
  | 0x10 | LIM_UP_FAR_VIEW |
  | 0x20 | USE_NO_ENEMY |
  | 0x40 | USE_NEAR_ENEMY |
  | 0x80 | MARGE_ROAD |
  | 0x100 | NO_EXTRA_AREA |
  | 0x200 | CAM_LIKE_SETTLE |
  | 0x400 | NOT_WARP |
  | 0x800 | INVALID_SV |

  0x20 and 0x40 filter records by whether an enemy is near (`vcSetNearRoadAryByCharaPos` in vc_main.c).
- **Placeholder values** replaced at load (`vcConvertCamFile` in vc_calc.c):
  - `ofs_watch_hy`: 10000/10001/10002 → −300/−375/−600;
  - `projection`: 0 → 448, 10001 → 384;
  - `trace_btm_hy`: 10000/10001/10002 → −300/−150/150;
  - CHASE `ofs_hy`: 10000/10001 → −500/−125;
  - FIX_ANG `ang_x`: 10000 → −π/6.
- **Limits:** at most 128 near records are considered (`vcSetNearRoadAryByCharaPos` in vc_main.c). The stage-wide file must
  fit in 2048 bytes, i.e. 15 records plus the terminator (**inferred** from the 0x80 record size).
- **Unknown:** the meanings of `kind_id`, `mv_y_type`, `rd_dir_type` and `proj_volume` values.

### Collision: `.cld`

`struct _CL_CLDHEADER` (types.h, 0x174) at offset 0. Every `*ofs` is a byte offset from the
file start, resolved on each access (the `CLD()` macro in src/Collision/cl_main.c).

| off | size | type | field |
|---|---|---|---|
| 0x000 | 4 | float | `sx`: block origin X |
| 0x004 | 4 | float | `sz`: block origin Z |
| 0x008 | 16 | int[4] | `b0size`…`b3size` (not read) |
| 0x018 | 4 | int | `csize` (not read) |
| 0x01C | 4 | int | `disable`: non-zero, the block is ignored |
| 0x020 | 0x40 | int[16] | `b0ofs`: per section, index list into the floor planes |
| 0x060 | 0x40 | int[16] | `b1ofs`: … into the wall planes |
| 0x0A0 | 0x40 | int[16] | `b2ofs`: … into the `ced` planes |
| 0x0E0 | 0x40 | int[16] | `b3ofs`: … into the `swd` planes |
| 0x120 | 0x40 | int[16] | `clofs`: … into the columns |
| 0x160 | 4 | u32 | `fldofs`: floor plane array |
| 0x164 | 4 | u32 | `wldofs`: wall plane array |
| 0x168 | 4 | u32 | `cedofs`: plane array |
| 0x16C | 4 | u32 | `swdofs`: plane array |
| 0x170 | 4 | u32 | `cldofs`: column array |

- **Index lists** are int arrays ended by −1 (`clCheckHitSwordVectorWall` in cl_main.c).
- **Sections.** Outdoors a file covers 20000 × 20000 units from (`sx`, `sz`), cut into 4 × 4
  sections of 5000; section = `j*4 + k`, j along Z, k along X (`clGetHitSectListMOVEOutDoor` in cl_main.c). Indoors only
  section 0 is used (`clGetHitSectListVECHITInDoor` in cl_main.c).
- **Primitives:**
  - `_CL_HITPOLY_PLANE` (types.h, 0x50): `kind` u8, `shape` u8, `pad` u16, `weight` u32,
    `material` u32, `flg` int, `p[4][4]` float. `shape == 0` is a triangle (p[0..2]), otherwise a
    quad (`clCheckHitSwordVectorWall` in cl_main.c).
  - `_CL_HITPOLY_COLUMN` (types.h, 0x30): the same 16-byte head, then `p[2][4]`.
    **Inferred** from the VU0 code (cl_calc2.c): p[0].xz is the centre, p[0].y/p[1].y the vertical
    extent, p[1].w the radius.
- **Which sets are used where:**
  - Floor height is a ray from y − 500 to y + 1500 against the floor planes only
    (`clCollectCharaHeightNormal` in cl_main.c). **Inferred:** +Y points down.
  - Walking tests walls, `swd` planes and columns (`clCheckBg2Chara` in cl_main.c).
  - Sword and sight rays test floors, walls, `ced` planes and columns, skipping `material == 12`
    (`clCheckHitSwordVector` in cl_main.c).
- **Unknown:** what `ced` and `swd` stand for, `kind`/`weight`/`flg` values, and other `material`
  values.

### Background shadows: `.kg2` (and the `.kg1` stream)

A stream of 16-byte quadwords with no offsets. Addresses inside it become DMA REF sources, so the
buffer must be 16-byte aligned (`sh2shd_make_reftag_pool_outdoor` in src/sh2shd/sh2shd_outdoor.c and the other `ref[…].ui32[1]` addresses).

| position | struct (types.h) | fields |
|---|---|---|
| qword 0 | `SHADOW_OUTDOOR_HEAD` (4844) | `kind` u16, `map_id` s16, `obj_num` s16, `reserve1-5` |
| per object +0 | `SHADOW_OUTDOOR_OBJ_HEAD` (4870) | `map_id` u32, `obj_id` s16, `geom_num` s16, `origin_x/y/z/w` s16 |
| object +1 … +5 | – | 5 qwords, see below |
| object +6 … | `geom_num` × geometry | each starts with `SHADOW_GEOM_HEAD` (4765): `vertex_num`, `prim`, `send_data_num`, `ee_memory_size`, `boundary_x/y/z/r` (all s16), and is `ee_memory_size` qwords long |

- **Header.** `kind` and `map_id` in the file are ignored: registration overwrites them with the
  `glb_crd` and block of the map ID (`_loadBgKG2_Regist` in loadbg_kg2.c).
- **Object quadwords +1…+5.** They are not settled. Initialisation copies +2…+5 as a 4×4 float
  matrix (`local_world`, `sh2shd_init_outdoor_man2` in sh2shd_outdoor.c). The packet builder sends +1…+4 as a V4-16
  unpack (`sh2shd_make_reftag_pool_outdoor` in sh2shd_outdoor.c).
- **Geometry blocks.** Each is sent from +2 by DMA REF depending on `prim`: in the spot-light builder 1-6
  always and 7-10 only when `spot_cam_angle < 0` (`sh2shd_make_reftag_pool_outdoor`,
  sh2shd_outdoor.c); the parallel-light builder is at `sh2shd_make_reftag_pool_outdoor_for_parallel` in sh2shd_outdoor.c.
- **Single-block objects.** When an object's first block has `prim` 3, 4 or 10, initialisation
  skips only that block, not the object's other blocks (`sh2shd_init_outdoor_man2` in sh2shd_outdoor.c). Either such
  objects always have one block or this is a bug; the code does not show which.
- **Limits.** 4 managers (`sh2shd_add_map` in sh2shd_shadow_model.c); `map_id == 7` is refused (the same function).
- **Unknown:** the `prim` values, `vertex_num`, `boundary_*`, `origin_*`, and the vertex data (VU1).

`.kg1` (character shadows) is the same stream with `SHADOW_CHAR_HEAD` (types.h; `char_id`,
`kind`, `obj_num`) and `SHADOW_CHAR_OBJ_HEAD` (types.h). It is drawn through
`sh2shd_add_char` (`sh2shd_Draw_ShadowChar` in sh2shd_shadow_model.c); James's variant is in
`sh2shd_make_reftag_pool_char` in src/sh2shd/sh2shd_char_jms.c.

### Fog collision: `.fcl`

`fogSetCollision` (src/Fog/fogdata.c) reads the stage's file into `fog_colis_data` (u16[6144],
12 KB, fogdata.c), and `fogSetCollisionMain` / `fogSetCollisionMain2`
(fogdata.c) parse it. Blocks follow each other:

1. `FOG_COLIS_HEAD` (types.h, 0xC): u16 `wall1, wall2, obj1, obj2, area, env`.
2. `wall1` × `FOG_COLIS_WALL` (types.h, 0xC): s16 `x0 y0 z0 x1 y1 z1`, in units of 50
   (`fogSetCollisionMain` in fogdata.c): a vertical quad from (x0, z0) to (x1, z1).
3. `wall2` × `FOG_COLIS_WALL2` (types.h, 0x18): four s16 xyz corners, units of 50.
4. `obj1` × 4 floats (x, y, z, size), registered as fog objects 100 + *i*.
5. `obj2` × 4 floats, registered as fog objects 500 + *i*.
6. `area` × `FOG_AREA_DATA` (types.h, 0xA): s16 `x0 z0 x1 z1` (units of 50), u16 env index.
7. Aligned to 4: `FOG_ENV_DATA` (types.h, 0x18) entries: s16 `PartNum MaxPos PartSize
   EscapeRange FloorY LimitY LimitHeight WaterY`, u8 `WindDef Flag Double Alpha`, float
   `GridRate`. Entry 0 is the default.

The header's `env` count is not read. Most `FOG_ENV_DATA` fields have no known meaning.

## Characters: `.mdl`, `.anm`, `.kg1`, `.cls`

**Which files a character uses.** This comes from tables in the executable, not from the files:
- `chr_mge_files` (types.h; `model_fid`, `anime_fid`, `cluster_fid`, `shadow_fid`, `mid`) and
  the `CharaData_*` lists (`struct CharaData_DemoList` in types.h);
- the tables themselves are in `weapon_file` in src/Event/chara_data_load.c and after.

**Loading.** `sh2gfw_LOAD_CharaModelData` (src/GFW/sh2gfw_Init_ModelDrawData.c) reads the
four files whole, each into its own buffer, and the game uses them where they land. Buffers are
rounded up to 0x2000 bytes (the `CDL_BLOCK_SIZE` macro, chara_data_load.c). James has fixed buffers:

| buffer | size | evidence |
|---|---|---|
| model | 0x138000 | `CharaDataLoadExecJames` in chara_data_load.c |
| animation | 0xE2000; a per-stage `.anm` is loaded at +0x84000 | `CharaDataLoadExecJames` in chara_data_load.c |
| shadow | 0x4000 | `CharaDataLoadExecJames` in chara_data_load.c |

### `.mdl`

No offset in the file is turned into a pointer. Offsets are added to a base on every use.

`sh2gfw_Model_Header` (types.h, 0x40) at offset 0:

| off | size | type | field | use |
|---|---|---|---|---|
| 0x00 | 1 | u8 | `NoTextureID` | non-zero: the model borrows another loaded model's textures; the donors are hard-coded (`Init_WithoutCharaTex` in sh2gfw_Init_ModelDrawData.c) |
| 0x01 | 3 | u8[3] | `padc` | – |
| 0x04 | 4 | u32 | `chara_id` | character ID; with bit 0x20 (after `Check_RevChara`) a mirrored model that uses the textures of `chara_id & ~0x20` (`sh2gfw_init_CharaModel_TextureData`, `init_ReverseCharaTex`) |
| 0x08 | 4 | u32 | `texnum` | number of texture blocks |
| 0x0C | 4 | u32 | `toTexHead_offset` | only used as a base when sharing textures (`Init_WithoutCharaTex`) |
| 0x10 | 4 | u32 | `toClutsHead_offset` | the same |
| 0x14 | 4 | u32 | `toModel_offset` | offset of `struct Model` |
| 0x18 | 4 | s32 | `toKg1_offset` | never read (shadows come from the separate `.kg1`) |
| 0x1C | 4 | s32 | `padi` | – |
| 0x20 | 32 | ptr[8] | `pTexMAN` | filled in at load time |

At 0x40 follow `texnum` u32 offsets to `sh2gfw_TEX_HEAD`s, then `texnum` u32 offsets to
`sh2gfw_CLUTS_HEAD`s, all from the file start (`init_CharaTex`). The texture format is the one in
[Textures](#textures-tex-and-tbn2). For models, `TEX_HEAD.check` must be 0x9999 in the
texture-sharing path (an assert in `Init_WithoutCharaTex`).

**Limits.** Arrays hold 4 texture blocks per model (`sh_Model.pTexMAN[4]`; `TB_change_VU1[4]`).
The code does not check the count. **Inferred:** more than 4 overwrite memory.

Textures are uploaded again every draw (`sh2_Model3UpdateTextures`,
src/Chacter_Draw/model3_sub_n.c).

`struct Model` (types.h, 0x80) at `toModel_offset`. All `*_offset` fields are from the start
of `struct Model`.

| off | field | use |
|---|---|---|
| 0x00 | `id` | must be 0xFFFF0003 (`MODEL_ID`) or 0xFFFE0003 (`MWORK_ID`) (assert, `init_CharaTex` in sh2gfw_Init_ModelDrawData.c) |
| 0x04 | `revision` | below 3 turns on the specular-map TEX0 (`sh2_Model3UpdateTextures` in model3_sub_n.c) |
| 0x08 | `initial_matrices_offset` | `n_skeletons` × float[4][4]; 0 = none (`Model3InitWork` in model3_n.c) |
| 0x0C | `n_skeletons` | – |
| 0x10 | `skeleton_structure_offset` | one parent byte per bone. On first use, values < 0xFE are halved in place and `flag` bit 0 is set (`Model3SkeletonStructure`, model3_n.c). **Inferred:** files store 2 × parent with `flag` bit 0 clear. 0xFF = no parent (`shCharacterGetSkeletons` in src/Chacter/skelton.c). |
| 0x14 / 0x18 | `n_skeleton_pairs`, `skeleton_pairs_offset` | `SkeletonPair {u8 parent_no, child_no}` (types.h); only `child_no` is used |
| 0x1C | `default_pcms_offset` | one float[4][4] per pair; envelope *i* = bone[child] × pcm[*i*] (`Model3UpdateEnvelopeMatrices` in model3_sub_n.c) |
| 0x20 / 0x24 | `n_vu1_parts`, `vu1_parts_offset` | chain of `struct Part`, stepped by `size` |
| 0x28 / 0x2C | `n_vu0_parts`, `vu0_parts_offset` | the same |
| 0x30 / 0x34 | `n_texture_blocks`, `texture_blocks_offset` | u32 indices into the model's texture blocks |
| 0x38 / 0x3C | `n_text_poses`, `text_poses_offset` | `TextPos {u32 block_index, u32 texture_no}` (types.h); `texture_no` selects the CLUT (`sh2_Model3UpdateTextures` in model3_sub_n.c) |
| 0x40 | `text_pos_params_offset` | not read |
| 0x44 / 0x48 | `n_cluster_nodes`, `cluster_nodes_offset` | `DefaultClusterNode {s16 x, y, z}` (types.h), the morph base |
| 0x4C / 0x50 | `n_clusters`, `clusters_offset` | `Cluster {u32 n_nodes, u32 element_offset}` (types.h) → `ClusterElement {s16 x, y, z, index}` (types.h) deltas (`MakeTransferDefaultClusterNodesPacket` in model3_cluster_n.c) |
| 0x54 / 0x58 | `n_func_data`, `func_data_offset` | not read |
| 0x5C, 0x60 | `hit_offset`, `box_offset` | not read |
| 0x64 | `flag` | bit 0 (see above); 0x40000000 specular map |
| 0x68, 0x6C | `relative_matrices_offset`, `relative_transes_offset` | not read |

**Runtime limits** (from array sizes; `ModelCommonWork`, types.h):
- 128 bones and 256 envelope matrices per model;
- 2048 cluster nodes and 64 text poses per model;
- 400 bones across all characters (`struct shSkeltonWork` in types.h).

`struct Part` (types.h, 0x90). Offsets are from the Part; the drawing code is
src/Chacter_Draw/model3_vu1_n.c and model3_vu0_n.c:

| off | field | use |
|---|---|---|
| 0x00 | `size` | bytes to the next part |
| 0x04 | `type` | not read |
| 0x08 / 0x0C | `packet_offset`, `packet_qwc` | a prebuilt VIF packet sent by DMA as it is (`MakeVu1PartTransferPacket` in model3_vu1_n.c); the C never looks inside |
| 0x10 | `xtop` | VU address for VIF ITOP (`DrawPart1` in model3_vu1_n.c) |
| 0x14 / 0x18 | `n_cluster_data`, `cluster_data_offset` | `ClusterData {u16 src, dst, n}` (types.h): morphed nodes to copy into VU memory |
| 0x1C / 0x20 / 0x2C | `n_skeletons`, `skeletons_offset`, `data_skeletons_offset` | u16 bone indices, and the VU address of their matrices (`MakeVu1PartTransferPacket` in model3_vu1_n.c) |
| 0x24 / 0x28 / 0x30 | `n_skeleton_pairs`, `skeleton_pairs_offset`, `data_skeleton_pairs_offset` | the same for envelope matrices (`MakeVu1PartTransferPacket` in model3_vu1_n.c) |
| 0x34 / 0x38 / 0x3C | `n_textures`, `text_pos_indices_offset`, `texture_params_offset` | a u16 `TextPos` index and a `TextureParam {u64 clamp, u64 tex1}` (types.h) per texture (`MakeNormalPacket` in model3_vu1_n.c) |
| 0x40 | `shading_type` | 1 flat, 2-4 lambert (4 adds specular); anything else asserts (`DrawPart1` in model3_vu1_n.c) |
| 0x41 | `specular_pos` | picks the normal or "over" draw path |
| 0x42 | `equipment_id` | drawn only if this bit of the 128-bit `equipment_flag` is set (`DrawParts1` in model3_vu1_n.c) |
| 0x44 | `backclip` | microprogram variant |
| 0x45 | `envmap_param` | environment-map pass on/off |
| 0x48-0x50 | `phong_param_a`, `phong_param_b`, `blinn_param` | floats |
| 0x60 / 0x70 / 0x80 | `diffuse`, `ambient`, `specular` | float[4] |

**Unknown:** the inside of the Part VIF packets (vertex and strip formats, decided by the VU
microcode), and the `func_data`, `hit`, `box` and `relative_*` blocks.

### `.anm`

There is no header or magic number. The decoder is `shExec` (src/Chacter/anime.c).

**Frame.**
- A u32 flag word (read as two u16, so no alignment is needed) holds a 4-bit code for each of 8
  bones; after 8 bones another word follows (`shExec` in anime.c).
- In each code, bit 3 is `is_key` and bits 0-2 are the type.
- Each bone's data follows according to its type:
  - 0: none;
  - 1: rotation;
  - 2: translation, then rotation;
  - 3-6: translation variants, then rotation, then axis and angle (`shExec` in anime.c).

**Values.**
- Rotation: 3 × s16, value / 4096 radians. The matrix is `transpose(RotMatrix(−rx, −ry, −rz))`
  (`RotTransposeMatrix`, `shExec` in anime.c).
- Translation: 3 × float32 for the root bone. For other bones it is 3 × 16-bit float (`GetSF`,
  anime.c; 1 sign, 5 exponent (+0x70), 10 mantissa). Its zero test can never be true, so
  0x0000 decodes to 2^−15.
- Axis: 3 × s16 / 32768, one s16 / 4096, one skipped u16 (`shExec` in anime.c). The loaded axis and
  angle are recomputed from the matrices at playback (`shCharacterAnimeCalcComplement` in anime.c).

**Frame placement.**
- Gameplay animations use fixed-size frames: frame *N* is at `base + frame_size × N`
  (`shCharacterPlayingAnimeExecMain` in anime.c). `frame_size` is a per-character-kind table in the executable
  (`shCharacterAnimeOneFrameSize`, src/Chacter/m3_sc.c). Start/end/loop/speed come from
  `_AnimeInfo` tables in code (types.h).
- Cutscene animations are decoded sequentially from a start offset taken from a table in the
  executable (`<chr>_anime_adr_list`, e.g. `shCharacterHumanDJAMESAnimeSet` in m3_djames.c). A frame whose bones are all
  type 0 is a hold frame (`shCharacterDramaAnimeExecMain` in anime.c).

**Playback** interpolates rotation by axis-angle and translation linearly, on a 4.12 fixed-point
clock.

**Possible bug.** The root-bone translation shared by types 3-6 (label `des_trans`) reads three
16-bit floats at u16 offsets 0, 2 and 4 and advances 6 u16s: it takes the first half of three
32-bit slots as half floats (`shExec` in anime.c). Child bones read three consecutive half floats
(`shExec` in anime.c).

### `.cls` (cluster animation)

Loader: src/Chacter_Draw/clani.c. `struct Header` (types.h, 0x10):

| off | type | field | use |
|---|---|---|---|
| 0x00 | u32 | `id` | must be 0x29843918 or 0x29853918, else "illegal data" (`ClusterAnimeSet` in clani.c) |
| 0x04 | u16 | `revision` | 1 selects the only evaluator (`Calc1`); other values give weight 0 |
| 0x06 | u16 | `flag` | not read |
| 0x08 | u16 | `n_clusters` | must equal the model's `n_clusters`, else "n_clusters mismatch" (`ClusterAnimeSet` in clani.c) |
| 0x0A | u16 | `reserved_0` | – |
| 0x0C | u16 | `n_frames` | loop/clamp length |
| 0x0E | u16 | `reserved_1` | – |
| 0x10 | u32[] | `offsets` | one per cluster, from the header |

- **Per-cluster data (revision 1):** `Cluster1 {u16 n_keys; Element1 {u16 frame; s16 weight}[]}`
  (types.h). Keys are found by binary search and interpolated linearly. The weight is
  value / 4096 (`Calc1` in clani.c). The clock is the skeletal animation's.
- **Cutscene files** hold several animations; their start offsets come from `<chr>_clani_adr_list`
  tables in the executable (`shCharacterHumanDJAMESAnimeSet` in m3_djames.c).
- **Use.** The weights scale the model's cluster deltas, which are applied by VU0 microcode
  (`Model3UpdateClusters`, model3_cluster_n.c). The scale of the s16 values is applied in
  microcode. **Unknown.**

### `.kg1`

The stream structure is in [Background shadows](#background-shadows-kg2-and-the-kg1-stream).
Parsed by `sh2shd_make_reftag_pool_char` (src/sh2shd/sh2shd_char_jms.c):
- the per-object `obj_id` is a bone index;
- only the first quadword after the object header is sent (sh2shd_char_jms.c);
- only geometry with `prim` 5 or 6 is sent (`sh2shd_make_reftag_pool_char` in sh2shd_char_jms.c).

Up to 16 characters cast shadows at once (`sh2shd_add_char` in sh2shd_shadow_model.c).

## Demo scripts: `.dds`

A stage overlay reads the whole `.dds` into a buffer and sets `DramaDemo_PlayInfo.adr_dds_top`
(types.h). Examples: `FcRead` into `MemShare_gp_data_buf` in
`EvProgVomitEddie` in src/Event/stage/stg_apart_e1f.c, or `CharaDataLoadExtra`. The demo's characters come from a
`CharaData_DemoList` (types.h; model, animation, shadow, cluster) loaded by
`CharaDataLoadDemo` (src/Event/chara_data_load.c).

**Not in the file.** Everything else is static data in the stage overlay's C:
- the animation list `adr_anim`;
- subtitle windows (`adr_msg_time`, `{start, end}` pairs ending at 0xFFFF);
- the first message number `msg_start`;
- the voice stream `stream_no` / `stream_start`;
- a position offset `add_pos_x/z`.

The `.dds` holds the camera, lights and character positions.

Reading is byte by byte (src/Event/demoview.c), so the file has no alignment requirement:
- `DdsReadShort`: u16 (demoview.c);
- `DdsReadFloat4`: float32 (demoview.c);
- `DdsReadFloat2`: a 16-bit float, 1 sign / 5 exponent / 10 mantissa bits, rebuilt as
  `sign << 31 | (exp + 0x70) << 23 | mant << 13`. There is no zero case, so 0x0000 decodes to 2^−15
  (demoview.c).

**Header** (`DramaDemoInit`, demoview.c):

| off | size | type | meaning |
|---|---|---|---|
| 0x00 | 3 | char | `"dds"` (asserted) |
| 0x03 | 13 | – | skipped, unknown |
| 0x10 | 2 | u16 | total frames |
| 0x12 | 2 | – | skipped, unknown |
| 0x14 | 1 | u8 | point light count |
| 0x15 | 1 | u8 | spot light count |
| 0x16 | 1 | u8 | infinite light count |
| 0x17 | 1 | – | skipped, unknown |
| 0x18 | 1 | u8 | character count (≤ 7) |
| 0x19 | 16 × n | char[16] | character names |

- **Lights.** The three light counts together must be ≤ 6 (assert).
- **Character names** are looked up in `anim_info[79]` (`DramaDemo_AnimInfo`, types.h),
  compiled into demoview.c. It maps a name to a character kind/id and to that character's
  animation-number range. A new character name needs a new `anim_info` entry.

**Records.** Each record is:
- a u16 frame number (0xFFFF ends the script);
- node blocks: a u8 node number, then that node's commands. Node 0xFF ends the record.

Nodes:
- 0: key;
- 1: camera;
- 2 … 2 + lights − 1: lights, in the order point, spot, infinite;
- above that: character *node − lights − 2*.

Inside a node each command is a u8 opcode followed by its operands; opcode 0x0B ends the node.

| node | opcode | operands | effect |
|---|---|---|---|
| key | 0x10 | – | sets status bits 0 and 1: bit 0 = cut (no interpolation, `DdsPlay` in demoview.c); bit 1 = positions in this record are absolute float32, otherwise float16 deltas from the last absolute values (`DdsPlayCamera` in demoview.c) |
| key | 0x11 | – | start the next animations from `adr_anim` |
| key | 0x14 | – | sets status bit 4; no reader found |
| key | 0x12, 0x13 | – | ignored |
| camera | 3 | 3 values | position |
| camera | 4 | 3 values | interest |
| camera | 5 | 3 × f16 | rotation, turned into an interest point and roll (`RotationToInterest` in demoview.c) |
| camera | 6 | f16 | roll |
| camera | 7 | f32 | plane; `scr_z = 1.14702 × plane` (`DdsPlay` in demoview.c) |
| light | 3 / 4 / 5 | as camera | position / interest / rotation |
| light | 8 | 3 × f16 | colour |
| light | 9 | 2 × f16 | falloff |
| light | 10 | 2 × f16 | cone |
| light | 1 / 2 | – | visible / hidden |
| character | 1 / 2 | – | visible / hidden (stored, not used) |
| character | 3 | 3 values | position; there is no rotation (rot.y is forced to π, `DdsPlay`, `DramaDemoSkipLast` in demoview.c) |

Opcode handlers, all in demoview.c: `DdsPlayKey` (key), `DdsPlayCamera` (camera),
`DdsPlayLight` (lights), `DdsPlayCharacter` (characters).

**Playback** (`DdsPlay`, demoview.c):
- Values are interpolated linearly between the last and next records; `add_pos_x/z` is added.
- The demo runs at 30 frames per second and ends at the header's frame count.

**Unknown:** the skipped header bytes, key opcodes 0x12-0x14, light roll and `cone[1]`.

## Save data (memory card)

All from src/MC/mc.c, src/MC/savedata.c and types.h.

### Files on the card

Up to 5 directories `BASLUS-20228 FILE-<n>` (`mc_Dname` in mc.c; `mcSetDirName`, mc.c), each
with:

| file | contents | size | written / checked |
|---|---|---|---|
| `BASLUS-20228 FILE-<n>` (same name as the directory) | `struct MC_DIRDATA` | 0x400 | `mcJobSaveData` / `mcJobCheckDir` in mc.c |
| `icon.sys` | `sceMcIconSys` built in code (`mc_IconSys` in mc.c); the title digit is patched per directory (`mcJobSaveIconSys` in mc.c) | 0x3C4 | `mcJobSaveIconSys` in mc.c |
| `icon` | `data/menu/mc/icondata` or `icondata2`, copied unchanged | 0x13EB8 or 0xD198 | `mcJobSaveIcon` in mc.c |
| `DATA-01` … `DATA-15` | `struct MC_SAVEDATA` | 0x2000 | `mcJobSaveData` / `mcJobLoadData`, `mcJobCheckDir` in mc.c |

- **Validity.** A directory is valid only with all of its directory file, `icon.sys` and `icon`
  (`mcJobCheckDir` in mc.c).
- **Slots.** A save slot is `file + 15 × directory` (`mcSaveData`, `mcJobSaveData` in mc.c).
- **Icon variant.** `icondata2` and a second `icon.sys` are used when the directory's clear code is
  0x60125A01 (`mcMakeDirData` in mc.c).
- **Unreachable jobs (inferred).** "Extra data" and "system data" code exists, but nothing in
  `src/` starts those jobs.

### `SAVE_DATA_ALL` (types.h, 0x1FC0)

| off | size | type | field |
|---|---|---|---|
| 0x000 | 0x1A10 | `struct SAVE_DATA` | `d` |
| 0x1A10 | 0x5B0 | char[1456] | `pad`, always zero (`SetSaveData` in savedata.c) |

`SAVE_DATA` (types.h), filled by `SetSaveData` (savedata.c) and restored by `ExtGameData` (savedata.c):

| off | size | type | field | notes |
|---|---|---|---|---|
| 0x000 | 4 | u32 | `version` | written as 4; loading accepts 3 or 4; `key_config` is restored only from 4 (`SetSaveData`, `ExtGameData`, `CheckSaveData` in savedata.c) |
| 0x004 | 2 | u16 | `scene` | save point, as a message number |
| 0x010 | 16 | float[4] | `jms_pos` | James x, y, z and rot.y |
| 0x020 | 0x8C | `Playing_Info` (types.h) | `playing` | stage, options, statistics, scores, clear state |
| 0x0AC | 0x4A0 | `GAME_FLAG_DATA` (types.h) | `game_flag` | `flag[255]`, `enemy[32]` bit arrays and puzzle state |
| 0x54C | 0x34 | `Item` (types.h) | `item` | item flags and counts |
| 0x580 | 0x1430 | `_Character_Info` (types.h) | `chara` | up to 32 `_CI_SubCharacter` (types.h, 0xA0 each); only kinds `>> 8` = 1 or 2 are saved (`shCharacterSetSaveData` in src/Chacter/chara_saveinfo.c) |
| 0x19B0 | 0x60 | `Pad_KeyConfig` (types.h) | `key_config` | button assignments |

Loading range-checks the result (`CheckSaveData`, savedata.c):
- version, stage < 53, non-negative floats, rank ≤ 100;
- option ranges, language ≤ 5;
- item-count caps. This check reads the current global `item`, not the loaded save's
  `SaveDataAll.d.item` (marked `@bug` in `CheckSaveData`, src/MC/savedata.c).

The meaning of individual `game_flag` bits, item indices and `scene` values is not traced here.

### `DATA-NN`: `MC_SAVEDATA` (types.h, 0x2000)

| off | size | field |
|---|---|---|
| 0x0000 | 4 | `csum1` |
| 0x0004 | 4 | `csum2` |
| 0x0008 | 0x1FE8 | `data`: the encrypted `SAVE_DATA_ALL` (0x1FC0) with a 0x28-byte key block inserted |
| 0x1FF0 | 4 | `csum3` |
| 0x1FF4 | 4 | `pad` (random) |
| 0x1FF8 | 8 | `csum4` |

**Helpers:**
- `rot(n, s)` is a 32-bit rotate left by `s & 31` (`mcRot`, mc.c). For s = 0 the C
  shifts by 32, which is undefined in C; what the compiled code does is not checked here.
- `shRandI` is `seed = (seed × 0x41C64E6D + 0x3039) & 0x7FFFFFFF` (src/SH2_common/sh_vu0.c).

**Encoding** (`mcEncodeStart` mc.c, then `mcCodec` mode 1, mc.c):

1. **Keys.** `key1` is 24 random bytes. `key2[0..14]` are 15 random bytes (`shRandI() >> 10`,
   low byte). `key2[15] = (sum of the other 39 bytes & 0xFF) ^ 0x21`.
   - `keysum` = the byte sum of all 40.
   - `csum` starts at `keysum`; `zcount` starts at the number of key bytes whose low nibble is 0.
2. **Keystream.** `key1` is read as three u64 words K1. Each `key2` byte repeated 8 times makes
   sixteen u64 words K2.
3. **Encrypt.** For each of the 0x3F8 u64 words of the plaintext:
   - `c = p ^ K1[i1] ^ K2[i2]`;
   - `i1` cycles 0-2; `i2` advances each time `i1` wraps, modulo 16;
   - for every byte of `c`: `zcount++` if its low nibble is 0, then `csum += byte`.
4. **Checksums.**
   - `csum1 = rot(csum, zcount)`;
   - `csum2 = rot(zcount − keysum − csum, zcount)`;
   - `csum3 = rot(csum × zcount, keysum)`.
5. **Insert the keys.** `k = zcount mod 0x3F8`. The last `k` ciphertext words move up by 5 words,
   and the 40 key bytes (`key1` then `key2`) go in at word `0x3F8 − k` of `data`.
6. **Final checksum.** `csum4` = XOR of the file's first 0x3FF u64 words.

**Decoding** (`mcDecodeStart` mc.c, mode 2 `mcCodec` in mc.c):

1. **Sums.** Recompute the byte sum and zero-nibble count over all 0x3FD words of `data` (keys
   included). XOR those words, the word at 0x1FF0 and the word at 0x0 into `csum4`.
2. **Check `csum1`, then `csum4`.**
3. **Keys.** Take the key block at word `0x3F8 − (zcount mod 0x3F8)`; check the `key2[15]` rule,
   then `csum2` and `csum3`.
4. **Decrypt.** Remove the key block and XOR-decrypt.
5. **Validate.**
   - `mcExtSaveData` (mc.c) compares scene, savecount, levels, clear kind and times with
     the directory's file entry.
   - `CheckSaveData` range-checks the result.
   - On any failure the previous data is restored (`mcCodec` in mc.c).

### Directory file: `MC_DIRDATA` (types.h, 0x400)

| off | size | type | field |
|---|---|---|---|
| 0x000 | 28 | char[28] | `message`: a fixed Shift-JIS string (`mc_message`, mc.c) that must match |
| 0x01C | 4 | u32 | `flag`: clear code, one of 0, 0x4E9C6817, 0x60125A01 (`mcMakeDirData`, `mcExtDirData` in mc.c) |
| 0x020 | 0x12C | `MC_FILEINFO[15]` (types.h, 0x14 each) | `file`: `dirid, fileid, scene, b_level, r_level, status, savecount, time, total_time, csum` |
| 0x14C | 0x8C | `Playing_Info` | `playing` |
| 0x1D8 | 4 | int | `lastsave` (−1: none) |
| 0x1DC | 4 | u32 | `id` (random on every write) |
| 0x1E0 | 16 | u32[4] | `csum` |
| 0x1F0 | 0x208 | u32[130] | `pad` (random) |
| 0x3F8 | 8 | u64 | `csum_all` |

**Writing** (`mcMakeDirData` / `mcMakeDirDataSub`, mc.c):

1. **Entry checksum.** `file[i].csum = 0xB0B8AE8C ^ XOR over j = 0..3 of rot(word_j × 0x573, j)`
   (`mcMakeDirData` in mc.c).
2. **Scramble** the 112 words at 0x020-0x1DF. The random generator is pushed with seed 0x23D.
   For each word *i* of plaintext `w`:
   - `csum += w`;
   - `n = rot(w + 0x5BC679D8, i) ^ rot(0x53F76697, −3i) ^ shRandI()`;
   - `csum2 ^= n` (`csum2` starts at 0xB0B8AE8C).
3. **Checksums.**
   - `csum[0] = rot(csum − csum2, 15)`;
   - `csum[1] = flag + csum × 0x23D + 0xB0B8AE8C`;
   - `csum[2] = (flag × csum2) ^ rot((csum + 1) × 0x573, 7)`;
   - `csum[3] = csum2 + id`;
   - `csum_all` = the 64-bit sum of the first 0x7F u64 words.

**Verifying** (`mcExtDirData`, mc.c) runs the checks in reverse:
- `csum_all`, the clear code;
- unscramble, then `csum[0..3]`;
- the message;
- each used entry's `fileid`, ranges (scene ≤ 25, b_level ≤ 4, r_level ≤ 3) and checksum;
- unused entries must be all zero.

## Other files

### Picture and menu images

- `data/menu/mc/savebg.raw`: drawn by `mcDrawBG` (src/MC/mc_menu.c) as four 512 × 128
  strips from a source that advances 0x600 bytes per row, with PSM 1. **Inferred:** a headerless
  512 × 512 24-bit image [ext: PSMCT24].
- `data/menu/mc/icondata`, `icondata2`: written to the card unchanged and never parsed (`mcJobSaveIcon` in mc.c).
  **Inferred:** standard PS2 icon files.

### Sound

- **`.sdb` (`data/sound/snd_data/<stage>.sdb`)**: the BGM area table.
  - Read by `SeBgmChange` (src/sound/sh_sound.c) into a 20 KB buffer and scanned by
    `sndGetSoundAryByCharaPos` (src/sound/snd_select.c).
  - It is an array of `_SOUND_DATA` (types.h, 0x40): a `_SND_ROAD_AREA` `lim_sw` (types.h,
    the same eight floats as the camera areas), `flags` int, `chanstat[7]` int.
  - Record 0 is the default. The scan ends at the first later record with `flags` bit 31.
  - `flags` bit *n* (n < 6) marks page *n* valid. `chanstat[track]` holds 5 bits per page: on/off,
    inner fade type − 1, outer fade type − 1 (macros at `SND_SET_NEAR_SOUND` in snd_select.c).
  - The hit test uses only x0, z0, x2, z2 and the heights (`sndGetSoundAryByCharaPos` in snd_select.c).
- **`.sfc` (`data/sound/s_force`)** and **`SOUND.DAT`**: `.sfc` is read straight into the sound
  driver's 3D work area (`SeSoundEffect3dLoad`, sh_sound.c). The driver's code isn't decompiled
  (its IOP side is an IRX module on the disc, and its EE side, `sd0712`, stays assembly), so
  neither format is visible to the game's C.

### Movies: `.pss`

- **Reading.** Read with `FcReadPart` in 64 KB units: `readMpeg` (src/movie/pss_main.c) asks for
  `READ_CHUNK_SIZE` bytes at a time from `strFileRead` (src/movie/pss_strfile.c), which calls
  `FcReadPart`. The data is demultiplexed with `sceMpegDemuxPssRing` (`readMpeg` in pss_main.c): one MPEG-2 video stream and one PCM
  stream.
- **Audio header.** The audio stream starts with a 0x28-byte header copied into `sshd` / `ssbd`
  (`struct anon_20_86dae77b`, `struct anon_8_7468da75` in types.h; `AU_HDR_SIZE`, `audioDecBeginPut` in src/movie/pss_audiodec.c).
- [ext] This is Sony's standard PSS layout.

### Stage overlays: `.bin`

- **Loading.** `StgOverlay` (src/Event/stg_overlay.c) reads the stage's overlay to
  `_ovl_start_addr` (0x1F01E00). It zeroes `word[5]` bytes at `+0x80 + word[3] + word[4]` (the
  bss after text and data) and calls `MWNotifyOverlayLoaded`.
- **Header.** The header words are described in [rom-map.md](rom-map.md).
- **Stage data.** The stage's `struct Stage_Data` is in the overlay's data (for example
  `stage_toilet`, src/Event/stage/stg_toilet.c). The executable finds it with a switch on the
  stage number (`StgOverlayGetStageData`, stg_overlay.c).

### `SYSTEM.CNF` check

- **What is read.** The disc check reads the first 2048 bytes of `system.cnf` (the member record
  `z_root_system_cnf__info`) and scans a line starting with `B` for the digits after `:`
  (`check_func` in src/Multi_thr/filesys/sh_chk_syscnf.c).
- **Acceptance.** The disc is accepted only if the number is 20228 (from `SLUS_202.28`)
  (`check_func` in sh_chk_syscnf.c).
- **Separate check.** `check_boot_file_name_in_system_cnf` separately asserts that the executable's
  own name matches `BOOT_FILE_NAME` (src/Multi_thr/sys/init_mt_sys.c).
