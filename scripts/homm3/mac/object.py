"""Read named code hunks from pinned MWLinkPPC `-dis` output.

The disassembler is used to resolve MWOB symbol boundaries, rather than
searching for target bytes inside an object (which can yield false matches).
"""
from __future__ import annotations

from dataclasses import dataclass
import re
import struct


class ObjectError(ValueError):
    pass


_HUNK = re.compile(r'^Hunk:\s+Kind=(\S+).*Name="([^"]+)"\(\d+\)\s+Size=(\d+)')
_WORD = re.compile(r'^([0-9A-Fa-f]{8}):\s+([0-9A-Fa-f]{8})(?:\s|$)')
_XREF = re.compile(r'^XRef:\s+Kind=(\S+)\s+Offset=\$([0-9A-Fa-f]{8})(?:.*Name="([^"]+)")?')


def native_symbol_names(data: bytes) -> dict[int, str]:
    """Read the pinned MWOB PowerPC component's indexed name table.

    MWLink's display buffer damages names longer than 190 characters. Native
    names are NUL-terminated, preceded by a two-byte hash; index zero is absent.
    This reads metadata only. MWLink still owns all code boundaries and bytes.
    """
    if len(data) < 48 or not data.startswith(b"MWOBPPC "):
        raise ObjectError("not a native PowerPC object")
    component = struct.unpack_from(">I", data, 40)[0]
    if component + 20 > len(data) or data[component:component + 4] != b"POWR":
        raise ObjectError("missing native PowerPC component")
    strings, count = struct.unpack_from(">II", data, component + 12)
    cursor = component + strings
    if count < 1 or count > len(data) // 3 or cursor < component + 20:
        raise ObjectError("invalid native symbol table extent")
    names = {}
    for index in range(1, count):
        cursor += 2  # The hash is not a string length or symbol identity.
        end = data.find(b"\0", cursor)
        if cursor >= len(data) or end < 0:
            raise ObjectError("truncated native symbol table")
        try:
            names[index] = data[cursor:end].decode("ascii")
        except UnicodeDecodeError as exc:
            raise ObjectError("unsupported native symbol encoding") from exc
        cursor = end + 1
    return names


def restore_listing_names(output: str, data: bytes) -> str:
    """Join each displayed name to its native index; reject inconsistent labels."""
    names = native_symbol_names(data)

    def verified(displayed, index):
        full = names.get(int(index))
        if full is None:
            raise ObjectError(f"listing name index {index} is outside the native table")
        if displayed != full and not (len(full) > 190 and len(displayed) == 191
                                      and displayed[:190] == full[:190]):
            raise ObjectError(f"listing name {index} disagrees with the native table")
        return full

    def field(match):
        return f'Name="{verified(match[1], match[2])}"({match[2]})'

    def entry(match):
        return match[1] + verified(match[3], match[2])

    output = re.sub(r'Name="([^"\n]*)"\((\d+)\)', field, output)
    header, separator, hunks = output.partition("Hunk:")
    header = re.sub(r'^([ \t]+(\d+): )(.*)$', entry, header, flags=re.MULTILINE)
    return header + separator + hunks


@dataclass(frozen=True)
class CodeHunk:
    name: str
    data: bytes
    xrefs: tuple[tuple[int, str, str | None], ...]


@dataclass(frozen=True)
class DataHunk:
    name: str
    storage_class: str
    data: bytes | None
    xrefs: tuple[tuple[int, str, str | None], ...]
    # UDATA reserves zero-filled storage but supplies no object initializer.
    initialized: bool = True
    # MWLink omits the middle of initialized data larger than 1 KiB. Such a
    # listing proves an extent, but not a payload. Never fill the gap with zeros.
    declared_size: int | None = None


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
    truncated = gap_allowed = False
    last_end = 0

    def finish():
        if current is not None:
            name, storage_class, size, initialized = current
            if ((not initialized and size <= 0)
                    or (initialized and (last_end != size or (not truncated and len(data) != size)))
                    or (not initialized and data)
                    or (not initialized and xrefs)
                    or any(offset < 0 or offset + 4 > size for offset, _, _ in xrefs)):
                raise ObjectError(f"{name}: invalid data hunk size or relocation")
            # Materialize the loader's zero-filled memory for size/hash checks,
            # while retaining whether MWOB actually emitted initializer bytes.
            result.append(DataHunk(name, storage_class,
                                   (None if truncated else bytes(data)) if initialized else bytes(size),
                                   tuple(xrefs), initialized, size))

    for raw in output.splitlines():
        line = raw.strip()
        match = _HUNK.match(line)
        if match:
            finish()
            current = None
            data, xrefs = bytearray(), []
            truncated = gap_allowed = False
            last_end = 0
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
            if line == "...":
                truncated = gap_allowed = True
                continue
            match = re.match(r'^([0-9A-Fa-f]{8}):\s+((?:[0-9A-Fa-f]{2}(?:\s+|$))+)', line)
            if match:
                offset = int(match.group(1), 16)
                if offset < last_end or (not gap_allowed and offset != last_end):
                    raise ObjectError(f"{current[0]}: noncontiguous data hunk")
                payload = bytes.fromhex(match.group(2))
                last_end = offset + len(payload)
                if last_end > current[2]:
                    raise ObjectError(f"{current[0]}: data exceeds hunk size")
                data.extend(payload)
                gap_allowed = False
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
