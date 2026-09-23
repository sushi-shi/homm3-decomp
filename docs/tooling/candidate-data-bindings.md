# Candidate data bindings

`homm3 sema coverage` now exports fresh VC6 storage and source-to-object bindings
alongside the exhaustive retail accounting. Full `homm3 build` also refreshes
these tables under `build/gen/data/` and emits the canonicalizer's
`build/gen/delink_data_bindings.tsv`.

This stage establishes candidate identities and source-sized retail projections.
It does **not** grant byte-match credit, prove retail object boundaries, or enable
ordinary data comparison. The strict comparison stage will enroll target spans
and pointer relationships together. Until then, the delinker retains its existing
vtables-only target manifest and synthesized topology; manufacturing candidate
section layouts from incomplete bindings would conceal the very gaps being audited.

```sh
homm3 build
homm3 sema coverage --build-vendor --output build/candidate-accounting
# Binding-only report, without rebuilding anything:
HOMM3_DIR="$PWD" PYTHONPATH=scripts python3 -m homm3.analysis.candidate_data
```

## Files and evidence flow

- `build/compiled_freshness.py` records content provenance in each raw object's
  `.compile.json`. It hashes source/include trees (including added files and
  `.inc` files), compiler binaries/headers, compiler options, manifests and wrapper
  implementations before and after compilation, then hashes the resulting object.
  Missing stamps, changed inputs/outputs and concurrent changes fail validation.
  Failed or timed-out compiles cannot receive a stamp. The wrapper clears `CL`
  and `_CL_`; the manifest profile owns compiler options.
  Stamps are declared Ninja outputs, so a missing stamp schedules compilation.
  The broad tree fingerprints can conservatively reject unaffected objects after
  sibling source/header edits; `ninja -t clean objects` followed by `homm3 build`
  restores all current stamps. Automatic stale-input scheduling belongs to the
  strict ordinary-build enrollment stage.
- `analysis/data_declarations.py` retains the compiler's definitions, their full
  symbol spellings, local/global scope and active macro expansion sites. Thus
  contradictory array dimensions retain both definition identities. Skipped
  bodies, failed parses and inactive/unparsed annotations remain explicit.
- `analysis/candidate_data.py` partitions every linkable COFF data section into
  physical spans, retaining unnamed prefixes, same-offset aliases, section
  alignment, storage, relocation target/addend and object/content hashes.
  COMMON records remain allocation requests, separate from section storage.
  Debug/linker-directive sections cannot count as program data.
- `build/data_manifest.py` coalesces repeated uses into concrete candidate
  section/offset bindings. Pooled literals use the generated model's shared
  semantic identity, rather than making a second symbol ledger. Ambiguous or
  unavailable evidence cannot enter the canonicalizer table.
- `sema/image_coverage.py` includes these artifacts without changing the original
  retail partition, DATA coverage, vendor proof or compiler-structure verdicts.

Generated tables:

| File | Meaning |
|---|---|
| `candidate-data.tsv` | Physical emitted spans, aliases, COMMON, relocations and provenance |
| `data-bindings.tsv` | Every parsed DATA declaration and compiler-data site, including rejected/unsupported bindings |
| `candidate-issues.tsv` | Missing, malformed or stale object evidence |
| `candidate-summary.json` | Status counts, distinct projected retail bytes, exact input hashes; zero match credit |
| `delink_data_bindings.tsv` | Unique candidate topology accepted by the canonicalizer API |
| `delink_data_binding_issues.tsv` | Bound source sites that lack usable canonicalization topology/identity |

The last two files live in `build/gen/`. Every other table is written both to the
requested accounting directory and, during full builds, to `build/gen/data/`.

## Binding rules

Ordinary DATA uses exact emitted symbol identity, scoped to definition units.
Known VC6 spellings are admitted for internal namespace-scope `_name` symbols
and the leading underscore on local-static decorated symbols. Arbitrary name,
content or address-band searches cannot recover a missing ordinary definition.
Conflicting dimensions/addresses, multiple owners and insufficient storage are
separate failures. Physical span lengths include possible padding and never
silently enlarge a logical array.

Narrow string literals require active source macro evidence and one emitted
`$SG` or narrow-string COMDAT allocation with the literal's exact bytes and
supported extent. The small decoder implements supported C escapes and adjacent
ASCII literals; floating expressions, macros, wide strings and unknown execution
encodings remain unsupported. Prefixes of embedded-NUL strings cannot bind as
shorter literals. Duplicate pools and one candidate claimed at distinct retail
addresses remain ambiguous. Suffix sharing is not guessed.

Guards require the local owner's mangled function scope, a VC6 unsigned-char
guard type and relocations to both owner and guard from one emitted function.
This also supports an inlined helper whose local symbols retain its original
scope. All eleven current guards bind as **one byte**, even when the physical
span to the next symbol is four bytes. Their initialized state, execution order
and retail access widths still require subsequent checks.

## Measured checkpoint

Pinned English GOG retail, 152 fresh VC6 units:

| Measurement | Result |
|---|---:|
| Emitted physical spans/COMMON records | 10,911 |
| Physical candidate bytes across all object copies | 400,115 |
| Bound ordinary DATA sites | 1,008 |
| Bound narrow-literal sites | 1,546 |
| Bound guard sites | 11 |
| Distinct source-sized retail projection | 194,314 bytes |
| Unique canonicalizer binding rows | 1,963 |
| Candidate bytes compared/matched | 0 / 0 |

Candidate physical bytes include duplicated object contributions and padding;
they are not an executable denominator. The projected retail union is also
distinct from the earlier declaration coverage. Original retail measurements
remain 470,624 data-section bytes, 205,235 declared bytes (including 293 overlap
bytes), 38,630 vendor-accounted gaps, 84,612 additional compiler-structure bytes
and 142,147 unaccounted bytes.

The remaining 96 binding-site failures are explicit: 40 missing ordinary
emissions, 21 missing pool emissions, 20 conflicting retail-address associations,
10 unsupported expressions, three unknown sizes and two conflicting sizes.
These include the different town-name dimensions and unresolved ABI spellings;
they must not become successful empty reports.

Validation: the full VC6 build and existing gates pass; 133 sema tests and 51
targeted tests pass. Controls cover truncated/changed/duplicate storage, local/external
identity, aliases, COMMON, literal ambiguity/escapes, byte guards, failed compile
provenance, freshness, export and canonicalizer integration. An independent TSV
audit rechecks raw section partitions, hashes, projected unions and 980 generated
compiler-data bindings against actual canonicalized COFF.

The broader build test discovery has one pre-existing failure:
`test_worktree_paths.test_link_quotes_output_map_objects_and_libraries` mocks
`run_wine(cmd, cwd, produced)`, while the unchanged linker calls
`run_wine(cmd, cwd)`. This is distinct from the passing full build and targeted
binding checks.

Table consumers, shared writer/reader storage, dynamic initialization and stride
units remain mandatory later stages of the [stack plan](data-matching-stack-plan.md).
