"""Scored Mac pairs: source MAC_ADDRESS claims with a body in a full-TU object.

A claim names a function; its Windows VA keys the Mac ledger. The candidate is
the claim's emitted hunk in `build/mac/obj/<unit>.o`, found by the
emitted-symbol join (`emitted.claim_bodies`). Call targets come from the same
join: every symbol a full-TU object emits or references resolves to the claim
of the one definition it names, next to the runtime, alias, zlib and import
glue maps. There is no separate list of admitted functions.
"""
from __future__ import annotations

from collections import defaultdict
from dataclasses import dataclass
from functools import lru_cache
import json
from pathlib import Path

from homm3.mac import emitted


class PairError(ValueError):
    pass


@dataclass(frozen=True)
class Pair:
    retail_va: int | None
    unit: str
    source: Path
    signature: str
    mac_section: int
    mac_offset: int
    mac_size: int
    mac_symbol: str
    identity: str
    line: int = 0
    source_hash: str = ""


@dataclass(frozen=True)
class Inventory:
    pairs: tuple[Pair, ...]
    unscored: tuple[dict, ...]      # Windows-VA claims without a scored body, with the reason
    symbols: dict[str, int]         # CodeWarrior symbol -> claimed Mac offset
    labels: dict[str, str]          # CodeWarrior symbol -> source name
    conflicts: tuple[str, ...]      # symbols naming two claimed offsets; left unresolved


def _universe(root: Path) -> set[str]:
    names = set()
    for index in sorted((root / "build/mac/obj").glob("*.hunks.json")):
        if not index.with_name(index.name.replace(".hunks.json", ".o")).is_file():
            continue
        for hunk in json.loads(index.read_text())["code"]:
            names.add(hunk["symbol"])
            names.update(reference[2] for reference in hunk["references"])
    return names


def load(root: Path, definitions=None, claims=None) -> Inventory:
    from homm3.mac import addresses
    if claims is None:
        claims, _windows, _problems = addresses.scan(root)
    if definitions is None:
        definitions = _definitions(root)
    objects = emitted.object_hunks(root)
    bodies = emitted.claim_bodies(root, definitions, claims, objects)
    bound = emitted.bind(definitions, claims)
    pairs, unscored = [], []
    offsets: dict[str, set[int]] = defaultdict(set)
    labels: dict[str, str] = {}
    by_key = defaultdict(list)
    for (claim, definition), row in zip(bound, bodies):
        if row["symbol"]:
            offsets[row["symbol"]].add(claim.offset)
            labels[row["symbol"]] = row["name"]
        if definition is not None:
            by_key[emitted.key(definition.name)].append((definition, claim))
        if claim.windows_va is None:
            continue
        if row["state"] != "emitted":
            unscored.append(dict(retail_va=f"0x{claim.windows_va:08x}", unit=row["unit"],
                                 file=claim.path, line=claim.line, reason=row["state"]))
            continue
        pairs.append(Pair(claim.windows_va, row["unit"] or row["object"], root / definition.file,
                          definition.name, 0, claim.offset, claim.size, row["symbol"],
                          claim.identity, claim.line, _fingerprint(root, definition)))
    # Referenced symbols whose own unit does not compile resolve by name.
    for symbol in _universe(root) - set(offsets):
        split = emitted._split(symbol)
        if split is None:
            continue
        candidates = by_key.get(emitted.key(split[0]), [])
        definition = emitted.owner(symbol, [item[0] for item in candidates])
        if definition is None:
            continue
        for candidate, claim in candidates:
            if candidate is definition:
                offsets[symbol].add(claim.offset)
                labels[symbol] = definition.name
    conflicts = tuple(sorted(symbol for symbol, found in offsets.items() if len(found) > 1))
    symbols = {symbol: next(iter(found)) for symbol, found in offsets.items() if len(found) == 1}
    seen = {}
    for pair in pairs:
        if pair.retail_va in seen:
            raise PairError(f"two Mac claims for Windows VA {pair.retail_va:#x}")
        seen[pair.retail_va] = pair
    return Inventory(tuple(sorted(pairs, key=lambda item: item.retail_va)), tuple(unscored),
                     symbols, labels, conflicts)


def _fingerprint(root: Path, definition) -> str:
    """The definition's own tokens, as the Windows ledger fingerprints them."""
    import re
    from homm3.core.cpp_tokens import fingerprint
    text = (root / definition.file).read_bytes()[definition.offset:definition.end].decode("utf-8", "replace")
    # Address annotations name the target, not the implementation.
    text = re.sub(r"\A(?:[ \t]*(?:VA|VA_COMPGEN|MAC_ADDRESS|MAC_COMPGEN_ADDRESS)\s*\([^\n]*\n)+", "", text)
    return fingerprint(text)


@lru_cache(maxsize=1)
def _cached_definitions(root: Path):
    from homm3.match.source_ownership import collect
    definitions, errors, _reached = collect(root)
    if errors:
        raise PairError("source ownership: " + "; ".join(errors[:5]))
    return definitions


def _definitions(root: Path):
    return _cached_definitions(root.resolve())


def claimed(root: Path) -> tuple[Pair, ...]:
    """Inspection targets exist independently of candidate compilation."""
    from homm3 import manifest
    from homm3.mac import addresses
    claims, _windows, problems = addresses.scan(root)
    if problems:
        raise PairError("; ".join(problems))
    units = {row["source"]: row["unit"] for row in manifest.units(root / "config/units.toml")}
    result = []
    for claim, definition in emitted.bind(_definitions(root), claims):
        owner = (definition.source_owner or definition.file) if definition else claim.path
        name = definition.name if definition else claim.label or claim.identity
        result.append(Pair(claim.windows_va, units.get(owner, ""), root / claim.path,
                           name, 0, claim.offset, claim.size, "", claim.identity, claim.line))
    return tuple(result)


def select_claim(root: Path, value: str) -> Pair:
    return _select(claimed(root), value, "claimed Mac targets")


def select(inventory: Inventory, value: str) -> Pair:
    """One scored pair by Windows VA, mac:[section:]offset, or name substring."""
    return _select(inventory.pairs, value, "scored Mac pairs")


def _select(targets, value: str, scope: str) -> Pair:
    if value.startswith("mac:"):
        parts = value[4:].split(":")
        if len(parts) not in (1, 2):
            raise PairError("Mac selector must be mac:<offset> or mac:<section>:<offset>")
        section = int(parts[0], 0) if len(parts) == 2 else 0
        offset = int(parts[-1], 0)
        matches = [pair for pair in targets
                   if pair.mac_section == section and pair.mac_offset <= offset < pair.mac_offset + pair.mac_size]
    else:
        try:
            address = int(value, 0)
        except ValueError:
            matches = [pair for pair in targets
                       if value in pair.signature or value == pair.unit]
        else:
            matches = [pair for pair in targets if pair.retail_va == address]
    if len(matches) != 1:
        raise PairError(f"selector {value!r} found {len(matches)} {scope}")
    return matches[0]
