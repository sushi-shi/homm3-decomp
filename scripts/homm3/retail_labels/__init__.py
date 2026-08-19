"""homm3.retail_labels - every label, as typed records.

One concern from two directions (the gruntz template): the committed
inventory tables and image-derived channels are PARSED (censuses /
providers / iat - parse-only, zero policy), the source annotations are
EXTRACTED (source.py - the lexical VA/DATA macro scan, the per-unit
base-obj authority join, and the per-TU fragment cache under
build/gen/claims/; its CLI is public as `homm3 labels`). Both directions
produce the same Claim record; the MODEL (homm3.model) is the only
consumer of the records and the ONLY place policy lives - authority
order, naming fallbacks, dedup spellings, the fatal claim gates.

A module here enforces syntax (columns, hex spelling, intra-table
consistency) and NOTHING else: no cross-channel joins, no precedence, no
enrichment decisions.

The one record every parser emits:

    Claim(rva, name, kind, channel, size, unit, meta)

kind is 'func' | 'data'; channel is the provenance string that
build/gen/symbol_names.csv has always carried (src-VA, src-VA+base,
zlib-map, runtime-map, iat-implib, reloc-target, ...); size is the claimed
extent in bytes, or None when the channel states none (serialized empty);
meta carries the channel's extras verbatim (dtor flags, compgen owners).

Divergences from gruntz, by doctrine:
  * channels keep homm3's historical provenance spellings - they are the
    symbol_names.csv contract every consumer (synth PDB, sema, status,
    vc6) already reads;
  * the extraction universe is src/*.c*, NOT the manifest: carcass TUs
    carry VA() claims without being manifest units, and the vendored zlib
    units are manifest units whose claims are a provider table;
  * names come from the lexical declarator scan + the base-obj authority
    join (the P0.2 interim binding) - the clang-IR channel is a future
    replacement inside source.py, not a porting target today.
"""

from __future__ import annotations

from typing import NamedTuple


class Claim(NamedTuple):
    rva: int
    name: str
    kind: str          # 'func' | 'data'
    channel: str       # the symbol_names.csv provenance string
    size: int | None   # claimed extent; None = channel states none
    unit: str
    meta: dict
