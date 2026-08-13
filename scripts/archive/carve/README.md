# homm3.carve — bootstrap function/vtable carving (RETIREMENT-SCOPED)

**This package is bootstrap scaffolding, not product.** It exists to produce two
reviewed inventories from the pinned retail `HEROES3.EXE` and nothing else. Once the
inventories are reviewed and admitted into `config/` with retail evidence, this whole
directory moves to `scripts/archive/` per the port-plan convention and is not
resurrected. It is deliberately **not** a `homm3` CLI subcommand — the durable CLI never
learns a verb that retirement must unlearn.

```
PYTHONDONTWRITEBYTECODE=1 PYTHONPATH=scripts python3 -m homm3.carve <stage>
```

## Deliverables

- `functions.tsv` — `rva  size` per function, ascending. **`size` INCLUDES the
  function's jump tables**: cl emits switch tables inside the `/Gy` COMDAT, so the
  matcher-true extent is code + tables. Ghidra's code-only body is an input, never the
  answer (its two defects on this target: vtable-only-reachable methods never carved,
  and sizes clipped at the last instruction). The contract is pinned empirically by
  `fixture/` — a VC6-compiled COFF whose section sizes are ground truth.
- `vtables.tsv` — `rva  function_count` per vtable.

Both land under `build/carve/` (gitignored) and stay there until admitted;
`emit-config` renders candidates to `build/carve/config-candidate/` and a human copies
them into `config/`.

**`build/` is disposable** (`homm3 clean` nukes it), so anything worth keeping must
reach `config/`. The later stages therefore write their deliverables there directly —
`emit-relocs`, `dna`, and `names` do; the two core inventories above go through the
candidate/copy path because they are hand-owned after admission. Every `config/` file
this package produces carries a banner saying whether it is MANUALLY MANAGED
(hand-owned, diff-don't-clobber) or GENERATED (regenerate rather than hand-edit).

## Stage graph

```
intake (S0)  hash-gate the exe -> build/carve/target.json
relocs (S1)  vendored find_relocs channels + imm/mem operand context
             -> reloc_sites.tsv, reloc_summary.txt
ghidra (S2)  PyGhidra + AIF, seed fixpoint  -> ghidra_functions.tsv, seed_log.tsv
tables (S4)  jump-table attribution (3 channels) -> jump_tables.tsv; orphan
             dispatch owners derived from pushed code literals ->
             derived_roots.tsv + rc 3 (rerun `ghidra --no-analyze`, then
             `tables`; `all` loops this automatically)
extents(S3+S6) chunk census + size synthesis -> chunk_census.tsv,
             functions_extended.tsv
vtables(S5+S8) run building/cutting/classification -> vtable_runs.tsv,
             vtables_detail.tsv, vtable_slots.tsv, vtables.tsv
audit  (S7)  terminator + partition(+negative control) + EH-funclet gates,
             attempt-1 diff; when green renders functions.tsv
fixture      COFF-truth assertions + tables.py round-trip (--full: mini-EXE pipeline)
all          fixture -> relocs -> ghidra -> tables -> extents -> vtables -> audit
emit-config  render admission candidates -> build/carve/config-candidate/
emit-relocs  render config/retail-relocs.tsv (vostok --reloc-manifest, format
             conformance-checked against its parser) + retail-reloc-evidence.tsv
dna          static-library attribution: masked archive/zlib identity, stock
             FID export, strings, imports, banding -> evidence/retail-library-
             bands.tsv + retail-function-libraries.tsv (GENERATED, not
             manually managed) + build/dna/*; see docs/exe-dna.md
names        TEMPORARY bulk export of function-name candidates from NH3API
             wrappers + the Dreamcast CodeView dump (+ DNA archive symbols)
             -> evidence/retail-function-names.csv (GENERATED; dissolves into
             source-tree annotations later)
relate       join names <-> functions <-> vtables for the pinned image
             -> evidence/retail-function-symbols.csv, retail-vtable-symbols.csv
hdmap        transfer NH3API names from HD Mod's sibling Heroes3.exe by unique
             masked byte identity ($HOMM3_HD_EXE) -> evidence/retail-hd-name-map.csv
dcmap        transfer Dreamcast CodeView names by link order (anchor + LIS +
             equal-count brackets) -> evidence/retail-dc-name-map.csv
dcxref       the Dreamcast build's own xref graph (SH4 literal pools + BSR,
             $HOMM3_DC_EXE) -> evidence/dc-xref-graph.tsv, plus the
             cross-pressing caller-set check -> evidence/retail-dc-xref-check.csv
gametree     materialize the game source tree (one carcass per Dreamcast
             compiland, retail VAs via the dcmap ties, per-TU unmatched
             residents) -> evidence/game-tree/ + evidence/retail-game-tree.csv
carcass      ADMISSION: materialize src/ (one .cpp per game compiland,
             RVA()/DC_ONLY() annotations, src/rva.h) + include/ (per-TU
             prototype headers); hand-owned afterwards, refuses to overwrite
funcmap      ADMISSION: the non-source function maps the synth-PDB needs ->
             config/retail-zlib-map.tsv (rva, size, name) +
             config/retail-runtime-map.tsv (crt+cxx: rva, symbol-or-label);
             game code carries NO config map - its RVA() macros in src/ are
             the map. Hand-owned afterwards, refuses to overwrite
naming       give EVERY carved function a name (total coverage, unique,
             evidence-tiered) -> evidence/retail-symbols.csv; needs the Ghidra
             xref export (ghidra/export_xrefs.py -> build/dna/
             function_xrefs.tsv)
```

Stages read their upstream TSVs from `build/carve/`; every TSV carries `#` provenance
lines (generator, exe sha256, date) then a tab-separated header. RVAs are `0x%x`,
sizes decimal. llvm-objdump refuses this image's header-resident load-config
directory, so S1 disassembles a byte-identical working copy with that one
8-byte directory entry zeroed (`build/carve/HEROES3.llvm-objdump.exe`);
Ghidra imports the real exe.

## Output schemas

| file | columns |
|---|---|
| `reloc_sites.tsv` | `site_rva value channel detail target_class ctx` |
| `seed_log.tsv` | `iter source site_rva target_rva result` |
| `ghidra_functions.tsv` | `entry_rva body_size chunks body_ranges name` |
| `derived_roots.tsv` | `entry_rva table_rva dispatch_rva evidence` |
| `chunk_census.tsv` | `entry_rva body_size chunks span_extent total_gap max_gap` |
| `jump_tables.tsv` | `table_rva size entry_count kind owner_rva dispatch_rva evidence` |
| `vtable_runs.tsv` | `run_rva slot_count piece_count` |
| `vtables_detail.tsv` | `piece_rva slot_count classification entry_targets interior_targets uncovered_targets cut_evidence` |
| `vtable_slots.tsv` | `piece_rva slot target_rva state` |
| `functions_extended.tsv` | `rva size body_size chunks table_bytes gap_bytes flags` |
| `gap_candidates.tsv` | `candidate_rva max_size aligned16 prologue_like attempt1_entry first_bytes` |
| `functions.tsv` | `rva size` |
| `vtables.tsv` | `rva function_count` |

## Ownership after admission

There is no override side-channel. Overlaps that no structural rule explains
are fatal here and get investigated here. Once admitted, the `config/` copies
are **manually owned**: `emit-config` stamps them "MANUALLY MANAGED - initially
generated", and later boundary corrections are edited directly into those rows
rather than regenerated.

`gap_candidates.tsv` is the residue investigation: functions are packed in
`.text`, so a post-padding residue-gap start is usually a function Ghidra
missed (dead/unreferenced code). Candidates are report-only - reviewed, never
auto-promoted.

## Oracles (report-only, never inputs)

`../decomp-attempt-1/config/{functions.csv,vtables.csv,text-map.tsv}` and NH3API's
wrapper addresses are cross-check oracles in `audit`; they never seed roots or sizes.
