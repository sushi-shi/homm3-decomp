# Handoff: data campaign for #112 + #113 (2026-09-30)

This records where the data-matching campaign stands and how to continue it.
Read [AGENTS.md](../../AGENTS.md) first; it is the policy for all matching work.

## After integration

The user subsequently authorized landing: #113 was integrated into #112,
#112 merged into `decomp-complete-4.0` at `22f0d9a4a`, and #78 was closed.
The review and landing instructions below describe the earlier handoff.
Game matching now continues on `codex/game-matching-integration-20260930`
with two workers. The byte-accounting gaps remain open; integration did not
waive or reclassify them.

## Continuation instructions (2026-09-30)

The user requested solo continuation in the primary checkout,
`/home/sheep/Projects/homm3/homm3-decomp`. Work continues on
`codex/data-campaign-solo-20260930`, starting at handoff commit `33c82d97b`.
The pre-existing `cc_wrap.py` Wine timeout edit is preserved separately.

When ready, merge #113 into #112. **Do not merge #112 into the default
branch:** the user will review it first. The user also authorized bringing
the needed link and startup fixes **from #78 into this campaign**. These
instructions supersede the landing order and undecided merge method below.

Latest checkpoint in the primary checkout:

- Full VC6/Mac build passes, including the new mandatory full executable link:
  153 objects, zero unresolved externals and zero duplicate-symbol warnings.
- The duplicate town-name table is now owned by `text.cpp`. The no-CD sentinel,
  radar byte stride and town animation timer fixes from #78 are integrated.
  These three inexact functions reset MAX to 98.99%, 90.53% and 89.67%; HIST
  retains their previous peaks.
- All seven recovered startup initializers occur once in the linked CRT table;
  game-context binding precedes its network consumer.
- The isolated Wine smoke reaches the main menu and loads Arrogance, including
  its adventure map, minimap and opening event dialog.
- Terrain-rule constructor/cleanup identities add 20 exact ledger matches.
  These are startup bodies excluded from the README function count.
- Five narrow locale facet pointers are now verified against their pinned
  `LIBCPMT.LIB` COMDATs. Their address identities close all ten `_Save`/`_Tidy`
  helpers. Five additional pinned `use_facet` cache pointers close eleven
  more functions. **README exact MAX is 4,407 / 4,785**, exceeding 4,395.
  The complete ledger is 4,435 / 4,813 exact; weighted executable MAX is 98.15%.
  These changes preserve the strict relocation comparison policy.
- Eight literal bytes (`C`, `r`, `w`, `a`, including terminators) are verified
  against the pinned runtime archive. The remaining CD literals total 101 B,
  not the older decision list's 109 B. Runtime comparison has zero findings.
- Local-static alignment lookup now uses the unique, per-unit emitted VC6 name
  bridge. Its regression test includes foreign-unit and ambiguous-name controls.
- Tooling tests: 30 data/header tests and 27 build/link tests pass.
- Compiled negative controls restore the pre-fix zero values for the six
  startup initializers: both RLE constants, game-context binding, seer-name
  binding, sound descriptors and archive contexts. All six lose their exact
  initializer comparison; unchanged scratch copies compare exact. A 156-row
  hero-table probe is rejected as `extent beyond candidate definition`
  (14,352 emitted bytes versus 14,996 required). Probe sources and objects
  stay under ignored `build/solo-startup-negative/`.
- Reviewed 13 proven-end padding rows (53 bytes), plus eight four-byte gaps
  between fixed-size terrain records and 27 further typed BSS gaps (119 B);
  two stale rows (9 bytes) are retired. The five `use_facet` cache pointers
  account for another 20 B. Fresh coverage has **982 missing file bytes /
  2,239 missing image bytes**, **703 / 2,507 unverified data bytes**, zero
  data mismatches, zero overlaps and zero extent violations. No stale padding
  rows remain. The zero-byte landing condition is still open.
- The user approved preparing **#112 for review with these gaps explicit**.
  Merge #113 into #112 after validation; keep #112 open for their review.
  This authorizes integration despite the remaining byte gaps, but does not
  authorize merging #112 into the default branch.

### Score changes for review against #112

The full-build checkpoint passes comparison against the preceding solo commit.
Comparison against #112's original `14c3d6415e` reports seven source-edit MAX
drops. HIST retains every previous peak; these remain review items:

| Function | #112 MAX | Current MAX | Source evidence / reason |
|---|---:|---:|---|
| `advManager::updateRadar` | 91.25% | 90.53% | #78 byte-stride correctness fix |
| `army::canCastSpell` | 99.99% | 76.10% | Mac byte-result structure and helper calls; Windows shared-tail recovery remains open |
| `earlySetup` | 99.57% | 98.99% | #78 no-CD sentinel guard |
| `NewfullMap::loadMapObjects` | 99.77% | 98.04% | Restored Dreamcast-evidenced `ResourceManager::dispose` call |
| `NewfullMap::readMapObjects` | 99.37% | 96.27% | Same canonical resource cleanup helper |
| `ResourceManager::getBitmapResourceSize` | 96.77% | 96.45% | Recovered game-context reference and const archive lists |
| `townManager::main` | 90.25% | 89.67% | #78 animation timer correctness fix |


## Goal and landing condition

- **#112** (`codex/gruntz-data-model-20260928`) adopts the Gruntz data model,
  strict relocation/data comparison and complete byte accounting.
- **#113** (`codex/retail-data-recovery-20260928`, stacked on #112) is the data
  recovery that uses it.
- Both land **together**, and only when:
  1. every retail byte is accounted for (see [Byte accounting](#byte-accounting));
  2. **exact MAX in the README exceeds 4,395**. That is main's count under the
     old, non-strict policy. #112's strict relocation identity reset it to 2,664,
     and the gap has been recovered since.
- **Merge method: not decided yet.** Two squash merges were mentioned, then
  withdrawn. Ask before merging.
- After landing, test #78 (the startup fix) against the result. #80 is parked.

## Historical state at the original handoff

The table below records the original, unpushed state at `70d038774`.
The combined campaign and subsequent solo checkpoints are now pushed to
#113's `codex/retail-data-recovery-20260928` branch. #112 remains unchanged
and open; the default branch has not been updated.

Branch refs live in the main repository's `.git`,
so the commits survive a wiped `/tmp`; the worktrees and their `build/`
directories do not.

| Ref | Commit | Role |
|---|---|---|
| `codex/bytes-lib-20260928` | `70d038774` | **Combined tip.** It merges everything below, and is where integration happens. Worktree `/tmp/homm3-bytes-lib`. |
| `codex/retail-data-recovery-20260928` (local #113) | `70d038774` | Fast-forwarded to the combined tip after every integration. Worktree `/tmp/homm3-gruntz-data-model`. |
| `origin/codex/retail-data-recovery-20260928` | `835a7c4b02` | #113 as pushed. The local branch is **166 commits ahead**. |
| `origin/codex/gruntz-data-model-20260928` | `14c3d6415e` | #112, unchanged. |

Lane worktrees are all merged into the combined tip; reuse them for new lanes.

| Worktree | Branch at merge | Last contents |
|---|---|---|
| `/tmp/homm3-match-a` | `codex/match-a-r6-20260929` | Matching lane A, units rmg*, mapcell, hero, seerhut, ai*, philai, spells, army*, town*, tradpost, cmbtmgr, game, events |
| `/tmp/homm3-bytes-text` | `codex/match-b-r5-20260929` | Matching lane B, every other unit |
| `/tmp/homm3-bytes-data` | `codex/identity-tools-20260929` | Identity tooling line: relocation pairing, ICF twins, names |

A backup bundle of every older branch and worktree snapshot is at
`~/Projects/homm3/archive/homm3-decomp-branches-20260928.bundle`.

### Numbers at `70d038774`

| Measure | Value |
|---|---:|
| Exact functions, README (MAX) | **4,386 / 4,785** (91.7%); 10 short of exceeding the bar |
| Exact functions, ledger rows at 100 | 4,394 / 4,802 |
| Weighted executable MAX | 98.15% |
| Game data: exact / unverified / mismatch (file bytes) | 283,497 / 711 / **0** |
| Game data pending its function's match (EH records) | 1,872 |
| Unaccounted file / image bytes | 1,027 / about 2,470 |
| Overlapping bytes | 0 |
| Initializer comparisons | 11,449 exact, 28 unavailable |

## Byte accounting

`homm3 verify data-coverage --all-bytes` writes `build/gen/data_coverage.json`.
Every byte of the file and of the loaded image lands in exactly one category.
The definitions are in [data matching](../tooling/data-matching.md), sections
"Complete byte accounting", "Verified game bytes" and "Reviewed padding".

| Category group | Categories | Meaning |
|---|---|---|
| Finish-line (must be zero) | `missing`, `game-data-unverified`, `game-data-mismatch`, `overlap` | Unaccounted or unproven bytes |
| Exact against our compiled objects | `game-code-exact`, `game-data-exact`, `game-bss-exact`, `source-initializer-exact`, `source-padding-exact`, `source-cleanup-exact` | Our compiled output reproduces these bytes |
| Verified against pinned libraries | `library-runtime`, `library-vendor`, `linker-import` | LIBCMT, LIBCPMT, zlib, UUID/DXGUID and the import libraries |
| Tracked by function scores | `game-code-unverified`, `game-data-pending-function` | EH records verify automatically once their owning function becomes exact |
| Reviewed or structural | `padding`, `structural`, `compiler-generated`, `compiler-metadata`, `patch-residue`, `import-structure`, `import-thunk` | See below |

- **`padding`:** credited only through reviewed rows in
  `config/retail/data-extents.tsv` / `code-extents.tsv`, each with a proof
  class. Inference only proposes rows (`homm3 verify padding --propose`).
- **Stale padding rows:** a row whose bytes an exact comparison now covers is
  stale; retire it with `homm3 verify padding --stale --retire`.
- **`import-structure`:** format-bounded bytes with unresolved provenance;
  post-link editing is a hypothesis, not an identified common patch. See the
  [import-table todo](import-table-post-link-edits.md).
- **The invariant:** a data byte is exact only when a compiled definition
  *emits* it at that address. A short array's tail is flagged; it is never
  absorbed as padding. Worklist: `homm3 verify data-worklist [--unit U]`.

## How to continue

1. **Bootstrap a worktree.**
   - Symlink `build/orig`, `build/wineprefix`, `build/homm3-toolchain-vc6-sp3`,
     `build/mac/sdk` and `build/mac/toolchain` to the primary checkout's `build/`.
   - `export HOMM3_DIR=<worktree>` in every shell.
   - Run `homm3` inside `nix develop <worktree>`. The `homm3` on PATH uses an
     unpatched vostok that fails with "invalid data manifest header".
   - Then do a full `homm3 build` (build → delink → build if the first run
     fails only on stale delinked targets).
2. **Match functions under AGENTS.md.**
   - Keep recovered helper calls. Never replace them with direct fields or
     pasted bodies for a higher score.
   - Preserve Mac pair scores. `MAC_ABSTRACTION_FROM` is only for a proven move
     to a higher-level helper.
   - DC-proven shapes stay even when collateral functions dip.
   - No code or braces added only to steer the inliner.
3. **Integrate each branch into `codex/bytes-lib-20260928`.**
   - `git merge` the branch.
   - Resolve `config/match_baseline.tsv` with `homm3 status merge-baseline`;
     never take a whole side. README is regenerated.
   - Run the full `homm3 build` and check all gates: banked rows, Mac
     preservation, source ownership/inventory, cleanliness, data coverage.
   - Run `homm3 status check --baseline-ref <parent>`.
   - Commit, then fast-forward `codex/retail-data-recovery-20260928`.
   - Never `git stash`: `refs/stash` is shared across worktrees.
4. **Memory is tight on this machine.** Run one heavy command per worktree,
   and kill idle `homm3` wineservers when a worker finishes.

### Best leads toward the 4,395 bar

- **`findSpellTarget` is 3 IL units short of the /Ob2 "save cliff".** C1XX
  saves a callee body for inlining only at IL cost ≤ 175. It holds back
  `validSpellTarget`, `initiateSpell` and `castSpell`. See [inliner](../vc6/inliner.md),
  "The save cliff decides refused far below budget".
- **`bool` vs `unsigned char`:** about 200 functions return `unsigned char`
  where Dreamcast's decorated names say `bool`. This is systematic and can
  shift inlining costs. The list is in `/tmp/homm3-match-a/build/lane/ret_sweep.txt`.
- **Accessors that raise inlining cost.** When a callee is too cheap, or a
  nested budget too generous, the original probably called getters or setters
  where our source reads fields directly. Dreamcast usually keeps them out of
  line, so check its disassembly first. See [inliner](../vc6/inliner.md), "Free
  accessor sites are the lever behind retail's refused calls".
- **Near-exact rows with real instruction differences:**
  `/tmp/homm3-bytes-data/build/identity-census.tsv`, rows `instr:*`.
- **Per-function probes** are recorded beside each function in source. The
  open ones:
  - TSellArtifactWindow: its first `setupNewTrade` site;
  - game::save;
  - ScenarioStruct::read: blocked while `readPackedBits` delegates to the
    Mac-evidenced `decodePackedBits`; keep the delegation;
  - CNewPlayerUpdateProc::finish, allArtifacts, sendChat, updateGameVars;
  - the fade functions, vwDrawShroud, CObjectType, fillZoneArea.
- **Held patches:** `/tmp/homm3-match-a/build/lanea/mac-held-*.patch` are
  blocked by the Mac preservation gate and do not qualify for
  `MAC_ABSTRACTION_FROM`, so they stay held.

## Decisions waiting on the user

The full list with context is in
`~/Projects/homm3/archive/decisions-needed-2026-09-29.md`:

1. **Fifteen slot-sized buffers (602 file bytes).** Accept the round size, with
   the proven range recorded?
2. **objecttype's facet initializer (128 B).** Admit a hypothesised dead
   `ostream <<` writer, or leave it unaccounted?
3. **The static vector at 0x52bda0 (80 B + 16 B).** Admit a placeholder
   compiland and element type, or leave it?
4. **Post-link edit residue** (imports, `strlen`, tail bytes). Accept it as a
   recorded category?
5. **Literals referenced only by patched-out `setupCDDrive` (109 B).** Treat
   them as patch residue?
6. **Victor gaps (about 480 file / 1,300 image bytes).** Nothing references
   them and the library isn't pinned.
7. **Merge method** for #112 + #113.
8. **Stale branch and worktree cleanup.**
9. **A `MAC_ABSTRACTION_FROM` waiver** used for a DC-proven type change
   (`updateQuestLogButton`).
10. **When workers resume, and how many.** There is currently a pause on new workers.

## Issues recorded at the original handoff

- **Resolved: missing link gate.** Full builds now link the executable and
  propagate link failures. `text.cpp` owns the single `g_townNames[9][16]`
  definition; the duplicate in `game.cpp` was removed.
- **Resolved: local-static alignment lookup.** The emitted-name bridge handles
  `initializeSSkillTraits`'s `secondarySkillLevelNames` (336 B, `.bss`) when
  Clang and VC6 use different local-scope spellings. Its comparison is exact.
- **Pre-existing test failures:** `verify/test_gruntz_data_contracts`
  (harvest), `build/test_worktree_paths` and `core/test_project_flow`.
- **The forcefeedback `t_initialize_failure` EH records** stay "unresolved" in
  the data manifest because their names embed the worktree path.

## #78 audit (2026-09-29)

We asked whether the data campaign had independently found the bugs #78 fixes
to make the game start.

- **Data fixes are present, but ported, not rediscovered.** The archive tables,
  sound descriptors, sprite RLE constants, seer-name binding, game-context
  binding and the 163 hero rows came from #113 commits `86372c939`,
  `e645a7a83` and `66cda6be6` on 2026-09-28. That was five days after #78,
  with identical identifiers.
- **One code fix was found independently.** The Host widget null guard in
  `TMultiPlayerWindow`, via Dreamcast lines 938–940.
- **Three code fixes subsequently integrated from #78:** the `earlySetup` no-CD guard, the
  radar row stride, and the town animation timer (`max(150, delta)`). They sit
  in functions that are not exact yet.
- **Now verified in the continuation:** compiled scratch copies restore the
  six pre-fix zero initializers and shorten the hero table to 156 rows. The
  comparisons reject all seven defects; unchanged copies compare exact. The
  reference-binding probes keep the current reference types while restoring
  the old null binding, and the two archive probes use zero constructor
  arguments to restore the old zero-filled records.

## Authorized review integration

1. Push the validated checkpoint to `codex/retail-data-recovery-20260928`.
2. Refresh the #113 description with the latest evidence and explicit gaps.
3. Merge #113 into #112, preserving the campaign history, and verify the
   resulting tree against the full-build checkpoint.
4. Refresh #112 for review with the same gaps and validation results.
5. Leave #112 open. The user will review before merging it into the default
   branch. The needed #78 fixes and isolated runtime smoke are already covered
   by the continuation above.
