# Consumer paths through bounded switch tables

Consumer analysis now follows supported indirect jump tables in raw retail and
candidate code. This exposes reads and writes previously hidden behind an
`indirect-control-flow` gap. It does not infer a switch from source names or
accept the table's pointer values as strict retail/candidate data identities.

`analysis/access_switches.py` decodes from the function entry and follows direct
edges. An unsigned `CMP`/`JA` or `CMP`/`JAE` guard can bound a direct dword table,
or a byte/word lookup followed by a dword target table. A partial-register lookup
requires zero upper bits; `MOVZX` supplies its own zero extension. The guarded
index must reach the dispatch unchanged, and the lookup must use the correct
element scale. At most 4,096 input values are enumerated. A larger domain remains
unproved rather than being truncated.

`analysis/data_functions.py` supplies bounded code bodies and field resolution.
Retail tables must reside in executable, non-writable image storage. Candidate
tables must reside in a non-writable code contribution, with each address and
target supplied by one local DIR32 relocation. The resolver checks symbol section,
addend and exact field width. Relocated literals, missing or overlapping
relocations, external targets and malformed instruction fields cannot establish
the proof. Relative branch relocations are resolved separately as REL32 fields.

All admitted table storage and destinations remain inside the supplied function
body. Index and target ranges cannot overlap within a proof, and no table may
overlap reachable instructions. Targets cannot
enter the dispatch prefix, table bytes or the middle of a decoded instruction.
The graph is expanded and every guard path rechecked until stable; a case path
that introduces a new guard bypass revokes the proof. Unsupported indirect paths
remain explicit gaps. The result describes paths reached through known edges,
not destinations of unknown indirect jumps or whole-program control flow.

`analysis/data_accesses.py` uses those edges for its existing conservative value
analysis. Loop recognition also sees switch edges, so an extra case entry cannot
silently reuse a loop's original domain. Unknown calls, memory mutation and
unsupported effects still lose value proof. Reaching a pixel write does not by
itself establish its row pitch, complete loop extent or behavioral equivalence.

## Export and checks

`build/data-match/data-access-switches.tsv` records the function identity, guard
and dispatch sites, input domain, default edge, table entries, full case map and
instruction-path bytes. These are structured JSON cells in the ordinary TSV
accounting. The consumer summary records switch/edge counts and hashes the new
analysis implementation. The exhaustive coverage command exports the same
evidence through the shared consumer exporter.

Generic tests cover direct and compressed tables, byte and word lookups, partial
registers, inclusive/exclusive guards, the case limit, changed valid entries,
wrong guards or indices, guard bypasses discovered before and after expansion,
truncation, instruction/table overlap, external and unresolved targets, writable
storage, malformed COFF fields, relocated constants and relative branch labels.
The fixtures use unrelated addresses and case counts.

The [PR #78 audit](pr78-data-contract-audit.md) records the resulting minimap
coverage and its remaining address-flow obligations.

## Measured checkpoint

The same implementation was replayed on PR #78's base `0dbc798f` and head
`6e854110`; all 515 tooling/input hashes agree between the isolated builds.
Both full builds and gates pass, and all 31 static/initialization TSV exports
remain byte-for-byte unchanged. Actual objdiff and independent strict-data and
startup-effect audits pass on both revisions.

Each revision reports 656 proved switches, 5,539 case edges and 29,032 input
cases across 474 candidate/retail function profiles. A separate audit walks the
raw code, checks guard predecessors and instruction boundaries, resolves the
COFF fields independently and executes every exported in-range case through its
dispatch prefix. Generic tooling tests pass (193 targeted tests).

| Consumer measurement | Base before | Base after | Head before | Head after |
|---|---:|---:|---:|---:|
| Access observations | 292,036 | 317,384 | 292,064 | 317,412 |
| Known address expressions | 139,863 | 150,630 | 139,889 | 150,656 |
| Indirect-control gaps | 768 | 164 | 768 | 164 |
| Observed storage differences | 60 | 77 | 59 | 76 |

The broader paths also expose more unsupported effects and unresolved accesses;
the remaining gaps are not successes. One prior expression agreement becomes
unproved because its callee summary is unavailable in the bounded analysis
context. An isolated replay can recover that read-only summary, so context/cache
handling remains a separate call-analysis obligation. Strict consumer exactness
still fails on the visible backlog.

Minimap observation rises from one write on each side to 28 retail and 27
candidate writes. All three switches are proved in both source revisions, but
the complete byte-stride relationship is still unproved: the current value
analysis loses field-load identity after calls or memory mutation. This stage
supplies the missing paths without changing that conservative rule.
