# Decision forcing: let C2 enumerate its own alternatives

The source is never changed. The unit's front end runs once (`/d1il`). Every
back-end run then replays fresh copies of the same four IL streams (`/d2il`),
so declarations, TU state, handle numbering and every C1 cost estimate stay
fixed. The inline-trace shim ([shim.md](shim.md)) overrides one chosen C2
decision at a time and keeps all others as C2 makes them. A forced object
shows what the *compiler* emits when that one decision goes the other way.
It is not a candidate for the matching ledger, and nothing it produces is
banked.

The point of forcing is classification. If forcing only C2-legal decisions
reproduces retail, the wall is *compiler state*, not missing statements.
The forced decisions then state the source target exactly: which site needs
which budget, or which live range needs which register.

## Commands

```sh
homm3 vc6 reach 0x5f7500            # force retail's observable inline decisions
homm3 vc6 force 0x5f7500 --rule '?drawHero@,?getMap@,5,E'
homm3 vc6 reach-all walls.tsv --jobs 6   # batch; first TSV column = retail VA
homm3 vc6 reg-reach 0x4a5610 [--with-inline]   # global register choices
homm3 vc6 reg-reach-all walls.tsv --jobs 8 --max-trials 100
homm3 vc6 merge-reach 0x4805e0                 # tail-merge vetoes
homm3 vc6 reg-reach-all walls.tsv --mode merge --jobs 8
homm3 vc6 cover a.cpp b.cpp                    # locate a C2 pass by coverage
```

Outputs go to `build/vc6/force/<unit>/<hash>/` (`reach.json`,
`register-reach.json`, `rules.tsv`, `sites.log`) and
`build/vc6/force/reach-all.{jsonl,md}`.

## Inertness

Every `force`, `reach` and `reg-reach` run first replays the function's
captured IL twice in the same workdir: once through the unmodified compiler,
once through the forcing shim with an empty rule list. The two whole objects
must agree outside the four COFF timestamp bytes. Otherwise the run stops
before reporting anything. The same output path matters because `/Z7`
records the object path in the object. Both replays use one capture, so
anonymous-namespace salts also match.

## Scope

The inline budget is a source fact: `caller_cb` and every callee's IL
cost come from the C++. Most later decisions (register allocation, block
layout, tail merging) follow from inlining. So stage A is the diagnostic
to use: force retail's observable inline decisions and see whether the
function then matches. If it does, the wall is a source problem, and the
reported per-site `need` is the delta to find in native evidence. Stages C
and D below are retained but secondary. Their batch results (below) show
they rarely explain a wall on their own.

### Stage A split over the stable walls (2026-10-07)

`reach-all` over the 242-row wall list (rmg/zlib excluded):

| verdict | walls |
| --- | ---: |
| exact under the strict compare | 4 |
| inline-state: retail reached by forcing | 5 |
| inline-state: shape reached (stack/immediate residue) | 1 |
| inline-state partial: forcing moves toward retail | 23 |
| not inline: call decisions already agree | 182 |
| not inline: no inline candidate sites | 19 |
| not inline: forcing does not move toward retail | 1 |
| replay error (customcampaign/game helpers, unresolved) | 7 |

The six reached walls are:

* `advManager::drawHeroPart` 0x40fe30, `vwDrawHeroPart` 0x5f7500 and
  `vwDrawHeroPartShadow` 0x5f7900: one site each, the fifth getMap, +16.
* `displayLCWinLoss` 0x4f2960: the LossConditionStruct constructor at two
  sites.
* `NewfullMap::readBlackBox` 0x4ff6b0: the spell resize's
  erase -> _Destroy, +26 at depth 3.
* `CTownDlg::createWin` 0x575e60: a nested size().

Twenty-nine walls are therefore inline-explained, in whole or in part.

## Inline forcing (stage A)

Hook: C2 RVA `0x19f8c`, the budget comparison inside the recursive
expander `0x199fa` ([inliner.md](inliner.md) §2). It replays the stolen
`mov ax,[edi+0x6d]; mov esi,[esp+0x48]`, then either:

| action | continuation | meaning |
| --- | --- | --- |
| none | `0x19f94` (original) | C2's own budget test |
| `E` | `0x19faf` | admitted: the cb > 40 charge (`0x19bac`), running total and the nested `budget / sites-remaining` expansion all follow |
| `K` | `0x19a94` | rejected: C2's own refusal path (C4710/C4714 bookkeeping) |

Sites that never reach `0x19f8c` cannot be forced. These are arity
mismatches (`0x19f63`), `inline_depth` (`0x19f7c`) and forceinline callees
(`0x19f87`). A post-substitution veto at `0x94964` can still revert an
admitted copy.

A rule is `owner<TAB>callee<TAB>occurrence<TAB>E|K` in
`HOMM3_VC6_INLINE_FORCE`. Owner and callee are substrings of the compiler
names (`*` matches any). The occurrence is the 1-based count of matching
budget comparisons within the selected root, and 0 means every match.

`reach` starts with no rules. It aligns our call stream with retail's
(`inline_model.ordered_divergence`, which pairs synthetic retail labels):

* each surplus call becomes an `E` on the earliest kept site of that callee;
* each missing call becomes a `K` on the latest admitted site.

It then replays and repeats until the strict instruction stream equals
retail's, or no rule changes anything. *Strict* means registers, stack
displacements and immediates are kept. Only absolute addresses, branch and
call targets, and switch-table addends are masked.

Each forced site's `need` line is the inversion: `budget +N` at that site.
The depth-1 budget is `clamp(2*caller_cb,1000,35000)` less earlier charges,
and a depth-n budget is the parent remainder divided by the sites ahead. That
is the source target, to be met with evidenced source (helper boundaries,
caller mass, callee bodies; see inliner.md). Forcing is not a fix.

### First result

`advManager::vwDrawHeroPart` (0x5f7500, 95.31%) and `vwDrawHeroPartShadow`
(0x5f7900) each reach retail **strictly** with one rule: expand the fifth
`getMap` inside the fifth `drawHero` expansion. Its budget is 29 against
cost 45, so the source target is +16 at the last `drawHero` site. That
equals 4 IL units per preceding accessor group, or +8 caller cb, with every
other decision unchanged. No register or control-flow residue remains.

## Register forcing (stage C)

Hook: C2 RVA `0x24748`, the end of the global coloring choice `0x245c3`
([regalloc.md](regalloc.md) §3b). EDI holds the chosen register and EBX
the live-range group. The stolen `lea eax,[edi*8]; sub eax,edi` is replayed.
`HOMM3_VC6_REG_FORCE="k:reg,..."` replaces the k-th decision of the selected
function, in C2's priority order (1=EAX ... 8=EDI). The replacement is used
only if the group's own candidate set (`group+0x20`, tested with C2's
member function `0x19b5`) contains it. That only keeps the register
encodable for the group. It does not make the result reachable: C2 rejected
the alternative, so no real source context need produce it. Every decision is logged as
`color k= chosen= eligible= priority=`.

`reg-reach` is a greedy search. It walks the decisions in order, tries every
other eligible register with the earlier choices fixed, keeps an improvement
and repeats once. With `--with-inline` the inline rules from `reach` are
applied first. Local scratch selection (`0x33273`, the rotating EAX/ECX/EDX
cursor) is not forced.

## Tail-merge forcing (stage D)

Located with `homm3 vc6 cover`. The shim plants an INT3 on each of the
2475 atlas function entries. A vectored handler counts each hit, restores
the byte, single-steps and re-arms, so one compile yields a per-function
hit table. Three scratch TUs isolated the mergers: four
`g(n); h(b); return R` arms with equal R (A), with all R different (B,
no merge) and with the last two equal (C). Entries hit only when a merge
exists are `0x3dea7` (re-links the matched tail), `0x37042`, `0x36fd7`
(deletes the duplicate) and `0x29a5` (adds the edge). Their common callers
are the two tail mergers:

| merger | called from | matched-count test | merge path | no-merge path |
| --- | --- | --- | --- | --- |
| `0x36aa0` (kind 1) | `0x3490e` | `[esp+0x10]` at `0x36afa` | `0x36b49` | `0x36b02` |
| `0x3e30b` (kind 2) | `0x367aa` | `[esp+0x18]` at `0x3e3e0` | `0x3e4a1` | `0x3e3f2` |

Both walk the two blocks' tails backwards (`0x36877` compares two
instructions) and merge when the matched count is nonzero. Kind 2 also has
a size heuristic after `0x3e4a1`. The hooks replay
`mov eax,[esp+n]; test eax,eax` and return to the original `ja`.
`HOMM3_VC6_MERGE_VETO="k,..."` zeroes the count of the k-th first-time pair
decision in the selected root. C2 retries a declined pair, so a vetoed
pair (EBP/EBX at the test) stays declined for the rest of that root. A
veto can only decline a merge C2 found; it cannot create one.

`merge-reach` greedily vetoes merges while strict similarity improves.
First results:

* `advManager::moveHero` (0x4805e0): one veto, 0.7779 -> 0.7924
  (partial).
* `TMultiPlayerWindow::onWidgetDeselect` (0x50f4e0, 24 merges) and
  `combatManager::processCombatMsg` (0x474d80, 62 merges): no veto helps.
  This matches the processCombatMsg note: there *retail* merges arms that
  we keep, because our arms compare against constants cached in ESI/EDI.
  That is constant-caching state, not a merge decision.

### Batch results for stages C and D (2026-10-07)

`reg-reach-all --mode merge` searched 120 of the 122 walls with a strict
similarity of at least 0.95, plus the inline-state walls, applying each
wall's saved inline rules first. No veto reached retail. Four improved
partially:

* `displayVCWinLoss` 0x4f15e0: 0.9868 -> 0.9927
* `initiateSpell` 0x59ec50: 0.9532 -> 0.9699
* `onSearch` 0x511660: 0.8618 -> 0.8815
* `seedPosition` 0x56b440: 0.9611 -> 0.9652

Of the rest, 86 showed no veto gain and 30 had no merges. `reg-reach` on
`doEventShrine` 0x4a5610 forced two of 27 colorings, 0.8562 -> 0.8746
(partial). Both stages stay available as diagnostics only. The register
batch was not run.

## Limits

* **Forced output is not reachable output.** Every forced decision
  (inline, register, merge veto) overrides a choice the real compiler made.
  A wall "reached by forcing" shows only that those bytes are encodable from
  this IL, not that any source context produces them. A forced inline
  `need` becomes evidence only after a real source change yields that budget.
  The set of assemblies a function can actually produce comes from varying
  the state that earlier code carries into it (handle numbers, the scratch
  register cursor, callee data, other back-end globals), not from forcing.
* The comparison is instruction-level and strict, not the objdiff
  percentage. A strict match is required for an "exact"/"reached" verdict.
* Retail call streams only reveal kept calls. Two decisions with identical
  call effects are distinguished by the replay, not by the rule derivation.
* Tail-merge forcing can only decline merges. Walls where retail merges
  more than we do (the constant-caching family) stay unclassified; the
  constant-to-register caching decision is not hooked.
* `reg-reach` is greedy and capped (`--max-trials`). An unreached wall can
  still need a combination of choices that no single improving step finds.
