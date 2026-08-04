# Vendor dependencies

This directory contains the third-party source and SDK material needed by the
target executable.

`zlib-1.1.3/` is the RETAIL zlib source: the official zlib 1.1.3 release plus
exactly one proven NWC deviation, applied in place and recorded in
`zlib-1.1.3/gzio.c.patch` (the same convention as the SDK directories below:
the tree holds the target state, the `.patch` documents the delta from the
official release). The deviation: retail's `check_header` compares signed
(`jl` at va 0x6064b5) where every official zlib 1.0.4-1.2.1 declares
`uInt len` (unsigned `jb`) - proven by a VC6 compile experiment; the retail
bytes bound NWC's edit to this single site. Nothing else may be reformatted,
patched, or annotated; project macros never enter this tree. The official
`zlib-1.1.3.tar.gz` SHA-256 is
`cae5847bc0e1cf113d3f70d037400da3e47c2e2b7b1c96b0b08447a5fbb906f4`.

The remaining dependencies were shipped as DLLs. Each directory keeps:

- `orig/`: the closest public, unmodified SDK header set;
- `include/`: a copy of that SDK header set used by the project;
- `*.patch`: only the proven changes needed for the shipped runtime; and
- `README.md`: version and provenance notes.

The original filenames, casing, declarations, typedefs, layouts, formatting,
and companion headers are preserved. A dependency that already has the exact
target version, such as Miles 5.0e, needs no patch.

A patch is an evidence ledger, not a claim that the available SDK has been
completely converted to the target version. It contains only differences that
we can currently prove from the shipped DLL, the executable, or another stated
source. An unchanged declaration may still belong to the available SDK version
and must be verified before the decompilation relies on it. Newly proved
differences should be added as separate, reviewable hunks.
