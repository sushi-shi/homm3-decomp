"""Reviewable, source-owned Mac pairing proposals and serialized admission."""
from __future__ import annotations

import fcntl
import hashlib
import json
from pathlib import Path
import re
import tempfile

from homm3 import manifest
from homm3.mac.pef import PEF
from homm3.mac.relocations import Address
from homm3.mac.source import Pair, SourceError, _claim, load_pairs
from homm3.mac import profiles


def candidate(root: Path, va: int, unit: str, data=()) -> Pair:
    units = manifest.by_unit(root / "config/units.toml")
    if unit not in units:
        raise SourceError(f"unknown owning unit {unit!r}")
    if not re.fullmatch(r"[A-Za-z0-9_-]+", unit):
        raise SourceError("unsafe unit name")
    source = root / units[unit]["source"]
    _, signature = _claim(source.read_text(), va, source, allow_declaration=True)
    profile = profiles.load(root, unit)
    if profile is None:
        raise SourceError(f"create config/mac/units/{unit}.toml with compiler settings before admission")
    return Pair(va, unit, source, signature,
                0, 0, 4, "", "unpaired compiler probe", tuple(data), unit, None, root)


def proposal(root: Path, pef: PEF, va: int, unit: str, at: Address,
             size: int, symbol: str, evidence: str, data=()) -> dict:
    probe = candidate(root, va, unit, data)
    if not evidence.strip():
        raise SourceError("record identity anchors and boundary evidence before proposing a pair")
    target = pef.code(at.section, at.offset, size)
    row = dict(retail_va=va, unit=unit, source=probe.source.relative_to(root).as_posix(),
               mac_section=at.section, mac_offset=at.offset, mac_size=size,
               mac_symbol=symbol, target_sha256=hashlib.sha256(target).hexdigest(),
               evidence=evidence)
    if data:
        row["data"] = list(data)
    load_pairs(root, [row])  # Ownership, unique identity/symbol and disjoint spans.
    return row


def render(row: dict) -> str:
    lines = ["[[functions]]"]
    for key, value in row.items():
        if key in ("retail_va", "mac_offset", "mac_size"):
            value = f"0x{value:x}"
        elif isinstance(value, list):
            value = "[" + ", ".join(f"0x{item:x}" for item in value) + "]"
        elif isinstance(value, str):
            value = json.dumps(value, ensure_ascii=False)
        lines.append(f"{key} = {value}")
    return "\n".join(lines) + "\n"


def write(root: Path, row: dict, *, admit: bool = False) -> Path:
    work = root / "build/mac/pairing"
    work.mkdir(parents=True, exist_ok=True)
    if not admit:
        path = work / f"{row['retail_va']:08x}.toml"
        path.write_text(render(row))
        return path
    # Workers own separate unit files. The lock also closes cross-unit
    # overlap/duplicate races during validation and the atomic replacement.
    with (work / "admission.lock").open("a") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        load_pairs(root, [row])
        path = root / "config/mac/functions" / (row["unit"] + ".toml")
        path.parent.mkdir(parents=True, exist_ok=True)
        old = path.read_text() if path.exists() else "# Reviewed source-owned Mac pairs.\n"
        temporary = None
        try:
            with tempfile.NamedTemporaryFile("w", dir=path.parent, delete=False) as stream:
                temporary = Path(stream.name)
                stream.write(old.rstrip() + "\n\n" + render(row))
            temporary.replace(path)
        finally:
            if temporary is not None:
                temporary.unlink(missing_ok=True)
        return path
