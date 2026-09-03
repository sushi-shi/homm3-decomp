# homm3.build - ninja-graph actors and the delink toolchain

Pipeline actors (the gruntz shape: `src → cl(wine) base objs → labels →
model → synth_pdb → vostok delink target objs → normalize both sides →
objdiff`). Labeling itself lives outside this package since the
retail_labels port: `homm3.retail_labels` extracts and parses (source
macros -> build/gen/claims/<unit>.tsv fragments; censuses/providers/iat
parse the admitted tables), and `homm3.model` is the one join that writes
build/gen/symbol_names.csv (rva,name,unit,size,kind,provenance) +
build/gen/compgen_claims.tsv.

```
configure    config/units.toml -> build.ninja (+ objdiff wiring later)
link         opt-in VC6 candidate link (/FORCE /NODEFAULTLIB /MAP)
synth_pdb    symbol_names.csv + pinned exe -> PDB-YAML -> llvm-pdbutil
             yaml2pdb -> DBI byte-patch -> build/pdb/HEROES3.pdb
data_manifest vostok's data-side tsvs at the PINNED schemas (1393e24):
             build/gen/delink_data_manifest.tsv (+ sections header,
             + the canonicalizer's DATA_COMPGEN bindings table,
             + hand-owned config/delink-reloc-aliases.tsv once)
canonicalize_data_symbols
             transform-before-compare (ported from homm2): volatile
             $SG/$T/$S<n>/$E<n> -> content-derived names; DATA_COMPGEN
             bindings -> __h3cg$ semantic identities; fail-closed reparse
normalize_objs
             thin driver: build/objdiff/{base,target} ->
             build/objdiff/normalized/ + .symbols.tsv sidecars
             + paired, resolved-target-preserving normalization of VC6's
             direct EH handler label against retail's last-funclet+size form
             + paired false-literal removal only when the candidate has no
             relocation and equals the target's resolved VA
             + paired aggregate+addend/field-symbol normalization only from
               a reviewed owner base or one equal-addend retail-data anchor;
               reviewed owners may resolve synthesized `data_<rva>` interior
               symbols without separately admitting every interior address,
               but only when both expressions resolve to the identical RVA;
               every transform reparses COFF and verifies unrelated sections,
               symbols and relocations

delink       THE LOOP (explicit invocation only, never in `homm3 build`):
             labels -> model -> synth_pdb -> data_manifest -> vostok
             -> build/delink/<unit>.c.obj -> copy units.toml scope to
             build/objdiff/target/ -> normalize both sides -> re-emit
             objdiff.json against the normalized copies
build        the `homm3 build` command (homm3.build.build): configure ->
             ninja -> normalize -> objdiff report -> overall line ->
             [normal tier] checkpoint-ledger refresh + observational dip
             report + fatal evidence/source gates + README score block +
             stale-delink warning. --fast stops after the %% line. The score
             ledger lives in homm3.match.status (config/match_baseline.tsv);
             a local percentage maximum is never a build gate.
```

`homm3 delink` runs the loop; `homm3 status` prints the per-unit table.

Dreamcast NB11 records are parsed into the compiler-neutral
`homm3.debug-shape.v1` model in `homm3.analysis.debug_shape`. The model carries
function extents, locals, scopes, source/breakpoint rows, emitted sizes, calls,
and branch counts for reconstruction tools. It contains no candidate/retail
comparison policy. In particular, Dreamcast SH4 structure is never compared
to candidate VC6 `/Z7` structure: `/Z7` labels only the candidate side of a
candidate-vs-retail x86 byte/relocation/CFG diff.

`INLINE_GATE(...)` is banned as source-false scaffolding. Statement-scoped
`inline_depth(0)` remains useful as a local compiler experiment, but committed
pins are historical debt: the cleanliness board prevents their count from
increasing and ratchets it down as natural source/compiler state replaces them.

The annotation macros live in `include/va.h` (absolute VAs in source, rvas
in every artifact). The delinker never runs inside `homm3 build`; explicit
invocation only, homm2-style.
