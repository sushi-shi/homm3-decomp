# Generated helper references

Replace the manual caller inventory with two generated graphs:

1. Read Mac calls, branch references, loader pointers and TOC references from
   the pinned PEF. Attribute code references to verified function spans and
   join them to ordinary `MAC_ADDRESS` / `VA` source claims. Keep unpaired
   callers and indirect destinations visible.
2. Parse ordinary Windows sources under their compiler profiles with libclang.
   Resolve callees by declaration identity, including overloads, qualifiers,
   operators and constructors. Index callers in both directions. Preserve
   source locations and expressions; never infer identity from a leaf name.
3. Join the graphs into a helper report: retained Mac callers, direct source
   uses, nested source paths, missing uses, extra uses and coverage gaps.
   A nested path is an inlining investigation lead, not automatic proof that
   Mac expanded that helper. Equal counts do not prove source equivalence.
4. Generate the recovery queue from this report. Cache parsed units with
   source/header/compiler/profile invalidation. Editing a helper refreshes
   its caller inventory; workers consume this queue instead of maintaining
   per-site prose inventories.

The report covers exact and unfinished functions alike. Partial TU scans,
parse errors, implicit lifetime gaps and unresolved dispatch stay explicit.
Reviewed inlining evidence is the only accepted explanation for different
helper boundaries. Existing manual findings remain historical evidence until
the generated report accounts for their sites.

No VC6 or CodeWarrior build is needed to generate reference graphs. Focused
tooling tests cover identity, reverse edges, missing callers and stale caches.

## Commands

```sh
homm3 mac helper-audit
homm3 mac helper-audit 0x00412340
homm3 mac helper-audit mac:0:0xc7f1c --json
homm3 mac helper-audit --unit fly
```

Use an actual Windows VA or Mac offset for the helper being investigated.
The complete run writes `build/mac/helper-audit.json`,
`helper-audit-calls.tsv` (Mac sites), and `helper-audit-source.tsv` (source uses).
The JSON holds both directions, expressions, exact declaration identities,
source locations, paths and diagnostics. A selector displays that function's
callers, callees and source uses; JSON also exposes loader/TOC references and
unpaired Mac callers. `--unit` writes separate `helper-audit-partial*` files and
cannot establish complete cross-TU propagation. Repeated `--unit` selects several
units. `--fresh` bypasses the dependency-checked parse cache.

After restoring a helper, rerun the full audit and query its identity. Work from
its generated caller comparisons and source comparisons. Review every unmatched
use and every proposed nested path against Mac assembly. There is no manual
caller-list manifest to keep synchronized. Existing `helper-queue` textual
presence flags remain discovery leads; use this resolved graph for source xrefs.
`mac calls` still compares compiled Mac candidate calls with retail and serves a
different purpose.

## Reading the queue

- `direct_count_agrees` means one exact declared callee has the same static site
  count. It does not check arguments, receivers, control flow or runtime frequency.
- `call_count_difference` and `missing_source_call` identify investigation leads.
- `inlining_path_requires_review` supplies an actual nested source path. Verify
  the Mac expansion before accepting the different boundary.
- `mac_inlining_requires_review` identifies a source call without a retained Mac
  caller/callee edge. This is not permission to remove the helper.
- `source_caller_unavailable`, `source_callee_unavailable`, and unpaired source
  states identify missing graph joins. Library labels alone do not establish a
  correspondence to a C++ overload.
- `tail_transfer_requires_review` retains branches to a different known function
  entry; verify tail calls or shared code before deciding their source boundary.
- Dispatch, parse errors and implicit cleanup remain explicit coverage gaps.
  Virtual calls have a declared callee but an unknown dynamic destination.
  Differing TU views of a header body cannot be combined into an inferred path.
- Deferred modules remain marked in the Mac queue. A whole-section branch scan
  retains unpaired references but does not establish function boundaries by itself.

Clang supplies source identities under the Windows preprocessor/profile view;
VC6 remains the game compiler. The graph is incomplete where Clang cannot parse
legacy source or expose implicit lifetime calls. Inspect `source.diagnostics`
and `source.gaps`; selecting every TU is a scope statement, not a claim that all
operations were parsed. No count or path automatically closes a recovery item.

The binary index now records indirect branches during the same instruction scan
and serves reverse xrefs from address indexes. For the separate compile/compare
path, [Mac build reuse](mac-build-performance.md) describes shared unit objects,
reference maps and freshness guards.

## Identified operations without a source mapping

`config/mac/target-observations.tsv` records reviewed binary operations while
source identity is still unresolved. The audit verifies each extent and byte
hash, attaches an `observation` to the function/callee record, and exports
`callee_operation` / `callee_category` in the calls TSV. Address queries display
the evidence. These descriptive operations are not linkage symbols or source
names; they never create AST identities, suppress a discrepancy, or mark a call
complete. Continue matching their source operation and following generated xrefs.
See the [111-target review](../matching/mac-unnamed-targets-20260926.md).
