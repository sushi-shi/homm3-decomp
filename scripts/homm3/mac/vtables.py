"""Bind externally emitted CodeWarrior vtables through reviewed PEF links."""
from __future__ import annotations

import hashlib
import re
import tomllib
from pathlib import Path

from homm3.mac.object import ObjectError
from homm3.mac.relocations import Address


def _sha(payload: bytes, digest: str) -> bool:
    return (isinstance(digest, str) and re.fullmatch(r"[0-9a-f]{64}", digest)
            and hashlib.sha256(payload).hexdigest() == digest)


def _class_body(root: Path, declaration: dict) -> str:
    path = declaration.get("source")
    name = declaration.get("class")
    methods = declaration.get("methods")
    if (not isinstance(path, str) or not isinstance(name, str)
            or not re.fullmatch(r"[A-Za-z_]\w*", name)
            or not isinstance(methods, list) or not methods
            or any(not isinstance(method, str) or not method.strip() for method in methods)):
        raise ObjectError("invalid reviewed vtable declaration")
    source = root / path
    if not source.is_file() or not source.resolve().is_relative_to(root.resolve()):
        raise ObjectError("reviewed vtable declaration source is unavailable")
    text = source.read_text()
    text = re.sub(r"/\*.*?\*/|//[^\n]*", " ", text, flags=re.S)
    match = re.search(r"\bclass\s+" + re.escape(name) + r"\b([^;{}]*)\{", text)
    if match is None:
        raise ObjectError(f"reviewed vtable class {name!r} is missing")
    base = declaration.get("base")
    if base is not None and (not isinstance(base, str)
                             or not re.search(r"\b" + re.escape(base) + r"\b", match[1])):
        raise ObjectError(f"reviewed vtable base for {name!r} differs")
    depth = 1
    end = match.end()
    while end < len(text) and depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    if depth:
        raise ObjectError(f"reviewed vtable class {name!r} is unclosed")
    body = re.sub(r"\s+", " ", text[match.end():end - 1])
    for method in methods:
        if re.sub(r"\s+", " ", method.strip()) not in body:
            raise ObjectError(f"reviewed virtual declaration missing in {name!r}: {method}")
    return body


def external_bindings(root: Path, pef, loader, code, hunks,
                      *, unit: str | None, retail_va: int | None) -> dict[str, Address]:
    """Return only source-proven vtables with complete loader-verified entries.

    The candidate must leave the vtable external. This never constructs or
    compares a candidate initializer using bytes from the retail PEF.
    """
    if not unit or retail_va is None:
        return {}
    path = root / "config/mac/vtables" / f"{unit}.toml"
    if not path.is_file():
        return {}
    result = {}
    toc = loader.toc()
    for row in tomllib.loads(path.read_text()).get("vtables", []):
        owners = row.get("owner_vas")
        if (not isinstance(owners, list) or not owners
                or any(not isinstance(va, int) or va <= 0 for va in owners)
                or len(set(owners)) != len(owners)):
            raise ObjectError("invalid reviewed external vtable owners")
        if retail_va not in owners:
            continue
        name = row.get("symbol")
        declarations = row.get("declarations")
        slots = row.get("slots")
        if (not isinstance(name, str) or not re.fullmatch(r"__vt__\d+[A-Za-z_]\w*", name)
                or name in result or not isinstance(declarations, list) or not declarations
                or not isinstance(slots, list) or not slots
                or not isinstance(row.get("evidence"), str) or not row["evidence"].strip()):
            raise ObjectError("invalid reviewed external vtable")
        for declaration in declarations:
            if not isinstance(declaration, dict):
                raise ObjectError("invalid reviewed vtable declaration")
            _class_body(root, declaration)
        cells = [h for h in hunks if h.name == name and h.storage_class == "TC"]
        if (len([h for h in hunks if h.name == name and h.storage_class in ("RW", "RO", "TD")])
                or len(cells) != 1 or cells[0].data != bytes(4)
                or cells[0].xrefs != ((0, "HUNK_XREF_32BIT", name),)
                or not any(kind == "HUNK_XREF_16BIT_IL" and symbol == name
                           for _, kind, symbol in code.xrefs)):
            raise ObjectError(f"reviewed external vtable {name!r} has no sole external TC cell")
        try:
            toc_at = Address(row["toc_section"], row["toc_offset"])
            target = Address(row["mac_section"], row["mac_offset"])
            rtti = Address(row["rtti_section"], row["rtti_offset"])
            size = row["mac_size"]
            if (size != 8 + 4 * len(slots) or target.section != toc.section
                    or pef.section(target.section).kind not in (1, 2, 3)
                    or loader.pointers.get(toc_at) != target
                    or loader.pointers.get(target) != rtti
                    or loader.pointers.get(Address(target.section, target.offset + 4)) is not None
                    or pef.read(target.section, target.offset + 4, 4) != bytes(4)
                    or not _sha(pef.read(target.section, target.offset, size), row["sha256"])
                    or not _sha(pef.read(rtti.section, rtti.offset, row["rtti_size"]),
                                row["rtti_sha256"])):
                raise ObjectError(f"reviewed external vtable {name!r} differs")
            for index, slot in enumerate(slots):
                descriptor = Address(slot["section"], slot["offset"])
                body = Address(slot["code_section"], slot["code_offset"])
                entry = Address(target.section, target.offset + 8 + index * 4)
                if (pef.section(body.section).kind != 0
                        or loader.pointers.get(entry) != descriptor
                        or loader.pointers.get(descriptor) != body
                        or loader.pointers.get(Address(descriptor.section, descriptor.offset + 4)) != toc
                        or not _sha(pef.read(descriptor.section, descriptor.offset, 8),
                                    slot["sha256"])
                        or not _sha(pef.read(body.section, body.offset, slot["code_size"]),
                                    slot["code_sha256"])):
                    raise ObjectError(f"reviewed external vtable {name!r} slot {index} differs")
        except (KeyError, TypeError, ValueError, IndexError) as exc:
            raise ObjectError(f"invalid reviewed external vtable {name!r}") from exc
        result[name] = target
    return result
