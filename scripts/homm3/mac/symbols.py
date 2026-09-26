"""Verified call destinations: source claims and the executable-wide maps."""
from __future__ import annotations

from pathlib import Path

from homm3.mac.pef import PEF
from homm3.mac.relocations import Address, CallTarget


class SymbolError(ValueError):
    pass


def targets(root: Path, pef: PEF, inventory=None) -> dict[str, CallTarget]:
    """CodeWarrior symbol -> Mac address, from claims, runtime/alias/zlib maps and glue."""
    from homm3.mac import glue, pairs, tables
    if inventory is None:
        inventory = pairs.load(root)
    spans = tables.read_functions(root)
    result = {}

    def add(symbol: str, offset: int, kind: str = "direct") -> None:
        if symbol in result:
            raise SymbolError(f"duplicate Mac code symbol {symbol!r}")
        if offset not in spans:
            raise SymbolError(f"Mac symbol {symbol!r} has no {tables.FUNCTIONS_TSV} row")
        if kind not in tables.CALL_KINDS:
            raise SymbolError(f"unsupported runtime call kind {kind!r}")
        pef.code(tables.CODE_SECTION, offset, spans[offset])
        result[symbol] = CallTarget(Address(tables.CODE_SECTION, offset), kind)

    for symbol, offset in inventory.symbols.items():
        add(symbol, offset)
    runtime = tables.read_runtime(root)
    kinds = {label.offset: label.call_kind for label in runtime}
    for label in runtime:
        add(label.name, label.offset, label.call_kind)
    for alias in tables.read_aliases(root):
        add(alias.name, alias.offset, kinds.get(alias.offset, "direct"))
    for row in tables.read_zlib(root):
        if row.name not in result:  # one body per name across the vendor units
            add(row.name, row.offset)
    for name, target in glue.imports(pef).items():
        if name in result:
            raise SymbolError(f"Mac import conflicts with defined code symbol {name!r}")
        result[name] = target
    return result


def addresses(root: Path, pef: PEF, inventory=None) -> dict[str, Address]:
    return {name: target.address for name, target in targets(root, pef, inventory).items()}
