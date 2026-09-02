# Track R - the RTM C2 (12.00.8168) generation A/B

Several open matching walls - merged-return / tail-duplication divergence in
BOTH directions (path.obj's cross-jumped inline tails, kbwin's AppWndProc,
ai_combat's tail-duplicated epilogues), the cross-TU callee-saved
register-swap signature (heroWindow::CenterWindow, button::Main,
TPickANumber), and the "CL-generation-capped" lea/zero-extension one-offs -
were filed against a *stale-CL-generation* hypothesis: retail was partly
built by the VC6 RTM back end (CL generation 8168), our pinned toolchain is
SP3 (C2 12.00.8447), and maybe the walls are generation artifacts, closable
with zero reverse-engineering.

Track R settles that hypothesis empirically. **Answer: no.** For every wall
in the corpus the RTM back end emits *byte-identical code* to the SP3 back
end; the entire measurable generation delta across the twelve owning TUs is
a branch-signedness choice on two loop guards plus one back-edge retarget -
and retail sides with SP3 at all three sites. The walls are real model
gaps, not generation artifacts. The corpus TUs demonstrably belong to the
Rich header's 8447 band.

## 1. The Rich-header evidence (retail bytes)

Decoded 2026-08-09 from the gated `HEROES3.EXE` (fixed base 0x400000; the
`Rich` marker precedes the PE header, entries XOR'd with the key stored
after it). Raw table, `(prodid, build) -> object count`:

| prodid | build | count | reading |
|---|---|---|---|
| 0x01 | 0 | 270 | Import0 (import thunks) |
| 0x00 | 0 | 69 | Unmarked |
| 0x0a | 8168 | 178 | Utc12_**C** objects, RTM generation (CRT/libs) |
| **0x0b** | **8168** | **26** | **Utc12_C++ objects, RTM generation** |
| **0x0b** | **8447** | **145** | **Utc12_C++ objects, SP3 generation (our pinned C2)** |
| 0x0c | 7291 | 12 | AliasObj60 |
| 0x0e | 7299 | 41 | Masm613 (CRT asm) |
| 0x04 | 8168 | 4 | Linker600-band tool |
| 0x13 | 8034 | 19 | (VC6-era tool, id not confidently named) |
| 0x13 | 9049 | 3 | (VC6-era tool, id not confidently named) |
| 0x06 | 1735 | 1 | Cvtres500 |

So 26 C++ objects in retail carry the RTM compiler stamp against 145 with
our pinned SP3 stamp. The Rich header aggregates counts only - it cannot
attribute a generation to a *named* object, which is exactly why the A/B
below matters: it lets the retail bytes of a specific function testify
which generation's output they match.

## 2. Sourcing provenance (archive.org, user-directed)

Staged 2026-08-09 at `../orig/vc6-rtm/` - OUTSIDE every repo, per the
ground-truth rule that game/toolchain bytes never enter git. Sidecar
`../orig/vc6-rtm/PROVENANCE.txt` carries the same record.

- **Primary source**: archive.org item
  `1998-10-01-visual-studio-6.0-enterprise-edition-disc-1`, file
  `VSE600EUN1.ISO` (642,975,744 B, item-published sha1
  `bd742f7cceaf2e3032982ba2b4582358f79b3b04`), path `VC98/BIN/C2.DLL`,
  fetched via archive.org's on-the-fly ISO extraction:
  `https://archive.org/download/1998-10-01-visual-studio-6.0-enterprise-edition-disc-1/VSE600EUN1.ISO/VC98%2FBIN%2FC2.DLL`
- **Cross-verification** (independent second rip, dual-rip discipline):
  item `vs-6.0-enterprise`, `VS6.0Enterprise_Disk1.iso` (645,496,832 B),
  same internal path - **byte-identical** C2.DLL.
- **Rejected during sourcing**: item
  `2000-01-01-visual-studio-6.0-enterprise-edition-disc-1`
  (`en_vs60_ent_d1.iso`) carries C2.DLL FileVersion **12.00.8141.0** - a
  pre-RTM (release-candidate) build masquerading as install media. Not
  admitted; recorded here so nobody re-fetches it.

The admitted subject and its siblings (all FileVersion 12.00.8168.0 from
`VS_FIXEDFILEINFO`):

| file | size | sha256 | role |
|---|---|---|---|
| `C2.DLL` | 737,329 | `45187b0b6288240f73272a7c61e6329c50048a76e57db3bb87b6f0229e09e27d` | **the Track R subject** |
| `C1.DLL` | 667,697 | `ee14a2603a1339f95db2e396751f7e51b50e421731229fb2416a01f974689d80` | staged, unused |
| `C1XX.DLL` | 1,183,795 | `4095e9e8de5ac4b13dbfe310abb8de711475b1a92c5f94375919947937f3237e` | staged, unused (the next lever - see §4) |
| `CL.EXE` | 65,536 | `d9220fdfaf355c81f26e7b03c5dc628ec72ff81c99dc38ed98b1ad4bdbbb309c` | byte-identical to the pinned SP3 driver stub |

Notable identities: the RTM `CL.EXE` equals the pinned toolchain's driver
byte for byte (SP3 never updated the driver - consistent with
`_toolchain.PINNED`'s "CL driver stub 12.00.8168"), and the RTM `C2.DLL`
has the *same size* as the SP3 one (737,329 B) with different bytes. The
RTM C2 exports exactly `{_InvokeCompilerPass@12, _AbortCompilerPass@4}`,
same as SP3 - the drop-in slot the shim work already proved.

## 3. The harness (`homm3.vc6.genab`)

`scripts/homm3/vc6/genab.py`; dispatcher verb not yet wired, run as:

```sh
nix develop .#build
PYTHONPATH=scripts python3 -m homm3.vc6.genab build-rtm   # create/refresh the overlay
PYTHONPATH=scripts python3 -m homm3.vc6.genab run         # A/B the corpus, write the TSV
PYTHONPATH=scripts python3 -m homm3.vc6.genab clean
```

- **Overlay**: the exact `shim/build.py` mechanism - `build/vc6/
  toolchain-rtm/msvc` with `bin/` copied (the driver's `LoadLibraryA` must
  resolve real files inside the overlay) and everything else symlinked;
  then the RTM DLL replaces `bin/C2.DLL`. The pinned tree is never touched.
- **Gating**: the pinned binaries are `_toolchain.resolve`-gated before
  copying; the RTM DLL is gated on sha256+size (pinned in `genab.RTM_PINNED`)
  + FileVersion 12.00.8168.0 + the exact two-export table, at overlay build
  AND on every re-run. A wrong pressing is a hard rc-2 abort; only *absence*
  degrades to `RTM-unavailable` rows, so the track stays drop-in.
- **Compiles** go through `homm3.core.cc_wrap` (the wrapper ninja runs)
  with each unit's own `config/units.toml` profile; `MSVC_DIR` selects the
  overlay for the RTM side. Objects cache under `build/vc6/genab/{sp3,rtm}/`.
- **Front/back mixing, measured**: the RTM C2 8168 accepts the IL of the
  pinned SP3 front end (C1XX 12.00.8472) without complaint - every corpus
  TU compiled clean, and the produced objects carry `@comp.id` build
  **8168** (0xb1fe8) where the SP3 objects carry **8447** (0xb20ff). The
  comp.id stamp doubles as the harness's in-loop control: the RTM run
  provably went through the RTM back end.
- **Metric**: three-way compare per function - ours-SP3 vs ours-RTM vs
  retail (the delinked `build/objdiff/target/<unit>.c.obj`, same
  llvm-objdump producer both sides; capstone over the image only as
  fallback, recorded in the `retail_producer` column).
  `align` = `homm3.vc6._align.distance` (unpaired register-visible
  masked-instruction slots), `flow` = `homm3.vc6._flow.distance`
  (branch-shape disagreements). Verdict grades at `align+flow == 0` -
  register-visible + branch-shape, deliberately weaker than byte-exact
  (absolute address operands stay masked); a byte ratchet still needs the
  real pipeline. Caveat: on switch-bearing bodies (ValidAttack) inline
  jump-table bytes inflate both retail-side distances; the `sp3_vs_rtm`
  column (same producer, same layout) is unaffected and is the
  generation-question column.
- **Verdicts**: `SP3-matches` | `RTM-closes` | `neither` |
  `RTM-unavailable`, one row per function in
  `evidence/vc6/c2-generation-verdicts.tsv` (regenerate, never hand-edit).
  rc 0 = ran with RTM available, 1 = `RTM-unavailable` rows present,
  2 = error.

**Corpus** (18 walls/controls + 3 sentinels): the six Track R walls -
path's `FindPath` / `GetAdjacentCellIndex` / `ValidAttack`, kbwin's
`AppWndProc`, town's `check_shipyard_square` / `get_legion_bonus` - plus
every function whose in-tree residual comment cites merged-return /
DUP-EXIT / tail-merge / the CL-generation class (`VideoRealignBuffers`,
`VideoClose`, `DoCompAI`, `cast_enchantment`, `button::Main`, `StartMP3`,
`CenterWindow`, `TPickANumber` ctor, `NextRandomFrame`,
`NextRandomSiegeEngineFrame`), two byte-exact controls whose comments cite
the DUP-EXIT shape as *closed* (`move_toward`,
`get_ranged_attack_value` - both must come back `SP3-matches` or the
harness is broken; they do), and the three §4 sentinels.

## 4. Verdicts (run 2026-08-09; TSV is authoritative)

Tally: **6 SP3-matches, 15 neither, 0 RTM-closes, 0 RTM-unavailable.**

The decisive column is `sp3_vs_rtm`: **0 for all 18 wall/control rows.**
The RTM back end reproduces our SP3 output *exactly* on every wall. At the
whole-object grade the statement is even stronger: for 10 of the 12 corpus
TUs the sp3/rtm object files are byte-identical outside the COFF
TimeDateStamp and the `@comp.id` stamp. The remaining two TUs differ in
exactly three functions - the sentinels:

| function | sp3 (align+flow) | rtm | sp3-vs-rtm | reading |
|---|---|---|---|---|
| `town::GiveSpells` | 18+35 | 20+34 | 7 | RTM farther (54 vs 53) |
| `town::initialize_spells` | 148+38 | 156+44 | 24 | RTM farther (200 vs 186) |
| `SetMenus` (kbwin) | 6+0 | 8+2 | 4 | RTM farther (10 vs 6) |

And the *content* of the whole generation delta, disassembled: in
`SetMenus` and `GiveSpells` the 8168 back end emits the **signed** loop
guard (`jl`, opcode 0x7c) where 8447 proves the induction variable
non-negative and emits the unsigned twin (`jb`, 0x72); in `GiveSpells` the
8447 build additionally jump-threads the loop back edge into that guard
site where 8168 targets past it. That is the *entire* observable
difference between C2 12.00.8168 and C2 12.00.8447 on ~120 KB of compiled
game code. Retail's bytes side with the SP3 choice at all three sites, so
these TUs sit in the Rich header's 145-object 8447 band - consistent with
the A/B and with every exact match this project has already banked under
SP3.

Consequences:

- **The stale-generation hypothesis is dead for these walls.** The
  merged-return / tail-duplication class (ValidAttack's cross-jumped
  inline tails, AppWndProc, cast_enchantment's duplicated epilogues,
  VideoClose's tail-duplicated top test), the callee-saved register-swap
  signature (CenterWindow, button::Main, TPickANumber, iconwdgt), and the
  one-instruction generation-class residuals (DoCompAI, StartMP3,
  VideoRealignBuffers) are all invariant under the C2 generation swap.
  They are real model gaps for the vc6 area's inliner/allocator phases,
  or front-end differences - not back-end vintage.
- **Residual comments citing "CL generation" should be re-worded** (a
  deliberate edit, not part of this track): the measured fact is now
  "invariant under C2 8168/8447", which is stronger and narrower than
  "generation-capped".
- **The one unexplored generation lever is the front end**: retail's 26
  RTM C++ objects were made by an 8168 *front* end too, and this A/B fed
  the RTM C2 only SP3-C1XX IL. C1XX.DLL 12.00.8168 is already staged and
  hash-recorded; a C1XX+C2 overlay is a 20-line variant of `build-rtm` if
  a future wall warrants it. (Also noted for `_flow.diagnose`: its
  SIGNEDNESS classification calls a jb/jl twin flip "nearly always a real
  source-type bug" - the sentinels prove a generation can flip it too.)
- The 26-object RTM band remains unattributed by name. If a future TU
  plateaus with a jb-vs-jl twin flip or an un-threaded back edge as its
  *only* residual, that TU is an RTM-band candidate: re-run
  `genab run --fn <fn>` and expect `RTM-closes` - the harness is standing
  and the verdict enum already covers it.

## 5. Pinned-input experiment record

> **[DATE] RTM C2 12.00.8168 admitted as a hash-pinned A/B-only input
> (Track R).** Sourced from archive.org
> (`1998-10-01-visual-studio-6.0-enterprise-edition-disc-1` /
> `VSE600EUN1.ISO` / `VC98/BIN/C2.DLL`; cross-verified byte-identical
> against `vs-6.0-enterprise` / `VS6.0Enterprise_Disk1.iso`), staged
> outside the repo at `../orig/vc6-rtm/C2.DLL`: sha256
> `45187b0b6288240f73272a7c61e6329c50048a76e57db3bb87b6f0229e09e27d`,
> 737,329 B, FileVersion 12.00.8168.0, exports exactly
> `{_InvokeCompilerPass@12, _AbortCompilerPass@4}`. Scope: A/B oracle
> compiles through the `homm3.vc6.genab` overlay ONLY - the pinned SP3
> toolchain remains the sole matching/ratchet verdict, the RTM DLL is
> never a build input, and `genab` hard-aborts on any hash/version/export
> mismatch. Sibling RTM binaries (C1.DLL, C1XX.DLL - hashes in
> `docs/vc6/rtm-generation.md` §2) are staged and recorded but NOT
> admitted; a front-end A/B would be a separate decision. Result already
> banked from the first corpus run (2026-08-09): the C2 generation swap is
> codegen-invariant on all 18 wall/control functions
> (`evidence/vc6/c2-generation-verdicts.tsv`, `sp3_vs_rtm == 0`), so the
> merged-return / tail-duplication and register-swap walls are NOT
> RTM-generation artifacts; the only generation-sensitive functions found
> (town::GiveSpells, town::initialize_spells, kbwin SetMenus - a jb/jl
> loop-guard twin plus one back-edge threading) side with SP3 against
> retail, placing those TUs in the Rich header's 8447 band.

## 6. Files

| path | role |
|---|---|
| `scripts/homm3/vc6/genab.py` | the A/B harness (overlay + corpus + verdicts) |
| `evidence/vc6/c2-generation-verdicts.tsv` | generated verdicts (regenerate, never hand-edit) |
| `../orig/vc6-rtm/{C2,C1,C1XX}.DLL, CL.EXE` | staged RTM binaries (outside every repo) |
| `../orig/vc6-rtm/PROVENANCE.txt` | staging-side provenance record |
| `build/vc6/toolchain-rtm/` | the RTM overlay (gitignored) |
| `build/vc6/genab/{sp3,rtm}/` | cached per-unit A/B objects (gitignored) |
