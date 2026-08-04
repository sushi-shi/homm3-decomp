# Dreamcast CodeView corpus (RoE pressing - reference evidence)

Extracted by `python3 -m homm3.analysis.dc_extract` from the cvdump text
in `../homm3-symbols/HoMM3-Dreamcast-Dump/` (itself the NB11 stream
embedded in the GD-ROM's `H3.EXE`, sha256 `cdbc7e75...`).

**Build**: WinCE SH (S_COMPILE says SH3, the linker says SH4), compiler
`Microsoft 32-bit C/C++ Optimizing Compiler 12.17.8370` (the eMbedded
VC / CE Platform Builder generation; most modules carry the CE
`MJ.MN.XXXX` version-stamp placeholder), project configuration
`Release_with_debug` - an **optimized release build with full debug
info**, not a debug build.

**Addresses are DC `.text` offsets** of another pressing. Names, types,
layouts, parameters, and locals are reference evidence for the retail
decompilation; retail claims still need the usual proof chain
(`evidence/retail-dc-name-map.csv` is the bridge where it exists).

| file | contents |
|---|---|
| functions.csv | every proc: extent, prologue/epilogue, file:line, counts |
| variables.csv | every named parameter and local (sp-relative, typed) |
| globals.csv | typed globals incl. `Class::`vftable'` symbols |
| publics.csv | all publics |
| constants.csv | named typed constants |
| classes.csv / members.csv | class layouts with member offsets + bases |
| enums.csv | enumerators with values |
| compile.csv | per-module compiler records |
