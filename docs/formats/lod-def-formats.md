# LOD and DEF resource evidence

This note records the evidence boundary for the Rust resource oracle under
`tools/`. It is deliberately narrower than a community format specification:
retail x86 bytes prove the facts labelled **retail**, Dreamcast CodeView/SH4
shape is labelled **DC-only**, and unclosed semantics remain explicit.

The cross-format inventory and current corpus/parity status are tracked in
[resource-format-matrix.md](resource-format-matrix.md).

## LOD archive

Retail `LODFile::open` (`0x004FA8A0`), `LODFile::pointAt` (`0x004FAA70`), and
`LODFile::read` (`0x004FAB20`) prove this layout:

- a `0x5c`-byte fixed header beginning with `LOD`;
- the version dword at `+0x04` and directory count at `+0x08`;
- `count` directory entries of `0x20` bytes starting at `+0x5c`;
- each entry contains `name[16]`, file offset, uncompressed size, attributes,
  and compressed size; and
- compressed size zero means stored bytes, while a nonzero value selects a
  zlib stream of that stored length and an output buffer of the uncompressed
  size.

The Rust parser validates the entire directory and every payload extent before
returning an archive. It preserves the attribute dword without assigning
unproven bit meanings.

## H3C campaign envelopes

The original RoE 1.0 and 1.1 LODs preserve an older H3C generation that is not
present in the Complete corpus. Version `1` stores its campaign header directly:
name and description are followed by one compact `(map name, compressed size,
prerequisite mask)` record per region, including explicit void regions, and
then the concatenated gzip map members. Later versions `4`, `5`, and `6` gzip
the header as the first member and carry the full region text, cutscene,
carry-over, and typed start-bonus records before their map members.

`homm3-map::campaign` parses both header records without allocation or `std`.
`homm3-oracle` owns only envelope selection, gzip inflation, and validation that
every declared compressed size equals its member extent. Both RoE pressings
validate all seven campaigns, 27 region descriptors, and 23 embedded maps;
the AB supplement validates 14/57/53 and the SoD disc 14/64/60 respectively.
These older-disc facts are corpus evidence; the pinned Complete executable
remains authoritative for its own campaign loader.

## DEF container

Retail `ResourceManager::GetSprite` (`0x0055C7B0`) proves the import shape:

- a `0x310`-byte header: resource type, full width, full height, group count,
  then 256 RGB triples;
- each variable group begins with four dwords, followed by `frames * 13` name
  bytes and `frames * 4` absolute member offsets;
- the first two group dwords are sequence id and frame count; the final two
  are preserved as unknown;
- types `64, 66, 67, 68, 69, 70, 71, 73` read a `0x20`-byte cropped frame
  header at each frame offset; and
- other imported types use sequential `0x10`-byte compact headers after the
  group tables, while their directory offsets point at payload bytes.

The cropped header fields are encoded size, encoding method, full extent,
stored extent, and signed crop origin. The compact header has encoded size,
encoding, width, and height; its stored extent is the full extent and its crop
origin is zero.

The locally installed Steam pressing and the original US RoE 1.0, RoE 1.1,
and Shadow of Death media share one explicit divergence. Directory offsets in
`SGTWMTA.DEF` and `SGTWMTB.DEF` point to a 16-byte compact header immediately
followed by its payload, even though type 64 selects cropped headers in the
pinned retail loader. This dates the files to the initial release; the AB
supplement need not repeat resources supplied by its required base install.
`Dialect::InterleavedCompactFrames` models only that placement. The CLI's
`--def-dialect known-interleaved` selector applies it only to those two
case-insensitive names; the default remains retail and does not autodetect or
silently fall back. The manifest validates all 39,939 Steam frames, 29,393 in
each RoE pressing, and 34,324 on the SoD disc, while the retail negative control
rejects the same 12 affected frames wherever the two files occur. `steam`
remains a compatibility alias for the selector, not the dialect's evidence
name.

## Frame packets

The on-disk encoding domain is `0..=3`:

| Value | Rust name | Row index | Packet grammar | Evidence status |
| ---: | --- | --- | --- | --- |
| 0 | raw | implicit row-major | literal bytes | retail `DrawTile` at `0x0047DD40` |
| 1 | general RLE | one little-endian `u32` offset per row | `(code, len-1)`; code `255` is followed by `len` literals, other codes are fills | retail `CSpriteFrame::Draw` at `0x0047C570` |
| 2 | tileset RLE | one little-endian `u16` offset per row | high 3 bits are code, low 5 bits are `len-1`; code 7 carries literals | retail `DrawTile` at `0x0047DD40` |
| 3 | adventure RLE | one little-endian `u16` offset per 32-pixel row cell | the same packed packet grammar, restarted at each cell | retail `DrawAdvObjImpl` at `0x0047D0A0` |

The DC dossiers are `CSpriteFrame::DrawTile` at `dc:0x76988` (local
`aLineOffset`) and `CSpriteFrame::DrawAdvObjImpl` at `dc:0x76060` (locals
`cellsPerLine` and `aCellOffset`). Retail independently proves the same word
tables and packed tag grammar. In `DrawTile`, codes 0–6 advance the destination
without painting and code 7 consumes literal palette indices. In
`DrawAdvObjImpl`, code 7 is literal, code 5 paints an optional caller flag
colour, and all other controls advance transparently. The retail function
starts encoding-3 draws at `sx >> 5` and continues through contiguous encoded
cells; installed adventure frames commonly have 96-pixel stored rows.

Every packet iterator requires exactly the stored row width. It rejects short
rows, runs crossing a row or 32-pixel cell, truncated literals, and offsets
outside the member. Encoding 3 requires the DC renderer's exact
`croppedWidth >> 5` cell model, so a stored width not divisible by 32 is
rejected rather than rounded into an invented partial cell.

## Renderer parity

Retail `CSpriteFrame::Draw` proves horizontal mirroring, destination/crop
clipping, palette lookup, and general-RLE `tblit`: encoded fill runs advance
without painting, but literal runs still paint. Its raw and packed dispatches
are proved by `DrawTile` and `DrawAdvObjImpl`. Packed control runs always leave
the ordinary destination untouched. The Rust `Blit` models those ordinary draw
paths with a caller-owned 16-bit palette and destination surface.

The opt-in C++ parity test compiles the current reconstructed
`src/cspriteframe.cpp` implementation rather than a handwritten oracle stub.
It compares 512 deterministic generated draws for each of the four encodings
(2,048 total), including clipping, mirroring, transparent general fills, packed
controls, and adventure rectangles spanning several 32-pixel cells. An
environment-selected corpus gate additionally compares one deterministic
clipped full-frame draw for every installed frame. The four local Steam LODs
are clean for all 39,939 frames: 665 raw, 26,849 general RLE, 187 tileset RLE,
and 12,238 adventure RLE. Each original US RoE pressing independently agrees
for all 29,393 frames: 665 raw, 18,888 general RLE, 187 tileset RLE, and 9,653
adventure RLE. The AB supplement agrees for 5,599 frames (4,374 general and
1,225 adventure), and the SoD disc agrees for all 34,324 frames: 665 raw,
22,459 general RLE, 187 tileset RLE, and 11,013 adventure RLE.

This remains a differential between independent Rust logic and reconstructed
C++ pointer arithmetic. Retail bytes remain the verdict for the C++ itself.
VC6 objdiff now reports byte-exact `CSpriteFrame::Draw` (1,125 bytes) and
`DrawAdvObjImpl` (1,099 bytes). `DrawTile` is behaviorally reconstructed but
remains non-exact at 83.2115%. It rose from 36.848793% after restoring the
retail/DC four-direction duplication, eight-pixel Duff loop, raw do/while rows,
indexed encoded for-rows, split packet schedule, and block-scoped row
destinations; all 81 branches and 10 returns agree with retail. The Rust gate
therefore claims behavioral agreement over its generated and installed
cohorts, while only the two exact candidates constitute retail byte closure.

Not claimed here:

- context-specific shadow, outline, alpha, and player-flag effects;
- vertical tile flipping (the ordinary `Draw` dispatch fixes it to false);
- semantic names for the two unknown group-header dwords; or
- whether every non-cropped type occurs in the pinned Complete corpus.

The PNG command intentionally renders stored palette indices rather than
inventing those context-specific effects.
