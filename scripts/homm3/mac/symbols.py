"""Verified call destinations from source-owned pairs and reviewed runtime code."""
from __future__ import annotations

import hashlib
from pathlib import Path

from homm3.mac.pef import PEF
from homm3.mac.relocations import Address, CallTarget
from homm3.mac.source import SourceError, load_pairs


def targets(root: Path, pef: PEF, *, pairs=None, refs=None) -> dict[str, CallTarget]:
    from homm3.mac import glue, references
    result = {}

    def add(symbol: str, section: int, offset: int, size: int) -> bytes:
        if symbol in result:
            raise SourceError(f"duplicate Mac code symbol {symbol!r}")
        data = pef.code(section, offset, size)
        result[symbol] = CallTarget(Address(section, offset))
        return data

    for pair in (load_pairs(root) if pairs is None else pairs):
        data = add(pair.mac_symbol, pair.mac_section, pair.mac_offset, pair.mac_size)
        if pair.target_sha256 and hashlib.sha256(data).hexdigest() != pair.target_sha256:
            raise SourceError(f"Mac pair {pair.retail_va:#x} has changed target bytes or extent")
    for ref in (references.load(root, pairs=pairs) if refs is None else refs):
        if ref.mac_symbol is None or ref.mac_symbol in result:
            data = pef.code(ref.mac_section, ref.mac_offset, ref.mac_size)
        else:
            data = add(ref.mac_symbol, ref.mac_section, ref.mac_offset, ref.mac_size)
        if hashlib.sha256(data).hexdigest() != ref.target_sha256:
            raise SourceError(f"Mac callee reference {ref.identity} has changed bytes or extent")
    # Runtime labels take their extent from the verified function inventory.
    from homm3.mac import addresses, tables
    spans = tables.read_functions(root)
    kinds = {label.offset: label.call_kind for label in tables.read_runtime(root)}
    labels = [(label.name, label.offset, label.call_kind) for label in tables.read_runtime(root)]
    labels += [(alias.name, alias.offset, kinds.get(alias.offset, "direct"))
               for alias in tables.read_aliases(root)]
    for name, offset, kind in labels:
        if offset not in spans:
            raise SourceError(f"Mac runtime symbol {name!r} has no {tables.FUNCTIONS_TSV} row")
        if kind not in tables.CALL_KINDS:
            raise SourceError(f"unsupported runtime call kind {kind!r}")
        add(name, tables.CODE_SECTION, offset, spans[offset])
        result[name] = CallTarget(result[name].address, kind)
    # A worker row not yet moved by `homm3 mac migrate` still resolves calls.
    for row in addresses.legacy_runtime(root):
        if row["symbol"] in result:
            continue
        data = add(row["symbol"], row["mac_section"], row["mac_offset"], row["mac_size"])
        if hashlib.sha256(data).hexdigest() != row["sha256"]:
            raise SourceError(f"Mac runtime symbol {row['symbol']!r} has a different target body")
        kind = row.get("call_kind", "direct")
        if kind not in tables.CALL_KINDS:
            raise SourceError(f"unsupported runtime call kind {kind!r}")
        result[row["symbol"]] = CallTarget(result[row["symbol"]].address, kind)
    for name, target in glue.imports(pef).items():
        if name in result:
            raise SourceError(f"Mac import conflicts with defined code symbol {name!r}")
        result[name] = target
    return result


def addresses(root: Path, pef: PEF) -> dict[str, Address]:
    return {name: target.address for name, target in targets(root, pef).items()}
