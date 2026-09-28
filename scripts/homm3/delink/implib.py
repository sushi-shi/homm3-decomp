"""homm3.delink.implib - PROVEN `__imp_` decorations for the IAT.

The data-topology delinker reconstructs `__imp__...` COFF relocations from the
PDB's .idata symbols and hard-errors on any IAT slot we did not name. The
decorated spelling is NEVER invented: a wrong stdcall @N is silent corruption.
It is sourced only from real artifacts, via the two REAL conventions:
  * vendor DLLs (mss32/smackw32) export an ALREADY-decorated name
    ("_AIL_init_sequence@12") -> the COFF symbol is "__imp_" + that export;
  * Win32 DLLs export undecorated ("CreateFileA") -> the COFF symbol carries
    the arg size ("__imp__CreateFileA@28"), recovered by normalizing.
Evidence: our own base objs (cl's real spellings) + the MSVC 5.0 / DX import
libraries, parsed directly (both the long-format COFF members and the short
import-header members). An ambiguous or unfound name is SKIPPED and reported.
"""

from __future__ import annotations

import re
import struct
from pathlib import Path

from homm3.core.coff import Coff


def _normalize(dec: str) -> str:
    """`__imp__CreateFileA@28` -> `CreateFileA` (the undecorated export name)."""
    b = dec[len("__imp_"):]
    if b[:1] in ("_", "@"):
        b = b[1:]
    return re.sub(r"@\d+$", "", b)


def _ar_members(path: Path):
    """Yield (member-name, body) for each member of a `!<arch>` library."""
    try:
        d = Path(path).read_bytes()
    except OSError:
        return
    if d[:8] != b"!<arch>\n":
        return
    i = 8
    while i + 60 <= len(d):
        name = d[i:i + 16].decode("latin1").strip()
        try:
            size = int(d[i + 48:i + 58].decode("latin1").strip() or 0)
        except ValueError:
            return
        yield name, d[i + 60:i + 60 + size]
        i += 60 + size + (size & 1)


def _member_symbols(body: bytes) -> list[str]:
    """Symbol names of one long-format (real i386 COFF) archive member."""
    if len(body) < 20 or struct.unpack_from("<H", body, 0)[0] != 0x014C:
        return []
    symoff = struct.unpack_from("<I", body, 8)[0]
    nsym = struct.unpack_from("<I", body, 12)[0]
    stroff = symoff + nsym * 18
    out, k = [], 0
    while k < nsym:
        o = symoff + k * 18
        if o + 18 > len(body):
            break
        raw = body[o:o + 8]
        zero, stro = struct.unpack("<II", raw)
        if zero == 0:
            try:
                nm = body[stroff + stro:body.index(b"\0", stroff + stro)] \
                    .decode("latin1")
            except ValueError:
                nm = ""
        else:
            nm = raw.rstrip(b"\0").decode("latin1", "replace")
        if nm:
            out.append(nm)
        k += 1 + body[o + 17]
    return out


def _short_import(body: bytes) -> tuple[str, str, int, int] | None:
    """(symbol, dll, ordinal_or_hint, name_type) for a short import member."""
    if len(body) < 20 or struct.unpack_from("<HH", body, 0) != (0, 0xFFFF):
        return None
    ordinal_or_hint, type_flags = struct.unpack_from("<HH", body, 16)
    try:
        end = body.index(b"\0", 20)
        sym = body[20:end].decode("latin1")
        dll = body[end + 1:body.index(b"\0", end + 1)].decode("latin1")
    except ValueError:
        return None
    return sym, dll, ordinal_or_hint, (type_flags >> 2) & 0x7


def _coff_import_ordinal_and_imp(body: bytes) -> tuple[int | None, str | None]:
    """(ordinal, `__imp_` symbol) for one long-format import-library member.

    For an ORDINAL import the member's `.idata$4` ILT entry holds the ordinal
    with the high bit set (a by-name import instead carries a relocation to
    `.idata$6`), and the member defines the `__imp__...` symbol. That pairing
    is the library's OWN ordinal->decoration binding.
    """
    if len(body) < 20 or struct.unpack_from("<H", body, 0)[0] != 0x014C:
        return None, None
    nsec = struct.unpack_from("<H", body, 2)[0]
    optsz = struct.unpack_from("<H", body, 16)[0]
    ordinal = None
    for s in range(nsec):
        o = 20 + optsz + s * 40
        nm = body[o:o + 8].rstrip(b"\0").decode("latin1", "replace")
        rawsz, rawptr = struct.unpack_from("<II", body, o + 16)
        if nm == ".idata$4" and rawsz >= 4 and rawptr:
            v = struct.unpack_from("<I", body, rawptr)[0]
            if v & 0x80000000:
                ordinal = v & 0xFFFF
    imp = next((n for n in _member_symbols(body) if n.startswith("__imp_")), None)
    return ordinal, imp


def collect_imp_decorations(base_dir: Path | None,
                            lib_paths: list[Path]) -> tuple[set, dict]:
    """(exact-set, {undecorated: decorated}) of every `__imp_` symbol seen.

    A name that maps to two different decorations is dropped from the
    normalized index (ambiguous => we refuse to choose).
    """
    exact: set[str] = set()
    if base_dir and Path(base_dir).is_dir():
        for obj in sorted(Path(base_dir).glob("*.obj")):
            try:
                names = Coff(obj).all_names()
            except (ValueError, OSError, struct.error):
                continue
            exact.update(n for n in names if n.startswith("__imp_"))
    for lib in lib_paths:
        for _name, body in _ar_members(lib):
            short = _short_import(body)
            if short is not None:
                exact.add("__imp_" + short[0])
                continue
            exact.update(n for n in _member_symbols(body)
                         if n.startswith("__imp_"))
    by_norm: dict[str, str | None] = {}
    for dec in exact:
        n = _normalize(dec)
        if n in by_norm and by_norm[n] != dec:
            by_norm[n] = None
        else:
            by_norm.setdefault(n, dec)
    return exact, {k: v for k, v in by_norm.items() if v}


def collect_ordinal_decorations(lib_paths: list[Path]) -> dict:
    """{(dll-lowercase, ordinal): `__imp__...`} from the import libraries.

    The PE carries no name for an ordinal import; the import lib's own
    ordinal->symbol binding is the only exact source. Conflicting bindings are
    dropped rather than chosen between.
    """
    table: dict[tuple[str, int], str | None] = {}

    def add(dll: str, ordinal: int, imp: str) -> None:
        key = (dll, ordinal)
        if key in table and table[key] != imp:
            table[key] = None
        else:
            table.setdefault(key, imp)

    for p in lib_paths:
        for name, body in _ar_members(p):
            dll = name.rstrip("/").lower()
            short = _short_import(body)
            if short is not None:
                sym, sdll, ordinal, name_type = short
                if name_type == 0:            # IMPORT_OBJECT_ORDINAL
                    add(sdll.lower(), ordinal, "__imp_" + sym)
                continue
            if not dll.endswith(".dll"):
                continue
            ordinal, imp = _coff_import_ordinal_and_imp(body)
            if ordinal is None or not imp:
                continue
            add(dll, ordinal, imp)
    return {k: v for k, v in table.items() if v}


def era_import_libs() -> list[Path]:
    """The MSVC 5.0 + DX SDK import libraries (non-recursive on purpose: the
    DX SDK ships a Borland/ subdir with another toolchain's conventions)."""
    import os
    dirs = []
    for var, subs in (("MSVC_DIR", ("lib",)), ("DXSDK_DIR", ("lib", "Lib"))):
        root = os.environ.get(var)
        if root:
            dirs += [Path(root) / sub for sub in subs]
    libs: list[Path] = []
    for d in dirs:
        if d.is_dir():
            for pat in ("*.LIB", "*.lib"):
                libs += sorted(d.glob(pat))
    return libs


def resolve_iat(slots, base_dir: Path | None) -> tuple[list, list]:
    """([(slot_rva, `__imp_...`)], unresolved [(slot_rva, label)]).

    `slots` is Image.import_slots(). Unresolvable slots are skipped and
    reported - never guessed.
    """
    libs = era_import_libs()
    exact, by_norm = collect_imp_decorations(base_dir, libs)
    by_ordinal = collect_ordinal_decorations(libs)
    syms, unresolved = [], []
    for slot, name, dll, ordinal in slots:
        dec = None
        if name is None:
            dec = by_ordinal.get((dll.lower(), ordinal))
            label = f"{dll} ordinal #{ordinal}"
        else:
            if "__imp_" + name in exact:   # vendor: the export IS the decoration
                dec = "__imp_" + name
            elif name in by_norm:          # win32: undecorated export -> @N
                dec = by_norm[name]
            label = f"{dll}!{name}"
        if dec:
            syms.append((slot, dec))
        else:
            unresolved.append((slot, label))
    syms.sort()
    return syms, unresolved
