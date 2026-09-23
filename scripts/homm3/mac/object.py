"""Read named code hunks from pinned MWLinkPPC `-dis` output.

The disassembler is used to resolve MWOB symbol boundaries, rather than
searching for target bytes inside an object (which can yield false matches).
"""
from __future__ import annotations

from dataclasses import dataclass
import re


class ObjectError(ValueError):
    pass


_HUNK = re.compile(r'^Hunk:\s+Kind=(\S+).*Name="([^"]+)"\(\d+\)\s+Size=(\d+)')
_WORD = re.compile(r'^([0-9A-Fa-f]{8}):\s+([0-9A-Fa-f]{8})(?:\s|$)')
_XREF = re.compile(r'^XRef:\s+Kind=(\S+)\s+Offset=\$([0-9A-Fa-f]{8})(?:.*Name="([^"]+)")?')


@dataclass(frozen=True)
class CodeHunk:
    name: str
    data: bytes
    xrefs: tuple[tuple[int, str, str | None], ...]


@dataclass(frozen=True)
class DataHunk:
    name: str
    storage_class: str
    data: bytes
    xrefs: tuple[tuple[int, str, str | None], ...]
    # UDATA reserves zero-filled storage but supplies no object initializer.
    initialized: bool = True


@dataclass(frozen=True)
class MetadataHunk:
    name: str
    storage_class: str
    size: int
    xrefs: tuple[tuple[int, str, str | None], ...]
    comparison: str = "not_compared"


def parse_metadata_hunks(output: str) -> list[MetadataHunk]:
    """MWLink prints TB exception tables symbolically, without their raw bytes.

    Retain their existence and relocations explicitly. They are outside the
    function-code verdict; they cannot be used as verified TOC data payloads.
    Halfword-aligned descriptor relocations are valid in these tables.
    """
    result = []
    current = None
    xrefs = []

    def finish():
        if current is not None:
            name, size = current
            if size <= 0 or any(at < 0 or at + 4 > size for at, _, _ in xrefs):
                raise ObjectError(f"{name}: invalid exception metadata extent")
            result.append(MetadataHunk(name, "TB", size, tuple(xrefs)))

    for raw in output.splitlines():
        line = raw.strip()
        match = _HUNK.match(line)
        if match:
            finish()
            current, xrefs = None, []
            if (match.group(1) in ("HUNK_GLOBAL_IDATA", "HUNK_LOCAL_IDATA")
                    and re.search(r'\bClass=TB\s', line)):
                current = (match.group(2), int(match.group(3)))
        elif current is not None:
            match = _XREF.match(line)
            if match:
                xrefs.append((int(match.group(2), 16), match.group(1), match.group(3)))
    finish()
    return result


def parse_data_hunks(output: str) -> list[DataHunk]:
    result = []
    current = None
    data = bytearray()
    xrefs = []

    def finish():
        if current is not None:
            name, storage_class, size, initialized = current
            if ((not initialized and size <= 0) or (initialized and len(data) != size)
                    or (not initialized and data)
                    or (not initialized and xrefs)
                    or any(offset < 0 or offset + 4 > size for offset, _, _ in xrefs)):
                raise ObjectError(f"{name}: invalid data hunk size or relocation")
            # Materialize the loader's zero-filled memory for size/hash checks,
            # while retaining whether MWOB actually emitted initializer bytes.
            result.append(DataHunk(name, storage_class,
                                   bytes(data) if initialized else bytes(size),
                                   tuple(xrefs), initialized))

    for raw in output.splitlines():
        line = raw.strip()
        match = _HUNK.match(line)
        if match:
            finish()
            current = None
            data, xrefs = bytearray(), []
            kind = match.group(1)
            if kind in ("HUNK_GLOBAL_IDATA", "HUNK_LOCAL_IDATA",
                        "HUNK_GLOBAL_UDATA", "HUNK_LOCAL_UDATA"):
                storage = re.search(r'\bClass=(\S+)', line)
                if storage is None:
                    raise ObjectError("data hunk lacks a storage class")
                if storage.group(1) == "TB":
                    continue  # Symbolic exception metadata, recorded separately.
                current = (match.group(2), storage.group(1), int(match.group(3)),
                           kind.endswith("_IDATA"))
        elif current is not None:
            match = re.match(r'^([0-9A-Fa-f]{8}):\s+((?:[0-9A-Fa-f]{2}(?:\s+|$))+)', line)
            if match:
                if int(match.group(1), 16) != len(data):
                    raise ObjectError(f"{current[0]}: noncontiguous data hunk")
                data.extend(bytes.fromhex(match.group(2)))
            else:
                match = _XREF.match(line)
                if match:
                    xrefs.append((int(match.group(2), 16), match.group(1), match.group(3)))
    finish()
    return result


def parse_code_hunks(output: str) -> list[CodeHunk]:
    result = []
    name = None
    kind = None
    size = 0
    words: list[tuple[int, bytes]] = []
    xrefs: list[tuple[int, str, str | None]] = []

    def finish():
        if name is None or kind not in ("HUNK_GLOBAL_CODE", "HUNK_LOCAL_CODE"):
            return
        if not words or words[0][0] != 0:
            raise ObjectError(f"{name}: code hunk has no entry word")
        for index, (offset, _) in enumerate(words):
            if offset != index * 4:
                raise ObjectError(f"{name}: noncontiguous disassembly at {offset:#x}")
        data = b"".join(word for _, word in words)
        if len(data) != size:
            raise ObjectError(f"{name}: disassembled {len(data)} bytes, hunk says {size}")
        if any(offset < 0 or offset + 4 > size for offset, _, _ in xrefs):
            raise ObjectError(f"{name}: out-of-bounds code relocation")
        result.append(CodeHunk(name, data, tuple(xrefs)))

    for raw in output.splitlines():
        line = raw.strip()
        match = _HUNK.match(line)
        if match:
            finish()
            kind, name, size = match.group(1), match.group(2), int(match.group(3))
            words, xrefs = [], []
            continue
        if name is None:
            continue
        match = _WORD.match(line)
        if match and kind in ("HUNK_GLOBAL_CODE", "HUNK_LOCAL_CODE"):
            words.append((int(match.group(1), 16), bytes.fromhex(match.group(2))))
            continue
        match = _XREF.match(line)
        if match and kind in ("HUNK_GLOBAL_CODE", "HUNK_LOCAL_CODE"):
            xrefs.append((int(match.group(2), 16), match.group(1), match.group(3)))
    finish()
    return result


def select_hunk(output: str, symbol: str) -> CodeHunk:
    matches = [hunk for hunk in parse_code_hunks(output) if hunk.name == symbol]
    if len(matches) != 1:
        raise ObjectError(f"expected one code hunk for {symbol!r}, found {len(matches)}")
    return matches[0]
