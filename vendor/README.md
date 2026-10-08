# Vendor dependencies

`zlib-1.1.3/` keeps the two headers of the official zlib 1.1.3 release that the
Loki engine units compile against (`GzBuf.cpp` includes `zlib.h`), and the
release README with its license. The official `zlib-1.1.3.tar.gz` SHA-256 is
`cae5847bc0e1cf113d3f70d037400da3e47c2e2b7b1c96b0b08447a5fbb906f4`. The
library h3maped links is zlib 1.0.8, built from its own release by
`homm3 loki toolchain --libs` (docs/loki/README.md, "Link").

The Windows game's zlib source and SDK headers (Bink, Smacker, Miles, IFC) live
on `decomp-complete-4.0`.
