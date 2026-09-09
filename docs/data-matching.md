# Executable data matching: audit and proposed next phase

Audit date: 2026-09-06. This documents the current HoMM3 pipeline and the local
`/home/sheep/Projects/gruntz` implementation. It changes no comparison settings,
target objects, source claims, or checkpoints. Counts below are observations of
the available build artifacts, not permanent inventories.

## What our current scores establish

Data comparison is deliberately disabled in HoMM3's comparison copies.
[`_drop_data_sections`](../scripts/homm3/build/normalize_objs.py) zeroes every
non-code section's size and relocation count, then makes its defined symbols
undefined. `_canonicalize_side` applies this to both sides. Raw compiler and
delinked objects remain intact. The comment records a functions-only decision
from 2026-08-06.

All 146 configured normalized object pairs have zero non-code section sizes.
Consequently, objdiff's report says `matched_data_percent: 100` with **zero data
bytes in the denominator**. This is an empty comparison, not data completion.
Tables embedded in `.text` can still affect code comparison; they do not amount
to a separate census of executable data.

There is a second, independent scoring issue. The pinned objdiff 3.7.3
[`report generate` implementation](https://github.com/encounter/objdiff/blob/v3.7.3/objdiff-cli/src/cmd/report.rs)
explicitly starts with `function_reloc_diffs: None`. Its normal interactive diff
default is `name_address`. Our generated
[`objdiff.json`](../scripts/homm3/build/configure.py) supplies no override, and
[`status.load_report`](../scripts/homm3/match/status.py) supplies none either.
The scoreboard therefore ignores function relocation differences by default;
the GUI's default comparison is stricter. Operand bytes covered by relocations
are not ordinary literal-byte comparisons.

A synthetic COFF probe established both effects with the installed CLI:

| Deliberate change | Current report | Explicit comparison |
| --- | --- | --- |
| Change an initialized global from 1 to 3, keeping the function and symbol identical | Function 100%; data 100% over zero bytes after stripping | Retaining data exposes a differing 8-byte section, 87.5% section similarity |
| Change `mov eax,[value]` to `mov eax,[value+4]` | Function 100% | `functionRelocDiffs=all`: 97.5% |

`all` is not a substitute for comparing data definitions. Its pointed-value
check uses the instruction literal display machinery, rather than comparing
every byte of arbitrary globals. The writable-global probe's function remained
100% even with `all` and retained data; its data section exposed the change.

For a frozen copy of all 146 normalized pairs, captured while HEAD was
`d846b684`, the same CLI produced:

| Measure | Current report defaults | Explicit `functionRelocDiffs=all` |
| --- | ---: | ---: |
| Functions compared | 4,370 | 4,370 |
| Exact functions | 3,700 | 1,774 |
| Exact function percentage | 84.66819% | 40.594967% |
| Fuzzy percentage | 95.83273% | 95.62654% |
| Data bytes compared | 0 | 0 |

Scores fell for 2,512 functions across 136 units; 1,926 lost exact status. These
are **unresolved comparison differences, not a count of semantic bugs**. For
example, `GetDesktopWidth` compares `data_28c874` with `_gUnnamed68c874`: the
source's `DATA(0x0068c874)` identifies the same address, but the data-name binding
is missing. Callee aliases and other relocation representations also need
adjudication before interpreting the strict count.

The experiment, input hashes, frozen objects and JSON reports were retained in
`/tmp/homm3-data-audit/`. The ordinary build report and baseline were not replaced.
The report comparison can be repeated against any current build:

```sh
objdiff-cli report generate -p build/objdiff -o /tmp/homm3-default.json
objdiff-cli report generate -p build/objdiff \
  -c functionRelocDiffs=all -o /tmp/homm3-strict.json
```

## How much retail data is mapped

The verified PE contains these data spans. The zero tail describes loader
storage, not proof of the original object file's `.bss` membership. File
alignment padding and the true end of emitted initialized data need separate
treatment.

| Region | Bytes |
| --- | ---: |
| `.rdata` readable extent | 145,705 |
| `.data` file-backed extent | 212,992 |
| `.data` mapped zero tail | 110,176 |
| `.rsrc` readable extent | 33,854 |

The symbol inventory observed during the audit contained 14,001 data labels:

- 11,625 anonymous relocation targets;
- 856 ordinary `src-DATA` claims;
- 846 `src-DATA_COMPGEN` claims and 10 guard claims;
- 364 vtables, 274 IAT slots and 26 reviewed relocation aliases.

Only 648 labels had sizes: the vtables, IAT slots and ten guards. Of 1,712 labels
with source-unit ownership, only those ten guards had sizes. Ordinary
[`DATA(...)` extraction](../scripts/homm3/retail_labels/source.py) currently
records `data_<rva>` and no extent; the function symbol-binding machinery does
not bind these claims to their actual variable declarations.

For example, `cursor.cpp` already declares `kWalkSpeedPixels[5]` at VA
`0x0063d6c8` and `kScrollDelayValues[5]` at `0x0063d6dc`. The generated model
retains their starts and unit but loses the names and 20-byte candidate extents.

[`data_manifest.py`](../scripts/homm3/build/data_manifest.py) emits only the
364 vtables: 12,164 bytes assigned to the synthetic `vtables.c` owner. That is
about 3.4% of the readable `.rdata` plus file-backed `.data`, **not a measured
percentage of all correctly reconstructed data**. Neither `vtables` nor `_data`
is an objdiff unit. Section and data-binding manifests are header-only.
[`delink.py`](../scripts/homm3/build/delink.py) passes the extent manifest but
does not pass a section manifest. The synthetic PDB puts all data names in one
`_data` module without type-derived extents.

Some data reaches ordinary raw target objects through the delinker's other
mechanisms: the observed 146 raw targets contained 99,912 `.data` bytes and
11,440 `.rdata` bytes. These are object contributions, potentially duplicated
or inferred, not unique retail-byte coverage. They are subsequently stripped.

Useful safeguards already exist: pinned-image hash verification, annotation
completeness checks, retained raw objects, canonicalization sidecars and
freshness stamps, reviewed relocation aliases, and tested normalization of
equivalent addresses. The missing layer is a global inventory of proven data
extents, ownership, coverage and values. The full build's present gates do not
establish those facts. `homm3 sema data` provides inspection, not a matching gate.

## What Gruntz adds

The current local implementation is more extensive than its historical progress
notes alone suggest. The useful reference points are:

| Concern | Local Gruntz implementation | HoMM3 today |
| --- | --- | --- |
| Scoring policy | `scripts/gruntz/compare/project.py` explicitly sets `functionRelocDiffs=all` | CLI report uses its relaxed default |
| Data retention | `compare/normalize.py` retains data while canonicalizing private names | Non-code sections emptied |
| Source identity and extent | `retail_labels/source.py` binds `DATA` to AST variables, type size and linkage | Address/unit and dense working name |
| Enrollment and topology | `delink/data_manifest.py`: globals, literals, FP pools, vtables, RTTI, EH metadata, ordinary sections and COMDAT copies | Vtable extent rows; empty topology/bindings |
| Access evidence | `verify/access_map.py`, `data_access.py`: widths, indexed accesses, bounded pointer propagation, array shortfalls and import-slot mistakes | Inspection tools; no corresponding data gate |
| Coverage and ownership | `verify/data_coverage.py`, `data_tu_order.py`: overlaps, uncovered touched ranges, TU ordering | No equivalent whole-data coverage audit |
| Pointer correctness | `verify/data_relocs.py`: resolved retail referents, addends, missing/extra relocations; orphan and unpaired units | No corresponding default gate |
| Linked image | `verify/link_tier.py`, opt-in closure/section/image checks | `homm3 link` is a forced-link layout study |

Gruntz runs data-relocation, access and coverage checks in its default normal
verification tier. They include defect-injection controls and distinguish
unresolved evidence from proven contradictions. Its observed generated manifest
had 9,483 rows, including 2,039 `src-DATA-sizeof` rows, 2,507 strings, 498 vtables,
1,816 RTTI rows, 1,504 EH rows, 50 FP constants and 1,069 provisional gap rows.
Its companion manifest had 4,895 sections. These counts include copies and
provisional entries; they are not unique, proven retail objects.

Two Gruntz objdiff patches deserve specific review:

- `nix/patches/objdiff-score-reloc-addend.patch` prevents a same-symbol absolute
  addend mismatch from being rescued by coincident section-relative addresses.
- `nix/patches/objdiff-bss-inferred-extent.patch` avoids comparing padding-derived
  BSS symbol spans as though both were explicit object sizes. This relaxation
  needs an independent extent audit; it does not prove BSS layouts match.

Gruntz builds its CLI with these patches but still downloads the unpatched GUI.
We should avoid reproducing that policy mismatch. Its historical 100% data
milestones are also not proof of complete executable coverage: coverage code
explicitly excludes the territory before the first and after the last claim,
and its static access map cannot follow arbitrary pointer flows through calls
or memory. Small zero gaps are classified heuristically. These limits should
remain visible, rather than becoming assertions that the image is complete.

## Adaptations required for HoMM3

**Our retail relocation directory is empty.** Gruntz's access map starts from
the executable's original HIGHLOW fixups. HoMM3 instead supplies the reviewed,
recovered [`retail-relocs.tsv`](../config/retail-relocs.tsv), with inference
channels documented in [`retail-reloc-evidence.tsv`](../config/retail-reloc-evidence.tsv).
The manifest itself states that its precision/recall calibration came from a
different executable, not this image. Absence from our manifest cannot prove
that a retail word is a scalar or that a candidate relocation is spurious.

Use decoded retail operands, proven vtable/pointer structures and independently
identified archive relocations as positive evidence. Preserve inferred,
reviewed, contradicted and unresolved states. For a candidate pointer field
whose owner, offset and referent are independently bound, compare the resulting
retail address directly with the stored retail word even if the recovered
relocation inventory has no row there. Report unresolved bindings explicitly.
This gives useful pointer checks without claiming complete pointer discovery.

Other adaptations are equally necessary:

- Vostok pins and TSV schemas differ: HoMM3 uses `1393e24` and its eight-column
  extent schema; Gruntz uses `81d34b2` plus local patches. Port the behavior
  deliberately against the selected pin, not its manifest text verbatim.
- Validate VC6 alignment, pooling, COMMON and initializer behavior with VC6.
  Gruntz's measured VC5 layout rules are hypotheses for this compiler.
- Keep candidate type size separate from proven retail extent. An AST `sizeof`
  alone can make an undersized object compare perfectly. Confirm ABI sizes
  with VC6 evidence and check them against retail accesses and independent
  boundaries. A next-label span includes possible padding or undiscovered data.
- The Dreamcast exporter supplies positive global/type/owner evidence. Use it
  to propose identities and aggregates; SH4 addresses and layouts do not prove
  x86 placement, extent or the Complete build's initializer values.
- Address executable families separately: ordinary globals, literals, BSS,
  vtables, runtime/EH tables, static-initializer arrays, imports, resources,
  embedded tables in `.text`, and alignment gaps. Runtime initialization needs
  its constructor/registration order checked as well as initial storage bytes.

## Recommended implementation sequence

1. **Make the current measurement honest.** Display data as disabled/zero bytes.
   Add a strict relocation report using explicit project options shared by the
   GUI and CLI. Preserve the existing checkpoint history with its scoring policy;
   do not silently compare maxima recorded under different policies. Classify
   strict differences by resolved identity before treating them as code defects.

2. **Complete data claim extraction and binding.** Extend the existing source
   claim channel with declaration identity, type, linkage, candidate size and
   alignment, confirmed against VC6 output. Keep source annotations authoritative
   for names and produce manifests from the model. Track retail extent evidence
   independently. Handle extern-only declarations, static locals, COMMON guards,
   pooled strings and identical-content copies explicitly.

3. **Add an independent retail coverage report.** Account for complete PE region
   ranges, including their leading/trailing unknown areas. Partition bytes into
   proven objects, system structures, provisional extents, reviewed padding and
   unknowns. Report overlaps and retail-touched gaps; keep undecoded accesses and
   unknown pointer flows visible. Count unique retail bytes once, regardless of
   how many COMDAT copies objdiff compares. Do not infer absence from zeroes.

4. **Enroll a bounded first set and compare data.** The cursor constant arrays
   are a straightforward pilot, followed by ordinary globals, reviewed vtables
   and independently identified zlib/CRT data. Emit correct per-unit definitions
   and section topology, materializing initialized target bytes only from retail.
   Candidate topology is a comparison arrangement, not retail source evidence.
   Preserve unknown gap bytes as explicit provisional target data instead of
   making them disappear. Fail on enrolled data in a unit objdiff never opens.

5. **Add referent and extent gates alongside objdiff.** Check pointer-table slots
   and symbol-relative addends against independently resolved retail addresses;
   audit access widths and bounded array accesses against declared storage.
   Require negative controls for a changed initializer, wrong pointer, changed
   addend, undersized array, overlapping claim and silently unscored data unit.
   Apply the BSS scoring relaxation only once genuine extent defects are caught
   independently. Adopt strict relocation handling consistently in both binaries.

6. **Extend to the complete image and real link.** Cover library data, EH and
   initializer tables, resource/import structures and embedded code-section
   tables. A successfully forced link does not prove closure or correct placement.
   Later compare link-assigned storage, alignment, initialization order and
   pointer referents, then exact RVAs when the full layout is reconstructed.

Maintain separate measurements for **retail bytes identified**, **bytes enrolled
in comparison**, **enrolled data matched**, and **pointer fields verified**.
This prevents a shrinking comparison denominator or a large zero-filled object
from appearing to establish complete data correctness.
