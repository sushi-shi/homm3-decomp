"""Source-owned Mac DATA pairs read from the data inventories and DATA claims."""
from __future__ import annotations

from dataclasses import dataclass
from functools import lru_cache
from pathlib import Path
import re
import tomllib

from homm3 import manifest as units_manifest


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
    owner_unit: str | None = None
    same_tu_array: bool = False
    same_tu_external: bool = False


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
    owner_by_source = {root / row["source"]: unit
                       for unit, row in units_manifest.by_unit(root / "config/units.toml").items()}
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
        same_tu_array = row.get("same_tu_array", False)
        if not isinstance(same_tu_array, bool) or (same_tu_array and not same_tu_definition):
            raise SourceError(f"{source}: DATA({va:#x}) same_tu_array requires same-TU storage")
        same_tu_external = row.get("same_tu_external", False)
        if (not isinstance(same_tu_external, bool)
                or (same_tu_external and (not same_tu_definition or same_tu_array))):
            raise SourceError(f"{source}: DATA({va:#x}) same_tu_external requires scalar same-TU storage")
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
                # definition in its original TU. CodeWarrior emits UDATA for
                # it; an extern-only declaration cannot establish ownership.
                # Keep this narrow: plain pointers and explicitly reviewed
                # arrays only; no initializer, reference, or qualified member.
                match = re.fullmatch(
                    r'\s*(?:static\s+)?(?:(?:unsigned|signed)\s+)?'
                    r'\w+(?:::\w+)*(?:\s*\*\s*|\s+)(\w+)\s*'
                    r'(?P<array>(?:\[[^\[\];]*\]\s*)*);',
                    declaration, re.DOTALL)
                if match and bool(match.group("array").strip()) != same_tu_array:
                    raise SourceError(f"{source}: DATA({va:#x}) same_tu_array differs from its declaration")
                if match and same_tu_external and re.match(r'\s*static\b', declaration):
                    raise SourceError(f"{source}: DATA({va:#x}) same_tu_external requires external linkage")
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
                               same_tu_definition, owner_by_source.get(source),
                               same_tu_array, same_tu_external))
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


