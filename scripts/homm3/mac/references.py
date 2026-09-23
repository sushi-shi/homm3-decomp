"""Reviewed source-owned callee identities without claiming a compiled match.

These spans record addresses before compilation support is finished. References
with a verified compiler symbol also resolve candidate calls; address-only
references must not invent a linkage name. Source VA claims or helper definitions
own their names. Boundary, hash and overlap checks also apply to helpers with
no established Windows VA; none enter the exact-score denominator.
"""
from dataclasses import dataclass
from pathlib import Path
import re
import tomllib

from homm3 import manifest
from homm3.mac.source import SourceError, _claim, load_pairs, source_helper


@dataclass(frozen=True)
class Reference:
    retail_va: int | None
    unit: str
    source: Path
    signature: str
    mac_section: int
    mac_offset: int
    mac_size: int
    mac_symbol: str | None
    target_sha256: str
    evidence: str
    source_helper: str | None = None

    @property
    def identity(self) -> str:
        return f"0x{self.retail_va:08x}" if self.retail_va is not None else f"source:{self.unit}:{self.source_helper}"


def load(root: Path) -> list[Reference]:
    paths = sorted((root / "config/mac/references").glob("*.toml"))
    if not paths:
        return []
    units = manifest.by_unit(root / "config/units.toml")
    paired = {p.retail_va: p for p in load_pairs(root)}
    seen = {}
    spans = list(paired.values())
    names = {p.mac_symbol: p.retail_va for p in spans}

    def location(row):
        return row.mac_section, row.mac_offset, row.mac_size, row.mac_symbol

    for path in paths:
        inventory = tomllib.loads(path.read_text())
        rows = [(False, row) for row in inventory.get("functions", [])]
        rows += [(True, row) for row in inventory.get("helpers", [])]
        for helper, row in rows:
            va, unit = (None if helper else row["retail_va"]), row["unit"]
            if unit not in units:
                raise SourceError(f"{path}: unknown callee owner {unit}")
            unit_source = root / units[unit]["source"]
            source = root / row.get("source", units[unit]["source"])
            if source != unit_source:
                # A retained header inline may have either a source VA or a
                # unique canonical definition in a registered fragment.
                # Keep no-VA helpers fragment-owned so a same-named header
                # declaration cannot stand in for the original body.
                if (not source.resolve().is_relative_to((root / "include").resolve())
                        or source.suffix not in (".h", ".inl")
                        or (helper and source.suffix != ".inl")):
                    raise SourceError(f"{path}: callee source must be its owning TU or a canonical project header")
                if source.suffix == ".inl":
                    from homm3.match.source_ownership import fragment_owners
                    if source.relative_to(root).as_posix() not in fragment_owners(root):
                        raise SourceError(f"{path}: callee header fragment has no canonical owner")
            selector = row.get("source_helper") if helper else None
            if helper:
                if "retail_va" in row or not isinstance(selector, str):
                    raise SourceError(f"{path}: source helper must have a definition selector and no Windows VA")
                _, signature, _ = source_helper(source.read_text(), selector, source)
            elif row.get("compgen_kind"):
                kind, type_name = row["compgen_kind"], row.get("compgen_type")
                if (kind not in ("CLASS_CTOR", "IMPLICIT_COPY_CTOR", "IMPLICIT_DTOR", "VECTOR_DTOR")
                        or not isinstance(type_name, str)
                        or not re.fullmatch(r"[A-Za-z_]\w*", type_name)):
                    raise SourceError(f"{path}: invalid compiler-generated callee claim {va:#x}")
                if kind == "VECTOR_DTOR" and row.get("mac_symbol") is not None:
                    raise SourceError(f"{path}: vector destructor references currently support addresses only")
                pattern = (r"^\s*VA_COMPGEN\(\s*" + re.escape(f"0x{va:08x}")
                           + r"\s*,\s*[^,]+,\s*" + kind + r"\s*,\s*" + type_name + r"\s*\)")
                if len(re.findall(pattern, source.read_text(), re.MULTILINE | re.IGNORECASE)) != 1:
                    raise SourceError(f"{path}: missing unique compiler-generated callee claim {va:#x}")
                signature = f"{type_name}::{kind}"
            else:
                _, signature = _claim(source.read_text(), va, source, allow_declaration=True)
            ref = Reference(va, unit, source, signature, row["mac_section"],
                            row["mac_offset"], row["mac_size"], row.get("mac_symbol"),
                            row["target_sha256"], row["evidence"], selector)
            key = ref.identity
            if (ref.mac_section < 0 or ref.mac_offset < 0 or ref.mac_size <= 0
                    or ref.mac_offset % 4 or ref.mac_size % 4
                    or (ref.mac_symbol is not None and
                        (not isinstance(ref.mac_symbol, str) or not ref.mac_symbol))
                    or not ref.evidence.strip()
                    or not re.fullmatch(r"[0-9a-f]{64}", ref.target_sha256)):
                raise SourceError(f"{path}: invalid callee reference {key}")
            if key in seen:
                if location(seen[key]) != location(ref) or seen[key].target_sha256 != ref.target_sha256:
                    raise SourceError(f"{path}: conflicting callee reference {key}")
                continue
            if va in paired:
                if (location(paired[va])[:3] != location(ref)[:3]
                        or ref.mac_symbol is not None and paired[va].mac_symbol != ref.mac_symbol):
                    raise SourceError(f"{path}: callee reference contradicts admitted pair {va:#x}")
                # Keep the reviewed hash check even if the scored legacy pair
                # did not record a per-span hash.
                if paired[va].target_sha256 and paired[va].target_sha256 != ref.target_sha256:
                    raise SourceError(f"{path}: callee/pair hash conflict {va:#x}")
                seen[key] = ref
                continue
            if ref.mac_symbol is not None and ref.mac_symbol in names:
                raise SourceError(f"{path}: duplicate callee symbol {ref.mac_symbol!r}")
            for other in spans:
                if (other.mac_section == ref.mac_section
                        and ref.mac_offset < other.mac_offset + other.mac_size
                        and other.mac_offset < ref.mac_offset + ref.mac_size):
                    raise SourceError(f"{path}: callee {key} overlaps {other.mac_symbol}")
            if ref.mac_symbol is not None:
                names[ref.mac_symbol] = key
            spans.append(ref)
            seen[key] = ref
    return list(seen.values())
