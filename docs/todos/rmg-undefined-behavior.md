# RMG undefined behavior

Retail behavior and its evidence are in
[rmg-undefined-behavior.md](../reference/rmg-undefined-behavior.md). The source
reproduces retail by default; `HOMM3_RMG_HOTFIX` (see `include/rmg.h`) replaces
the crashing and uninitialized cases with defined behavior. Still open:

1. **HOTFIX is unexecuted.** Run a HOTFIX candidate over the recorded crash
   cases and confirm every one finishes.
2. **Residue models are sampled.** `testSlot` and the added water zones assume
   nonzero stack/heap residue (one `rand()` draw); a zeroed residue would make
   retail skip the draw. Oracle cases with `heapByte` 0 differ by design.
3. **Unchecked input contracts.** Malformed assets, templates or out-of-range
   requests can still misbehave (request range checks, `% size()` sites,
   density overflow, outline/Voronoi/pattern-table contracts); HOTFIX does not
   cover them.
4. **Not an exhaustive audit.** Other uninitialized reads may exist.
