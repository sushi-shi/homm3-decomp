# Remaining data gaps before the PR #78 audit

This review uses the unchanged game source at `0dbc798f804317c176c014ac28936ac59dbd1b49`
with matching tooling through [PR #93](https://github.com/sushi-shi/homm3-decomp/pull/93),
`675101a669b6de4fd4854ec0f8736f44f8c5527f`. The full build, raw-object audits and
exhaustive coverage export passed. Generated TSVs remain the authoritative
accounting; the tables below summarize that exact checkpoint.

Reproduce the partition with `homm3 build` followed by
`homm3 sema coverage --output build/data-coverage`. Select image rows in `.rdata`,
`.data` and `.bss` from `coverage.tsv`; do not add file-domain rows or sum
overlapping candidate projections. The pinned PE represents virtual zero storage
inside `.data`, so zero-fill accounting does not depend on a separate `.bss` name.

## Matching obligations

| Distinct retail bytes | Count |
|---|---:|
| Fixed bytes matched | 135,683 |
| Independently resolved pointer bytes matched | 22,268 |
| Static zero-fill agreement | 109,192 |
| Fixed differences | 72 |
| Pointer differences | 16 |
| Zero-fill differences | 1,051 |
| Missing relocation evidence | 1,755 |
| Unresolved pointer identity | 17,048 |
| Conflicting storage identities | 2,508 |
| Unenrolled | 181,031 |
| Total | 470,624 |

All bytes have a row and a status. Differences, missing evidence, conflicts and
unenrolled bytes are not matches. Initialized matches total 157,951 bytes. Zero agreement does not prove
initialization, ownership or reader/writer identity.

The 181,031 unenrolled bytes split into 52,673 with validated compiler structure,
14,678 with vendor attribution, and 113,680 without established ownership. Those
are disjoint accounting categories, not candidate equality claims.

## Compiler and vendor evidence still lacking candidate comparison

| Compiler structure | Unenrolled bytes |
|---|---:|
| EH FuncInfo | 19,516 |
| EH unwind maps | 18,792 |
| CRT initializer tables | 4,596 |
| RTTI type descriptors | 2,541 |
| Vtables | 2,180 |
| RTTI base class descriptors | 1,152 |
| RTTI complete object locators | 840 |
| EH try maps | 700 |
| RTTI class hierarchies | 672 |
| EH catch maps | 560 |
| EH catchable types | 420 |
| RTTI base class arrays | 412 |
| EH catchable type arrays | 200 |
| EH throw information | 64 |
| Import terminator | 20 |
| RTTI locator slots | 8 |

Retail structural validation supplies these extents, but does not establish an
independently placed emitted counterpart. Code-anchored EH recovery already
compares 886 records; the remaining records need additional owning-code proofs
or independently justified compiler contribution relationships. Initializer
registrations and supported effects have their separate reports. A registration
match cannot be substituted for a static match of the whole CRT table.

Vendor-attributed unenrolled bytes include 6,278 from LIBCMT, 235 from LIBCPMT,
100 from zlib and 8,065 from PE imports. Import descriptors, names and loader
cells remain accounted to their DLLs; object-data comparison does not establish
the final candidate linker's import-table layout.

The vendor binding report withholds ten ambiguous-definition rows, ten
conflicting-placement rows and eight COMMON requests whose initializer selection
is unproved. The COMMON requests cover 16 distinct CRT bytes. The possible strong
definitions have no independently anchored code contribution in their owning
members. Startup alternatives also retain more than one member with compatible
code evidence. Selecting whichever initializer matches retail, or treating every
COMMON request as zero-initialized, would manufacture evidence. Keep these
alternatives until actual linker selection or another independent proof resolves
them. Arbitrary Microsoft archive members are not compiler-copy aliases.

## Unidentified ownership

| Unenrolled ownership gap | Bytes |
|---|---:|
| Provisional initialized data, including string leads | 63,829 |
| Unknown initialized data | 38,662 |
| Unknown virtual zero storage | 9,438 |
| Unknown raw section tail | 1,750 |
| Provisional raw section tail | 1 |

Neither a printable sequence nor zero-filled spacing proves source ownership or
alignment. These rows retain references, neighboring claims and next actions in
`data-unaccounted.tsv`. Their union is 113,680 bytes. Total unexplained ownership
is 114,175 bytes because another 495 enrolled bytes still lack that proof: 248
unresolved pointer bytes, 161 zero-fill agreements, 52 binding conflicts, 28
missing-relocation bytes and six fixed differences.

No code-bound source extent remains unknown. Three source annotation extents are
still unavailable: an incomplete end-marker array and two incomplete field-view
arrays. Two other declarations have conflicting sizes and shapes, including the
town-name reader repaired by PR #78. All seven formerly unbound DATA sites now
have emitted bindings. Compiler-pool annotation extent gaps and 26 TUs with
excluded or skipped bodies remain explicit source-evidence limitations.

This closes the baseline inventory review, not data matching. The next required
step is the [current PR #78 audit](data-matching-stack-plan.md): check each repaired
relationship using identical generic tooling on its base and head, and extend
the tooling wherever the existing evidence fails to diagnose the defect.
