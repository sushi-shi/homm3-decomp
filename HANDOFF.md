# Handoff: Windows editor matching (2026-09-24)

Worktree: `/home/sheep/Projects/homm3/homm3-editor`.
Branch: `codex/editor-matching` (the resumed worktree was detached; the older
`claude/editor-matching` branch from the previous handoff was absent).
Always export `HOMM3_DIR=$PWD` and `PYTHONPATH=$PWD/scripts` for this worktree.
Use `nix develop .#build` for VC6/Wine.

## Completed

Editor setup is committed as `f579f404` on `codex/editor-matching`. No push was
requested or performed; no upstream is set. This handoff remains a local file.

- Dreamcast tooling, inputs, source filters and gates retired in commit
  `72557eee` (`Retire Dreamcast tooling from the editor worktree`). Mac tooling
  was never in this branch. Documentation and matching skills now describe the
  Windows evidence workflow. Historical source evidence remains in comments.
- Stale source-comment paths and inert annotations were converted to historical
  descriptions. All 3,655 tracked function source hashes stayed identical.
  Removed `build/orig/dreamcast/`.
- Cleanup full build passes all gates: MAX 4279/4780 exact, 97.14% weighted over
  built units, no MAX reset. The README's executable-universe metric is 97.10%.
- Checked Downloads as requested. Extracted GOG editors from the nested archive
  `heroes-of-might-and-magic-iii-complete-edition-patch-gog.zip`, installer
  `setup_homm3_complete_2.0.0.16.exe`. GOG and Steam editor sections are identical;
  Steam differs only in the PE checksum/certificate directory and a 14,840-byte
  appended certificate table. GOG files are pinned in `config/project.toml` and
  staged in `build/orig/`.
- Added optional `homm3 init --map-editor-exe` / `--campaign-editor-exe` inputs.
- Added `homm3 editor <h3maped|h3ccmped> <TU> [--profile PROFILE] [--json]`.
  It freshly compiles into `build/editor/<target>/<profile>/`, scans complete
  unrelocated function bytes, and compares against reviewed seed spans. It does
  not modify game claims, targets or scores. Relocated functions are skipped;
  substring hits without reviewed extents are not exact functions.
- Fresh `homm3 editor h3maped gzinflatebuf` proves all 84 bytes of
  `TGzInflateBuf::getByte` at `0x0044960f` exact with the new `/O1` profile.
  The `/O2` control emits 128 bytes and does not match. The campaign editor has
  no exact copy of this helper; no campaign counterpart was claimed.
- Seed inventories under `config/retail/h3maped/` and `h3ccmped/`: the map helper
  and `_strlen`, `_memset`, `_strcmp` in both editors. Each CRT body equals its
  named game counterpart in every byte. No broad runtime band is exempted.
- Durable setup/provenance/evidence: `docs/matching/editors.md`.

## Validation

- Tooling suite: `python3 -m unittest discover -s scripts/homm3 -t scripts -p 'test_*.py'`.
  The `-t scripts` is important: otherwise relative imports and mock patches use
  different module identities, causing artificial extra failures.
- Latest suite: 746 tests, same three pre-existing failing cases as the original
  baseline (two stale README column-order assertions and the worktree link test's
  stale `run_wine` mock signature), one skipped test. New tests pass.
- `python3 -m homm3.core.lint` covers the new editor module and its test.
- Final editor integration full build also passes every gate with unchanged MAX
  (4279/4780; no ledger changes); the reviewed Ruff/Pyright scope passes.
- Logs: `build/cleanup-build-final.log`, `build/editor-build-final.log`,
  `build/editor-tests.log`, `build/editor-lint.log`.

## Next substantive work

The current editor command is a first discovery path, not a complete matching
pipeline. The editor inventories are explicitly partial.

1. Review full editor function boundaries and static MFC/CRT identities. Old
   Ghidra discoveries were copied to `build/editor/discovery/{h3maped,h3ccmped}.funcs.tsv`.
   Do not admit Ghidra's discontiguous body byte counts as contiguous carve sizes.
   Handoff estimates: about 7,935 real map functions / 2,847 campaign functions,
   each with about 2,100 static library routines. These are not admitted totals.
2. Establish more shared-source bindings and compiler settings. `/O1` is proven
   for the map helper; the profile's other switches inherit the game TU and are
   provisional. Map `TGzInflateBuf::underflow` at `0x00449ab6` remains a non-exact
   lead; no recovered C++ was changed for the pilot.
3. Integrate editor relocation-aware delinking/comparison, per-target annotation
   ownership and independent score ledgers. `build`, `status`, and `sema` still
   select the game. Never use game VAs or scores as editor progress.

Scratch inputs and reports remain in ignored `build/`; no executables are
committed. GOG extraction: `build/editor-input-review/gog/app/`. The old `/tmp`
experiment directory is no longer required for the reproduced getByte pilot.

## Follow-up: README and early direct fingerprints

- User explicitly requested **leave all TSVs unchanged for now**. All 20 tracked
  TSVs still equal HEAD. Do not empty or refill them without further direction.
- README now presents editor pins/setup first, keeps inherited game score content
  unchanged in a details block, and links the direct fingerprint findings.
  These documentation changes are currently uncommitted.
- Compiled all 152 manifest units under VC6 SP3/Wine with `/O1`, preserving the
  remaining per-TU flags, into `build/editor-fid/objects/`. No source edits.
- Scanned pinned MFC/CRT library COFF members; compared all 14 zlib `/O1` objects
  with the existing verified game `/O2` objects. MFC/CRT were not rebuilt.
- The user pointed out Ghidra's incorrect switch sizes and said we can use our
  own simple matcher. Final scanner `build/editor-fid/fingerprint.py` uses only
  COFF symbols/relocations and PE bytes/imports. It validates same-span references
  including switch table destinations; it does not consume disassembler output.
  COFF spans include tables/alignment and are not admitted function sizes.
- Final local results: `build/editor-fid/README.md`, `early-matches.json`,
  `fingerprint-candidates.json`, `fingerprint-review.json`. The earlier Ghidra
  experiment remains scratch only and is not required by this final pipeline.
- Unique symbol/address candidates: shared source 173 map / 7 campaign; MFC
  1085 / 968; CRT 439 / 382; zlib O2 53 / 53. These are discovery leads,
  not reviewed function totals or scores. Generic functions can remain ambiguous.
- Zlib O1 found only one candidate per editor. Keep the O2 zlib profile hypothesis
  separate from O1 shared-source findings. `inflate_blocks` includes two tables
  with 14 entries; all entries and both table bases agree in both editor images.
- Verified object/source hashes, executable pins, and TSV freeze. Negative
  controls modifying a switch target and an IAT slot in memory were rejected;
  original binaries and the other editor remained untouched.

## Follow-up: order-based identities

- Used existing anchors and preserved link order to verify all 17 named Victor
  functions in each editor. Game-to-map displacement is -0x1077c0; game-to-campaign
  is -0x1a0a60 for every function. Each game retail body matches all non-relocation
  bytes; all 26 named internal references and 39 Win32 import references agree
  per editor. This identifies retail bodies without overstating C++ match scores.
- Identified 15 shared map-editor addresses through order, call sites, vtables,
  and reviewed semantics: gzip constructor / underflow / deleting destructor,
  countPlacedZoneConnections (existing provisional source name), and the two
  terrain-rule method sets with their folded deleting destructor.
- Isolated /O1 /Oy controls for gzinflatebuf and rmg_terrain reproduce non-relocation
  bytes for 23/25 reviewed vtable slots. Canonical profiles remain unchanged.
- Editor 0x449c1e is a real four-byte little-endian reader: four calls to the exact
  readByte, shifts 8/16/24, called twice after inflateEnd from underflow. Original
  spelling unknown. This strengthens the missing-helper hypothesis recorded in
  src/gzinflatebuf.cpp; no source reconstruction was adopted yet.
- Evidence/reproduction: build/editor-order/README.md, victor-identities.json,
  shared-identities.json and the adjacent review/control artifacts. No Ghidra.
- Documentation updated in docs/matching/editors.md. Source/header/profile hashes,
  all game scores and all 20 tracked TSVs remain unchanged. Changes still uncommitted.

## Follow-up: common code and engine-reuse evidence

- User requested direct cross-editor pairing even without function names, then
  asked how much code came from the engine. Both are now measured conservatively.
- `build/editor-common/compare.py` directly reads pinned PE code/imports, discovers
  candidate entries from calls/code-pointer runs, follows CFGs with Capstone and
  checks bounded switch tables. No source names, Ghidra, or function inventories
  are used to form pairs. Names are attached afterward.
- Final `common-identities.json`: 1309 independent body pairs, 127 unnamed,
  207749 nonoverlapping decoded instruction bytes per editor; 5 nested pairs and
  33 ambiguous groups retained separately. Four contradictory code-reference
  pairs were rejected. 14 functions have 19 checked switch tables. 597 data
  addresses map consistently; 945 pairs have no unresolved/different direct
  callee shapes. Other call qualifications are explicit; these are not carve sizes.
- Named-corpus categories: 820 MFC, 302 CRT/C++ runtime, 43 zlib, 17 Victor.
  Unknown pairs have stable neutral common_<fingerprint> IDs.
- Added isolated /O1 /Oy control compilation for all 134 non-vendor/non-Victor
  project units. All compiled. Strict source ownership and credible entry/body
  checks find 108 map-editor engine candidates (101 RMG, 2 artifact, 2 objecttype,
  3 gzinflatebuf), 16293 matching COFF-span bytes / 16275 decoded code bytes.
  Campaign has zero unambiguous engine-source matches in this pass; do NOT treat
  that as no engine reuse, or the map count as total engine reuse. Prior 15 shared
  identity findings partly overlap and are recorded separately.
- Six checks pass: all 17 independently reviewed Victor pairs reproduce, plus
  five in-memory negative controls for a fixed constant, switch target, import,
  known callee, and repeated-data alias. Source/header/profile hashes, 134 control
  object hashes, pins, and all 20 tracked TSVs verified unchanged.
- Read `build/editor-common/README.md` for artifacts and reproduction; durable
  summary in docs/matching/editors.md. No source/profile/score/TSV adoption.

## Follow-up: reviewing and extending the 108 engine anchors

- User asked to map the 108 engine candidates using the Victor-style evidence
  approach and asked where the results are recorded. Durable full address tables
  now live in `docs/matching/editor-engine.md`, linked by README and editors.md.
  This is documentation of reviewed evidence, not a second build symbol ledger.
  Source annotations/comments still own names; all TSVs remain frozen.
- Rechecked all 108 complete source-control spans. Their 390 relocations include
  62 references to independent seed identities, 152 with wider corpus support,
  31 FS exception-chain references at zero, and 145 recorded external bindings
  requiring further review. No contradictory repeated named bindings.
- Extended to 157 source-owned map identities (146 RMG, 11 other helpers): 131
  match all non-relocation bytes; 26 have exact-caller or anchored-vtable identity
  evidence with differing bodies. Nine additions use matching bodies uniquely
  bounded by two independently verified order anchors. Calls/vtables propagate
  additional identities, including generate, writeMap, road and river functions.
- Five earlier reviewed addresses remain outside the 157 table: gzip ctor,
  countPlacedZoneConnections, gzip deleting dtor, folded terrain-rule deleting
  dtor, table-rule isSpecialFrame. Union is 162 distinct addresses; two are
  generated destructor roles. The unnamed four-byte trailer reader is separate.
- Order is not universal: setTerrain moves; terrain paintRectangle/getPackedCell
  exchange order; gzip helper placement differs. No constant VA delta or original
  TU boundary is claimed for RMG/shared engine code.
- Important correction to earlier source-only campaign result: named GAME RETAIL
  transfers locate makeArtifactSlotMask at campaign 0x401ee0 (267 bytes), and
  makeArtifactComponentMask at 0x402360 (290 bytes). All non-relocation bytes,
  normalized decoded instructions/branches/string contents agree. Initializers
  0x402770 and 0x402490 pass the same 14 slot-mask and 12 combination-recipe
  argument lists as game 0x44cc00 and 0x44c960. The initializers differ in code;
  they are not counted as extra exact bodies or ordinary source functions.
- Scratch pipeline: `build/editor-engine/review.py`, `order-probes.py`,
  `checks.py --with-order`, `report.py`; outputs review.json, order-probes.json,
  checks.json. The report generator writes the durable Markdown table. Read its
  reproduction section before rerunning: the initial review precedes order probes.
- Eight checks passed, including in-memory fixed-byte/callee/vtable mutations,
  campaign helper/argument comparisons, 21 object hashes, pins, original source/
  header/profile baseline and all 20 tracked TSVs. Original binary files unchanged.
  No C++/header/profile/score/inventory edits or fresh game build; docs and scratch
  evidence only. Changes remain uncommitted.

## Source layout decision

- User chose to retain existing game/shared files directly under `src/` and
  `include/`, with `editor/` and `campaign_editor/` subdirectories beneath both.
  This supersedes the proposed migration into mirrored `game/` directories.
- Created the four editor directories with `.gitkeep` placeholders and recorded
  the convention in README and AGENTS.md. Shared definitions remain canonical;
  no existing source/header paths, build inputs or TSVs changed. The directories
  contain no speculative C++ stubs or unadmitted compilation units.
