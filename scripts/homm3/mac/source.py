"""Extract the one authored C++ body associated with an existing Windows VA."""
from __future__ import annotations

from dataclasses import dataclass, replace
from functools import lru_cache
from pathlib import Path
import re
import tomllib

from homm3 import manifest as units_manifest
from homm3.mac import profiles


class SourceError(ValueError):
    pass


@lru_cache(maxsize=64)
def _masked_source(text: str) -> str:
    """Reuse lexical scans by complete contents, never by path or timestamp.

    Large TUs can own many pairs/callees. Re-reading a changed file gives a new
    key immediately, including same-size edits with a preserved timestamp.
    """
    from homm3.retail_labels.source import mask_lexical_noise
    return mask_lexical_noise(text)


@dataclass(frozen=True)
class Pair:
    retail_va: int
    unit: str
    source: Path
    signature: str
    shim: Path
    mac_section: int
    mac_offset: int
    mac_size: int
    mac_symbol: str
    evidence: str
    data: tuple[int, ...] = ()
    compile_group: str | None = None
    target_sha256: str | None = None
    project_root: Path | None = None


@dataclass(frozen=True)
class DataPair:
    retail_va: int
    name: str
    definition: str
    mac_section: int
    mac_offset: int
    mac_size: int
    sha256: str
    declaration_only: bool = False
    mac_symbol: str = ""
    local_owner_va: int | None = None
    read_only: bool = False
    same_tu_definition: bool = False


def data_rows(root: Path, kind: str) -> list[dict]:
    """Shared and worker-owned data inventories use the same validation path."""
    paths = [root / "config/mac/data.toml", *sorted((root / "config/mac/data").glob("*.toml"))]
    rows = []
    for path in paths:
        if path.is_file():
            rows.extend(tomllib.loads(path.read_text()).get(kind, []))
    return rows


def _data_scope(masked: str, position: int) -> tuple[str, ...]:
    """Resolve ordinary named class/namespace scopes; reject local/complex ones.

    This is a deliberately bounded declarator reader, not a C++ parser. Never
    turn a function-local or template member into an unqualified global merely
    because its DATA declaration has a familiar spelling.
    """
    stack = []
    previous = 0
    for token in re.finditer(r'[{};]', masked[:position]):
        if token[0] == "{":
            head = masked[previous:token.start()]
            match = re.search(r'\b(?:class|struct|namespace)\s+(\w+)\s*(?::[^{};<>]*)?$', head)
            scope = match[1] if match and not re.search(r'[<>()=]', head) else None
            stack.append(scope)
        elif token[0] == "}":
            if not stack:
                raise SourceError("unbalanced source scope before DATA claim")
            stack.pop()
        previous = token.end()
    if any(scope is None for scope in stack):
        raise SourceError("DATA claim in local, anonymous or template scope needs explicit extraction support")
    return tuple(stack)


def load_data(root: Path) -> list[DataPair]:
    """Read names and definitions from source-owned DATA declarations."""
    from homm3.retail_labels.source import macro_invocations, DATA_HEAD_RE
    from homm3.match.source_ownership import fragment_owners
    rows = data_rows(root, "data")
    fragments_by_owner: dict[str, list[tuple[Path, int]]] = {}
    for fragment, (owner, include_at) in fragment_owners(root).items():
        fragments_by_owner.setdefault(owner, []).append((root / fragment, include_at))
    fragment_text: dict[Path, tuple[str, str]] = {}
    result = []
    seen = {}
    for row in rows:
        va = row["retail_va"]
        if not row["evidence"].strip():
            raise SourceError(f"unproven Mac data pair {va:#x}")
        identity = {key: value for key, value in row.items() if key != "evidence"}
        if va in seen:
            if seen[va] != identity:
                raise SourceError(f"conflicting Mac data pair {va:#x}")
            continue
        seen[va] = identity
        source = root / row["source"]
        owner_raw = source.read_text()
        owner_masked = _masked_source(owner_raw)
        claim_sources = [(source, owner_raw, owner_masked, None)]
        # Registered fragments retain their original include position. Search
        # them even if the owner has a claim, so duplicates fail closed.
        for path, include_at in fragments_by_owner.get(row["source"], ()):
            if path not in fragment_text:
                fragment_raw = path.read_text()
                fragment_text[path] = (fragment_raw, _masked_source(fragment_raw))
            fragment_raw, fragment_masked = fragment_text[path]
            claim_sources.append((path, fragment_raw, fragment_masked, include_at))
        claims = [(path, text, masked, end, include_at)
                  for path, text, masked, include_at in claim_sources
                  for _, end, args, _ in macro_invocations(masked, DATA_HEAD_RE, text)
                  if end is not None and len(args) == 1 and int(args[0], 16) == va]
        if len(claims) != 1:
            raise SourceError(f"{source}: expected one DATA({va:#x}) claim")
        claim_path, raw, masked, claim_end, include_at = claims[0]
        start = claim_end + 1
        end = masked.find(";", start)
        declaration = masked[start:end + 1]
        declaration_only = row.get("declaration_only", False)
        same_tu_definition = row.get("same_tu_definition", False)
        if not isinstance(same_tu_definition, bool):
            raise SourceError(f"{source}: DATA({va:#x}) same_tu_definition must be boolean")
        if same_tu_definition and declaration_only:
            raise SourceError(f"{source}: DATA({va:#x}) cannot be both external and same-TU storage")
        local_owner = row.get("owner_va")
        local_signature = None
        if local_owner is not None:
            if include_at is not None:
                raise SourceError(f"{claim_path}: fragment-local DATA owner_va needs explicit extraction support")
            if not isinstance(local_owner, int):
                raise SourceError(f"{source}: DATA owner_va must be a Windows function VA")
            owner_start, local_signature = _claim(raw, local_owner, source)
            owner_brace = masked.find("{", owner_start)
            owner_end = _function_end(raw, owner_brace)
            if not owner_brace < start < end < owner_end:
                raise SourceError(f"{source}: DATA({va:#x}) is outside its claimed function owner")
            if declaration_only or not re.match(r'\s*static\s+const\b', declaration):
                raise SourceError(f"{source}: local DATA requires a canonical static const initializer")
            if "mac_symbol" in row:
                raise SourceError(f"{source}: local DATA symbol counters must not be pinned")
            scope = ()
        else:
            try:
                scope = _data_scope(masked, start)
                if include_at is not None and _data_scope(owner_masked, include_at):
                    raise SourceError("source fragment DATA claim is not included at file scope")
            except SourceError as exc:
                raise SourceError(f"{claim_path}: DATA({va:#x}): {exc}") from exc
        if declaration_only:
            # The candidate refers to storage defined outside its compilation
            # group. Its source owner can carry a definition or an extern; no
            # initializer is injected or scored by this address-only binding.
            head = declaration.split("=", 1)[0].rstrip("; \n\t")
            match = re.fullmatch(
                r'\s*(?:(?:extern|static)\s+)?(?:(?:const|volatile|unsigned|signed|long|short)\s+)*'
                r'\w+(?:::\w+)*(?:\s+|\s*\*\s*(?:const\s+)?)'
                r'(?:(?P<plain>\w+(?:::\w+)*)|\(\s*&\s*(?P<reference>\w+)\s*\))'
                r'\s*(?:\[[^\[\];]*\]\s*)*', head, re.DOTALL)
            name = (match.group("plain") or match.group("reference")) if match else None
            definition = re.sub(r'^\s*(?:(?:extern|static)\s+)?', 'extern ', head, count=1) + ";"
        else:
            if same_tu_definition:
                # A source-owned uninitialized global still has a real
                # definition in its original TU. CodeWarrior can address that
                # storage directly from the TOC; an extern-only shim cannot.
                # Keep this narrow: no initializer, pointer, reference, or
                # qualified member declarator is inferred from the row.
                match = re.fullmatch(
                    r'\s*(?:static\s+)?(?:(?:unsigned|signed)\s+)?'
                    r'\w+(?:::\w+)*\s+(\w+)\s*;', declaration, re.DOTALL)
            else:
                match = re.fullmatch(r'\s*(?:(?:static|const|unsigned|signed|long|short)\s+)*'
                                     r'\w+\s+(\w+)\s*(?:\[[^\]]*\]\s*)*=.*;',
                                     declaration, re.DOTALL)
            name = match.group(1) if match else None
            definition = raw[start:end + 1].strip()
        if end < 0 or match is None:
            raise SourceError(f"{source}: unsupported Mac data definition at {va:#x}")
        if scope:
            if "::" in name or not re.match(r'\s*static\b', declaration):
                raise SourceError(f"{source}: scoped DATA claim {va:#x} must be an ordinary static member")
            name = "::".join((*scope, name))
        if "::" in name:
            if not declaration_only:
                raise SourceError(f"{source}: qualified DATA initializers need additional matching support")
            # The member declaration belongs inside its canonical class view.
            # An out-of-class extern would define storage or be invalid C++.
            definition = ""
            if not isinstance(row.get("mac_symbol"), str) or not row["mac_symbol"].strip():
                raise SourceError(f"{source}: qualified DATA claim {va:#x} needs its emitted mac_symbol")
        symbol = row.get("mac_symbol", name)
        if not isinstance(symbol, str) or not re.fullmatch(r'\S+', symbol):
            raise SourceError(f"{source}: invalid Mac data symbol at {va:#x}")
        if local_owner is not None:
            name = local_signature.split()[-1] + "::" + name
            definition = ""  # Already present inside the owning authored body.
        # CodeWarrior may place named immutable tables in a code-section pool.
        # Infer constness only for the bounded object/array declarators this
        # reader accepts. A pointer-to-const or reference is not const storage.
        head = declaration.split("=", 1)[0]
        storage = re.sub(r'^\s*(?:(?:extern|static)\s+)?', '', head, count=1)
        qualifiers = re.match(r'(?:(?:const|volatile|unsigned|signed|long|short)\s+)*', storage)[0]
        read_only = "const" in qualifiers.split() and "*" not in head and "&" not in head
        result.append(DataPair(va, name, definition,
                               row["mac_section"], row["mac_offset"], row["mac_size"], row["sha256"],
                               declaration_only, symbol, local_owner, read_only,
                               same_tu_definition))
    return result


def _claim(text: str, va: int, source: Path, *, allow_declaration: bool = False) -> tuple[int, str]:
    marker = re.compile(r"\bVA\(\s*" + re.escape(f"0x{va:08x}") + r"\s*,", re.IGNORECASE)
    claims = list(marker.finditer(_masked_source(text)))
    if len(claims) != 1:
        raise SourceError(f"{source}: expected one VA({va:#x}) claim")
    after = text.find("\n", claims[0].end()) + 1
    if after <= 0:
        raise SourceError(f"{source}: VA({va:#x}) has no following definition")
    # VA belongs immediately above the authored definition. Skip only leading
    # whitespace/comments, never search ahead for a second named function.
    cursor = after
    while True:
        cursor += len(text[cursor:]) - len(text[cursor:].lstrip())
        if text.startswith("//", cursor):
            cursor = text.find("\n", cursor) + 1
        elif text.startswith("/*", cursor):
            end = text.find("*/", cursor + 2)
            if end < 0:
                raise SourceError(f"{source}: unterminated comment after VA({va:#x})")
            cursor = end + 2
        else:
            break
        if cursor <= 0:
            raise SourceError(f"{source}: no definition after VA({va:#x})")
    brace = text.find("{", cursor)
    if allow_declaration:
        # Some source annotations deliberately live on forward declarations
        # to preserve the repository's retail-VA ordering. A callee identity
        # can use that declaration; compilation still requires an actual body.
        semicolon = text.find(";", cursor)
        if 0 <= semicolon - cursor < 1000 and (brace < 0 or semicolon < brace):
            declaration = text[cursor:semicolon + 1]
            if re.fullmatch(r'[\w:~<>*&\s]+\([^;{}]*\)\s*(?:const\s*)?;', declaration):
                return cursor, " ".join(declaration.split("(", 1)[0].split())
    if brace < 0 or brace - cursor > 1000:
        raise SourceError(f"{source}: missing nearby function body for VA({va:#x})")
    head = " ".join(text[cursor:brace].split()).split("(", 1)[0].strip()
    if not head or "VA(" in head or ";" in head:
        raise SourceError(f"{source}: invalid definition after VA({va:#x})")
    return cursor, head


def load_pairs(root: Path, extra_rows: list[dict] | None = None) -> list[Pair]:
    paths = [root / "config/mac/functions.toml", *sorted((root / "config/mac/functions").glob("*.toml"))]
    rows = []
    for path in paths:
        if path.is_file():
            with path.open("rb") as stream:
                rows.extend(tomllib.load(stream).get("functions", []))
    rows.extend(extra_rows or [])
    source_by_unit = {unit["unit"]: root / unit["source"] for unit in
                      units_manifest.load(root / "config/units.toml")["unit"]}
    pairs = []
    seen = set()
    symbols = set()
    spans: dict[int, list[tuple[int, int, int]]] = {}
    for row in rows:
        va = row["retail_va"]
        if va in seen:
            raise SourceError(f"duplicate Mac pair for Windows VA {va:#x}")
        if not row["mac_symbol"] or row["mac_symbol"] in symbols:
            raise SourceError(f"empty or duplicate Mac symbol {row['mac_symbol']!r}")
        symbols.add(row["mac_symbol"])
        digest = row.get("target_sha256")
        if digest is not None and not re.fullmatch(r"[0-9a-f]{64}", digest):
            raise SourceError(f"invalid Mac target hash for Windows VA {va:#x}")
        section, offset, size = row["mac_section"], row["mac_offset"], row["mac_size"]
        if section < 0 or offset < 0 or size <= 0 or offset % 4 or size % 4:
            raise SourceError(f"invalid Mac code span for Windows VA {va:#x}")
        if not row["evidence"].strip():
            raise SourceError(f"missing pairing evidence for Windows VA {va:#x}")
        for start, end, other_va in spans.get(section, []):
            if offset < end and start < offset + size:
                raise SourceError(f"Mac span for Windows VA {va:#x} overlaps {other_va:#x}")
        seen.add(va)
        spans.setdefault(section, []).append((offset, offset + size, va))
        source = root / row["source"]
        if source_by_unit.get(row["unit"]) != source:
            raise SourceError(f"Mac pair {va:#x} is not owned by unit {row['unit']}")
        _, signature = _claim(source.read_text(), va, source, allow_declaration=True)
        profile = profiles.load(root, row["unit"])
        if profile and "shim" in row:
            raise SourceError(f"{row['unit']}: paired-body profile replaces per-function shims")
        if not profile and "shim" not in row:
            raise SourceError(f"{row['unit']}: pair needs a Mac unit profile or bootstrap shim")
        pairs.append(Pair(va, row["unit"], source,
                          signature, root / (profile.preamble if profile else row["shim"]),
                          row["mac_section"], row["mac_offset"],
                          row["mac_size"], row["mac_symbol"], row["evidence"],
                          tuple(row.get("data", [])), row["unit"] if profile else None,
                          digest, root))
    return pairs


def _function_end(text: str, brace: int) -> int:
    depth = 0
    state = "code"
    index = brace
    while index < len(text):
        ch = text[index]
        next_ch = text[index + 1] if index + 1 < len(text) else ""
        if state == "code":
            if ch == "/" and next_ch == "/":
                state = "line"; index += 2; continue
            if ch == "/" and next_ch == "*":
                state = "block"; index += 2; continue
            if ch in ("'", '"'):
                state = ch; index += 1; continue
            if ch == "{":
                depth += 1
            elif ch == "}":
                depth -= 1
                if depth == 0:
                    return index + 1
        elif state == "line":
            if ch == "\n":
                state = "code"
        elif state == "block":
            if ch == "*" and next_ch == "/":
                state = "code"; index += 2; continue
        elif ch == "\\":
            index += 2; continue
        elif ch == state:
            state = "code"
        index += 1
    raise SourceError("unterminated function body")


def extract_body(pair: Pair) -> str:
    text = pair.source.read_text()
    start, signature = _claim(text, pair.retail_va, pair.source, allow_declaration=True)
    if signature != pair.signature:
        raise SourceError(f"{pair.source}: source label changed since pair loading")
    masked = _masked_source(text)
    brace = masked.find("{", start)
    semicolon = masked.find(";", start)
    # Some retail VAs label a later retained emission slot while the one
    # canonical source definition appears earlier. Preserve this declaration
    # in its original position; source_helpers supplies the actual body.
    if 0 <= semicolon - start < 1000 and (brace < 0 or semicolon < brace):
        return text[start:semicolon + 1] + "\n"
    end = _function_end(text, brace)
    body = text[start:end]
    if re.search(r"\bVA\(", body):
        raise SourceError("extracted body contains another VA claim")
    return body + "\n"


def source_helper(text: str, selector: str, source: Path) -> tuple[int, str, str]:
    """Locate one ordinary source definition without inventing a Windows VA.

    Selectors are names already present in the owning TU. An overload may use
    its exact authored parameter declaration, including parameter names and
    trailing const. Whitespace is insignificant. Local definitions, templates
    and unsupported declaration forms fail explicitly. A declaration/call
    cannot substitute for the canonical body.
    """
    selection = re.fullmatch(r'(?P<name>\w+(?:::\w+)*)(?P<parameters>\s*\([^;{}]*\)\s*(?:const)?)?', selector)
    if not selection:
        raise SourceError(f"{source}: invalid source helper selector {selector!r}")
    name = selection['name']
    parameters = selection['parameters']
    masked = _masked_source(text)
    pattern = re.compile(
        r'^[ \t]*(?P<prefix>(?:[\w:*&]+\s+)+)' + re.escape(name)
        + r'(?P<parameters>\s*\([^;{}]*\)\s*(?:const\s*)?)\{', re.MULTILINE)
    matches = []
    for match in pattern.finditer(masked):
        if re.search(r'\b(?:return|if|while|switch|for|typedef|template)\b', match['prefix']):
            continue
        if parameters is not None and re.sub(r'\s+', '', parameters) != re.sub(r'\s+', '', match['parameters']):
            continue
        if _data_scope(masked, match.start()):
            raise SourceError(f"{source}: nested source helper needs explicit extraction support")
        brace = match.end() - 1
        end = _function_end(text, brace)
        start = match.start() + len(match[0]) - len(match[0].lstrip())
        signature = " ".join(masked[start:brace].split())
        matches.append((start, signature, text[start:end] + "\n"))
    if len(matches) != 1:
        raise SourceError(f"{source}: expected one source helper definition for {selector!r}; found {len(matches)}")
    return matches[0]


def individual_source(pair: Pair) -> str:
    definitions = []
    if pair.data:
        claims = {claim.retail_va: claim for claim in load_data(pair.project_root or pair.source.parent.parent)}
        for va in pair.data:
            if va not in claims:
                raise SourceError(f"{pair.signature}: missing Mac data pair {va:#x}")
            if not claims[va].definition:
                raise SourceError(f"{pair.signature}: qualified data {va:#x} must be declared in its class view")
            definitions.append(claims[va].definition)
    return "\n\n".join([pair.shim.read_text().rstrip(), *definitions, extract_body(pair)])


def source_identity(pair: Pair, generated: bytes) -> bytes:
    """Own implementation identity excludes changes to neighboring bodies."""
    if not pair.compile_group:
        return generated
    root = pair.project_root or pair.source.parent.parent
    profile = profiles.load(root, pair.compile_group)
    header_inputs = profiles.headers(root, profile)
    authored = individual_source(pair)
    if extract_body(pair).rstrip().endswith(";"):
        # A VA-marked redeclaration is an emission anchor, not the body whose
        # source hash must follow the actual implementation. Require its one
        # canonical body to be selected as a source helper in this profile.
        name = pair.signature.rsplit(" ", 1)[-1]
        selectors = [selector for selector in profile.source_helpers
                     if selector.split("(", 1)[0].strip() == name]
        if len(selectors) != 1:
            raise SourceError(f"{pair.signature}: VA redeclaration needs one canonical source helper")
        authored += "\n" + source_helper(pair.source.read_text(), selectors[0], pair.source)[2]
    return (authored.encode() + b"\0" +
            b"\0".join(name.encode() + b"\0" + data
                       for name, data in sorted(header_inputs.items())))


def candidate_source(pair: Pair) -> str:
    if not pair.compile_group:
        return individual_source(pair)
    root = pair.project_root or pair.source.parent.parent
    profile = profiles.load(root, pair.compile_group)
    peers = [p for p in load_pairs(root) if p.compile_group == pair.compile_group]
    if pair.retail_va not in {p.retail_va for p in peers}:
        peers.append(pair)  # A compile-only probe does not admit a target span.
    elif pair.data:
        # `mac compile VA --data ...` may probe an additional source-owned
        # global for a body that is already admitted. Preserve its reviewed
        # code span while compiling the requested data definition in this
        # temporary candidate group; admission remains an explicit edit.
        peers = [replace(p, data=tuple(sorted(set(p.data) | set(pair.data))))
                 if p.retail_va == pair.retail_va else p for p in peers]
    if any(p.source != pair.source for p in peers):
        raise SourceError("a Mac compilation group must have one authored source")
    text = pair.source.read_text()
    definitions = []
    seen = {p.retail_va for p in peers}
    for va in profile.helpers:
        if va not in seen:
            _, signature = _claim(text, va, pair.source, allow_declaration=True)
            peers.append(Pair(va, pair.unit, pair.source, signature, pair.shim,
                              0, 0, 4, "", "source helper"))
            seen.add(va)
    for peer in peers:
        start, _ = _claim(text, peer.retail_va, peer.source, allow_declaration=True)
        definitions.append((start, extract_body(peer)))
    for selector in profile.source_helpers:
        start, _, body = source_helper(text, selector, pair.source)
        if start not in {at for at, _ in definitions}:
            definitions.append((start, body))
    data_vas = {va for peer in peers for va in peer.data}
    claims = {claim.retail_va: claim for claim in load_data(root)} if data_vas else {}
    for va in data_vas:
        if va not in claims:
            raise SourceError(f"{pair.unit}: missing Mac data pair {va:#x}")
        definition = claims[va].definition
        if not definition:
            raise SourceError(f"{pair.unit}: qualified data {va:#x} must be declared in its class view")
        start = text.find(definition)
        if start < 0:
            raise SourceError(f"{pair.unit}: data definition {va:#x} is outside owning source")
        definitions.append((start, definition))
    header_names = (["include/compiler.h"] if profile.native_headers else [])
    header_names += [profile.preamble, *profile.extra_headers]
    return ('\n'.join('#include "' + name + '"' for name in header_names) + '\n\n' +
            "\n\n".join(definition for _, definition in sorted(definitions)) + "\n")


def compile_scope(pair: Pair) -> str:
    return "paired_bodies_in_source_order" if pair.compile_group else "isolated_function_with_layout_shim"
