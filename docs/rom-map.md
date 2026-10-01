# Disc & executable map

## Disc (`baserom/disc/`)
- `SYSTEM.CNF`: `BOOT2 = cdrom0:\SLUS_202.28;1`, `VER = 0.10`, `VMODE = NTSC`.
- `SLUS_202.28`: main ELF, unstripped, DWARF 1.
- `GX/*.BIN`: 12 overlay binaries (AEE AEW AEX AEY AOT AST AWX AWY FST OBS TOI TWE) plus
  `XXX.BIN` (512 bytes, dated 2001-05-30, maybe a placeholder).
- `DATA/*.MGF`: archives `BG CHR DEMO SOUND MENU PIC MOVIE ETC` (+ `DUMMY.DMY`). An `.mgf` has no
  directory the game reads: it is a concatenation of 2048-byte-aligned member files (sub-archives
  nested at most two deep), whose offsets and sizes are compiled into the executable's file index;
  see [formats.md](formats.md#finding-files-the-file-index-and-mgf-archives).
- `IOP/LIB224/`: Sony IOP modules (runtime library 2.2.4) and `IOPRP*.IMG` replacement images.
- `IOP/SD0712/`: sound driver `SOUNDCD/SOUNDHD/SD_CD/SD_HD/SDSTR.IRX` + `SOUND.DAT`.
  The EE side of the driver is linked in `main` (`M:\select\sound\sd0712\ee\`).

The build needs only `SLUS_202.28` and `GX/*.BIN` from the disc.

## `SLUS_202.28` (ELF32 MIPS, R5900, entry `0x100008`)
| section | vaddr | size | notes |
|---|---|---|---|
| `main` | `0x00100000` | `0x2C7D80` in file, memsz `0x1E01E00` | text + data; bss to `0x1F01E00` |
| `gx_*`, `gy_*`, `gz_*` (51) | `0x01F01E00` | 0 in ELF | overlay slots; sizes only in program headers |
| `o_align` | `0x01F08A00` | 0 | |
| `.relmain`, `.rel<ovl>` | – | – | full relocations for main and every overlay |
| `.symtab` / `.strtab` | – | ~26.6k symbols | |
| `.debug` / `.line` | – | 7.0 MB / 0.9 MB | DWARF 1 |
| `.mwcats` ×52 | – | – | Metrowerks linker data |

### Code
- Text in `main`: `0x1000C0`–`0x2992A8`, 5,126 function symbols (1.57 MB).
  - 4,189 have DWARF: 4,157 of the game's, from 325 source files under
    `E:\work\sh2(CVS全取得)\src\` (the DWARF attributes a few out-of-line copies of inline
    functions to 5 headers); 25 of the sound driver's EE side (`M:\select\sound\sd0712\ee\sd_call.c`);
    and 7 of the Metrowerks runtime (`gcc_wrapper.c`, `mwUtils_PS2.c`).
  - 937 have no DWARF: Sony's EE libraries (`sce*`), the C library (`printf`, `malloc`,
    `__ieee754_*`; its internal names, such as `_calloc_r`, `__sfvwrite` and `_Balloc`, are
    newlib's) and the game's own `libSh*` libraries.
- Overlays: 495 function symbols (0.23 MB) across 51 overlays; 481 of them have DWARF. **Only 12
  overlay binaries are on this disc.** The ELF has relocations, symbols and DWARF for all 51 but no code bytes. The executable's
  file index names all 51 overlays as loose files (`./gx`, `./gy`, `./gz/*.bin`), so the 39
  missing ones are simply absent from this disc ([formats.md](formats.md)). Their function names
  are listed in [missing-overlays.md](missing-overlays.md).

### Source tree (directories by function count, from DWARF, main and overlays; subdirectories such as `GFW/gfw_test` are counted separately)
```
src/Chacter/         746   src/LoadBg/          126   src/Font/             66
src/Enemy/           536   src/view/            112   src/Collision/        66
src/Event/stage/     509   src/MC/              100   src/DS_Pad/           59
src/Effect2/         474   src/Fog/              92   src/Multi_thr/dma/    52
src/GFW/             240   src/sh2shd/           82   src/sound/            47
src/Event/           212   src/SH2_common/       81   ... 20 more
src/Chacter_Draw/    203   src/DBG/              71
src/Effect/          165   src/Item/             66
src/Multi_thr/filesys/ 130  src/movie/          129
```

## Overlays (Metrowerks format)
All overlays link at `0x1F01E00`. The ELF has per-overlay symbols `_gx_<name>_{segment,text,data,bss}_{start,end,size}`
and `_static_init[_end]`. The BIN on disc is the loaded image from `segment_start` to `data_end`:

| offset | contents |
|---|---|
| `+0x00` | header, 0x40 bytes: `"MWo3"`, version 8, load address, text/data/bss sizes (the text size counts from `+0x40`), static-init start/end, and at `+0x20` the overlay's name (`gx_aee`), zero-padded |
| `+0x40` | start of the text section (`_text_start`); its first 0x40 bytes are zero |
| `+0x80` | the first function: the code of one stage source file (e.g. `Event/stage/stg_apart_w1f.c`) |
| `_data_start` | data (incl. float constants and function statics) up to `_data_end` = end of file |
| `_bss_start` | bss, not stored |

(Checked on all 12 overlays on the disc: `_text_start` is `+0x40` and the first function symbol is
at `+0x80` in each. The build keeps `+0x00`-`+0x80` as one binary blob.)

On disc: `gx_toi obs fst twe aex aey aew aee awx awy ast aot`. Not on disc: `gx_tww bow thv hvn`, all `gy_*`
and `gz_*` (the ELF still has their symbols, relocations and DWARF).
