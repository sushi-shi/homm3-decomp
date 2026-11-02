# Open PR verification

Reviewed the open PR snapshot on 2026-11-02. There are 25 matching PRs
(#191–#215) and three older feature drafts (#118, #119, #146). All were drafts
with empty automated-check arrays at the snapshot; author-reported byte counts
are not independent verification. The user authorized #191–#194 and #215
subject to verification, and asked for #195's helper reuse to be investigated.
All 28 entries received static review; the three older feature drafts received
limited scope/risk triage, not the exhaustive matching-diff review used for
#191–#215. Other PRs remain for the user's review. No review comments were
posted. This document consolidates the five workers' source/native reviews
and independent compiler results; author descriptions are not compiler evidence.

## Dispositions

This table records source/native review. Compiler results and landing state
are recorded separately below; “no static blocker” does not mean byte-verified.

| PR | Reviewed head | Finding |
| --- | --- | --- |
| [#118](https://github.com/sushi-shi/homm3-decomp/pull/118) | [3db401c5a954](https://github.com/sushi-shi/homm3-decomp/commit/3db401c5a954c167665b2e34b61d0d7622a79134) | Conflicting feature draft; unconditional content/layout expansion changes the retail target. |
| [#119](https://github.com/sushi-shi/homm3-decomp/pull/119) | [59c1efbd8993](https://github.com/sushi-shi/homm3-decomp/commit/59c1efbd8993b89c5ab4eebafc02ceb0c53a76b1) | Conflicting feature draft; UI behavior and full historical comparisons remain unverified. |
| [#146](https://github.com/sushi-shi/homm3-decomp/pull/146) | [c873a3a16af2](https://github.com/sushi-shi/homm3-decomp/commit/c873a3a16af23fad98f2b5746144a9e0918d0226) | Conflicting unfinished integration; HotA generation remains explicitly gated. |
| [#191](https://github.com/sushi-shi/homm3-decomp/pull/191) | [67e44b625339](https://github.com/sushi-shi/homm3-decomp/commit/67e44b6253393a74e508f3560d4ba3277e9df5d2) | Raw PR blocked. Corrected local integration retains only cmbtmgr/spells; original command body and source-owned identities retained. |
| [#192](https://github.com/sushi-shi/homm3-decomp/pull/192) | [3b6ad1004a5e](https://github.com/sushi-shi/homm3-decomp/commit/3b6ad1004a5e2a187586cbc2ae965c36b1634641) | Verified isolated target at 100%; locally integrated. Reused native local; source comment reconciled. |
| [#193](https://github.com/sushi-shi/homm3-decomp/pull/193) | [15a40826f920](https://github.com/sushi-shi/homm3-decomp/commit/15a40826f920da85a295b8cbd8596672c0bb4c1c) | Verified isolated target at 100%; locally integrated. Canonical signed-byte read preserves existing short-read behavior. |
| [#194](https://github.com/sushi-shi/homm3-decomp/pull/194) | [875484190b65](https://github.com/sushi-shi/homm3-decomp/commit/875484190b65c38e8081d9cb9bfcd358ece600d7) | Verified isolated target at 100%; locally integrated. Retail proves both free and paid purchases consume stock. |
| [#195](https://github.com/sushi-shi/homm3-decomp/pull/195) | [aba1fd5f5b92](https://github.com/sushi-shi/homm3-decomp/commit/aba1fd5f5b92d7be2955444b5db51498da352f75) | Raw and shared-writer candidates reach 100%; tested canonical shared writer locally committed, pending landing gates. |
| [#196](https://github.com/sushi-shi/homm3-decomp/pull/196) | [626f09a97e2b](https://github.com/sushi-shi/homm3-decomp/commit/626f09a97e2b8701a02de09df11e41b38cdc9e31) | Verified isolated target at 100%; native shutdown-load order supports the loop. |
| [#197](https://github.com/sushi-shi/homm3-decomp/pull/197) | [07d26ae16a6b](https://github.com/sushi-shi/homm3-decomp/commit/07d26ae16a6b8ca04e17ce803bcfb208307116b8) | Verified isolated target at 100%, including three calling units; explicit convention and element references. |
| [#198](https://github.com/sushi-shi/homm3-decomp/pull/198) | [8d6f0e147085](https://github.com/sushi-shi/homm3-decomp/commit/8d6f0e14708568d4f4ddff31b118e67d24aa9a25) | Verified isolated target at 100%; unused current-layout overlay types remain as cleanup. |
| [#199](https://github.com/sushi-shi/homm3-decomp/pull/199) | [2b6cbbba84c8](https://github.com/sushi-shi/homm3-decomp/commit/2b6cbbba84c8d56ce7764c02df6d33d0c0a22535) | Verified isolated target at 100%; natural scope change, with EH-source claims qualified below. |
| [#200](https://github.com/sushi-shi/homm3-decomp/pull/200) | [4e9e9ddc7532](https://github.com/sushi-shi/homm3-decomp/commit/4e9e9ddc753228ffc0ce51cc969a0ebd5cc8c0df) | Blocked: otherwise unused double assignment added solely for x87 slot steering. |
| [#201](https://github.com/sushi-shi/homm3-decomp/pull/201) | [fc20da881727](https://github.com/sushi-shi/homm3-decomp/commit/fc20da881727bab5a12716363840058cf3e5cf5f) | Verified isolated target at 100%; plausible side/index reuse preserves helpers. |
| [#202](https://github.com/sushi-shi/homm3-decomp/pull/202) | [5304445fa962](https://github.com/sushi-shi/homm3-decomp/commit/5304445fa9623127eea6e330bf9d2a55067bc135) | Verified isolated target at 100%; ordinary countdown loops in existing scopes. |
| [#203](https://github.com/sushi-shi/homm3-decomp/pull/203) | [ca740ddc8589](https://github.com/sushi-shi/homm3-decomp/commit/ca740ddc858977cdbd5c1ab397e32655d469e70c) | Verified isolated target at 100%; native names and ordinary pointer reuse. |
| [#204](https://github.com/sushi-shi/homm3-decomp/pull/204) | [8c00622c85a9](https://github.com/sushi-shi/homm3-decomp/commit/8c00622c85a962e6f568052d141fd9595f684b7c) | Blocked: committed inline_depth(0) and unsupported platform store-order fork. |
| [#205](https://github.com/sushi-shi/homm3-decomp/pull/205) | [f79ca22a26db](https://github.com/sushi-shi/homm3-decomp/commit/f79ca22a26db43077d6ddb28082cc654717d0629) | Two isolated targets reach 100%; canonical getAlignment reuse and supported normalization. |
| [#206](https://github.com/sushi-shi/homm3-decomp/pull/206) | [6fd55e1fc74e](https://github.com/sushi-shi/homm3-decomp/commit/6fd55e1fc74e19df8f54ccfc7026fb43c031532c) | Two isolated targets reach 100%; natural assignment-result reference and index reuse. |
| [#207](https://github.com/sushi-shi/homm3-decomp/pull/207) | [d1c7d737633b](https://github.com/sushi-shi/homm3-decomp/commit/d1c7d737633bdf41a06d94ed9925292742e48752) | Three isolated targets reach 100%; payload layout retained and Mac-supported guarded returns. |
| [#208](https://github.com/sushi-shi/homm3-decomp/pull/208) | [51bc31aca836](https://github.com/sushi-shi/homm3-decomp/commit/51bc31aca836fffe7fe797c42d6f1c5917b8dd3c) | Verified isolated target at 100%; ordinary coordinates and widget-ID reuse. |
| [#209](https://github.com/sushi-shi/homm3-decomp/pull/209) | [a4747241e2ee](https://github.com/sushi-shi/homm3-decomp/commit/a4747241e2ee6a41f69552d230384ce1cca49d27) | Verified isolated target at 100%; meaningful snapshot, original local still a hypothesis. |
| [#210](https://github.com/sushi-shi/homm3-decomp/pull/210) | [82ab38093585](https://github.com/sushi-shi/homm3-decomp/commit/82ab38093585f3f67ed81319701ddaee430b1e1c) | Two isolated targets reach 100%; pure operand ordering retains dimension helpers. |
| [#211](https://github.com/sushi-shi/homm3-decomp/pull/211) | [2c81723e8d53](https://github.com/sushi-shi/homm3-decomp/commit/2c81723e8d53fbe23bb8f5bbd59b6bb368c01d53) | Blocked: recovered getGeneratorBonus removed; known bare lifetime probe reinstated. |
| [#212](https://github.com/sushi-shi/homm3-decomp/pull/212) | [5b35a5e11aa0](https://github.com/sushi-shi/homm3-decomp/commit/5b35a5e11aa022609955962753996fbc0bf5ccce) | Revise constructor: discarded assignment ternary justified only by compiler budget. |
| [#213](https://github.com/sushi-shi/homm3-decomp/pull/213) | [b2d4a6ad8cc6](https://github.com/sushi-shi/homm3-decomp/commit/b2d4a6ad8cc6b7e49175c35209431d484e09f040) | Revise: new unnecessary goto; /Gr already provides the claimed fastcall ABI. |
| [#214](https://github.com/sushi-shi/homm3-decomp/pull/214) | [aa04d6b3b1ab](https://github.com/sushi-shi/homm3-decomp/commit/aa04d6b3b1ab8da4b04af03ef2b58c6daadbba51) | Blocked: unchecked GAME alias admission and redundant compiler-state guard. |
| [#215](https://github.com/sushi-shi/homm3-decomp/pull/215) | [b5a507f312d4](https://github.com/sushi-shi/homm3-decomp/commit/b5a507f312d44441c59619fd5caa46abbb8dda6c) | Verified isolated game target at 100%; locally integrated with residual note refreshed. Full editor comparison also reaches 100% (previously 91.4699%). |

## Blocking findings

Line positions below refer to the downloaded PR diffs, before integration.

- **#191, `src/command.cpp:462–465`:** direct `m_facing` logic replaces the recovered
  `needToTurn` call. The expression is equivalent over this loop's bounded
  direction values, but discards an established native helper boundary.
  Preserve the call. The new `config/retail/runtime-aliases.tsv` also derives
  a retail identity from the authored declaration/VC6 symbol; those are not
  independent proof. The existing alias reader admits rows unconditionally
  as library identities. Do not introduce a second game-symbol ledger.
- **#200, `src/ai_tactical.cpp:1073–1075`:** the new `combined` double is assigned but
  never read. Its comment expressly identifies compiler scratch-slot steering
  as its purpose; the removed comment already recorded and rejected this
  exact probe. Missing DC locals alone would not prove absence; the unused
  operation and its stated compiler-only purpose are the issue.
- **#204, `src/mapcell.cpp:2175–2182`, `:3340–3348`:** the committed `inline_depth(0)` fence is a
  diagnostic expressly forbidden in recovered source. A Windows-only fork
  of two independent stores also lacks source evidence for that platform
  distinction. The image-name reference can be considered separately.
- **#211, `src/town.cpp:1320`, `:262–266`:** direct generator-bonus indexing drops a DC-proven
  accessor. A bare block reinstates an earlier exact but unsupported lifetime
  probe. Recover the source boundary rather than discarding it for a score.
- **#212, `src/townmgr.cpp:6845–6849`:** the discarded nested assignment ternary is
  justified only by changing statement/inliner cost. The separate dispatch
  value is reasonable; the constructor spelling needs source justification.
- **#213, `src/kb.cpp:1695`:** an added goto targets the statement immediately
  after its switch, while an ordinary break expresses that source transfer.
  Native evidence for this extra label is missing. The owning profile already
  uses `/Gr`, so explicit fastcall is not an established ABI repair.
- **#214, `config/retail/runtime-aliases.tsv:5–17`, `src/events.cpp:1692`:** thirteen GAME aliases bypass source-owned identity generation and
  can remain stale after source changes. A redundant friendly-path team check
  is explicitly compiler-significant and has no demonstrated source role.
  These findings do not establish that the numerical addresses are wrong.
  The independently DC-proven static cheat-checker linkage is a valid separate
  recovery, and should not be discarded with the unsupported changes.

## Packed-bit writer (#195)

The PR's new TU-local void writer duplicates `rmg.cpp`'s existing
`writePackedBits` template. Its six RMG callers serialize hero availability
and disabled artifact/spell/skill masks. `hero::save` contains the same
48-bit pack/write operation; `playerData::save` writes 12 bits. Native Mac
`0xcd1f0..0xcd25c` and `0xf3d84..0xf3e08` independently show zero-filled
buffers, const bit tests, LSB-first packing and one stream write. The hero
body's counted zero-fill is evidence against its current memset spelling.

The tested shared candidate puts one ordinary template body in `packed_bits.h`,
retains the existing RMG integer success-result contract, and calls it from
player and hero saving. Cross-TU visibility supports header ownership; the
original helper name and exact original placement remain inferred. No forced
inline control is added. RMG's existing return comparison remains part of
its retained helper; player/hero callers continue ignoring the write result.
Both the raw PR and this shared-writer candidate independently reproduce
`playerData::save` at 100%, with zero projected MAX losses, including the
checked hero/RMG consumers. The shared version is locally applied for
integration, not yet published; it preserves one existing implementation
rather than adding the PR's duplicate TU-local void writer.

Other similar code is not automatically interchangeable:

- The editors already own `writeBitset` for their different `TRawOStream`
  interface, including its stream/error semantics. A matching bit order does
  not justify changing that stream boundary.
- `NewfullMap::saveObjectType` writes four masks and checks each returned byte
  count using `< 6`. The existing RMG helper returns equality-as-success;
  blindly replacing the checked paths would change their result contract.
  Keep those paths pending separate native/error-contract evidence.

## Older feature drafts

These were triaged separately from the matching batch. Their complete file
lists, scope, mergeability and selected high-risk mechanisms were inspected;
this was not a fresh runtime certification or an exhaustive review of every
generated data row.

- **#118:** content counts and save version change unconditionally, and
  `va.h` imports the expanded-content header. This is an intentional Cove
  extension, not a byte-preserving Complete reconstruction. It currently
  conflicts with the target branch. Campaign transfer, multiplayer and full
  historical comparisons are unverified; the PR description's old mergeable
  claim is stale. Keep it as a separate feature review.
- **#119:** the new inventory unit is explicitly `compare = false`, while
  native hero and merchant paths are changed. The patch deliberately adds
  gameplay behavior, and its own validation stops before inventory interaction
  testing/full matching. It currently conflicts. Selection, scroll identity,
  duplicate sales, UI lifecycle and multiplayer behavior still require feature
  validation before this can be called ready.
- **#146:** the resource parsers use bounded byte views and checked arithmetic;
  resource codecs other than stored/zlib remain explicitly rejected. The
  placement entry rejects non-Complete rulesets, matching the documented
  incomplete HotA state. The corpus-message adjustment preserves the specific
  known fault classifier rather than swallowing arbitrary failures. No hidden
  validation bypass was identified in those reviewed mechanisms. Whole-map
  HotA output and independent DLL replay remain absent, and the branch conflicts.

## Source and native qualifications for accepted candidates

These findings supplement the dispositions and measurements; an exact score
is not proof of unique original C++ text.

| PR | Source review and remaining qualification |
| --- | --- |
| #192 | DC `dc:0x034630` names `start_point`; its path-search value is dead before reassignment. Reusing it is distinct from the previously rejected extra point local. The source-line gap does not independently prove the assignment. |
| #193 | `readValue<signed char>` retains the same one-byte signed read, ignored short-read result and artifact conversion. No new error-handling regression is introduced. |
| #194 | Retail `0x0042d420` branches free purchases past resource deduction into the common stock-decrement tail. The unchanged short counter is a tested source spelling, not a type proven by optimized x86 alone. |
| #196 | Retail shutdown loads at `0x0059a6d5` and `0x0059a73f/0x0059a747` support the short-circuit loop condition. Sample query, timeout, sleep and cleanup retain their order. |
| #197 | Explicit `__fastcall` agrees with the documented ECX-first `/Gr` interface; per-element references retain `questActiveforPlayer` and insertion order. Header and all three calling units were checked. This does not establish a newly repaired ABI. |
| #198 | Each legacy dword is captured before overlapping `NewmapCell`/`ExtraInfoUnion` writes. Existing bit widths and enum storage stay intact. Retail receives the cell in ECX and version in EDX. Unused `CurrentArtifactInfo`, `CurrentUpgradeWagonInfo`, and `CurrentVisitedInfo` declarations remain a separate cleanup lead. |
| #199 | Moving path construction before `try` leaves stream construction, call, destruction and exception handler together. The retail EH store at `+0x50` occurs during path copying; scheduling does not prove all original path work was outside the exception region. No independent raw EH-data audit is claimed. |
| #201 | Reused side/index values feed the same `markCreatureEffect` calls; all `getOwningSide` helpers and damage sequencing remain. Missing names in the older optimized DC local inventory do not disprove meaningful extra locals. |
| #202 | Positive fixed countdowns preserve iteration counts and widget order. Row scopes already existed; this introduces no bare lifetime block. |
| #203 | Every reused town pointer is assigned on its path; cursor offsets read initialized values. `path_cell` and `new_cursor` are native spellings, and cursor/trigger helpers remain. |
| #205 | `game::getAlignment` owns the version-0 elemental exception and replaces duplicated logic correctly. Mac `0:0x2f990` supports alliance normalization's operation/helper path, not a unique count of scalar names. The native `_N` parameter evidence remains. |
| #206 | Binding to assignment's returned reference preserves member assignment and retirement order. Sequential loops reset the reused signed index over bounded collections. Complete-only `rehomeCampaignHeroSetup` has Mac, not DC, evidence; Mac register reuse does not prove unique source locals. |
| #207 | Eight named int payload members retain the former array's order/layout; names are role-derived. Mac `0:0x175674` supports the guarded-return shape and retained `getHeaderDirectory`. Index expressions retain values and branch order. |
| #208 | Folded coordinate expressions are pure; six loops each initialize the reused widget ID. No new compiler-only operation or scope appears. The new comparison supersedes the older failed probe only in this tested context. |
| #209 | A meaningful quantity snapshot follows the same broadcast, with no intervening mutation/call/volatile read. It is a plausible load/lifetime hypothesis, not an independently recovered native local spelling. |
| #210 | Pure integer operands change order; dimension and map-item helpers remain. This is an ordinary spelling hypothesis, not native proof of original operand order. |
| #215 | Existing index/count locals move within the same scope; the position constructor is empty and the accessor copies a value without mutation. The game target was verified; the editor `0x004a2ffa` comparison remains part of integration gates. |

The accepted partial changes inside blocked PRs remain distinct from approval
of those PRs. #212's dispatch-value cache and #213's delayed initialization
are reasonable independent changes. #214's static cheat-checker linkage is
positively supported by DC `dc:0x3b0`; its native `sErr` spelling and several
ordinary local/reference edits have no demonstrated behavior defect. The
`_Eos`/`_Grow` ownership moves still need fresh emitted-object ownership proof,
and campaign frame-home claims require raw operand review. None of these
observations validates the unchecked aliases or redundant guard.

## Independent compiler verification

Each worker first refreshed the relevant live units with `homm3 build --fast`
using the pinned per-TU VC6 profiles. Disposable source-family manifests then
applied the downloaded source/header differences to the cleanup/integration
base. These were isolated PR-delta checks, **not full historical PR-head
checkouts**, merged-branch validation, final linking or independent raw-byte
accounting. Reported percentages use the repository's normalized function
comparison. Mac evidence informed review; no new PR-specific Mac score is
claimed by these isolated Windows checks.

Every recorded family passed manifest validation, unchanged-source control,
opposite-corner compile/code-relocation reproduction and retained-candidate
recompilation. Worker families exhausted two states and produced two distinct
objects each. The coordinator exhausted nine distinct states with eight
reproduced elites; it tested separate patches and a combined reviewed option.
Source-family runs themselves changed no authored source or banked MAX.
Rejected source models were not compiled solely to reproduce their claimed
100% result.

Scores below are baseline **MAX → candidate observed score**, in percent.
A loss count applies to all compared rows in the listed units, not just the
target. All accepted isolated candidates below have **zero projected MAX
losses**. The corrected #191 experiment's command loss is separately recorded.

| PR/candidate | Compared units | Target observations |
| --- | --- | --- |
| #192 | Coordinator nine-unit context below | `attemptTeleport` 99.9755 → 100 |
| #193 | Coordinator nine-unit context | `readBlackBox` 95.6262 → 100 |
| #194 | Coordinator nine-unit context | `doBestPurchase` 97.2922 → 100 |
| #195 raw and shared | Coordinator nine-unit context, including `hero` and `rmg` | `playerData::save` 99.9557 → 100 for both |
| #196 | `soundmgr` | `waitEndSampleThread` 91.9474 → 100 |
| #197 | `questlogwindow`, `advmgr`, `hero`, `swapmgr` | `doQuestLog` 96.5417 → 100 |
| #198 | `mapcell`, `game` | `upgradeCellExtraInfo` 96.2635 → 100 |
| #199 | `game` | Filename `newMap` 99.3800 → 100 |
| #201 | `army` | `doAttack` 99.9232 → 100 |
| #202 | `adventuremapwindow` | Constructor 98.4879 → 100 |
| #203 | `advmgr` | `processHover` 99.7070 → 100 |
| #205 | `ai_player` | `chooseWeakestArmy` 90.1357 → 100; `valueOfAddingArmy` 94.2397 → 100 |
| #206 | `customcampaign` | `rehomeCampaignHeroSetup` 99.9526 → 100; `completeCurrentMap` 99.9847 → 100 |
| #207 | `singleselectionwindow` | `finish` 87.0155 → 100; `getHeaders` 94.7119 → 100; `setNewPlayerSlot` 98.8866 → 100; collateral `onMapFileNameMsg` 91.1860 → 96.6777 (still non-exact) |
| #208 | `systemoptionswindow` | Constructor 98.3461 → 100 |
| #209 | `tradpost` | `TGiveResourceWindow::update` 99.9819 → 100 |
| #210 | `rmg` | `initializeZones` 96.6803 → 100; `placeZoneTreasures` 99.9892 → 100 |
| #215 | Coordinator nine-unit context | `recenterZone` 95.5000 → 100 (game) |

For #207, fresh baseline CUR for `getHeaders` was 94.6708, below its unchanged
94.7119 MAX; the control reproduced that CUR, and the candidate beats both.
The candidate needed only an evidence-comment context adaptation before all
six original PR hunks applied; no semantic hunk was omitted or rewritten.

### Corrected #191 and local integration

The nine-state run also tested #191 with `needToTurn` restored and the GAME
alias removed. `initNonVisualVars` reached 100 from 99.9631 and `showMassSpell`
reached 100 from 96.3708, but the remaining `setCombatDirections` restructuring
fell from 91.3686 to 91.0034. This was an edited-function loss, not unexplained
unchanged-source CUR noise. It is recorded rather than hidden behind the two
gains.

Follow-up native review supports retaining the prior command body: Mac
`0x82958`/`0x82a58` initialize validity accumulators before guarded tests, and
`0x82c0c` begins a separate fallback loop. DC `dc:0x06b66c` names
`attack_angle` and `closest` separately; shared stack slots are optimizer
clues, not unique source declarations. The optional counter/scope/materialized
condition edits recover no additional established source fact that would
justify adopting this candidate's loss.

The final **local** #191 integration therefore keeps only `cmbtmgr`/`spells`,
retains the complete previous `command.cpp` body, and omits the manual runtime
alias. It does not adopt the measured losing command candidate. #192, #193,
#194 and #215 are also locally integrated. #195's tested shared writer is
locally committed with the duplicate removed. The five cleanup lane
commits are locally integrated separately from these PR decisions. The combined 19-unit targeted Windows comparison passed with seven MAX gains
and no MAX losses. Full editor/Mac landing gates remain the next step;
isolated results do not substitute for them. No PR has been merged on GitHub or
published by this work at this checkpoint, including corrected #191.

### Reproduction artifacts and context

Artifacts are ignored experiments on the SSD, not a second symbol/score
ledger. All reviewed head refs remain in the disposition table. The original
review inputs and the five detailed worker reports are under
`/tmp/homm3-pr-review-20261102/`; `types-compiler-results.json` records candidate
IDs, control outcomes, exact counts, score changes and output paths for
#196/#201/#205/#206/#207. The following directories retain manifests,
isolated source, compiler logs and measured candidate checkpoints.

| Context/worktree | PR | Source-family result directory under `build/source-families/` |
| --- | --- | --- |
| `/tmp/homm3-cleanup-integration-20261102` | #192/#193/#194/#195 raw and shared/#215/#191 corrected/combined | `ad71b60779bc2e79e40f` |
| `/tmp/homm3-cleanup-types-20261102`, commit `ebb23b1a84590ea4aa9b54db7a8e586f45a380f3` | #196 | `207f4d14c6bc8c81efec` |
| Same types context | #201 | `7d231cd4002b95ffe641` |
| Same types context | #205 | `6976c537252d00c43d00` |
| Same types context | #206 | `d01972b482890365c18f` |
| Same types context | #207 | `7b2084db6ec8db53f4f1` |
| `/tmp/homm3-cleanup-editors-20261102` | #197 | `177f6036419a7d35f70b` |
| `/tmp/homm3-cleanup-enums-20261102`, commit `6e71ce804` | #198 | `15e5572da94b090d321a` |
| Same enums context | #199 | `3dd1d2fdeb4cd33e6c91` |
| Same enums context | #210 | `73bb1be1d63fecf1c23c` |
| `/tmp/homm3-cleanup-names-20261102`, commit `d413ed6b6` | #202 | `eb047bc851e60e3bc1f8` |
| `/tmp/homm3-cleanup-flow-20261102`, base `eb0115861` | #203 | `ed3b104c79ad31a32580` |
| Same flow context | #208 | `226553ba84322f7d3373` |
| Same flow context | #209 | `7026a4362f738989bd18` |

The coordinator context compares `cmbtmgr`, `command`, `spells`, `ai_player`,
`mapcell`, `game`, `hero`, `rmg`, and `customcampaign`; its log is
`build/pr-candidates/verification.log`. Worker manifests/logs are under
`build/pr-review-<number>/`, except #197's console log is
`build/pr197-source-family.log`. Worker invocations used
`--width 50 --keep 1 --generations 1 --jobs 2`; these finite two-state
families did not invent variants to fill the width. The nine-state
coordinator run kept eight elites. Preserve these context qualifications when
quoting a score outside this document.

## Integration checkpoint

The full game build compiled, compared and linked with no unresolved externals
and no source-edit MAX losses. The first full map-editor pass found a cleanup
regression outside the reviewed PRs: `_setTriggerMask` fell from 100% to
98.72% after replacing its shared exit with a direct return. The game had
remained exact. Landing was stopped before publication; the original goto
was restored and its rejected probe recorded beside the function. This is
why the lane-local checks were not treated as complete cross-image proof.

The restored trigger body was recompiled at 100% in both images; both
ledgers were refreshed with no remaining source-edit MAX loss. The same
full editor comparison independently raises #215 recenterZone from
91.4699% to 100%, while the shared packed-bit writer preserves its RMG
editor consumers. Publication remains subject to the restarted full gates.
