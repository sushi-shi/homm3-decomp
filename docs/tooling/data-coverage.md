# Retail data in delinker and objdiff

`homm3 build` now compares data in the ordinary `build/objdiff/objdiff.json`
project. It compiles VC6 objects, regenerates the data TSVs, passes admitted
allocations to Vostok, and retains data sections and relocations in objdiff's
comparison objects. Microsoft runtime and other independently placed vendor
objects appear in the same project. There is no separate packed-data project.

## Inputs and generated files

Source `DATA`, `DATA_COMPGEN` and guard annotations own source identities.
Compiler declarations supply intended extents; raw VC6 COFF supplies actual
storage, symbols and relocations. Independently matched code references can
place otherwise unnamed compiler/vendor allocations. Equal bytes alone never
establish an address or owner. `config/retail/data-extents.tsv` and
`config/retail/vtables.tsv` hold separately admitted retail extents.

| Generated TSV | Purpose |
| --- | --- |
| `build/gen/data/retail-data.tsv` | Exhaustive, non-overlapping partition of retail `.rdata`, `.data` and `.bss`, including virtual zero-fill. Each range is delivered, a gap, or a conflict. |
| `build/gen/data/data-bindings.tsv` | Annotation/code/vendor evidence and candidate binding failures. |
| `build/gen/data/candidate-data.tsv` | Actual raw COFF allocations, symbols and relocations. |
| `build/gen/delink_data_manifest.tsv` | The exact eight-column input passed to Vostok: owner, RVA, size, storage, alignment, optional section placement and scope. |
| `build/gen/delink_data_bindings.tsv` | Generated candidate symbol identities for those same definitions; raw object hashes prevent using stale locations after recompilation. |
| `build/gen/delink_data_symbols.tsv` | Names supplied to the synthetic PDB for the manifest's RVAs. |
| `build/gen/delink_data_units.tsv` | Vendor/standalone objects added to ordinary objdiff. |
| `build/gen/delink_data_issues.tsv` | Rejected ownership, overlapping ranges and missing candidate storage. |

Vostok obtains names from the synthetic PDB and copies initialized target bytes
from retail. BSS definitions require zero retail storage. A nonzero retail
initializer remains initialized on the target even when the candidate wrongly
uses BSS. Unknown section placement stays `-`; Vostok packs admitted definitions
rather than inventing the candidate's inter-object padding in retail.

One physical retail allocation has one delinker owner. Compatible pooled or
vendor copies receive the same comparison identity. Conflicting identities are
withheld, and their bytes remain visible in the range map. A short DATA array
does not consume the following gap. Reviewed vtables without recovered candidate
storage remain visible as unmatched target data in the `vtables` unit.

The pinned delinker has a small patch in
`patches/vostok-reviewed-data-aliases.patch`: an existing relocation alias whose
owner is now a manifest definition uses that definition, including an explicit
one-past addend. It no longer creates a competing four-byte fallback copy.

## What the scores prove

Objdiff compares the real delinked target objects with raw-candidate comparison
copies. Data is no longer truncated during normalization. Its totals describe
object sections, including padding, copies and unrecovered fallback fragments;
they are **not** the unique retail-byte coverage denominator.

The default build also exports `build/data-match/data-byte-verdicts.tsv` and
`data-match-summary.json` from the same fresh binding evidence. This bounded
static check records unresolved pointer owners/addends, missing relocations and
conflicting bindings explicitly. It prevents objdiff's name-or-address relocation
comparison from being interpreted as stronger pointer proof than it supplies.
It does not construct a second comparison object or project.

`homm3 build --require-data-exact` fails while static coverage or comparison is
incomplete. Ordinary builds report that backlog without requiring an unfinished
decompilation to be exact. Vostok's global `--strict` switch is not enabled while
uncovered data references remain: it rejects the first such reference. The TSV
keeps the complete backlog visible instead.

Zero-filled storage does not establish that constructors ran, and equal static
bytes do not prove reader/writer dimensions or units. Those remain reconstruction
work. Initialization interpreters and consumer-flow analysis are outside this
pipeline.

For a wider executable audit, run:

```sh
homm3 sema coverage --output build/retail-accounting
```

`coverage.tsv` partitions the entire file and image. `data-gaps.tsv` compares that
partition with compiler-sized DATA declarations. `data-unaccounted.tsv` retains
remaining gaps after vendor/compiler ownership evidence; library-looking bytes
are not silently declared game-complete. `--require-data-complete` makes source
coverage gaps, overlaps, extern-only storage and incomplete declaration analysis
fatal. These coverage gates are distinct from byte equality.
