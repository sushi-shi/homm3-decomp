# Vendor dependencies

This directory contains the third-party source and SDK material needed by the
target executable.

`zlib-1.1.3/` is a verbatim copy of the official zlib 1.1.3 release. Nothing in
that directory may be reformatted, patched, annotated, or otherwise changed.
Build-specific flags and matching metadata belong outside the source snapshot.
The official `zlib-1.1.3.tar.gz` SHA-256 is
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
