# Retail data coverage and linking

`homm3 sema coverage` accounts for every byte in the retail `.rdata`, `.data`
and `.bss` extents, independently of candidate objects and function scores.
It distinguishes identified storage from unknown bytes; complete accounting
is not complete reconstruction.

```sh
homm3 sema coverage --output build/data-coverage
homm3 sema coverage --json
homm3 sema coverage --require-complete
```

The output directory receives `coverage.tsv`, an exhaustive disjoint byte map,
and `coverage.json`, the same map with extent evidence, address labels, reference
sites and unadmitted pointer-value candidates. RVAs and ends are end-exclusive;
add the JSON's `image_base` to obtain VAs. Each TSV claim number indexes the
JSON `claims` list. Input hashes identify the retail image and inventories used.
The optional generated symbol inventory is hashed too; it supplies navigation
labels, never extent proof. Regenerate labels through the normal build after
source changes.

The command returns nonzero for overlapping claims or invalid extents.
`--require-complete` also fails while unknown or provisional bytes remain.
It does not run a compiler, change score policy, or bank a match.

## What establishes an extent

- Reviewed vtables come directly from `config/retail/vtables.tsv`.
- PE import descriptors, lookup/IAT entries, DLL names and hint/name records
  are bounded from retail structures, including their required terminators.
- Other reviewed extents belong in `config/retail/data-extents.tsv`, with their
  retail boundary evidence and an `object`, `system`, `padding` or `provisional`
  classification. Names remain source-owned; this is not a second name ledger.
- Everything else remains `unknown`. Source labels and reference targets split
  the navigation map but do not establish object boundaries. Candidate array
  sizes and the distance to the next label do not prove retail extents.

Overlapping extents are explicitly classified as `overlap` and counted once.
A label inside a proven object does not create another extent. Leading and
trailing gaps, all-zero runs, raw bytes after a section's virtual extent, and
zero-filled virtual tails remain represented. Nothing is inferred to be padding
merely because it contains zeros.

The pinned retail image has no separate `.bss` PE section. The report calls the
zero-filled tail of `.data` `.bss` while retaining `.data` as its owning section.
Standalone `.bss` sections are supported as well. Section alignment gaps outside
these extents, executable-section tables, resources and headers are outside this
report's denominator, not silently counted as data coverage.

References come from the admitted relocation inventory and its evidence table;
withheld rows remain visible. Every file-backed data byte offset is also scanned
for a 32-bit value falling inside the image. Unadmitted values are investigative
leads, including unaligned values; numeric coincidence does not prove a pointer.
Access widths, indirect/computed references, and completeness of the relocation
inventory remain unverified. A reference to one byte never establishes that an
entire neighboring unknown span was accessed.

Candidate comparison enrollment, matched bytes and verified candidate pointer
fields remain separate, explicitly unmeasured quantities in this report.

## Investigation checkpoint, 2026-09-21

| Retail range (RVA) | Storage | Bytes | Proven object/system bytes | Unknown bytes |
| --- | --- | ---: | ---: | ---: |
| `0x23a000:0x25d929` | `.rdata`, initialized | 145,705 | 20,249 | 125,456 |
| `0x25d929:0x25e000` | `.rdata`, raw tail | 1,751 | 0 | 1,751 |
| `0x25e000:0x292000` | `.data`, initialized | 212,992 | 0 | 212,992 |
| `0x292000:0x2ace60` | `.data`, zero-filled / `.bss` | 110,176 | 0 | 110,176 |
| **Total** | | **470,624** | **20,249 (4.30%)** | **450,375** |

The proven inventory here contains 364 vtables / 12,164 bytes and 8,085 bytes of
PE import structures. There are no overlapping or invalid extents. The generated
inventory provides 14,003 data address labels. The reference report retains
51,381 edges touching these data regions (51,250 admitted, 131 withheld), plus
3,536 unadmitted image-pointer-value candidates. These are evidence counts, not
claims that all pointers or data objects are understood.

The matching pipeline remains substantially less complete than the function
scores suggest:

- `data_manifest.generate()` emits only the vtables, all owned by synthetic
  `vtables.c`. That unit is absent from the 152-unit objdiff configuration.
- The data section-topology and compiler-generated-data binding manifests are
  header-only, even though source `DATA_COMPGEN` annotations now exist. Their
  generation still needs implementation. The delinker currently receives the
  data manifest but not the data section-topology manifest.
- The current objdiff report has no counted data-byte total, yet reports
  `matched_data_percent = 100`. This empty denominator is not evidence of data
  reconstruction. The report producer/configuration also do not explicitly
  select strict relocation comparison; function scores are not a referent gate.

A fresh diagnostic link of all 152 raw candidate objects produces an EXE with
**905 unresolved symbols and 5 duplicate-definition warnings**. It uses
`/FORCE /NODEFAULTLIB` and the default diagnostic entry point, so it establishes
neither closure nor runnability. Unresolved symbols include 177 `__imp_` symbols,
622 decorated C++ symbols and 106 other symbols; missing libraries, globals and
functions must be distinguished before interpreting this as reconstruction debt.

The five ignored duplicate definitions are `soundManager::serviceSounds`,
`g_systemOptionsHelp`, `g_secondarySkillLevels`, `g_combatOptionsHelp` and
`g_hallInfo`. Each needs ownership review before a real link can be trusted.
The linker wrapper now preserves the full log and parses decorated C++ names;
its previous first-word parser incorrectly reported only 288 distinct entries.

Reproduce the diagnostic link separately from matching:

```sh
python3 -m homm3.build.link --out build/data-link-investigation/HEROES3.diagnostic.EXE
```

The map, complete `.link.log`, response file and `.unresolved.txt` are retained
beside the diagnostic EXE. Link order remains alphabetical by default; the
wrapper supports explicit object order for layout experiments.

## Remaining work

Use the byte map to admit retail extents with evidence, beginning with ordinary
arrays/globals and referenced unknown spans. Resolve extent overlaps rather than
choosing a winner. Then bind those objects to source declarations and candidate
COFF storage, enroll their owning units, and add independent pointer-referent and
access-width gates. Keep the denominator fixed as enrollment grows. Exact linked
RVAs, initialization order, library/EH data and whole-image coverage remain later
closure requirements; the [data matching checklist](../todos/data-matching.md)
tracks them separately.
