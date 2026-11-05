# Windows editor matching

This worktree reuses recovered game source for the map editor (`h3maped.exe`)
and campaign editor (`h3ccmped.exe`). Editor discovery is isolated from the
inherited game build, address claims and score ledger. `homm3 build`, `status`
and `sema` still describe `HEROES3.EXE`.

## Pinned inputs

The editors were extracted from GOG's `setup_homm3_complete_2.0.0.16.exe`, found
inside the nested `Heroes of Might and Magic III (Complete Edition) + Patch
[GOG].zip` archive. The accompanying `patch_heroes_of_might_and_magic_3_complete_2.0.1.17.exe`
contains a replacement game executable and no editor executable.

| File | Bytes | SHA-256 |
| --- | ---: | --- |
| `h3maped.exe` | 2,134,016 | `4480fba145c9f885942cc668d4bce430fe39c0fa482d1a6e58f96318ab857a37` |
| `h3ccmped.exe` | 1,212,416 | `93e98214c1d21d320281f3ef6b8db5bb2ab5a1056b84bf0cf34815d01f7533c1` |

Both are PE32 images at `0x00400000`, with linker version 6.0. Their PE timestamps
are 2000-09-05 18:10:57 UTC and 18:16:24 UTC respectively. The pins live in
`config/project.toml`; binaries stay under ignored `build/orig/`.

The installed Steam copies have different whole-file hashes:

- Map: `72f20adb7f098317e183d52b95d145178bde866d1fb222652338a936fe1f8441`.
- Campaign: `82d51fd454ad1e2f3ff257d5e9f58159b60743295b9c4248e283587b8f5ff655`.

For each pair, all four PE sections (`.text`, `.rdata`, `.data`, `.rsrc`) have
identical addresses, extents and raw contents. Steam adds a 14,840-byte certificate
table at EOF. Within the GOG file's extent, exactly seven bytes change, all in
the PE checksum and certificate-table directory. Thus the handoff's Steam code
observations transfer to these GOG images; the input verifier still requires the
exact pinned GOG files.

## Run the pilot

```sh
nix develop .#build
export HOMM3_DIR="$PWD"
homm3 init --exe /path/to/HEROES3.EXE \
  --map-editor-exe /path/to/h3maped.exe \
  --campaign-editor-exe /path/to/h3ccmped.exe
homm3 editor h3maped gzinflatebuf
homm3 editor h3maped gzinflatebuf --profile game_o2_mt_gr_windows
homm3 editor h3ccmped gzinflatebuf
```

The editor inputs are optional for game initialization. Environment equivalents
are `HOMM3_MAP_EDITOR_EXE` and `HOMM3_CAMPAIGN_EDITOR_EXE`. Each editor command
verifies its selected input before compilation; it does not require the other
editor. `homm3 init` still initializes the shared game toolchain and checkpoint.

`homm3 editor` accepts any existing source TU and named compiler profile. It
always compiles a fresh object through the ordinary VC6 wrapper into
`build/editor/<target>/<profile>/<unit>.obj`, then writes a JSON report beside
it. `--json` also prints the report. It does not extract game annotations into
editor claims or update game scores.

The first default profile is `editor_o1_mt_gr_windows`. Only the optimization
choice has this pilot's direct comparative evidence; the remaining switches
inherit the game's `gzinflatebuf` profile and remain provisional for editors.

| Candidate | Editor address | Result |
| --- | --- | --- |
| `TGzInflateBuf::getByte`, `/O1` | map `0x0044960f` | All 84 bytes exact, no relocations, unique occurrence in executable sections |
| Same source, game `/O2` profile | map | 128-byte candidate; no exact occurrence |
| Same source, `/O1` | campaign | No exact occurrence; no counterpart claimed |

The field accesses and refill behavior identify the shared helper. Its source
comment records the editor address; its `VA` remains the game's identity. This
is one verified helper, not closure of the `gzinflatebuf` TU or proof of an
editor-wide compiler profile. The nearby map `underflow` candidate at
`0x00449ab6` remains a non-exact lead from the handoff.

The scanner requires identical complete bytes and an independently reviewed
function extent before reporting `exact-bytes`. It records substring hits with
unreviewed extents separately, and preserves duplicate occurrences. Functions
with any COFF relocation are skipped, not masked into apparent matches. Name
identity, library classification and duplicate hits still require review.

## Direct COFF fingerprints

The broader discovery pass compiled all **152/152 manifest units with `/O1`**
under pinned VC6 SP3/Wine, preserving each TU's other compiler switches. It also
scanned the shipped `NAFXCW.LIB` MFC archive and `LIBCMT`, `LIBC`, `LIBCPMT`,
`LIBCP`, and `LIBCIMT` runtime archives. MFC/CRT were read from those libraries,
not rebuilt; the shipped MFC release makefile specifies `/O1 /GyF`.

The direct scanner searches executable sections for complete COFF ownership
spans. Every byte outside relocation slots must agree. Same-span references,
including switch table destinations and table bases, must resolve to the exact
relative offsets; named imports must agree with the PE IAT. Ordinal imports and
other external references remain explicit unresolved evidence. It requires no
Ghidra entries, sizes, or hashes.

**A matched COFF span is not an admitted retail function size.** It can include
inline switch tables and alignment. Do not use a disassembler's body byte count
as the carve length. Discovery results remain separate from the frozen TSVs.

| Example | Map VA | Campaign VA | Evidence |
| --- | --- | --- | --- |
| `type_random_map::markCoastalTiles` | `0x0049a1ef` | — | Unique unrelocated 297-byte span, every byte equal under `/O1` |
| `victorUnpackFourPlanes` | `0x004fcdb0` | `0x00463b10` | Unique unrelocated 99-byte spans, every byte equal under `/O1` |
| `CWnd::RunModalLoop` | `0x00503649` | `0x004694e5` | 351-byte spans; all non-relocation bytes and six import references agree |
| CRT `__input` | `0x004ead45` | `0x00455fd4` | 2,597-byte spans; all non-relocation bytes agree, external references need review |
| Zlib `inflate_blocks` | `0x004d8b50` | `0x00442060` | 3,264-byte spans under `/O2`; all non-relocation bytes and 16 internal references agree |

Zlib's `/O1` corpus produces only one candidate per editor. The existing verified
game `/O2 /ML /Gr /TC /D_WINDOWS` control produces **53 unique candidates in each
editor**, including 17 unrelocated byte-identical spans. This supports a separate
`/O2` zlib profile. For `inflate_blocks`, two table bases and all fourteen jump
table destinations agree. An in-memory negative control changing one destination
is rejected even though every byte outside relocations still matches; a wrong
IAT-slot control is also rejected.

Local reproduction, complete candidate JSON, compile flags, object hashes and
validation are in `build/editor-fid/README.md`. The one-off scripts and binaries
remain ignored. Source, vendor files, compiler profiles, claims, scores, and all
20 tracked TSVs were unchanged. These are identity leads, not an editor score or
proof of one compiler profile for every component.

## Extending identities through link order

The planar-kernel anchors locate the entire currently named Victor sequence in
both editors. All **17 named functions** preserve their game order and spacing:
subtract `0x1077c0` from the game VA for the map editor, or `0x1a0a60` for the
campaign editor. Each body was independently checked against the named game
retail body: all non-relocation bytes agree, as do 26 references among those
functions and 39 named Win32 import references per editor. This establishes
identities even where the recovered C++ does not yet reproduce the library.

For example, `allocimage` is at map `0x004fbdd0` / campaign `0x00462b30`,
`flipimage` at `0x004fc360` / `0x004630c0`, `loadpcx` at `0x004fc640` /
`0x004633a0`, and `pcxinfo` at `0x004fcae0` / `0x00463840`. The provisional
reconstructed Victor filenames do not establish original object boundaries;
the observed order here is the verified retail sequence.

Shared-source anchors and vtables additionally identify 15 map-editor addresses:
the gzip constructor (`0x00449663`), `underflow` (`0x00449ab6`), its deleting
destructor, the source's provisional `countPlacedZoneConnections` helper
(`0x004a1967`), and both terrain-rule method sets. The pattern-rule constructor
anchors vtable `0x00543780`; the exact table-rule transition selector anchors
`0x0054379c`. Slot roles, field accesses, named calls and nearby source order
corroborate the method identities, including tiny functions omitted by the first
fingerprint scan. Folded destructor and stream-buffer methods retain aliases.

An isolated `/O1 /Oy` control reproduces every non-relocation byte for 23 of 25
reviewed vtable slots. This gives additional evidence for frame-pointer omission
in those shared functions; the checked-in profiles remain unchanged. The gzip
`underflow` and pattern-rule transition selector still differ. At `0x00449c1e`,
an additional helper calls `readByte` four times and combines shifts 8, 16 and
24; `underflow` calls it twice after `inflateEnd`. Its original name is unknown,
but this proves a four-byte trailer-reader boundary absent from the current
reconstruction's eight separate byte calls.

The full address tables, reference checks, flags and reproduction steps are in
`build/editor-order/README.md` and adjacent JSON. These findings do not change
TSVs, source, editor carve lengths, or reconstruction scores.

## Common code across the editors

A direct PE-to-PE comparison, independent of source names and Ghidra, pairs
**1,309 function candidates**, including **127 with no name in either existing
corpus**. Their nonoverlapping decoded instruction ranges cover **207,749 bytes
in each executable**. This is approximately 16.8% of the map editor's executable
section and 37.9% of the campaign editor's; those section sizes also include
alignment and embedded data, so these are not function-completion percentages.

The comparison follows control flow from candidate entries supported by calls
or code-pointer arrays. It preserves instruction bytes and branch offsets while
normalizing address operands, with import/string identities and local targets
checked separately. Fourteen paired functions contain nineteen checked switch
tables. Five nested entries and thirty-three ambiguous groups remain separate.
Unknown indirect jumps do not receive guessed boundaries. Of the retained
pairs, 945 also have no differing or unresolved direct callee shapes; remaining
reference qualifications are explicit in the JSON. Data references consistently
map 597 addresses between the executables.

Names are attached after pairing: 820 pairs have MFC corpus support, 302 CRT/C++
runtime support, 43 zlib support and 17 verified Victor identities. The remaining
127 use neutral fingerprint IDs. For example, unnamed map `0x004fa780` pairs
with campaign `0x004614e0` over 1,315 decoded instruction bytes. These labels and
categories remain discovery evidence rather than inventory admission.

For the separate engine-reuse question, a fresh `/O1 /Oy` corpus of 134 project
units supplements the earlier controls. Requiring an explicit non-generated
source owner, an unambiguous fingerprint and a corroborated entry/body yields
**108 map-editor engine candidates**: 101 RMG functions, two artifact helpers,
two object-type helpers and three gzip helpers. They account for 16,293 matching
COFF-span bytes (16,275 decoded instruction bytes). The campaign editor has no
unambiguous engine-source candidate in this pass. This is an incomplete
source-backed count, not evidence of zero campaign reuse or a total engine-reuse
percentage. The prior order/vtable findings overlap and are not added twice.

The subsequent [engine identity review](editor-engine.md) rechecks all 108
anchors and extends the map to **157 source-owned map-editor identities**,
including 146 RMG functions. Of these, 131 have matching non-relocation bytes;
26 are located by named calls or anchored vtable slots with differing bodies.
The earlier manual review contributes five further addresses outside that
source-owned table, including two compiler-generated destructor roles.

The same review finds **two campaign-editor artifact helpers** by transferring
the named game-retail bodies: `makeArtifactSlotMask` at `0x00401ee0` and
`makeArtifactComponentMask` at `0x00402360`. Both preserve their instructions,
branch structure and referenced strings. Their callers pass the same fourteen
slot-mask and twelve combination-recipe argument lists as the game. Thus the
earlier zero is specific to the `/O1` source scan, not the current reviewed map.

`build/editor-common/README.md` documents reproduction and limits;
`common-identities.json` contains paired addresses, neutral IDs, code ranges,
switches and references; `engine-reuse.json` records source ownership. The
decoder reproduces all seventeen independent Victor pairs. Negative controls
reject changes to constants, switch destinations, imports, known callees and
data aliases. Source, profiles, scores and all twenty tracked TSVs are unchanged.

## Recording discoveries

This page is the durable evidence index. The [engine review](editor-engine.md)
preserves its full reviewed address tables, individual evidence and remaining
uncertainties under `docs/matching/`. Owning source annotations and comments
provide semantic names; the documentation is not consumed as a second symbol
ledger. Full generated candidates, relocation details, compiler controls and
one-off experiments remain under ignored `build/editor-*/` and can be removed
without losing the documented conclusions.

The TSV inventories remain frozen. Later admission requires reviewed contiguous
extents in the selected editor's inventory; a documented identity, COFF span or
decoded instruction count alone does not establish that extent or a score.

## Inventories and remaining setup

`config/retail/h3maped/` and `config/retail/h3ccmped/` contain **seed inventories**,
not complete editor censuses. `functions.tsv` owns reviewed contiguous RVA/size
pairs. `runtime-map.tsv` initially identifies three CRT bodies in each editor:

| CRT body | Map VA | Campaign VA | Game VA |
| --- | --- | --- | --- |
| `_strlen` (123 bytes) | `0x004e62c0` | `0x00451350` | `0x0061a700` |
| `_memset` (88 bytes) | `0x004e71c0` | `0x00450b70` | `0x0061b210` |
| `_strcmp` (129 bytes) | `0x004e7e60` | `0x00454730` | `0x0061c9b0` |

Every listed CRT body is an exact full-byte copy of the game's already named
runtime function; Ghidra's independent boundaries agree. These maps document
identities and are not a blanket exemption for address bands. MFC and the rest
of the static runtime remain to be classified.

The handoff's Ghidra discovery counted approximately 7,935 map-editor and 2,847
campaign-editor functions after excluding unwind funclets, with approximately
2,100 static-library bodies in each. Those are discovery estimates, not admitted
coverage totals. Unreviewed Ghidra output belongs under `build/`; in particular,
a discontiguous Ghidra body size is not automatically a contiguous carve span.

Next work is to review function boundaries and MFC/CRT identities, establish
additional shared-source bindings and compiler profiles, and integrate editor
relocation-aware delinking/comparison and a separate score ledger. The current
command deliberately reports discovery results without claiming full editor
build or coverage support.
