# Resource-format matrix

This is the closure ledger for every resource/container family shipped with
Heroes III or produced by its save path. “Corpus clean” means every item in the
named locally installed Steam or original-disc corpus passed the independent
Rust reader. It does **not** promote those bytes above the pinned English GOG
Complete 4.0 executable, and it does not turn an external implementation into
retail proof.

The format libraries are `no_std`, allocation-free, dependency-free, and
forbid `unsafe`. Filesystem access, zlib/gzip inflation, PNG output, and corpus
aggregation stay in `homm3-oracle` at the `std` edge.

| Family | Engine-owned byte surface | Independent implementation | Retail/candidate evidence | Current verdict |
| --- | --- | --- | --- | --- |
| LOD | `0x5c` header, `0x20` directory records, stored/zlib member extent | `homm3-lod` | Retail `LODFile::open/pointAt/read` at `0x4fa8a0/0x4faa70/0x4fab20` | Corpus clean: Steam 4 archives/11,981 members plus every original-disc row below |
| SND | count plus `name[40], offset, size` records | `homm3-archive` | Exact reconstructed `ResourceManager::GetSoundFile` at `0x55c130`; Dreamcast `SoundHeaderStruct` | Corpus clean: 2 archives, 1,232 RIFF members |
| VID | count plus `name[40], offset`; next offset/EOF supplies size | `homm3-archive` | Dreamcast `AnimHeaderStruct` size 44; retail passes payloads to RAD | Corpus clean: 2 archives, 180 members |
| gzip envelope | RFC header options, raw-deflate extent, CRC/ISIZE trailer | `homm3-archive::gzip`; inflation/CRC at `std` edge | Retail uses zlib `gzopen` for H3M/H3C/save streams | Clean for Steam's 160 standalone maps/141 H3C members and the discs' 251 standalone maps/187 H3C members; original RoE H3C headers are direct records, not gzip members |
| DEF container | groups, names, frame offsets, compact/cropped/interleaved headers, palette | `homm3-def` | Retail `ResourceManager::GetSprite` at `0x55c7b0`; the named interleaved rule is pressing evidence, not retail proof | Corpus clean under explicit `known-interleaved`: Steam 39,939; RoE 1.0 29,393; RoE 1.1 29,393; AB 5,599; SoD 34,324. Retail default deliberately rejects the 12 interleaved `SGTWMTA/B` frames wherever those files occur |
| DEF encoding 0/1 | raw rows and general RLE | `homm3-def` | Retail `Draw`/`DrawTile` at `0x47c570/0x47dd40`; `Draw` is byte-exact and 512 generated draws per encoding agree with the actual reconstructed C++ | Retail grammar closed; generated and installed-corpus renderer differentials clean |
| DEF encoding 2/3 | tileset/adventure packed RLE | `homm3-def` | Retail `DrawTile`/`DrawAdvObjImpl` at `0x47dd40/0x47d0a0`, corroborated by DC source shape; `DrawAdvObjImpl` is byte-exact and 512 generated draws per encoding agree with reconstructed C++ | Retail grammar closed; multi-cell generated draws and all installed frames agree; `DrawTile` remains an explicit 83.2115% candidate |
| archived PCX | 12-byte `DataSize,width,height`, indexed pixels + RGB palette or packed-24 pixels | `homm3-resource::Bitmap` | Retail `GetBitmap816` at `0x55a800` and exact `LoadBitmap16` at `0x55ac40`; source diff confirms read order | Corpus clean: 7,215/7,215 |
| PAL | 24 opaque bytes + 256 RGBA records; tail ignored | `homm3-resource::Palette` | `LoadPalette`/`GetPalette24` at `0x55b060/0x55b470`; candidate semantics complete with codegen residuals | Corpus clean: 3/3, including 4- and 120-byte tails |
| FNT | `0x1020` spec, ABC rows, glyph offsets and payloads | `homm3-resource::Font` | `LoadFontData` at `0x55b750`, exact `LoadFont`; candidate semantics complete with cleanup residual | Corpus clean: 9/9, 403,839 glyph bytes |
| MSK | width/height plus six draw and six shadow bytes | `homm3-resource::Mask` | Retail `NewfullMap::readObjectType` at `0x503780` | Corpus clean: 1,670/1,670 |
| TXT/spreadsheet | CRLF rows, tabs, quote wrapper/collapse rules | `homm3-resource::Text/Spreadsheet` | Exact `TTextResource` and `TSpreadsheetResource` loaders at `0x5bbba0/0x5bbe70` | Corpus clean: 142 files, 18,573 rows, 288,511 cells |
| XMI | IFF chunk extents, `FORM XDIR`, `CAT XMID`, track/event envelope | `homm3-resource::iff::Xmidi` | Engine hands event semantics to Miles | Corpus clean: 1 file, 1 track; playback codec remains external |
| IFR | opaque force-feedback project bytes | magic/extent census at `std` edge | Retail `0x4b654b` reads `H3Shad.ifr` wholesale and hands it to Immersion code | Engine handoff closed: 1 blob, 36,158 bytes; internal IFR semantics intentionally external |
| WAV | byte-exact SND member extraction | SND payload borrowing + RIFF census | Engine hands bytes to Miles | Engine handoff closed; audio decoding intentionally external |
| SMK/BIK | byte-exact VID member extraction | VID payload borrowing + magic census | Engine hands bytes to Smacker/Bink | Engine handoff closed: 116 SMK2, 63 BIKb, 1 BIKi; video decoding intentionally external |
| MP3 | ordinary file path/stream handoff | filesystem inventory only | Retail `soundManager` uses Miles stream calls | Engine handoff identified; no codec duplication planned |
| H3C | direct version-1 compact header or gzipped version-4/5/6 full header, 3–12 scenario descriptors, typed bonuses, concatenated gzip maps | `homm3-map::campaign` plus `homm3-oracle` member splitter | Retail region-count table; Dreamcast campaign dossier; original-media bytes; VCMI used only as an independent hypothesis cross-check | Corpus clean: Steam 28 campaigns/121 scenarios/113 maps; original discs 42/175/159. Retail campaign constructor is not yet reconstructed |
| H3M/TUT header | versions 14/21/28, player slots, victory/loss, hero masks/setups, 31-byte tail | `homm3-map::MapHeader` | Retail `NewSMapHeader::Read` at `0x4c4390`; candidate stream semantics complete at 91.5614% | Corpus clean: Steam 160 standalone/113 campaign maps plus discs 251/159 |
| H3M/TUT world prefix | artifact/spell/skill masks, rumours, optional custom-hero setup records | `homm3-map::WorldPrefix` | Reconstructed `game::LoadMap` and `read_map_hero_setups` preserve the stream order | Corpus clean over the same 411 standalone and 272 campaign-map observations |
| H3M/TUT terrain/templates | one/two 7-byte-per-cell layers, object-type table, placed-object count | `homm3-map::Terrain/ObjectTable` | Reconstructed `readMapLayer`, `readMapObjects`, and `readObjectType` | Corpus clean through every named map; Steam standalone totals are 1,967,328 cells and 65,131 templates |
| H3M/TUT placed objects/events | 12-byte object prefixes, every class payload, quest/Seer variants, nested town events, global events, 124-byte zero trailer | `homm3-map::MapBody` | Retail `NewfullMap::readObject` dispatch and class readers; retail trait-table initializer supplies all 46 class remaps; candidate stream/callee sequence agrees | Corpus clean to EOF over Steam's 160 standalone/113 campaign maps and the discs' 251/159; Steam standalone totals are 378,873 objects, 5,454,548 object bytes, and 666 global events |
| GM/TGM/CGM saves | gzip-wrapped `SavedGameHeader`, saved map/cells/objects/quests/events, object pools, players/towns/heroes, campaign carry-over state, and replay records | `homm3-save` for all retail-accepted revisions 16–18 and 25–42; `homm3-oracle saves` owns gzip/filesystem work | Retail `SavedGameHeader::Load`, `game::Load`, all nested versioned readers and their candidate mirrors; current writes are cross-checked against the serializers, while Dreamcast dossiers recover helper/source boundaries | All 21 accepted revisions reach exact EOF in generated full streams, with non-empty gates for every historical width family, complete current-stream truncation coverage, and hostile-input coverage. **Real-corpus validation remains open** because the installed Steam `Games/` directory contains no saves |

## Installed-corpus totals

The full local corpus now covers the installed Steam Complete data and all four
original US disc lineages recovered from the supplied archive:

| Corpus | LOD members | DEF frames | H3C campaigns/maps | Standalone maps | SND/VID members |
| --- | ---: | ---: | ---: | ---: | ---: |
| Steam Complete | 11,981 | 39,939 | 28 / 113 | 160 | 1,232 / 180 |
| RoE 1.0 | 5,239 | 29,393 | 7 / 23 | 42 | 922 / 84 |
| RoE 1.1 | 5,239 | 29,393 | 7 / 23 | 49 | 922 / 84 |
| Armageddon's Blade supplement | 3,060 | 5,599 | 14 / 53 | 56 | 174 / 77 |
| Shadow of Death | 8,921 | 34,324 | 14 / 60 | 104 | 1,115 / 143 |

The disc SND/VID columns aggregate the installed and loose play-disc archives,
so they are corpus observations rather than unique-member counts. Every listed
engine-owned payload and map body passes. The two anomalous DEF members occur
already in both RoE pressings and recur in later complete data; they pass only
under the named `known-interleaved` manifest. The strict retail default
continues to reject the same 12 affected frames as its negative control. With
that manifest selected, the Rust-to-candidate render differential passes every
listed frame in addition to 2,048 generated cases.

The remaining save closure cohort is therefore bounded to acquisition of real
save files from current and historical revisions. A family may be called 100%
only when its format bytes, installed corpus, and applicable
retail/reconstructed-C++ differential gates all agree; external codecs are
closed at the byte-handoff boundary the engine itself owns.
