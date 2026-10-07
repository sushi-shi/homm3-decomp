# Context variants: which assemblies one body compiles into

A function whose own source never changed can still compile to different
bytes. The ledger records this as CUR below MAX. This page names the
compiler inputs, outside the function's own text, that move those bytes.
`homm3 vc6 variants` sweeps those inputs over values that real contexts
produce. It never overrides a C2 decision; the decision-forcing hooks in
[decision-forcing.md](decision-forcing.md) are a different instrument, and
forced bytes are not reachable output.

## Ground truth

Take the ledger's CUR≠MAX rows that have a recorded MAX commit. Recompile
each one in its MAX-commit tree (O) and in the current tree (H). Both
trees are linked under equal-length paths, so resolved include spellings
cannot differ. Each pair is captured with `/d1il` and replayed with `/d2il`.

* The O replay reproduces the banked MAX-commit object for every pair.
* The H replay reproduces the HEAD object for every pair except one: army,
  whose current tree has moved on.
* 21 functions compile differently in O and H. Those 21 are the set below.

The function's IL segment was then compared between O and H. Records were
found by name in `gl`, with the `ex`/`sy` starts read from the record tail.
Line records (`4f 01|02 <int>`) were normalised, and 4-byte symbol handles
were mapped one-to-one.

| IL comparison | functions |
| --- | ---: |
| identical except handle and line renumbering, order kept | 10 |
| semantic difference in the function's own IL | 11 |

The semantic differences come from referenced declarations, not from the
function's text. Seven insert the same `33 41 00 20 2c 12 00` sequence
after a call, which is the normalisation of a callee that now returns
`bool`. army's member constant changed (`80 f4 00 00 00`). cspriteframe's
callee `CSpriteFrame::clip` changed its parameters from `unsigned char` to
`bool`.

## Channels, with evidence

**Symbol-handle offset: not a channel.** For drawing and philai,
`k` handle-consuming `typedef`s were placed before the function and,
separately, at the top of the unit (k = 0..47). Every compile gave the
same function bytes. Uniform renumbering does not move codegen, which
agrees with `get_simple_attack_effect` in [handle-order.md](handle-order.md).

**C2 carried state: the phase flag `0x9f120`.** A shim hook snapshots
C2's writable sections (`.bssbe`, `.data`, `.databe`) when the selected
function reaches the inliner. The O and H snapshots were diffed, and each
non-pointer dword was transplanted alone into the H replay.

* Across the 21 cases, exactly one dword changes the result.
* drawing's `showCreatureSpellError` becomes the O bytes when `0x9f120 = 1`
  is written.

The per-function driver `0x13615` sets that flag to 1 in phase 1
(`0x136f6`) and to 0 in phase 2 (`0x13916`). It sets it back to 1 in phase
3 (`0x1398e`) only when the function record's `+0x0c` field is zero.
Nothing resets it before the next function's early pass reads it at
`0x5b11` (inside `0x5739`). The previous function's kind therefore leaks
into this one, and both values occur in real units.

**Cost records: the root's own and its callees'.** Each record's IL cost
sits at `sym+0x6d`, and the shim reads it at the budget test. Writing O's
costs into the H replay, at the root's main (`0x1995c`) or at a callee's
first budget test, turns H into O's exact bytes for 5 of the 21:

| case | cost record set to its O value |
| --- | --- |
| customcampaign `SCampaign::completeCurrentMap` | `game::getHero` 41 |
| rmg `placeZoneTreasures`, `placeKeyTentGuard`, `assembleTreasureGroup` | `TRmgMap::getMapItem(int,int)` 41 |
| victorylossconditions `checkForArtifactWin` | `game::getHero` 41 and root 910 |

A cost changes when the source of that callee, or of a declaration the
function uses, changes. In customcampaign and victorylossconditions, the
function's own IL also changed semantically. Those changes alter no byte
once the costs are restored.

**Callee bodies and declaration semantics: not context.** The remaining
15 need a different callee body (a different nested inline sequence), or
a referenced declaration's type, to come back. For example, cspriteframe
needs `clip`'s `bool` parameters reverted, and overview needs its callees'
bodies reverted. These are source changes in dependencies. The variant set
correctly excludes MAX for them.

## Validation on the 21

`homm3 vc6 variants` was run in the current tree (the CUR context), with
`--against` set to each MAX-commit object and 80 replays per function. The
captured context reproduces CUR in all 21. The set also contains the MAX
bytes for 7:

| case | channel value that gives MAX |
| --- | --- |
| drawing `showCreatureSpellError` | phase=1 |
| customcampaign `completeCurrentMap` | root-cost 1052..1056 |
| customcampaign `ScenarioStruct::read` | cost of a `bitset_iterator` comparison or `bitset::reference` conversion = 41 |
| rmg `placeZoneTreasures`, `placeKeyTentGuard`, `assembleTreasureGroup` | cost[`getMapItem(int,int)`] = 41 |
| victorylossconditions `checkForArtifactWin` | root-cost 915..916 |

The other 14 are explained by dependency source:

* In 9, the function's own IL changed semantically: the `bool`
  normalisation after a call, army's member constant, or a changed local
  table.
* In 5, a callee's body changed, and the nested inline call sequence
  differs between O and H: processKeyPress, cspriteframe's `clip`,
  overview, and rmg `prepareJunctionZone`/`tryPlaceAdditionalTown`.

No context channel can bring these back, and the tool reports *source
differs* for them, which is the correct verdict.

Callee flags (`sym+0x73`, the auto-inline and saved-body bits) are the
same in O and H for every candidate in all 21 traces. Flags are not a
channel here.

## Classification of the stable walls (2026-10-07)

`variants-all` was run over the 242-row wall list (rmg/zlib excluded),
with 48 replays per function:

| verdict | walls |
| --- | ---: |
| not reachable by context channels: source differs | 222 |
| body correct: retail is reachable in a real context | 9 |
| exact in the captured context (strict stream; the residue is elsewhere) | 4 |
| not swept: no inliner entry | 7 |

For the reachable walls, the context the source must recreate is:

| wall | context giving retail |
| --- | --- |
| `vwDrawHeroPart` 0x5f7500, `vwDrawHeroPartShadow` 0x5f7900 | root cost >= 657 (captured 649) |
| `advManager::drawHeroPart` 0x40fe30 | root cost >= 719 (captured 707) |
| `displayLCWinLoss` 0x4f2960 | `LossConditionStruct` constructor cost <= 40 |
| `CNewPlayerUpdateProc::finish` 0x5795a0 | `CNetMsg` constructor cost 58 |
| `NewfullMap::readBlackBox` 0x4ff6b0 | spell-vector `size`/`copy`/`_Destroy` cost <= 40 |
| `NewfullMap::save` 0x4fdf40 | `vector<BlackBoxData>::operator[]` cost 41..42 |
| `aiEnterTown` 0x5253d0 | phase=0 (the preceding function's kind) |
| `loadSeerHutTextColumn` 0x56c120 | phase=1 |

The root-cost rows match the inline-forcing deficit derived independently
in [decision-forcing.md](decision-forcing.md): vwDrawHeroPart needs +16
budget, which is +8 root cost.

## The tool

```sh
homm3 vc6 variants 0x4922f0 [--against OLD.obj] [--max-replays 80]
homm3 vc6 variants-all walls.tsv --jobs 8 --max-replays 60
```

It runs as follows:

1. The unit's IL is captured once.
2. A plain replay must equal the shim replay with no channel set
   (inertness).
3. The captured context's trace supplies the root cost, the phase flag at
   entry and every budget test. The sweep then covers:
   * the other phase value;
   * the root cost on a 4-unit grid within ±24, refined by bisection
     wherever neighbouring outputs differ;
   * each callee's cost, at the values where one of its recorded tests
     would decide the other way (the free threshold 40/41 and the site
     budget edge), within ±12 of its cost;
   * pairs of output-moving callee and root values;
   * the other phase combined with every output-moving value.
4. Outputs are deduplicated by relocation-masked function bytes.
5. A variant counts as retail when its strict instruction stream equals
   retail's (`inline_force.strict_stream`).

Verdicts:

* *exact in the captured context*;
* *body correct: retail is reachable in a real context*, which lists the
  channel values, i.e. the context the source must recreate;
* *not reachable by context channels: source differs*.

Outputs go to `build/vc6/variants/<unit>/<hash>/variants.json` and
`build/vc6/variants/variants-all.{jsonl,md}`.

## Limits

* The cost windows (±12 per callee, ±24 root) bracket the measured edits
  (2..6 units). A cost outside them is a larger source change.
* Only the phase flag is swept among the carried state. Single-dword
  transplants found nothing else outside pointer-valued state, and 2 to 5
  transplants per case crashed C2 (table counters), so those dwords stay
  unclassified.
* A cost value is reachable only if some spelling of that callee yields
  it. The callee's other callers move with it, and this tool does not check
  them.
