# Disposable TU-state sweep

VC6 can emit different code for an unchanged function when the surrounding
translation unit changes. `homm3 vc6 state-sweep` compiles disposable source
copies to measure that effect. It never inserts probes into authored C++.

The default `includes` family keeps the original behavior: each trial shuffles
five to ten project headers absent from the TU's include closure and inserts
them after the initial directives. Gruntz's broader parser-state families are
also available: `forest`, `typedef`, `typedef-count`, `enum`, `struct`, `class`,
`packed`, `member`, `extern`, `static-data`, `prototype`, `function`, and `mixed`.
`forest` shuffles typedefs, classes, prototypes and static function definitions;
`mixed` combines all declaration kinds. `typedef-count` walks one additional
typedef per trial, which can expose a handle-count phase missed by random
forests.

```sh
homm3 vc6 state-sweep --unit army --fn '?doAttack@army@@QAEEPAV1@H@Z' \
  --families forest,enum,mixed --insertion both \
  --trials 96 --jobs 8 --bank
```

`--fn` selects one `MAX < HIST` function as a nearby insertion point. Every
configured function in its TU is scored and eligible for banking. `--insertion top`
places declarations after the initial directive block; `target` places them
beside the selected function; `both` uses independent declarations at both
sites. The default is `top` for the original `includes` family and `target`
for declaration families. Mixed family lists always place project includes at
the top. `--seed` and `--max-declarations` control reproducible searches.

Each trial records its family, placement, exact generated declarations, header
list, and function scores under ignored `build/tu-state-sweep/`. The cache key
includes the source, target object, headers, compiler, scoring code, family,
insertion, seed, and trial count. The summary records score increases and
decreases. Incompatible variants are retained as failed trials.

Banking fails closed:

- each prospective improvement is recompiled and must reproduce to four decimals;
- authored source and `config/match_baseline.tsv` must not change during the sweep;
- headers, compiler/profile, retail targets and scoring code must stay unchanged;
- a row with a function hash must still have that exact live hash;
- CUR remains the clean canonical-build score;
- only MAX is raised, with HIST raised too for a new all-time peak.

These probes establish reachability in a compiler state, not recovered source.
An authentic source change still requires the ordinary VC6 build and retail
comparison. Historical exact observations may remain out of reach in today's
source and header population; see [historical match evidence](../reconstruction/historical-matches.md).
