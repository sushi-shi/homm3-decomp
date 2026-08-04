# scripts/archive - retired tools. Do NOT resurrect.

Everything here already did its job; the durable results are admitted
tables under `config/`, generated corpora under `evidence/`, and the
hand-owned `src/` + `include/` carcass. The metrics these tools drove
stay live as gates in the running pipeline.

`carve/` - the bootstrap carving pipeline (2026-08: intake, reloc sweep,
Ghidra carve, DNA attribution, the naming layers, the HD/Dreamcast
transfer maps, the one-off admissions). It ran as `python3 -m homm3.carve
<stage>` when it lived at `scripts/homm3/carve/`; it is no longer
importable from here and no runtime module depends on it. If a boundary
correction ever demands re-running a stage, do it in a throwaway checkout
of the commit that archived it rather than resurrecting the package.
