# Complete retail byte accounting

`homm3 sema coverage` accounts for **every file byte and every image RVA** in the
verified retail executable. It produces an actionable TSV without silently
excluding gaps, section tails, zero-filled storage, or shared symbols.
Accounting is complete; semantic identification and candidate matching are
separate, unfinished tasks. Unknown bytes remain explicit work items.

```sh
homm3 sema coverage --output build/retail-accounting
homm3 sema coverage --json
homm3 sema coverage --require-complete
```

When a development shell points at another checkout, use the explicit worktree:

```sh
HOMM3_DIR="$PWD" PYTHONPATH=scripts python3 -m homm3 sema coverage \
  --output build/retail-accounting
```

The executable is loaded through the normal hash gate. Only output artifacts
are written; source claims, the score ledger and comparison policy are unchanged.
No candidate compilation is required. `--data-only` retains the earlier
`.rdata`/`.data`/zero-tail report for compatibility.

## Artifacts and how to act on them

- **`coverage.tsv`** is the exhaustive partition. Each row contains its coordinate
  domain, start/end, size, corresponding RVA/VA/file offset when available,
  section, storage, evidence category, all supported owners, source paths,
  address labels, evidence, provisional leads, reference counts, byte preview
  and a concrete next action. Numeric claim IDs are supplemental: the evidence
  and owners are already written directly into the row.
- **`backlog.tsv`** contains unresolved image spans plus file-only bytes, ordered
  by conflicts, incoming references, available labels and size. It omits the
  duplicate file view of mapped storage. Start here for reconstruction work.
- **`claims.tsv`** retains every proven and provisional extent, its evidence,
  owner and optional reviewed sharing group.
- **`labels.tsv`** preserves every optional symbol spelling, source location and
  originating inventory/claim fragment. Recorded aliases can include canonicalized
  anonymous-namespace spellings; they do not prove distinct source objects.
- **`references.tsv`** retains admitted and withheld reference sites across the
  entire image. Operand values are re-read from retail, with an explicit check
  against the evidence table's recorded value.
- **`summary.json`** gives fixed denominators, byte counts, PE regions, input and
  implementation hashes, invalid claims and limitations.

Offsets are hexadecimal; ends are exclusive. Blank coordinates mean that the
mapping does not exist. List fields use JSON arrays. Text containing tabs or
newlines is JSON-escaped, so one record always occupies one physical TSV line.
The dialect follows the repository's literal-tab convention, with no CSV quoting.
For Python, use `csv.DictReader(stream, delimiter="\t", quoting=csv.QUOTE_NONE)`
after skipping `#` banner lines, or `homm3.core.tsv.read`. Literal quotes in byte
previews and JSON fields must not be interpreted as CSV quoting.

Filter `backlog.tsv` by section, label, source or evidence. Locate a row's RVA in
`references.tsv` to inspect its consumers, then use `homm3 sema xref 0xVA --to`
and `homm3 sema data 0xVA` for retail evidence. Zero-filled storage has no file
bytes to read; use its accesses and initialization instead. A byte preview is
at most 32 bytes and does not imply an object boundary.

For example, a row may identify a vtable extent with several source labels,
a compiler EH map with a validated handler stub, or a zero-filled span whose
address label is known but whose object extent remains unknown. The last case
stays in the backlog even when the candidate has a convenient array size.

## Separate file and image denominators

The file partition covers `[0, file_size)`: DOS/PE headers, header slack, all
raw section bytes, raw tails, inter-section file gaps and any overlay.

The image partition covers `[0, SizeOfImage)`: headers, section-backed storage,
zero-filled virtual tails and image alignment/unassigned gaps. A gap is marked
as such; the tool does not claim that an unassigned RVA is a source object or
readable process memory. A raw tail is recorded without assuming padding.

These are **two views**, not additive progress totals. Shared backing bytes
occur once in each view. Within either view every byte occurs exactly once,
even if multiple symbols or claims refer to it.

The pinned image has no standalone `.bss` PE section. Its `.data` section has
a 110,176-byte zero-filled virtual tail. Standalone BSS sections are also handled.
A function extent may include jump tables or other embedded data: the report
identifies its admitted owner, not every byte's instruction/data role.

## Evidence and sharing policy

Proven extents are collected from:

- `config/retail/functions.tsv`, including its admitted embedded-table extents;
- `config/retail/vtables.tsv`;
- bounded PE headers, imports and resource structures/payloads;
- validated VC6 FuncInfo records and their unwind/try/catch maps, anchored by
  handler stubs whose relative jump resolves to the admitted `___CxxFrameHandler`;
- explicit reviewed `config/retail/data-extents.tsv` rows.

Recognizable terminated printable runs are **provisional string candidates**.
Source labels, candidate sizes, proximity to another symbol and all-zero content
never prove retail extents. A provisional observation overlapping a proven
object remains visible as a lead without overriding the proven ownership.

The optional `build/gen/symbol_names.csv` supplies navigation labels.
`build/gen/claims/*.tsv` adds all per-TU aliases before the model's single-row
join can discard them. Their content hashes are recorded. Missing optional
navigation is explicitly reported, and never changes the retail denominator.
These are generated snapshots, not freshness-verified source declarations;
regenerate them through the normal build after source changes. Source file
paths and names remain owned by existing annotations and unit configuration.

Several names at the same proven object start survive as recorded owners of the
same bytes. The `sharing` column distinguishes recorded symbol aliases from
compatible extent claims and conflicts. Aliases are navigation evidence, not a
claim that separate source objects intentionally share storage. Identical kind/start/extent claims are compatible shared evidence.
A validated EH handler contained in its admitted function is also compatible.
Other intersecting proven extents become **conflicts**, except where a common
nonempty `shared_group` explicitly records reviewed shared storage. Sharing
never increases the byte denominator. Names alone at an unknown address are
retained as address labels and do not establish a shared object extent.

The reviewed extent table stores boundary evidence, not another symbol-name
ledger. Add a row only with independent retail support. `provisional` rows may
record a hypothesis while leaving it unfinished. A `padding` classification
also needs evidence; zero content alone is insufficient.

## Gates and limitations

The tool verifies both exhaustive partitions before exporting them. It returns
nonzero for invalid extents or incompatible overlaps. `--require-complete` also
fails if unknown or provisional bytes remain; this is the future identification
completion gate, not the current accounting gate.

The reference inventory is reconstructed and incomplete. Access widths,
computed pointers, pointer flows through memory/calls and absence of references
remain unproved. A reference to one address does not prove that the adjacent
unknown span is one accessed object. The earlier data-only report additionally
provides a scan of unadmitted image-pointer-value candidates.

Identified extents do not prove correct source definitions, initialization,
candidate data enrollment, strict pointer matching, or link closure. No match
percentage is inferred from this report. Those tasks remain in the
[data matching checklist](../todos/data-matching.md).

## Measured retail result

The 2026-09-21 run used the pinned SHA-256
`057c9d88e7206f6669a4615de2c6e02ab6c4e2d570a9e2badf07fe0bd6247274`.

| Independent domain | Accounted bytes | Identified extents | Provisional | Unknown |
| --- | ---: | ---: | ---: | ---: |
| File | **2,732,032 / 2,732,032** | 2,399,493 | 84,398 | 248,141 |
| Image | **2,842,624 / 2,842,624** | 2,399,493 | 84,398 | 358,733 |

The run retains 57,088 recorded reference sites and 23,822 extent claims,
including 11,950 admitted functions, 364 vtables, 1,147 validated EH FuncInfo
records and their stubs, and 12 resource payloads. It preserves 1,498 bytes per domain with multiple recorded symbol spellings
through available source aliases; these are not necessarily distinct objects. There are no invalid
extents or conflicting overlaps. All unknown and provisional bytes remain
in the actionable backlog; they are not claimed to be reconstructed.

Implementation and acceptance criteria are recorded in the
[accounting plan](retail-accounting-plan.md). Regression tests cover exhaustive
partitions, file/image boundaries, raw tails and BSS, compatible aliases,
conflicts, reviewed partial sharing, malformed resources and EH evidence,
missing optional navigation, reference anchors, and multiline-safe TSV export.
