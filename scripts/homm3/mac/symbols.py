"""Verified call destinations from source-owned pairs and reviewed runtime code."""
from __future__ import annotations

import hashlib
from pathlib import Path
import tomllib

from homm3.mac.pef import PEF
from homm3.mac.relocations import Address, CallTarget
from homm3.mac.source import SourceError, load_pairs


def targets(root: Path, pef: PEF) -> dict[str, CallTarget]:
    from homm3.mac import glue, references
    result = {}

    def add(symbol: str, section: int, offset: int, size: int) -> bytes:
        if symbol in result:
            raise SourceError(f"duplicate Mac code symbol {symbol!r}")
        data = pef.code(section, offset, size)
        result[symbol] = CallTarget(Address(section, offset))
        return data

    for pair in load_pairs(root):
        data = add(pair.mac_symbol, pair.mac_section, pair.mac_offset, pair.mac_size)
        if pair.target_sha256 and hashlib.sha256(data).hexdigest() != pair.target_sha256:
            raise SourceError(f"Mac pair {pair.retail_va:#x} has changed target bytes or extent")
    for ref in references.load(root):
        if ref.mac_symbol is None or ref.mac_symbol in result:
            data = pef.code(ref.mac_section, ref.mac_offset, ref.mac_size)
        else:
            data = add(ref.mac_symbol, ref.mac_section, ref.mac_offset, ref.mac_size)
        if hashlib.sha256(data).hexdigest() != ref.target_sha256:
            raise SourceError(f"Mac callee reference {ref.identity} has changed bytes or extent")
    with (root / "config/mac/runtime.toml").open("rb") as stream:
        rows = tomllib.load(stream).get("functions", [])
    for row in rows:
        if not row["evidence"].strip():
            raise SourceError(f"Mac runtime symbol {row['symbol']!r} lacks evidence")
        data = add(row["symbol"], row["mac_section"], row["mac_offset"], row["mac_size"])
        if hashlib.sha256(data).hexdigest() != row["sha256"]:
            raise SourceError(f"Mac runtime symbol {row['symbol']!r} has a different target body")
        kind = row.get("call_kind", "direct")
        if kind not in ("direct", "indirect_tvector"):
            raise SourceError(f"unsupported runtime call kind {kind!r}")
        result[row["symbol"]] = CallTarget(result[row["symbol"]].address, kind)
    for name, target in glue.imports(pef).items():
        if name in result:
            raise SourceError(f"Mac import conflicts with defined code symbol {name!r}")
        result[name] = target
    return result


def addresses(root: Path, pef: PEF) -> dict[str, Address]:
    return {name: target.address for name, target in targets(root, pef).items()}
