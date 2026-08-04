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

zlib_names   recover real names for zlib-map working labels by masked
             per-function identity against our compiled base objs; updates
             config/retail-zlib-map.tsv in place (bracket pass + gates)
delink       THE LOOP (explicit invocation only, never in `homm3 build`):
             labels -> synth_pdb -> data_manifest -> vostok
             -> build/delink/<unit>.c.obj -> copy units.toml scope to
             build/objdiff/target/ -> normalize both sides -> re-emit
             objdiff.json against the normalized copies
```

Run `python3 -m homm3.build.delink`, then `objdiff-cli report generate`
inside build/objdiff (or open the GUI) for real per-unit match numbers.

The annotation macros live in `include/va.h` (absolute VAs in source, rvas
in every artifact). The delinker never runs inside `homm3 build`; explicit
invocation only, homm2-style.
