"""Executable-wide Mac function tables, mirroring config/retail.

  config/mac/functions.tsv        offset, size
        Every verified function span in PEF code section 0: source-owned
        game functions, library/runtime bodies and import glue alike.
  config/mac/runtime-map.tsv      offset, name, owner, call_kind, evidence
        Library, runtime and compiler-support labels (label only; the extent
        is the functions.tsv row). owner is msl_c, msl_cxx, cw_runtime,
        compiler_generated, or empty when unknown. Never game functions.
  config/mac/runtime-aliases.tsv  offset, name, evidence
        Additional linkage names of one folded runtime body.
  config/mac/glue-map.tsv         offset, name, library
        Loader-proven CFM import glue (homm3.mac.glue); 24-byte stubs.
  config/mac/dispositions.tsv     identity, disposition, evidence
        Evidenced reasons a source function has no Mac address.

The pinned PEF digest fixes every span's bytes, so rows carry no per-row
hash; `validate` re-derives what it can from the executable.
"""
from __future__ import annotations

from collections import Counter
from dataclasses import dataclass
from functools import lru_cache
from pathlib import Path
import re

from homm3.core.tsv import read as read_tsv, write as write_tsv

CODE_SECTION = 0
FUNCTIONS_TSV = "config/mac/functions.tsv"
RUNTIME_MAP_TSV = "config/mac/runtime-map.tsv"
RUNTIME_ALIASES_TSV = "config/mac/runtime-aliases.tsv"
GLUE_MAP_TSV = "config/mac/glue-map.tsv"
DISPOSITIONS_TSV = "config/mac/dispositions.tsv"

DISPOSITIONS = ("inlined_only", "mac_only", "windows_only", "folded", "platform_rewritten")
OWNERS = ("msl_c", "msl_cxx", "cw_runtime", "compiler_generated", "")
CALL_KINDS = ("direct", "indirect_tvector")
HEX_RE = re.compile(r"0x[0-9a-f]+$")
#: CodeWarrior ABI entries that compiler-generated code calls directly:
#: array construction/destruction, pointer-to-function glue, exception
#: throw/catch, global destructor registration, struct copy and FP conversion.
CW_RUNTIME = frozenset({"__ptr_glue", "__construct_array", "__destroy_arr", "__throw",
                        "__end__catch", "__copy", "__register_global_object",
                        "__cvt_fp2unsigned"})


class TableError(ValueError):
    pass


@dataclass(frozen=True)
class RuntimeLabel:
    offset: int
    name: str
    owner: str = ""
    call_kind: str = "direct"
    evidence: str = ""


@dataclass(frozen=True)
class Alias:
    offset: int
    name: str
    evidence: str = ""


@dataclass(frozen=True)
class GlueStub:
    offset: int
    name: str
    library: str


def _hex(value: str, where: str) -> int:
    if not HEX_RE.match(value):
        raise TableError(f"{where}: expected lowercase 0x hex, found {value!r}")
    return int(value, 16)


def _rows(root: Path, relative: str, columns: list[str]) -> list[dict[str, str]]:
    path = root / relative
    if not path.is_file():
        return []
    _banner, header, rows = read_tsv(path)
    if header != columns:
        raise TableError(f"{relative}: expected columns {', '.join(columns)}")
    return rows


def read_functions(root: Path) -> dict[int, int]:
    spans: dict[int, int] = {}
    for row in _rows(root, FUNCTIONS_TSV, ["offset", "size"]):
        offset = _hex(row["offset"], FUNCTIONS_TSV)
        if offset in spans:
            raise TableError(f"{FUNCTIONS_TSV}: duplicate offset {offset:#x}")
        spans[offset] = _hex(row["size"], FUNCTIONS_TSV)
    return spans


def read_runtime(root: Path) -> list[RuntimeLabel]:
    return [RuntimeLabel(_hex(row["offset"], RUNTIME_MAP_TSV), row["name"], row["owner"],
                         row["call_kind"], row["evidence"])
            for row in _rows(root, RUNTIME_MAP_TSV,
                             ["offset", "name", "owner", "call_kind", "evidence"])]


def read_aliases(root: Path) -> list[Alias]:
    return [Alias(_hex(row["offset"], RUNTIME_ALIASES_TSV), row["name"], row["evidence"])
            for row in _rows(root, RUNTIME_ALIASES_TSV, ["offset", "name", "evidence"])]


def read_glue(root: Path) -> list[GlueStub]:
    return [GlueStub(_hex(row["offset"], GLUE_MAP_TSV), row["name"], row["library"])
            for row in _rows(root, GLUE_MAP_TSV, ["offset", "name", "library"])]


def read_dispositions(root: Path) -> dict[str, tuple[str, str]]:
    result = {}
    for row in _rows(root, DISPOSITIONS_TSV, ["identity", "disposition", "evidence"]):
        if row["disposition"] not in DISPOSITIONS:
            raise TableError(f"{DISPOSITIONS_TSV}: unknown disposition {row['disposition']!r}")
        if not row["evidence"].strip():
            raise TableError(f"{DISPOSITIONS_TSV}: {row['identity']} lacks evidence")
        if row["identity"] in result:
            raise TableError(f"{DISPOSITIONS_TSV}: duplicate identity {row['identity']}")
        result[row["identity"]] = (row["disposition"], row["evidence"])
    return result


def runtime_names(root: Path) -> dict[int, list[str]]:
    """offset -> every runtime linkage name, primary first."""
    names: dict[int, list[str]] = {}
    for label in read_runtime(root):
        names.setdefault(label.offset, []).insert(0, label.name)
    for alias in read_aliases(root):
        names.setdefault(alias.offset, []).append(alias.name)
    return names


# --- provenance ---------------------------------------------------------------

@lru_cache(maxsize=2)
def _msl_c_declarations(root: Path) -> str:
    """Text of the staged MSL C/Mac headers (Latin-1, as shipped)."""
    folder = root / "build/mac/sdk"
    texts = []
    for tree in ("c", "mac"):
        for path in sorted((folder / tree).rglob("*")):
            if path.is_file():
                texts.append(path.read_bytes().decode("latin-1"))
    return "\n".join(texts)


def runtime_owner(root: Path, name: str) -> str:
    """Provenance from positive evidence only; empty when unknown.

    msl_cxx: an MSL C++ namespace (`3std`, `Metrowerks`) in the linkage
    name, MSL operator new/delete, or an `msl_` working label. msl_c: a C
    function declared by the staged MSL C/Mac headers. cw_runtime: a known
    CodeWarrior ABI entry. compiler_generated: a `generated_` working label.
    """
    bare = name.lstrip(".")
    if (bare.startswith("msl_") or re.search(r"(?<![A-Za-z])3std(?![a-z])|Metrowerks", bare)
            or re.match(r"__(?:nw|nwa|dl|dla)__F", bare)):
        return "msl_cxx"
    if bare.startswith("generated_"):
        return "compiler_generated"
    if bare in CW_RUNTIME:
        return "cw_runtime"
    if re.fullmatch(r"[A-Za-z_]\w*", bare) and re.search(
            r"[\s*]" + re.escape(bare) + r"\s*\(", _msl_c_declarations(root)):
        return "msl_c"
    return ""


# --- writing -------------------------------------------------------------------

def banner(lines: tuple[str, ...], pef_sha256: str, pef_size: int) -> list[str]:
    return [
        "# MANUALLY MANAGED - INITIALLY GENERATED by `homm3 mac migrate` from the",
        "# reviewed config/mac inventories and hand-owned after admission.",
        *(f"# {line}" for line in lines),
        f"# exe: Heroes PEF sha256={pef_sha256} size={pef_size}",
        "# Offsets are relative to PEF code section 0; lowercase 0x hex.",
    ]


def write(root: Path, spans: dict[int, int], runtime: list[RuntimeLabel],
          aliases: list[Alias], glue: list[GlueStub]) -> None:
    from homm3.core import inputs
    digest, length = inputs.MAC.sha256, inputs.MAC.size
    write_tsv(root / FUNCTIONS_TSV,
              banner(("Every verified function span in the pinned PEF code section; names",
                      "come from source MAC_ADDRESS claims, the runtime maps and glue map."),
                     digest, length),
              ["offset", "size"], [[f"0x{o:x}", f"0x{s:x}"] for o, s in sorted(spans.items())])
    write_tsv(root / RUNTIME_MAP_TSV,
              banner(("Library, runtime and compiler-support code: label only; the extent",
                      "is the functions.tsv row at the same offset. Not game functions.",
                      "owner: " + ", ".join(owner for owner in OWNERS if owner)
                      + ", or empty when unknown (see homm3.mac.tables)."),
                     digest, length),
              ["offset", "name", "owner", "call_kind", "evidence"],
              [[f"0x{row.offset:x}", row.name, row.owner, row.call_kind, row.evidence]
               for row in sorted(runtime, key=lambda row: row.offset)])
    write_tsv(root / RUNTIME_ALIASES_TSV,
              banner(("Additional linkage names of one folded runtime body in runtime-map.tsv.",),
                     digest, length),
              ["offset", "name", "evidence"],
              [[f"0x{row.offset:x}", row.name, row.evidence]
               for row in sorted(aliases, key=lambda row: (row.offset, row.name))])
    write_tsv(root / GLUE_MAP_TSV,
              banner(("Loader-proven CFM import glue (homm3.mac.glue): lwz r12 from a TOC",
                      "cell relocated to an imported transition vector, then the fixed",
                      "five-instruction jump. 24-byte stubs; not game functions."),
                     digest, length),
              ["offset", "name", "library"],
              [[f"0x{row.offset:x}", row.name, row.library]
               for row in sorted(glue, key=lambda row: row.offset)])
    if not (root / DISPOSITIONS_TSV).is_file():
        write_tsv(root / DISPOSITIONS_TSV,
                  ["# MANUALLY MANAGED - evidenced Mac dispositions for source functions",
                   "# without a MAC_ADDRESS. identity is a `homm3 mac parity` identity;",
                   "# disposition is " + ", ".join(DISPOSITIONS) + "."],
                  ["identity", "disposition", "evidence"], [])


# --- validation ------------------------------------------------------------------

def validate(root: Path, pef) -> list[str]:
    """Table-only defects: bounds, overlap, owners, glue proof and uniqueness."""
    from homm3.mac import glue as glue_module
    problems = []
    try:
        spans = read_functions(root)
        runtime = read_runtime(root)
        aliases = read_aliases(root)
        glue = read_glue(root)
        read_dispositions(root)
    except (TableError, ValueError) as exc:
        return [str(exc)]
    code = pef.section(CODE_SECTION)
    if code.kind != 0:
        return ["PEF section 0 is not the code section"]
    limit = code.packed_size
    ordered = sorted(spans.items())
    for offset, size in ordered:
        if offset % 4 or size <= 0 or size % 4 or offset + size > limit:
            problems.append(f"{FUNCTIONS_TSV}: {offset:#x}+{size:#x} is not an in-bounds code span")
    for (offset, size), (following, _) in zip(ordered, ordered[1:]):
        if following < offset + size:
            problems.append(f"{FUNCTIONS_TSV}: {offset:#x}+{size:#x} overlaps {following:#x}")
    for offset, count in Counter(row.offset for row in runtime).items():
        if count > 1:
            problems.append(f"{RUNTIME_MAP_TSV}: duplicate offset {offset:#x}; use {RUNTIME_ALIASES_TSV}")
    runtime_offsets = {row.offset for row in runtime}
    for row in runtime:
        if row.offset not in spans:
            problems.append(f"{RUNTIME_MAP_TSV}: {row.name} at {row.offset:#x} has no {FUNCTIONS_TSV} row")
        if row.owner not in OWNERS:
            problems.append(f"{RUNTIME_MAP_TSV}: {row.name} has unknown owner {row.owner!r}")
        if row.call_kind not in CALL_KINDS:
            problems.append(f"{RUNTIME_MAP_TSV}: {row.name} has unknown call kind {row.call_kind!r}")
        if not row.evidence.strip():
            problems.append(f"{RUNTIME_MAP_TSV}: {row.name} lacks evidence")
    for alias in aliases:
        if alias.offset not in runtime_offsets:
            problems.append(f"{RUNTIME_ALIASES_TSV}: {alias.name} at {alias.offset:#x} has no runtime-map row")
    proven = {(address.offset, name, library)
              for address, library, name in glue_module.stubs(pef)}
    for stub in glue:
        if (stub.offset, stub.name, stub.library) not in proven:
            problems.append(f"{GLUE_MAP_TSV}: {stub.name} at {stub.offset:#x} is not a loader-proven import stub")
        if spans.get(stub.offset) != glue_module.GLUE_SIZE:
            problems.append(f"{GLUE_MAP_TSV}: {stub.name} at {stub.offset:#x} needs a "
                            f"{glue_module.GLUE_SIZE:#x}-byte {FUNCTIONS_TSV} row")
        if stub.offset in runtime_offsets:
            problems.append(f"{GLUE_MAP_TSV}: {stub.name} at {stub.offset:#x} is also a runtime label")
    names = Counter([*(row.name for row in runtime), *(alias.name for alias in aliases),
                     *(stub.name for stub in glue)])
    for name, count in names.items():
        if count > 1:
            problems.append(f"runtime/glue maps: duplicate name {name}")
    return problems
