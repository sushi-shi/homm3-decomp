"""homm3.walls.pairscan - the shared normalized-pair substrate for sieves.

The sieves (aggregate-copies, eh-frame, global-refs) all read the SAME
evidence objdiff scored: the normalized comparison copies under
build/objdiff/normalized/{base,target}, via delink.coffx topology and
tool.objdump decoding. One module owns the function-window rule (a window is
cut at the NEXT DEFINED SYMBOL in its own section; cl's `$L<n>`/`$name$<n>`
block labels belong to the enclosing COMDAT and are never boundaries) and
the `$S<hash>` / `??_E` canonicalisation, so the code side and the data side
cannot drift apart.
"""

from __future__ import annotations

import re
import struct
from pathlib import Path

from homm3.core.paths import BUILD
from homm3.core.msvc_names import anonymous_namespaces
from homm3.delink.coffx import Obj

NORM = BUILD / "objdiff/normalized"

MEM_EXECUTE = 0x20000000
DIR32, REL32 = 0x06, 0x14

LOCAL_LABEL = re.compile(r"^\$(?:L\d+|\w+\$\d+)$")
# the three spellings one TU-local static reaches a join under: canonicalized
# (`$Sdata_data_<digest>_<n>`), cl's own CodeView counter, and the Model's
# ordinal-free name - bare `$S`, or `$S<rva>` where several units share a name.
LOCAL_STATIC_SUFFIX = re.compile(r"\$S(?:data_data_[0-9a-f]+_[0-9]+|[0-9]*)$")
_VDTOR = re.compile(r"^\?\?_E")
COMPGEN = re.compile(r"^_?\$[ES][0-9]+$|^\$anon_(?:data|f32|f64)_[0-9a-f]+_[0-9]+$")


def is_local_label(name: str) -> bool:
    return bool(LOCAL_LABEL.match(name))


def canon(name: str) -> str:
    """Strip named-static suffixes and fold the weak vector-deleting alias.

    A bare compiler ordinal is an identity, not a named static's suffix:
    stripping ``_$S56`` to ``_`` aliases a real source datum named ``_``.
    """
    name = anonymous_namespaces(name)
    if COMPGEN.fullmatch(name):
        return name
    return _VDTOR.sub("??_G", LOCAL_STATIC_SUFFIX.sub("", name))


def pairs(units=None) -> dict[str, tuple[Path, Path]]:
    """{unit: (base_obj, target_obj)} for every normalized pair on disk."""
    out = {}
    for base in sorted((NORM / "base").glob("*.obj")):
        unit = base.stem
        if units and unit not in units:
            continue
        target = NORM / "target" / f"{unit}.c.obj"
        if target.is_file():
            out[unit] = (base, target)
    return out


def require_pairs(units=None) -> dict[str, tuple[Path, Path]]:
    """`pairs()`, but an EMPTY result is an ERROR, not an answer. A sieve that
    prints '0 mismatches' from an unbuilt tree reads as a clean run."""
    import sys
    found = pairs(units)
    if not found:
        print(f"[walls] no normalized base/target pair under {NORM}"
              + (f" for {', '.join(sorted(units))}" if units else "")
              + " - run `homm3 build` (or `homm3 compare`) first",
              file=sys.stderr)
        raise SystemExit(2)
    return found


def scores():
    """{(unit, symbol): fuzzy%} + the live unit set, from the report."""
    from homm3.walls.inventory import report_scores
    _path, sc = report_scores()
    return sc, {u for (u, _s) in sc}


def functions(obj: Obj) -> dict[str, tuple[int, int, int]]:
    """{symbol: (secnum, start, end)} for every function body in the object.
    `end` = the next NON-LABEL defined symbol in the section, or its size."""
    out: dict[str, tuple[int, int, int]] = {}
    for secnum in range(1, obj.nsec + 1):
        sec = obj.section_table[secnum - 1]
        if not sec["characteristics"] & MEM_EXECUTE:
            continue
        members = [(v, n) for v, n, _s in obj.section_members(secnum)
                   if not is_local_label(n)]
        size = len(obj.section_payload(secnum)) or sec["size"]
        for i, (val, nm) in enumerate(members):
            end = members[i + 1][0] if i + 1 < len(members) else size
            out[nm] = (secnum, val, end)
    return out


def fn_relocs(obj: Obj, secnum: int, lo: int, hi: int):
    """[(offset, name, type, addend)] inside one window, addend read from the
    section payload bytes at the site (COFF stores it in the field itself)."""
    payload = obj.section_payload(secnum)
    out = []
    for off, (name, typ) in obj.typed_relocations(secnum).items():
        if not lo <= off < hi:
            continue
        addend = struct.unpack_from("<I", payload, off)[0] \
            if off + 4 <= len(payload) else 0
        out.append((off, name, typ, addend))
    out.sort()
    return out


def insns(obj: Obj, secnum: int, lo: int, hi: int, *, intel: bool = True):
    """[(offset, mnemonic, operands)] for one window, via tool.objdump.
    Offsets are window-relative; trailing int3/nop pad is trimmed first."""
    from homm3.tool import objdump
    body = obj.section_payload(secnum)[lo:hi]
    body = body.rstrip(b"\xcc").rstrip(b"\x90")
    if not body:
        return []
    text = objdump.disassemble(body, vma=0, intel=intel)
    out = []
    for line in text.splitlines():
        if ":\t" not in line:
            continue
        addr, rest = line.split(":\t", 1)
        parts = rest.split("\t")
        if len(parts) < 2:
            continue
        asm = parts[-1].strip()
        if not asm or asm.startswith("."):
            continue
        toks = asm.split(None, 1)
        mn = toks[0]
        ops = toks[1] if len(toks) > 1 else ""
        # fold prefixes the sieves care about into the mnemonic
        if mn in ("rep", "repz", "repnz", "repe", "repne", "lock") and ops:
            t2 = ops.split(None, 1)
            mn = f"{mn} {t2[0]}"
            ops = t2[1] if len(t2) > 1 else ""
        try:
            out.append((int(addr.strip(), 16), mn, ops))
        except ValueError:
            continue
    return out
