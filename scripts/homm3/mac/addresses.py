"""Source-owned Mac function addresses and the pinned-PEF function inventory.

``MAC_ADDRESS(offset, size)`` is the Mac counterpart of ``VA(addr, size)``.
Its values are the start and extent relative to the pinned PEF's single code
section (section 0). It claims functions only; globals, constants and
literals keep their reviewed Mac data inventories.

Placement is the identity join:

  VA(0x004d8720, 0x568) MAC_ADDRESS(0x0f3fe4, 0x568)
                   the Mac body of the function this VA claims. The pair is
                   written on one line so every existing VA-to-declarator
                   scanner keeps its adjacency.
  MAC_ADDRESS(0x0f1db4, 0x60)       (own line, directly above a definition)
                   a source function with no Windows VA.
  VA_COMPGEN(...) MAC_COMPGEN_ADDRESS(offset, size, KIND, Owner)
                   a compiler-generated body; kind and owner must agree with
                   the VA_COMPGEN it sits beside. A standalone
                   MAC_COMPGEN_ADDRESS names a Mac-only generated body.

The executable-wide tables mirror ``config/retail``:

  config/mac/functions.tsv        offset, size - every verified code span
  config/mac/runtime-map.tsv      offset, name - library/runtime labels
  config/mac/runtime-aliases.tsv  offset, name - folded extra names
  config/mac/dispositions.tsv     identity, disposition, evidence

Every source address claim and runtime label must resolve to exactly one
``functions.tsv`` row, exactly as a retail ``VA()`` must land on a carved
``config/retail/functions.tsv`` entry. The reviewed TOML inventories remain
the legacy selected-body build input; ``migrate`` copies their spans into
source annotations and these tables, and ``check`` proves both agree.
"""
from __future__ import annotations

from collections import Counter, defaultdict
from dataclasses import dataclass, replace
from functools import lru_cache
import hashlib
from pathlib import Path
import re
import tomllib

from homm3.core.tsv import write as write_tsv

from homm3.mac.tables import (CODE_SECTION, DISPOSITIONS_TSV, FUNCTIONS_TSV, Alias,
                               GlueStub, RuntimeLabel, TableError, read_aliases,
                               read_dispositions, read_functions, read_glue, read_runtime,
                               runtime_owner)

MAC_HEAD_RE = re.compile(r"\bMAC_ADDRESS\s*\(")
MAC_COMPGEN_HEAD_RE = re.compile(r"\bMAC_COMPGEN_ADDRESS\s*\(")
VA_HEAD_RE = re.compile(r"(?m)^[ \t]*VA\s*\(")
VA_COMPGEN_HEAD_RE = re.compile(r"(?m)^[ \t]*VA_COMPGEN\s*\(")
HEX_RE = re.compile(r"0x[0-9a-fA-F]+$")
SIZE_RE = re.compile(r"0x[0-9a-fA-F]+$|\d+$")
IDENT_RE = re.compile(r"[A-Za-z_]\w*$")
#: Source lines a backward or forward declarator walk steps over.
ANNOTATION_LINE_PREFIXES = ("MAC_ADDRESS(", "MAC_COMPGEN_ADDRESS(")


class AddressError(TableError):
    pass


@dataclass(frozen=True)
class Claim:
    path: str             # repository-relative source or header
    line: int
    offset: int
    size: int
    windows_va: int | None = None
    compgen: tuple[str, str] | None = None   # (kind, owner)
    label: str = ""       # declarator working label for standalone claims
    anchor: int = 0       # character offset of the owning VA or definition
    parameters: str = ""  # standalone claims: separates overloads

    @property
    def identity(self) -> str:
        if self.compgen is not None:
            if self.windows_va is not None:
                return f"compgen:0x{self.windows_va:08x}"
            return f"compgen:{self.path}:{self.compgen[0]}:{self.compgen[1]}"
        if self.windows_va is not None:
            return f"va:0x{self.windows_va:08x}"
        return f"source:{self.path}:{self.label}{self.parameters}"

    @property
    def where(self) -> str:
        return f"{self.path}:{self.line}"


@dataclass(frozen=True)
class WindowsClaim:
    path: str
    line: int
    va: int
    size: int
    compgen: tuple[str, str] | None
    label: str
    mac: Claim | None = None


def spell(offset: int, size: int) -> str:
    return f"MAC_ADDRESS(0x{offset:06x}, 0x{size:x})"


def spell_compgen(offset: int, size: int, kind: str, owner: str) -> str:
    return f"MAC_COMPGEN_ADDRESS(0x{offset:06x}, 0x{size:x}, {kind}, {owner})"


# --- lexical scan -----------------------------------------------------------

def _mask(text: str) -> str:
    from homm3.retail_labels.source import mask_lexical_noise
    return mask_lexical_noise(text)


def _invocations(masked: str, head: re.Pattern, raw: str):
    from homm3.retail_labels.source import macro_invocations
    return macro_invocations(masked, head, raw)


def _line_of(text: str, offset: int) -> int:
    return text.count("\n", 0, offset) + 1


def _follower(masked: str, after: int) -> int | None:
    """Offset of the first code line after `after`, skipping blank/comment
    lines and other Mac annotation lines."""
    cursor = masked.find("\n", after)
    while cursor >= 0:
        start = cursor + 1
        end = masked.find("\n", start)
        line = masked[start:end if end >= 0 else len(masked)]
        stripped = line.strip()
        if stripped and not stripped.startswith(ANNOTATION_LINE_PREFIXES):
            return start + len(line) - len(line.lstrip())
        cursor = end
    return None


def _declarator(declaration: str) -> tuple[str, str]:
    """(name, parameter list) of a function declarator; ('', '') otherwise."""
    head = re.split(r"[;{]", declaration, maxsplit=1)[0]
    if "(" not in head:
        return "", ""
    before = head.split("(", 1)[0]
    if "=" in before:
        return "", ""  # an initialized object, not a function declarator
    operator = re.search(r"((?:[~\w]+::)*operator\s*\S+?)\s*$", before)
    if operator:
        name = re.sub(r"\s+", "", operator.group(1))
    else:
        names = re.findall(r"[~\w]+(?:::[~\w]+)*", before)
        name = names[-1] if names else ""
    opening = len(before)
    depth, index = 0, opening
    while index < len(declaration):
        if declaration[index] == "(":
            depth += 1
        elif declaration[index] == ")":
            depth -= 1
            if depth == 0:
                break
        index += 1
    parameters = " ".join(declaration[opening:index + 1].split())
    return name, re.sub(r"\s*([(),*&])\s*", r"\1", parameters)


def _declarator_label(declaration: str) -> str:
    """Working label from a function declarator; empty for non-functions."""
    return _declarator(declaration)[0]


def scan_text(raw: str, path: str) -> tuple[list[Claim], list[WindowsClaim], list[str]]:
    """Mac and Windows function claims of one file, and their defects."""
    masked = _mask(raw)
    problems: list[str] = []
    windows: list[tuple[int, int, WindowsClaim]] = []   # (start, end, claim)
    owned_definitions: dict[int | None, int] = {}         # declarator -> its VA
    for head, compgen in ((VA_HEAD_RE, False), (VA_COMPGEN_HEAD_RE, True)):
        for start, end, args, _ in _invocations(masked, head, raw):
            if end is None or len(args) != (4 if compgen else 2):
                continue  # retail_labels owns VA syntax errors
            try:
                va, size = int(args[0], 16), int(args[1], 0)
            except ValueError:
                continue
            label = ""
            if not compgen:
                follower = _follower(masked, end)
                if follower is not None:
                    label = _declarator_label(masked[follower:follower + 2000])
            if not compgen:
                owned_definitions[_follower(masked, end)] = va
            windows.append((start, end, WindowsClaim(
                path, _line_of(raw, start), va, size,
                (args[2], args[3]) if compgen else None, label)))
    by_end = {end: claim for _start, end, claim in windows}

    claims: list[Claim] = []
    for head, compgen in ((MAC_HEAD_RE, False), (MAC_COMPGEN_HEAD_RE, True)):
        macro = "MAC_COMPGEN_ADDRESS" if compgen else "MAC_ADDRESS"
        for start, end, args, _ in _invocations(masked, head, raw):
            where = f"{path}:{_line_of(raw, start)}"
            if masked[masked.rfind("\n", 0, start) + 1:start].lstrip().startswith("#"):
                continue  # the macro's own definition, or a directive mentioning it
            if end is None:
                problems.append(f"{where}: {macro}( is never closed")
                continue
            arity = 4 if compgen else 2
            if len(args) != arity:
                problems.append(f"{where}: {macro} takes {arity} arguments, found {len(args)}")
                continue
            if not HEX_RE.match(args[0]) or not SIZE_RE.match(args[1]):
                problems.append(f"{where}: {macro} offset/size are malformed: {args[:2]!r}")
                continue
            if compgen and not (IDENT_RE.match(args[2]) and IDENT_RE.match(args[3])):
                problems.append(f"{where}: MAC_COMPGEN_ADDRESS kind/owner are malformed")
                continue
            offset, size = int(args[0], 16), int(args[1], 0)
            if size <= 0 or offset % 4 or size % 4:
                problems.append(f"{where}: {macro} span {offset:#x}+{size:#x} is not a word-aligned code span")
                continue
            # A same-line VA/VA_COMPGEN pairs with this claim.
            line_start = masked.rfind("\n", 0, start) + 1
            before = masked[line_start:start]
            partner = None
            if before.strip():
                stripped = before.rstrip()
                partner = by_end.get(line_start + len(stripped) - 1)
                if partner is None:
                    problems.append(f"{where}: {macro} must follow its VA/VA_COMPGEN on the same line "
                                    "or stand alone above a definition")
                    continue
            if partner is not None:
                if compgen != (partner.compgen is not None):
                    wanted = "MAC_COMPGEN_ADDRESS" if partner.compgen else "MAC_ADDRESS"
                    problems.append(f"{where}: use {wanted} beside this "
                                    f"{'VA_COMPGEN' if partner.compgen else 'VA'}")
                    continue
                if compgen and partner.compgen != (args[2], args[3]):
                    problems.append(f"{where}: MAC_COMPGEN_ADDRESS {args[2]}/{args[3]} disagrees "
                                    f"with VA_COMPGEN {partner.compgen[0]}/{partner.compgen[1]}")
                    continue
                claims.append(Claim(path, _line_of(raw, start), offset, size, partner.va,
                                    partner.compgen, partner.label, start))
                continue
            if compgen:
                claims.append(Claim(path, _line_of(raw, start), offset, size, None,
                                    (args[2], args[3]), "", start))
                continue
            follower = _follower(masked, end)
            if follower is None:
                problems.append(f"{where}: orphan MAC_ADDRESS - no definition follows")
                continue
            if re.match(r"(?:VA|VA_COMPGEN)\s*\(", masked[follower:]):
                problems.append(f"{where}: write MAC_ADDRESS on the line of the VA it pairs with")
                continue
            if ACCESS_LABEL_RE.match(masked, follower):
                problems.append(f"{where}: write MAC_ADDRESS below the access label, "
                                "directly above its definition")
                continue
            if follower in owned_definitions:
                problems.append(f"{where}: this definition is VA({owned_definitions[follower]:#010x}); "
                                "write its Mac address on that VA line")
                continue
            label, parameters = _declarator(masked[follower:follower + 2000])
            if not label:
                problems.append(f"{where}: MAC_ADDRESS claims functions only; the following "
                                "declaration is not a function")
                continue
            claims.append(Claim(path, _line_of(raw, start), offset, size, None, None, label,
                                follower, parameters))
    claims.sort(key=lambda claim: claim.anchor)
    paired = {claim.windows_va: claim for claim in claims if claim.windows_va is not None}
    windows_claims = [replace(claim, mac=paired.get(claim.va))
                      for _start, _end, claim in sorted(windows, key=lambda item: item[0])]
    return claims, windows_claims, problems


def source_paths(root: Path) -> list[Path]:
    """Authored sources directly under src/, and project headers."""
    paths = sorted(path for path in (root / "src").iterdir()
                   if path.is_file() and path.suffix.lower() in {".c", ".cc", ".cpp", ".cxx"})
    paths += sorted(path for path in (root / "include").rglob("*")
                    if path.is_file() and path.suffix.lower() in {".h", ".hpp", ".inl"})
    return paths


def scan(root: Path) -> tuple[list[Claim], list[WindowsClaim], list[str]]:
    claims, windows, problems = [], [], []
    for path in source_paths(root):
        raw = path.read_text(errors="replace")
        if "MAC_" not in raw and "VA" not in raw:
            continue
        found, retail, defects = scan_text(raw, path.relative_to(root).as_posix())
        claims += found
        windows += retail
        problems += defects
    return claims, windows, problems + claim_problems(claims)


def claim_problems(claims: list[Claim]) -> list[str]:
    """Ownership defects visible without the target: duplicates and overlap."""
    problems = []
    for identity, count in Counter(claim.identity for claim in claims).items():
        if count > 1:
            where = ", ".join(claim.where for claim in claims if claim.identity == identity)
            problems.append(f"{identity}: {count} Mac address claims ({where})")
    ordered = sorted(claims, key=lambda claim: (claim.offset, claim.size))
    for previous, current in zip(ordered, ordered[1:]):
        if current.offset < previous.offset + previous.size:
            problems.append(f"{current.where}: Mac span {current.offset:#x}+{current.size:#x} "
                            f"overlaps {previous.where} {previous.offset:#x}+{previous.size:#x}")
    return problems


# --- reviewed TOML inventories ----------------------------------------------

@dataclass(frozen=True)
class Reviewed:
    """One reviewed Mac function span from the legacy TOML inventories."""
    kind: str                  # pair | reference | helper | compgen | runtime
    offset: int
    size: int
    sha256: str | None
    source: Path | None = None
    windows_va: int | None = None
    helper: str | None = None
    in_class: bool = False
    compgen: tuple[str, str] | None = None
    symbol: str | None = None
    evidence: str = ""
    call_kind: str = "direct"


def reviewed(root: Path) -> list[Reviewed]:
    """Pairs, references and runtime rows, validated by their own loaders."""
    from homm3.mac import references
    from homm3.mac.source import load_pairs
    rows = []
    for pair in load_pairs(root):
        if pair.mac_section != CODE_SECTION:
            raise AddressError(f"Mac pair {pair.retail_va:#x} is outside code section 0")
        rows.append(Reviewed("pair", pair.mac_offset, pair.mac_size, pair.target_sha256,
                             pair.source, pair.retail_va, symbol=pair.mac_symbol))
    raw_rows = {}
    for path in sorted((root / "config/mac/references").glob("*.toml")):
        inventory = tomllib.loads(path.read_text())
        for row in inventory.get("functions", []):
            raw_rows[("va", row["retail_va"])] = row
        for row in inventory.get("helpers", []):
            raw_rows[("helper", row["unit"], row["source_helper"],
                      row.get("source"))] = row
    paired = {row.windows_va for row in rows}
    for ref in references.load(root):
        if ref.retail_va in paired:
            continue  # references.load proved it repeats the admitted pair
        if ref.mac_section != CODE_SECTION:
            raise AddressError(f"Mac reference {ref.identity} is outside code section 0")
        if ref.retail_va is None:
            raw = raw_rows.get(("helper", ref.unit, ref.source_helper,
                                ref.source.relative_to(root).as_posix()))
            if raw is None:
                raw = raw_rows.get(("helper", ref.unit, ref.source_helper, None), {})
            rows.append(Reviewed("helper", ref.mac_offset, ref.mac_size, ref.target_sha256,
                                 ref.source, None, ref.source_helper,
                                 bool(raw.get("in_class", False)), symbol=ref.mac_symbol))
            continue
        raw = raw_rows.get(("va", ref.retail_va), {})
        compgen = ((raw["compgen_kind"], raw["compgen_type"])
                   if raw.get("compgen_kind") else None)
        rows.append(Reviewed("compgen" if compgen else "reference", ref.mac_offset,
                             ref.mac_size, ref.target_sha256, ref.source, ref.retail_va,
                             compgen=compgen, symbol=ref.mac_symbol))
    for row in legacy_runtime(root):
        rows.append(Reviewed("runtime", row["mac_offset"], row["mac_size"], row["sha256"],
                             symbol=row["symbol"], evidence=row["evidence"],
                             call_kind=row.get("call_kind", "direct")))
    return rows


def legacy_runtime(root: Path) -> list[dict]:
    """Runtime function rows still in config/mac/runtime.toml, awaiting migrate.

    The runtime maps own these labels; a TOML row appears only when a worker
    branch adds one before `homm3 mac migrate` moves it.
    """
    path = root / "config/mac/runtime.toml"
    rows = tomllib.loads(path.read_text()).get("functions", []) if path.is_file() else []
    for row in rows:
        if row["mac_section"] != CODE_SECTION:
            raise AddressError(f"Mac runtime {row['symbol']} is outside code section 0")
        if not row["evidence"].strip():
            raise AddressError(f"Mac runtime {row['symbol']} lacks evidence")
    return rows


# --- migration ---------------------------------------------------------------

def _windows_ends(text: str, masked: str) -> dict[tuple[bool, int], list[int]]:
    """(is_compgen, va) -> closing-paren offsets of that Windows claim."""
    ends: dict[tuple[bool, int], list[int]] = defaultdict(list)
    for head, compgen in ((VA_HEAD_RE, False), (VA_COMPGEN_HEAD_RE, True)):
        for _start, end, args, _ in _invocations(masked, head, text):
            if end is not None and args and HEX_RE.match(args[0]):
                ends[(compgen, int(args[0], 16))].append(end)
    return ends


def _after_windows_claim(text: str, end: int, spelling: str) -> tuple[int, str] | None:
    """Insertion after the claim's closing paren, or None when already equal."""
    line_end = text.find("\n", end)
    tail = text[end + 1:line_end if line_end >= 0 else len(text)]
    existing = re.match(r"\s*(MAC_(?:COMPGEN_)?ADDRESS\([^)]*\))", tail)
    if existing:
        if re.sub(r"\s+", "", existing.group(1)) != re.sub(r"\s+", "", spelling):
            raise AddressError(f"source has {existing.group(1)}, reviewed {spelling}")
        return None
    return end + 1, " " + spelling


ACCESS_LABEL_RE = re.compile(r"(?:(?:public|protected|private)\s*:(?!:)\s*)+")


def _above_definition(text: str, start: int, spelling: str) -> tuple[int, str] | None:
    # An in-class finder match can begin at a preceding access label; the
    # claim belongs on the declarator's own line, below any comment block.
    masked = _mask(text)
    label = ACCESS_LABEL_RE.match(masked, start)
    if label:
        start = label.end()
        while masked[start].isspace():
            start += 1
    line_start = text.rfind("\n", 0, start) + 1
    indent = text[line_start:start]
    if indent.strip():
        raise AddressError(f"definition does not begin its line: {text[line_start:start + 40]!r}")
    previous_start = text.rfind("\n", 0, max(line_start - 1, 0)) + 1
    previous = text[previous_start:max(line_start - 1, 0)].strip()
    if previous.startswith("MAC_ADDRESS("):
        if re.sub(r"\s+", "", previous) != re.sub(r"\s+", "", spelling):
            raise AddressError(f"source has {previous}, reviewed {spelling}")
        return None
    return line_start, indent + spelling + "\n"


def migrate(root: Path, pef) -> dict[str, int]:
    """Copy reviewed TOML spans into source annotations and the TSV tables.

    Idempotent and all-or-nothing: every edit is computed against the current
    text and every table is validated before anything is written. An equal
    existing annotation or row is kept; a different one fails. Rows already
    present in the TSVs but absent from TOML are kept.
    """
    from homm3.mac.source import class_header_helper, source_helper
    rows = reviewed(root)
    counts: Counter = Counter()
    for row in rows:
        data = pef.code(CODE_SECTION, row.offset, row.size)
        if row.sha256 and hashlib.sha256(data).hexdigest() != row.sha256:
            raise AddressError(f"reviewed {row.kind} at {row.offset:#x} no longer hashes to its TOML digest")

    from homm3.mac import glue as glue_module
    spans = read_functions(root)
    detected = [GlueStub(address.offset, name, library)
                for address, library, name in glue_module.stubs(pef)]
    wanted = [(row.offset, row.size, f"reviewed {row.kind}") for row in rows]
    wanted += [(stub.offset, glue_module.GLUE_SIZE, "import glue") for stub in detected]
    for offset, size, what in wanted:
        if spans.get(offset, size) != size:
            raise AddressError(f"{FUNCTIONS_TSV}: {offset:#x} has size {spans[offset]:#x}, "
                               f"{what} has {size:#x}")
        spans[offset] = size
    ordered = sorted(spans.items())
    for (offset, size), (following, _) in zip(ordered, ordered[1:]):
        if following < offset + size:
            raise AddressError(f"{FUNCTIONS_TSV}: {offset:#x}+{size:#x} overlaps {following:#x}")

    by_file: dict[Path, list[Reviewed]] = defaultdict(list)
    for row in rows:
        if row.kind != "runtime":
            by_file[row.source].append(row)
    rewritten: dict[Path, str] = {}
    for path, file_rows in sorted(by_file.items()):
        text = path.read_text()
        ends = _windows_ends(text, _mask(text))
        insertions = []
        for row in file_rows:
            try:
                if row.kind == "helper":
                    finder = (class_header_helper if path.suffix == ".h" or row.in_class
                              else source_helper)
                    start, _signature, _body = finder(text, row.helper, path)
                    edit = _above_definition(text, start, spell(row.offset, row.size))
                else:
                    compgen = row.kind == "compgen"
                    sites = ends.get((compgen, row.windows_va), [])
                    if len(sites) != 1:
                        raise AddressError(f"expected one {'VA_COMPGEN' if compgen else 'VA'}"
                                           f"({row.windows_va:#010x}) claim, found {len(sites)}")
                    spelling = (spell_compgen(row.offset, row.size, *row.compgen) if compgen
                                else spell(row.offset, row.size))
                    edit = _after_windows_claim(text, sites[0], spelling)
            except ValueError as exc:
                raise AddressError(f"{path.relative_to(root)}: {exc}") from exc
            counts["annotated" if edit else "unchanged"] += 1
            if edit:
                insertions.append(edit)
        if len({at for at, _ in insertions}) != len(insertions):
            raise AddressError(f"{path.relative_to(root)}: two Mac claims at one source position")
        for at, inserted in sorted(insertions, reverse=True):
            text = text[:at] + inserted + text[at:]
        if insertions:
            rewritten[path] = text

    primary = {label.offset: label for label in read_runtime(root)}
    aliases = {(alias.offset, alias.name): alias for alias in read_aliases(root)}
    for row in rows:
        if row.kind != "runtime":
            continue
        label = primary.get(row.offset)
        if label is None:
            primary[row.offset] = RuntimeLabel(row.offset, row.symbol, runtime_owner(root, row.symbol),
                                               row.call_kind, row.evidence)
        elif label.name != row.symbol and (row.offset, row.symbol) not in aliases:
            aliases[(row.offset, row.symbol)] = Alias(row.offset, row.symbol, row.evidence)
    glue = {stub.offset: stub for stub in read_glue(root)}
    for stub in detected:
        glue.setdefault(stub.offset, stub)

    for path, text in rewritten.items():
        path.write_text(text)
    counts["files"] = len(rewritten)
    from homm3.mac import tables
    tables.write(root, spans, list(primary.values()), list(aliases.values()), list(glue.values()))
    counts["functions"] = len(ordered)
    counts["runtime"] = len(primary)
    counts["aliases"] = len(aliases)
    counts["glue"] = len(glue)
    return dict(counts)


# --- verification -----------------------------------------------------------

def check(root: Path, pef, claims: list[Claim], windows: list[WindowsClaim]) -> list[str]:
    """Tables, source claims against them, and agreement with review rows."""
    from homm3.mac import tables
    problems = tables.validate(root, pef)
    try:
        spans = read_functions(root)
        runtime = read_runtime(root)
        aliases = read_aliases(root)
        glue = read_glue(root)
        dispositions = read_dispositions(root)
    except ValueError as exc:
        return [*problems, str(exc)]
    limit = pef.section(CODE_SECTION).packed_size
    library = {row.offset: row.name for row in runtime}
    library.update({stub.offset: stub.name for stub in glue})
    library.update({row.offset: row.name for row in tables.read_zlib(root)})
    for claim in claims:
        if claim.offset + claim.size > limit:
            problems.append(f"{claim.where}: Mac span {claim.offset:#x}+{claim.size:#x} "
                            f"exceeds code section size {limit:#x}")
        if spans.get(claim.offset) != claim.size:
            problems.append(f"{claim.where}: {claim.identity} Mac span {claim.offset:#x}+{claim.size:#x} "
                            f"is not a {FUNCTIONS_TSV} row")
        if claim.offset in library:
            problems.append(f"{claim.where}: {claim.identity} Mac span {claim.offset:#x} is also "
                            f"library label {library[claim.offset]}")
    runtime_names = {(row.offset, row.name) for row in runtime}
    runtime_names |= {(alias.offset, alias.name) for alias in aliases}
    runtime_spans = {row.offset for row in runtime}

    by_identity = {claim.identity: claim for claim in claims}
    helpers = defaultdict(list)
    for claim in claims:
        if claim.windows_va is None and claim.compgen is None:
            helpers[(claim.path, claim.offset, claim.size)].append(claim)
    for row in reviewed(root):
        label = row.symbol or (f"{row.windows_va:#010x}" if row.windows_va else row.helper)
        data = pef.code(CODE_SECTION, row.offset, row.size)
        if row.sha256 and hashlib.sha256(data).hexdigest() != row.sha256:
            problems.append(f"reviewed {row.kind} {label}: target bytes differ from the TOML digest")
        if spans.get(row.offset) != row.size:
            problems.append(f"reviewed {row.kind} {label}: {row.offset:#x}+{row.size:#x} "
                            f"is not a {FUNCTIONS_TSV} row")
        if row.kind == "runtime":
            if (row.offset, row.symbol) not in runtime_names or row.offset not in runtime_spans:
                problems.append(f"reviewed runtime {row.symbol}: missing from the runtime maps")
            continue
        if row.kind == "helper":
            relative = row.source.relative_to(root).as_posix()
            if not helpers.get((relative, row.offset, row.size)):
                problems.append(f"reviewed helper {relative}:{row.helper}: no MAC_ADDRESS "
                                f"{row.offset:#x}+{row.size:#x} above its definition")
            continue
        identity = (f"compgen:0x{row.windows_va:08x}" if row.kind == "compgen"
                    else f"va:0x{row.windows_va:08x}")
        claim = by_identity.get(identity)
        if claim is None:
            problems.append(f"reviewed {row.kind} {identity}: source has no Mac address claim")
        elif (claim.offset, claim.size) != (row.offset, row.size):
            problems.append(f"{claim.where}: {identity} claims {claim.offset:#x}+{claim.size:#x}, "
                            f"reviewed {row.offset:#x}+{row.size:#x}")
    identities = {claim.identity for claim in claims}
    identities |= {f"{'compgen' if w.compgen else 'va'}:0x{w.va:08x}" for w in windows}
    for identity, (disposition, _evidence) in dispositions.items():
        if identity in by_identity:
            problems.append(f"{DISPOSITIONS_TSV}: {identity} is {disposition} but has a Mac address")
        elif identity.startswith(("va:", "compgen:0x")) and identity not in identities:
            problems.append(f"{DISPOSITIONS_TSV}: {identity} names no source claim")
    return problems


# --- parity index ------------------------------------------------------------

@lru_cache(maxsize=4)
def _unit_sources(root: Path) -> dict[str, str]:
    """Source or registered fragment path -> owning unit name."""
    from homm3 import manifest
    from homm3.match.source_ownership import fragment_owners
    units = {entry["source"]: name for name, entry in
             manifest.by_unit(root / "config/units.toml").items()}
    fragments = {fragment: units.get(owner, "") for fragment, (owner, _at)
                 in fragment_owners(root).items()}
    return {**fragments, **units}


def unit_of(root: Path, path: str) -> str:
    return _unit_sources(root).get(path, "")


def index(root: Path, claims: list[Claim], windows: list[WindowsClaim],
          definitions=None) -> tuple[list[dict], list[str]]:
    """One row per source-owned function: Windows claim and/or definition.

    Rows keyed by Windows VA come from the lexical claim scan, so carcass
    stubs the compiler never sees still appear. With AST `definitions`
    (homm3.match.source_ownership), every other authored function body is
    added as a source-only row, and standalone MAC_ADDRESS claims must bind
    to one of them.
    """
    dispositions = read_dispositions(root)
    rows = []
    problems = []

    def state(identity: str, claim: Claim | None) -> str:
        if claim is not None:
            return "located"
        return dispositions.get(identity, ("unlocated",))[0]

    for claim in windows:
        identity = f"{'compgen' if claim.compgen else 'va'}:0x{claim.va:08x}"
        name = (f"{claim.compgen[1]}::{claim.compgen[0]}" if claim.compgen else claim.label)
        rows.append(dict(identity=identity, unit=unit_of(root, claim.path), file=claim.path,
                         line=claim.line, name=name, windows_va=f"0x{claim.va:08x}",
                         mac_offset=f"0x{claim.mac.offset:06x}" if claim.mac else "",
                         mac_size=f"0x{claim.mac.size:x}" if claim.mac else "",
                         state=state(identity, claim.mac)))
    standalone = [claim for claim in claims if claim.windows_va is None]
    bound: set[str] = set()
    if definitions is not None:
        # The analysis arm annotates each definition with its own MAC_ADDRESS;
        # bind by that attribute, never by nearby lines or names.
        by_span = {(claim.path, claim.offset, claim.size): claim
                   for claim in standalone if claim.compgen is None}
        paired = {claim.windows_va: claim for claim in claims if claim.windows_va is not None}
        seen = set()
        for definition in definitions:
            if definition.va is not None and definition.mac_offset is not None:
                claim = paired.get(definition.va)
                if claim is None or (claim.offset, claim.size) != (definition.mac_offset, definition.mac_size):
                    problems.append(f"{definition.file}:{definition.line}: {definition.name} carries "
                                    f"MAC_ADDRESS({definition.mac_offset:#x}, {definition.mac_size:#x}) "
                                    f"that its VA({definition.va:#010x}) claim does not")
            if definition.va is not None or definition.additional_instances:
                continue
            key = (definition.file, definition.offset)
            if key in seen:
                continue
            seen.add(key)
            # Template bodies have no emitted symbol; their written signature
            # still separates overloads.
            identity = (f"source:{definition.file}:"
                        f"{definition.mangled or definition.name + ' ' + definition.signature}")
            claim = None
            if definition.mac_offset is not None:
                claim = by_span.get((definition.file, definition.mac_offset, definition.mac_size))
                if claim is None:
                    problems.append(f"{definition.file}:{definition.line}: {definition.name} carries "
                                    f"MAC_ADDRESS({definition.mac_offset:#x}, {definition.mac_size:#x}) "
                                    "with no matching source claim")
                else:
                    bound.add(claim.identity)
            rows.append(dict(identity=identity, unit=unit_of(root, definition.source_owner or definition.file),
                             file=definition.file, line=definition.line, name=definition.name,
                             windows_va="",
                             mac_offset=f"0x{claim.offset:06x}" if claim else "",
                             mac_size=f"0x{claim.size:x}" if claim else "",
                             state=state(identity, claim) if claim else
                             dispositions.get(identity, dispositions.get(
                                 f"source:{definition.file}:{definition.name}", ("unlocated",)))[0]))
    for claim in standalone:
        if claim.compgen is not None:
            rows.append(dict(identity=claim.identity, unit=unit_of(root, claim.path), file=claim.path,
                             line=claim.line, name=f"{claim.compgen[1]}::{claim.compgen[0]}",
                             windows_va="", mac_offset=f"0x{claim.offset:06x}",
                             mac_size=f"0x{claim.size:x}", state="located"))
        elif definitions is not None and claim.identity not in bound:
            problems.append(f"{claim.where}: MAC_ADDRESS binds to no unique authored definition "
                            f"({claim.label})")
        elif definitions is None:
            rows.append(dict(identity=claim.identity, unit=unit_of(root, claim.path), file=claim.path,
                             line=claim.line, name=claim.label, windows_va="",
                             mac_offset=f"0x{claim.offset:06x}", mac_size=f"0x{claim.size:x}",
                             state="located"))
    counts = Counter(row["identity"] for row in rows)
    for identity, count in counts.items():
        if count > 1:
            problems.append(f"parity: {identity} appears {count} times")
    rows.sort(key=lambda row: (row["unit"] or "~", row["file"], int(row["line"])))
    return rows, problems


def coverage(root: Path, pef, claims: list[Claim]) -> dict:
    """Code-section byte accounting over the verified function inventory.

    Rows are split by owner: a source claim, a runtime label, or neither.
    Bytes outside every row stay `unresolved`; they are never dropped from
    the denominator.
    """
    spans = read_functions(root)
    source = {claim.offset for claim in claims}
    runtime = {label.offset for label in read_runtime(root)}
    glue = {stub.offset for stub in read_glue(root)}
    from homm3.mac import tables
    vendor = {row.offset for row in tables.read_zlib(root)}
    counts, sizes = Counter(), Counter()
    for offset, size in spans.items():
        owner = ("source" if offset in source else "runtime" if offset in runtime
                 else "glue" if offset in glue else "vendor" if offset in vendor else "unowned")
        counts[owner] += 1
        sizes[owner] += size
    code = pef.section(CODE_SECTION).packed_size
    covered = sum(spans.values())
    return {"code_section_bytes": code, "function_rows": len(spans),
            "rows_by_owner": dict(counts), "bytes_by_owner": dict(sizes),
            "covered_bytes": covered, "unresolved_bytes": code - covered,
            "covered_percent": round(100.0 * covered / code, 3) if code else 0.0}


PARITY_COLUMNS = ["identity", "unit", "file", "line", "name", "windows_va",
                  "mac_offset", "mac_size", "state"]


def write_index(root: Path, rows: list[dict]) -> Path:
    path = root / "build/gen/mac/parity.tsv"
    write_tsv(path, ["# GENERATED by `homm3 mac parity`; do not edit."], PARITY_COLUMNS,
              [{key: str(row[key]) for key in PARITY_COLUMNS} for row in rows])
    return path


def summary(rows: list[dict]) -> dict:
    by_unit = defaultdict(Counter)
    for row in rows:
        by_unit[row["unit"] or "(header)"][row["state"]] += 1
    totals = Counter(row["state"] for row in rows)
    return {"totals": dict(totals), "units": {unit: dict(counts)
                                              for unit, counts in sorted(by_unit.items())}}
