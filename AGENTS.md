# HoMM3 Decomp Project Guide

Binary-matching decompilation of Heroes of Might and Magic III Complete. The goal is C++
that reproduces the retail MSVC 6.0 object code.

## Ground truth

- The retail executable is authoritative for code, data, and addresses:
  **English GOG Complete 4.0 (engine 3.2)**, `HEROES3.EXE`, 2,732,032 bytes, SHA-256
  `057c9d88e7206f6669a4615de2c6e02ab6c4e2d570a9e2badf07fe0bd6247274`, fixed base
  `0x00400000`, no `.reloc` directory, no CodeView stream.
- Evidence sources ranked below retail bytes:
  - **Dreamcast CodeView dump** (`../homm3-symbols/HoMM3-Dreamcast-Dump`): proves names,
    types, and layouts for the *Dreamcast* build. Cross-architecture — an x86 identity
    still requires retail-byte proof. The dump's own executable is at
    `../orig/dreamcast/H3.EXE` (WinCE SH4 PE, 8,425,752 B, sha256 `cdbc7e75…`, from two
    byte-identical GD-ROM rips; the embedded NB11 CodeView stream is what the dump
    prints), referenced via `$HOMM3_DC_EXE` — its own xref graph lives in
    `evidence/dc-xref-graph.tsv`.
  - **NH3API** (`../homm3-symbols/NH3API`): external and **contradicted on addresses**.
    Of its 874 embedded wrapper addresses, only **120 land on a carved function entry**;
    **751 fall inside a function body** — many mid-instruction (e.g. `0x5d70` is the last
    byte of the `cmp ecx,8` at `0x5d6e`) — and 3 are unowned (measured 2026-08-04 against
    `config/retail-functions.tsv`). They describe a different address space (HD Mod), so
    an NH3API address is NOT evidence for a boundary on this image. Its *names* and
    signatures remain useful once an address is independently proven.
    Correction: an earlier note here claimed "873 land on x86 entry patterns". That
    misread attempt-1's scorer — its `exec` column (873) only means the address lies in
    an executable section; its prologue-like column was 100. Do not resurrect the claim.
  - **decomp-attempt-1** (`../decomp-attempt-1`): the abandoned first attempt and a
    secondary evidence source. Its admitted inventories are now hand-owned here; its
    remaining artifacts may be consulted as hypotheses but never outrank retail bytes.
- Local retail copies live outside every repo at `../orig/` (safekeeping;
  hash-verify before use). The repo never contains game bytes; `build/` is gitignored.

## Primary matching instrument: Dreamcast source shape

**`homm3 dreamcast` is the campaign's most important reconstruction tool.** For every
non-exact game function with a Dreamcast counterpart, run the Dreamcast evidence pass
before speculative C++ rewrites. It recovers source facts that retail VC6 `/O2 /Ob2`
erased and that x86 disassembly alone cannot recover:

- original helper boundaries and names;
- signatures, parameter types, and a lower-bound local-variable inventory;
- lexical scopes and likely local lifetimes;
- statement order and grouping, with per-statement calls and branch counts;
- accessor calls that retail inlined into anonymous loads or tests;
- constructor, destructor, and RAII boundaries folded out of retail; and
- separation between shared RoE-era code, WinCE-specific code, and later Complete edits.

The mandatory first-pass workflow is:

```sh
homm3 dreamcast show 0x00524dd0
homm3 dreamcast asm 0x00524dd0 --blocks
homm3 dreamcast inline-clues 0x00524dd0
homm3 sema diff 0x00524dd0 --summary
homm3 sema diff 0x00524dd0 --structure
homm3 sema diff 0x00524dd0 --source
```

For a very large dispatcher, scope the view to the current arm instead of
reading hundreds of unrelated blocks. Ranges are function-local and end
exclusive; give each side its own range when earlier codegen shifted the arm:

```sh
homm3 sema diff 0x0059fe30 --base-range +0xb90:+0xc60 \
  --target-range +0xc00:+0xcdc --structure
```

`show` is the compact function dossier. `asm --blocks` labels SH4 assembly with
CodeView `bp` line/breakpoint boundaries, lexical `scope` boundaries, and inferred
basic blocks (`B0`, `B1`, ...). It is the detailed view for understanding the older
source statement groups before forming a retail hypothesis. Selectors may also be exact or
unambiguous names, `module.obj:0xOFF`, or `dc:0xOFF`; use `homm3 dreamcast find NAME`
to locate a function and `homm3 dreamcast stats` to audit corpus coverage.
`inline-clues` reports only positive Dreamcast compiler residue: another source file's
line rows inside the procedure, or a same-file jump into a named earlier helper. Treat
the recovered helper boundary, nested scopes, and surviving inlinee locals as source
facts when retail agrees. NB11 has no explicit inline-site record, so an empty report
never proves that the helper remained out of line; a separately emitted helper body also
does not disprove an inline copy.
`homm3 sema diff <selector> --structure` is the explicit candidate-vs-retail CFG
checkpoint: it reports block flow and size differences without pretending that
cross-architecture Dreamcast instruction counts should match.

Dreamcast NB11 extraction populates the compiler-neutral `homm3.debug-shape.v1` IR in
`scripts/homm3/analysis/debug_shape.py`: function extents, parameters/locals, lexical
scopes, source and zero-emission rows, emitted sizes, branch counts, and call sites. The
IR is reusable by other debug-format parsers and skeleton/dossier generators. It has no
comparison policy and contains no candidate or retail fields. Never compare this SH4
shape to candidate VC6 `/Z7` shape. `/Z7` is used only to label which candidate C++
statement emitted a region in the candidate-vs-retail x86 diff.

The campaign loop is **Dreamcast dossier -> retail source-labelled diff -> C++
hypothesis -> VC6 retail checkpoint**. Start with signatures, locals, scopes, helper calls,
and statement groups named by the dossier. Do not start with blind source permutation
when this evidence is available. If a hypothesis changes a claim or decorated signature,
run `homm3 build`, `homm3 delink`, then `homm3 build` again. Objdiff percentages are
checkpoints, not admissibility invariants: restoring a coherent Dreamcast-proven class,
helper, scope, or statement shape may temporarily lower several local scores before the
surrounding source reaches retail's lowering. Do not remove a positive Dreamcast fact or
invent "retail skew" merely because an isolated rewrite scores lower. Reject Dreamcast
shape only when retail bytes directly contradict its semantics, ABI, layout, or CFG—not
when the current candidate has a lower similarity percentage. Preserve prior score
peaks as history, continue coherent reconstruction through expected dips, and record
useful negative codegen classifications so the next matcher does not repeat an exhausted
sweep. A current-score dip in an unrelated function is observational and must never fail
the build while that function's max/history checkpoint remains banked. A wide header
blast radius is not a reason to avoid an evidence-backed class or
interface correction: max/history exists to bank the including-TU dips while the coherent
header state is carried forward. Measure and document that collateral, but do not restore
a source-false declaration merely to recover a current exact count.
`homm3 vc6 queue` is admission-first: it ranks functions that have no diffable compiled
body by retail size, largest first. Do not polish already admitted functions until this
queue is empty. The deferred `homm3 vc6 queue --polish` campaign sorts admitted unfinished
work by ascending effective MAX and excludes every banked-exact current dip.

This remains a cross-architecture, older-revision source oracle, **not a second byte
target**. A missing Dreamcast statement or call never proves retail lacks it, and
Dreamcast source shape must be rejected when Complete's x86 bytes disagree. Retail is
always the verdict. See `docs/dc-line-tables.md` for interpretation and
`docs/dreamcast-proof-40.md` for the first bounded proof: four exact cohort closures,
one exact spillover, and one signature-driven near-closure from 40 non-exact functions.

Do not build a regex roster or automated structure comparator between Dreamcast and our
source. Positive Dreamcast facts are reconstruction evidence to record beside the source
and test against retail semantics/bytes; they are not equality constraints on candidate
instruction, block, branch, call, statement, local, or scope counts. Architecture,
compiler, and revision differences make those counts incomparable.

## Toolchain

- Compiler: VC6 SP3 `CL.EXE` under Wine; linker: VC6 `LINK.EXE` 6.00.8447 (the
  generation that built retail). `homm3 init` fetches the toolchain tarball from the
  pinned `toolchain-vc6-sp3` GitHub release via `gh` and verifies its SHA-256.
- The Wine prefix is **stateless**: INCLUDE passes per invocation, `cl.exe` is invoked by
  absolute path, libraries pass at link time. No registry PATH/INCLUDE/LIB writes exist
  to go stale (deliberate divergence from the Gruntz template).
- No MSDIS stub is needed for VC6 linking (unlike Gruntz's VC5): LINK 8447 statically
  imports only mspdb60/msvcrt/kernel32.
- zlib TUs compile with `/O2 /ML /Gr /TC /D_WINDOWS`. Game-TU profiles arrive with the
  first game unit.
- The Wine VC6 build is the sole verdict on a match; clang/clangd is editor tooling only.

## Build

```sh
nix develop .#build       # from the repo root
homm3 init                # one-time: toolchain + wine prefix + smoke compile
homm3 build               # configure + ninja (all manifest units)
homm3 link                # opt-in candidate link: /FORCE /NODEFAULTLIB /MAP,
                          #   .map layout study + unresolved-externals punch list
homm3 clean               # whole-build/ nuke; init restores everything
```

`ninja` alone works for rapid iteration once configured. `homm3 build` never invokes the
delinker (that step is not yet ported).

## Source-labelled matching

Use the candidate's VC6 debug lines to localize a mismatch to the C++ statement that
lowered into it:

```sh
homm3 sema disasm 0x00554400 --base --source
homm3 sema diff 0x00554400 --source
```

`--source` compiles a separate cached `/Z7` object under `build/debug/`; it never adds
debug flags to the matching object. The tooling verifies that the selected function's
code bytes are identical before applying the line offsets. In the diff view, assembly is
aligned first and source headings are attached afterwards, so comments cannot change the
verdict. Use the first `!!` statement to choose where to inspect or respell the source.

These labels describe the **candidate build only**. They are navigation evidence about
which candidate statement produced an instruction span, not evidence for retail source
semantics, and they never outrank the retail bytes or objdiff score.

## Inline-boundary matching

Treat the source declaration and the retail lowering as separate facts. A Dreamcast-
proved `inline` helper remains `inline` even when one Complete caller emits an out-of-
line call. Confirm the mismatch with
`homm3 vc6 predict-inline src/<unit>.cpp --fn <selector>` and inspect the named call
sequence, not only the aggregate call count. For a manifest-owned source, the command
infers the retail unit; use `--against` only to override that reference.

Dreamcast may expose both a standalone procedure and expansions of that same helper in
callers. Preserve that as one canonical source helper: match its retained retail body
when retail has one, and match every retail caller's call/expansion decision separately.
An exact standalone body does not excuse a wrong caller boundary, and an exact caller
does not permit deleting or hand-flattening the helper. Dreamcast proves the source
relationship; Complete's x86 bytes remain authoritative for whether each site calls or
expands it.

The converse is required too. When Dreamcast retains a call to an ordinary non-`inline`
helper but Complete VC6 `/Ob2` expands it, preserve the source call and the ordinary
helper declaration/definition, then reproduce the retail auto-inline decision by making
the real body available in the original TU and source order. Do not paste the helper's
body into the caller or add a false `inline` keyword. `advManager::MoveHero` is the
canonical control: Dreamcast calls `GetMoveShowIt` at `cursor.cpp:601`, while Complete
expands that same source helper. Dreamcast call/expansion residue ratchets the source
boundary; retail x86 independently ratchets the per-site lowering.

`#pragma inline_depth(0)` may be used locally as a diagnostic: if suppressing one
expansion improves the retail structure, that identifies missing natural compiler state.
It is not reconstructed source. `INLINE_GATE` is likewise a no-op artifact that the
original developers did not write. Do not add or commit either construct. Existing
inline-depth pins are historical debt and the cleanliness ratchet permits only their
removal. Replace them by recovering the real declaration, body visibility, source order,
local lifetime, release-elided operation, or original TU/PCH state that made VC6 choose
the retail boundary naturally. Do not move or remove an `inline` declaration, create a
second source-false declaration, or add synthetic caller mass merely to steer VC6's
budget. Use `#pragma auto_inline(off)` only as an uncommitted diagnostic when every
affected call site can be inspected.

A Dreamcast line gap may be evidence of a release-elided `VERIFY`, `ASSERT`, or `TRACE`,
but does not prove one by itself. When a meaningful recovered invariant is also plausible,
spell its release form as `HOMM3_RELEASE_VERIFY(expression)`. The expression must encode
that invariant; self-assignments, unreachable branches, dummy helper calls, and repeated
budget-only carrier doses are forbidden. Record the line-table and codegen evidence beside
every retained carrier.

Every temporary inline-depth experiment or retained release-VERIFY carrier must carry a
source comment naming the caller, callee, and retail/Dreamcast evidence, plus a negative
control that proves flattening or de-inlining fails. An inline-depth experiment must be
removed before commit; record any reusable compiler finding under `docs/vc6/`. Bank
percentage peaks in
max/history; an unrelated current-score dip is not permission to undo a proven helper
boundary.

`homm3 status check` fingerprints each VA-owned function's own source definition. It reports
a score regression only when that source hash changed and its score fell from the preceding
current checkpoint. An unchanged function is never reported as a drop, even when header/TU
optimizer state or a delink-generation change moves its current score below MAX. A changed
function that improves while still below MAX is also not a drop. Unattributable functions
are unknown, not edited. The banked MAX remains monotone and is the later polish frontier
used by `homm3 vc6 queue --polish`; it does not displace the current admission-first queue.

## Tooling layout

One importable package (`scripts/homm3/`), one CLI (`homm3`), grouped by role:
`core/` (shared primitives — `cc_wrap`, `common`, `image`), `build/` (ninja-graph
actors and the delink loop), `match/` (status/ratchet + the function-universe
classifier), `init/` (toolchain/prefix setup). Retired tools live in
`scripts/archive/` (the carve bootstrap pipeline is there — do not resurrect).
Later phases add `ghidra/`, `sema/`, `audit/` per the port plan. Add a tool to its
role package, not to a new top-level file.

## Repository model (contracts; enforcement lands with each phase)

- **Source is the authority for names** once game TUs land: annotation macros in source,
  a generated label map re-derived every build, authority-checked against symbol tables.
  No hand-maintained symbol ledger. (Annotation contract = open decision P0.2.)
- **`config/` vs `evidence/`**: `config/` holds only hand-admitted retail inventories
  (functions, relocs, vtables — MANUALLY MANAGED after admission) and build manifests;
  `evidence/` holds GENERATED analysis deliverables (DNA bands, name maps, joins —
  regenerate, never hand-edit).
- **Vendored sources stay pristine**: no macros in `vendor/`; zlib's rva→symbol map is
  the one reviewed table `config/retail-zlib-map.tsv`.
- **Gates must be able to fail**: every future fatal gate ships with a negative control
  proving it still detects its defect.
- The delinker is vostok, pinned in the flake at the upstream stacked-queue head; it
  recovers exact code relocations for stripped PEs directly from a synthesized PDB —
  exactly what this `.reloc`-less target needs.

## References

- Function-specific failed probes belong beside the affected source function;
  reusable compiler-wide findings belong under `docs/vc6/`. Do not create a
  second chronological decision log.
- `~/Projects/gruntz` — the architecture template (pipeline, gates, conventions).
- `~/Projects/homm2/homm2-decomp` (branch `decomp-pol-2.0`) — the mature sibling
  project; its README/CLAUDE/AGENTS are the style reference for this repo.
- `../decomp-attempt-1/docs/` — target provenance (`target.md`), coverage and layout
  evidence from the first attempt.
- `AGENTS.md` — durable agent policy for this repo.
