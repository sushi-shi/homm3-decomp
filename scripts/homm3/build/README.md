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

delink       THE LOOP (explicit invocation only, never in `homm3 build`):
             labels -> model -> synth_pdb -> data_manifest -> vostok
             -> build/delink/<unit>.c.obj -> copy units.toml scope to
             build/objdiff/target/ -> normalize both sides -> re-emit
             objdiff.json against the normalized copies
build        the `homm3 build` command (homm3.build.build): configure ->
             ninja -> normalize -> objdiff report -> overall line ->
             [normal tier] baseline raise + ratchet check (FATAL on a drop)
             + README score block + stale-delink warning. --fast stops
             after the %% line. The ratchet lives in homm3.match.status
             (config/match_baseline.tsv).
```

`homm3 delink` runs the loop; `homm3 status` prints the per-unit table.

The annotation macros live in `include/va.h` (absolute VAs in source, rvas
in every artifact). The delinker never runs inside `homm3 build`; explicit
invocation only, homm2-style.
