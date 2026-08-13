# Gruntz field note: applying `wall-identifier` outside HOMM3

Date: 2026-08-09

## Scope

The `wall-identifier` doctrine was applied to Gruntz's reconstructed
`TmDeflectStep` while it was plateaued below exact match under pinned MSVC 5.0
SP3. This was a methodology test, not a direct invocation of
`homm3 vc6 diagnose`: the command is coupled to HOMM3's unit manifest, compiler
profile, objdiff layout and symbol naming. The Markdown taxonomy and register
model were portable enough to apply manually with Gruntz's native tools.

No HOMM3 implementation or source was copied into Gruntz.

## Starting evidence

The retained Gruntz candidate scored 99.264100% in the raw batch scorer
(`21060` candidate bytes versus `21031` retail bytes, with `728` versus `720`
ordered relocations). Its `0x2a8` frame agreed with retail.

Gruntz's branch comparator reported the same ordered 556 conditional branches
and 96 returns. That initially made the remaining adjacent instruction swaps
look like a register-homing or scheduler wall.

Before using the HOMM3 guidance, several local families had already been
exhausted without improvement:

- ten real `Coord` declaration-scope arrangements compiled byte-identically;
- six complete declaration layouts (grouped, interleaved, reversed and paired)
  compiled byte-identically;
- a 256-cell Cartesian matrix varied the persistent flag/coordinate types and
  the candidate-cell, side-X and side-Y types. It produced nine byte clusters,
  but its best sixteen variants were all the unchanged 99.264100% candidate.
  Candidate-cell and side-X `i32`/`u32`/`long` spellings were neutral; changing
  the persistent scalars or side-Y type only made the result worse.

This is useful negative evidence for the register model: these spelling and
scope changes were optimized away or copy-propagated and could not change the
relevant pseudo creation order.

## Taxonomy result

The skill's ordering—inline first, then control-flow, then registers—made the
classification explicit. After an authoritative single-job rebuild,
section-specific COFF relocation counts exposed this call multiset:

| Referent | Gruntz base | Retail |
|---|---:|---:|
| `Coord::Set` | 75 | 75 |
| `CMapMgr::CellFlagsAt` | 75 | 75 |
| `CGruntzMgr::GetTileGrid` | 75 | 75 |
| `TmFlagsAllow` | 37 | 37 |
| `CGrunt::EntrancePx` | 6 | 6 |
| `g_gameReg` | 163 | 163 |

All helper-call and global-reference counts agree. The ordered relocation
streams have a 719/720 LCS; their non-helper residue is the switch table's
self-versus-local-label identity and eight candidate table-tail relocations.
Together with the exact branch/return sequence and equal frame, this routes the
case to **register/scheduler analysis**, not an inline-boundary wall.

An earlier read appeared to show `77/75` `Set` and `CellFlagsAt`, `76/75`
`GetTileGrid`, and `164/166` `g_gameReg`. That read used the raw object left by
the final temporary scalar trial after the source-restoring batch completed.
The normalized report and raw base object were temporarily out of sync. A
single-job build restored the authoritative pair and disproved the inliner
diagnosis. Wall classification therefore needs a clean-build provenance check,
not merely a plausible-looking object path.

The raw block skeleton looked much worse after the first size-changing join,
whereas the ordered branch sequence still agreed. This reinforced a second
lesson: block-number alignment after a local size drift is not a substitute for
the ordered branch/return comparison.

## What transferred well

- The doctrine order prevented more blind register and declaration sweeps.
- A clean section-local relocation multiset cheaply ruled out an inliner wall;
  the stale-object negative control exposed the required provenance check.
- The register model explained why type aliases and declaration reshuffles can
  be completely neutral when values are copy-propagated.
- Keeping multiple distinct near-match families remained useful: the current
  short-join candidate, an exact-length reference-alias candidate, and an
  explicit branch-feed candidate preserve different compiler decisions for a
  later beam instead of forcing a greedy single winner.

## What did not transfer directly

`homm3 vc6 diagnose`, `predict-inline` and `why-reg --model` cannot consume a
Gruntz `unit:function` today. Their loaders assume HOMM3 paths, manifests,
compiler flags and report schemas. The underlying classifiers are portable;
the command adapter is not.

A useful port would add a Gruntz input adapter that supplies:

1. the base and delinked-target COFF section for one function;
2. the unit's exact VC5 flags and include closure;
3. ordered call-relocation, branch/return, frame and aligned-register features;
4. the existing Gruntz Cartesian runner as the confirmation step.

That adapter should preserve the skill's output contract: classify first,
name the controlling lever, propose several ranked candidates, and let the
pinned compiler settle each hypothesis.
