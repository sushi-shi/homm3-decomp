# Vendor contributions in retail DATA gaps

`homm3 sema coverage` now attributes DATA gaps to matching vendor COFF
contributions and named DLL import records. It keeps the complete file/image
partitions and the original DATA declaration totals intact. A vendor attribution
does not manufacture a game DATA declaration or remove bytes from the denominator.

```sh
homm3 sema coverage --build-vendor --output build/vendor-accounting
```

The first run compiles the 14 pristine zlib 1.1.3 units with their manifest VC6
profiles into private `build/gen/vendor-data/` objects. Subsequent runs reuse
them only when the source/header/profile/compiler fingerprint and every object
hash agree. Ordinary coverage does not invoke Wine: missing or stale objects
are explicit issues, and their contributions cannot account for gaps until
`--build-vendor` refreshes them. `vendor/`, comparison objects and source
annotations are unchanged.

## Evidence flow

`analysis/vendor_data.py` reads the pinned toolchain's `LIBCMT.LIB` and
`LIBCPMT.LIB`, plus those fresh zlib objects. Existing `runtime-map.tsv` and
`zlib-map.tsv` function identities supply possible code placements. The pass
then compares the emitted code contribution against retail, requiring matching
nonrelocation bytes and at least eight fixed bytes. Only trailing code-alignment
NOPs may be excluded; data extents are never trimmed to fit.

The original COFF relocations locate referenced sections and COMMON symbols.
The solver retains DIR32, DIR32NB and signed REL32 addends and symbol-relative
offsets. It compares every fixed byte across the entire data contribution and
checks each relocated pointer against the resolved symbol/section graph. BSS
and COMMON require a reference from that graph and an exact emitted extent;
the tool never searches for zero runs. Initialized and zero-filled portions
are checked against the PE backing separately.

A verified contribution needs a surviving path from an admitted, matched code
anchor, consistent placement, matching fixed bytes, and no unresolved or
contradictory relocation. Candidate pointers cannot propagate proof through
other data. An invalid or conflicting anchor cannot make its children verified.
All reached candidates and rejected byte/extent comparisons remain exported.
Section-symbol auxiliary records and COMMON allocations are handled explicitly.

`sema/vendor_coverage.py` overlays these ranges on the declaration-aware map.
It also names PE import descriptors, IAT/lookup slots and terminators, DLL names,
and hint/name objects by their owning DLL. These are executable loader records;
they do not claim to contain the DLL's code. Import padding and the final
all-zero directory descriptor are not assigned to a nearby DLL.

## Reports

- `coverage.tsv` adds `vendor_status`, all `vendor_ids`, `vendor_libraries`,
  `vendor_verified_libraries`, `vendor_members`, `vendor_evidence`, and
  `data_accounting_status`. Candidate alternatives remain listed, but only
  verified libraries receive attribution in byte totals.
- `data-gaps.tsv` still contains **every range without a usable sized DATA**,
  with the vendor columns explaining which ranges are accounted for.
- `data-unaccounted.tsv` contains the remaining DATA gaps: verified, unambiguous
  vendor contributions are excluded; candidates and conflicting ranges stay.
- `vendor-data.tsv` retains each reached contribution's library/member, input
  location/hash, COFF section, RVA/size/end, symbols and offsets, fixed-byte and
  relocation counts, unresolved/conflicting pointer names, anchor path and
  verdict. PE import records carry their direct structural evidence.
- `vendor-issues.tsv` reports missing/stale inputs, invalid objects, unmatched
  admitted code anchors, and conflicting vendor extents.
- `summary.json` adds vendor byte unions, per-library coverage, input hashes,
  input completeness, and reached/unreached COFF contribution counts.

Exact overlapping extents retain all library alternatives and count once.
Different overlapping extents stay conflicting until reviewed. Per-library
totals may share bytes and must not be summed as independent progress.
Neither retail-identification categories nor DATA declaration/definition
verdicts are promoted by this layer. The existing `--require-data-complete`
gate continues to require actual DATA coverage; vendor accounting has its own
explicit unresolved-byte measurement.

## Measured result

Measured on the pinned retail image with parent PR #73 at `483e2093`:

| DATA gap accounting | Bytes |
| --- | ---: |
| Original DATA gaps | 265,389 |
| Microsoft LIBCMT contributions | 14,868 |
| Microsoft LIBCPMT contributions | 6,114 |
| zlib contributions | 9,592 |
| Named DLL import records | 8,056 |
| Unique vendor-accounted gap bytes | **38,630** |
| Remaining unaccounted gaps | **226,759** |

Examples include zlib's 1,024-byte CRC table at RVA `0x244500`, its 4,360-byte
fixed-tree contribution at `0x28d998`, LIBCMT's 640-byte `__iob` contribution at
`0x28fd78` and 4,096-byte `__bufin` COMMON allocation at `0x2abe40`, and
LIBCPMT's 496/500-byte narrow/wide stream storage at `0x2ab1e0`/`0x2ab3d0`.
These are COFF contribution extents, which may include several source objects
and internal alignment; they are not newly invented individual object sizes.

The report also retains 9,266 candidate bytes and 20 bytes with conflicting
vendor extents, and reports 17 unmatched admitted code anchors. The file and
image denominators remain 2,732,032 and 2,842,624 respectively; DATA sections
still total 470,624 bytes. Regression tests cover byte corruption, relocation
addends, missing anchors, contradictory placements, unresolved pointer tables,
COMMON/BSS bounds, archive names, cache freshness, import ownership, overlaps
and TSV export. All 145 extraction/sema tests pass. An independent byte-level audit checks the exported vendor
union and remaining gaps against unchanged retail and DATA totals.

This pass follows reachable contributions from the available exact library
inputs. Unreached archive sections are not presumed linked, and unmatched or
unavailable libraries are not presumed absent. Reconstructed vendor components
such as Victor remain covered through their source-owned DATA declarations.
The remaining gaps still require investigation; no address band, nearest
library symbol, identical string, or zero content alone establishes ownership.
