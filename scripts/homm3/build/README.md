# homm3.build - ninja-graph actors and the delink toolchain

Pipeline actors (the gruntz shape: `src → cl(wine) base objs → labels →
synth_pdb → vostok delink target objs → normalize both sides → objdiff`):

```
configure    config/units.toml -> build.ninja (+ objdiff wiring later)
link         opt-in VC6 candidate link (/FORCE /NODEFAULTLIB /MAP)
labels       src/ VA-family annotations (include/va.h contract v2) +
             config maps -> build/gen/symbol_names.csv, the synth-PDB
             inventory (rva,name,unit,size,kind,provenance)
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

Smoke delink (not yet the P2.3 loop):
  vostok-delinker --pdb-path build/pdb/HEROES3.pdb \
    --exe-path ../orig/HEROES3.EXE --output-path build/delink-smoke \
    --engine-path 'c:\proj\' --reloc-manifest config/retail-relocs.tsv \
    --data-manifest build/gen/delink_data_manifest.tsv
```

The annotation macros live in `include/va.h` (absolute VAs in source, rvas
in every artifact). The delinker never runs inside `homm3 build`; explicit
invocation only, homm2-style.
