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
