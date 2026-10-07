# Why a function's bytes change when it hasn't

Summary of the October 2026 VC6 SP3 findings (C1XX 12.00.8472, C2.DLL
12.00.8447) on why the same function body compiles to different bytes after
unrelated edits. The ledger shows this as CUR below MAX. Details, addresses
and evidence live in the linked pages; this page is the map.

## The model: one body, many assemblies

C2 compiles a unit's functions one after another. Some of its state is not
reset between functions, and some of the numbering it receives from C1XX
depends on everything parsed earlier. So a function F compiles to one of
several assemblies, chosen by what precedes it. `homm3 vc6 compile-m <unit>`
lists those assemblies for every function of a unit (1-to-M compilation),
under IL replay. It changes only state that real earlier source produces; it
never generates source and never overrides a C2 decision.

## The three state channels

| Channel | What sets it | Which edits move it | What it changes | Scale | Page |
| --- | --- | --- | --- | --- | --- |
| Callee compile order (callee record `+0x14` bit `0x800`) | C2 sets it when it compiles the callee's own body | Moving functions, or changing which earlier function first uses a helper: a compiler-generated member's body is emitted after its first user | How an inlined body expands. For example, an inlined `~BlackBoxData` keeps per-member EH state stores only while that destructor is uncompiled | ~584 functions in the 64 units measured; 72 of them non-exact | [unstable-state.md](unstable-state.md) §3 |
| Phase flag (`.bssbe 0x9f120`) | The global optimizer's driver `0x13615`; the reader is `0x5739` | Only what is compiled before a unit's first globally optimized function: out-of-line definitions, dynamic initializers, generated destructors | Folding of known-outcome branches (constant tests from inlined helpers), then scheduling and registers | 281 of 8,769 functions (3.2%); 96 game functions; never inlining | [phase-flag.md](phase-flag.md) |
| Declaration offset, period 64 (`.bssbe 0x9d750` hash) | The regalloc setup pass `0x2cac2` hashes constant operands by value % 64 and data-symbol addresses by handle % 64 | Any handle-consuming declaration created before a data symbol the function addresses, including include order | Live-range numbering, then single-use temporary substitution, scheduling and registers | 18 of 9,368 game functions | [handle-period.md](handle-period.md) |

Things that are **not** channels: symbol handles of the function itself, its
parameters, locals, callees or types; callee flags (`sym+0x73`); `.bssbe`
across units (zeroed per TU).

Callee costs (`sym+0x6d`) do change inline decisions, but they move only when
the callee's own source changes, so they count as a related edit, not an
unrelated one.

## What it means for matching

- **Walls.** Of the 242 stable walls, 2 are reachable through these states,
  both via the phase flag (`aiEnterTown` needs 0, `loadSeerHutTextColumn`
  needs 1). 226 need source changes. No wall needs a declaration offset or a
  callee-order prefix.
- **CUR < MAX.** Of 21 ground-truth CUR≠MAX functions, 7 were pure context and
  14 were real dependency changes: the function's IL changed (for example
  inserted bool normalisation), or a callee's body changed.
- **Source-order clues.** A needed phase or callee-order state constrains the
  original file:
  - retail needing phase 1 for our first function means something
    globally optimized came before it. DC `drawing.obj` starts with `$E`
    dynamic initializers before `show_creature_spell_error`.
  - Retail `readBlackBoxData` keeps the EH state stores, so its inline
    destructors were not compiled yet when it was.
- **Concrete targets.** `initializeGameData` matches retail with 13–26 more
  handles (or 38–51 fewer) before `town.h:496`. Find the real declarations in
  the DC and Mac headers; do not pad.
- **Fragile exact functions.** Six are exact only near offset 0, for example
  `CEnterNameEdit::onKillFocus`, which breaks at +3 handles.

## Verification

- Shifted-handle replays equal real compiles for 487 of 487 functions.
- Fuzz verification found no unexplained escapes after each fix: 3,239
  unrelated edits and about 425,000 function checks over 21 units. Each
  escape it found led to a missing channel, which was then added.
- Probes `c13_phase_flag_first_function` and the handle-period probes are in
  `scripts/homm3/vc6/probes`.

## Tools

| Command | Purpose |
| --- | --- |
| `homm3 vc6 state <VA>` | Read the channel values a context gives a function |
| `homm3 vc6 compile-m <unit> [--function VA] [--against OBJ]` | 1-to-M compile of a unit |
| `homm3 vc6 compile-m-walls walls.tsv` | Classify walls by reachability |
| `homm3 vc6 phase-census [units]` | Phase-sensitivity census |
| `homm3 vc6 fuzz-verify <unit> --edits N` | Soundness check for the predicted sets |
| `homm3 vc6 variants`, `reach`, `force` | Earlier tools. Forced decisions are not reachable output ([decision-forcing.md](decision-forcing.md)) |

## Open questions

- Why a compiled callee expands without its EH state stores: the C2 code
  that consults bit `0x800` during inlining is not traced.
- The handle-hash release test and operand filter, and why only 0.19% of
  functions respond.
- Whether include order matters beyond the handle hash, for example through
  emitted constructs in headers or template instantiation points. An analytic
  include-order map is in progress.
- `compile-m` sweeps callee-order prefixes only at the captured phase and
  offset, not as a full product.

## Related findings from the same period

- [Alternate builds](../oracles/alternate-builds.md) (PR #185): Buka is VC6
  SP5. SP5 is no more stable than SP3 at `/O2`. Collateral moves between
  commits: SP3 41, SP5 44 of about 1,580 unchanged-source functions.
- Two-spelling probes: VC6 erases inline-vs-pasted helpers, split-vs-joined
  guards and redundant guards, while CodeWarrior and GCC keep them. The
  `inline` keyword itself does change VC6 `/Ob2` decisions.
  See `.agents/skills/mac-evidence/SKILL.md`.
