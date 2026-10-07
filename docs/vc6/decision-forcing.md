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
member function `0x19b5`) contains it, so only allocations C2 itself
considered legal are produced. Every decision is logged as
`color k= chosen= eligible= priority=`.

`reg-reach` is a greedy search. It walks the decisions in order, tries every
other eligible register with the earlier choices fixed, keeps an improvement
and repeats once. With `--with-inline` the inline rules from `reach` are
applied first. Local scratch selection (`0x33273`, the rotating EAX/ECX/EDX
cursor) is not forced.

## Limits

* The comparison is instruction-level and strict, not the objdiff
  percentage. A strict match is required for an "exact"/"reached" verdict.
* Retail call streams only reveal kept calls. Two decisions with identical
  call effects are distinguished by the replay, not by the rule derivation.
* Stage D (CFG tail merging / cross-jumping) is not hooked yet. Those walls
  classify as "not inline" and, unless `reg-reach` reaches them, as
  "not global-register state".
