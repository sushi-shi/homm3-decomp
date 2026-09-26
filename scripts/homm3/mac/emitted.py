"""Join full-TU CodeWarrior emitted symbols with authored source definitions.

For every unit with a full-TU object (`ninja mac:<unit>`), report:

  emitted symbols      each code hunk, with its source owner: the unit's own
                       source, a project header, another source file, an MSL
                       library template (std/Metrowerks), or none
  not emitted          non-template function bodies authored in the unit's
                       source (or its registered fragments) with no hunk -
                       inlined everywhere, conditionally compiled out, or
                       unused; the report does not decide which

The join is by qualified name. CodeWarrior's ARM-style mangling is decoded
only as far as the qualified name; overloads sharing that name are reported
as ambiguous rather than guessed.
"""
from __future__ import annotations

from collections import Counter, defaultdict
import json
from pathlib import Path
import re

from homm3.core.tsv import write as write_tsv

OPERATORS = {
    "__as": "operator=", "__eq": "operator==", "__ne": "operator!=", "__lt": "operator<",
    "__gt": "operator>", "__le": "operator<=", "__ge": "operator>=", "__pl": "operator+",
    "__mi": "operator-", "__ml": "operator*", "__dv": "operator/", "__md": "operator%",
    "__apl": "operator+=", "__ami": "operator-=", "__amu": "operator*=", "__adv": "operator/=",
    "__vc": "operator[]", "__cl": "operator()", "__rf": "operator->", "__pp": "operator++",
    "__mm": "operator--", "__nw": "operator new", "__dl": "operator delete",
    "__nwa": "operator new[]", "__dla": "operator delete[]", "__ls": "operator<<",
    "__rs": "operator>>", "__nt": "operator!", "__aa": "operator&&", "__oo": "operator||",
    "__ad": "operator&", "__or": "operator|", "__er": "operator^", "__co": "operator~",
}


def _qualifiers(text: str) -> tuple[list[str], str] | None:
    """Parse `<len><name>` or `Q<n><len><name>...`; return (parts, remainder)."""
    def one(at: int) -> tuple[str, int] | None:
        digits = re.match(r"\d+", text[at:])
        if not digits:
            return None
        length = int(digits.group())
        start = at + len(digits.group())
        if length <= 0 or start + length > len(text):
            return None
        return text[start:start + length], start + length

    if text.startswith("Q") and len(text) > 1 and text[1].isdigit():
        parts, at = [], 2
        for _ in range(int(text[1])):
            part = one(at)
            if part is None:
                return None
            parts.append(part[0])
            at = part[1]
        return parts, text[at:]
    part = one(0)
    return None if part is None else ([part[0]], text[part[1]:])


def _split(symbol: str) -> tuple[str, bool, str] | None:
    """(qualified name, const member, parameter code) of a function symbol."""
    text = symbol.lstrip(".")
    for at in range(1, len(text) - 2):
        if text[at:at + 2] != "__":
            continue
        name, rest = text[:at], text[at + 2:]
        if rest.startswith("F"):
            parts, remainder = [], rest
        else:
            parsed = _qualifiers(rest)
            if parsed is None:
                continue
            parts, remainder = parsed
        const = remainder.startswith("CF")
        if const:
            remainder = remainder[1:]
        if not remainder.startswith("F"):
            continue
        if name == "__ct" and parts:
            name = parts[-1]
        elif name == "__dt" and parts:
            name = "~" + parts[-1]
        else:
            name = OPERATORS.get(name, name)
        return "::".join([*parts, name]), const, remainder[1:]
    return None


def demangle(symbol: str) -> str:
    """Qualified C++ name of a CodeWarrior linkage name; C names pass through."""
    split = _split(symbol)
    return split[0] if split else symbol.lstrip(".")


def _type_end(code: str, at: int) -> int:
    """End of one ARM-mangled type in `code`; ValueError if unsupported."""
    while at < len(code) and code[at] in "CVUSPR":
        at += 1
    if at >= len(code):
        raise ValueError("truncated type")
    head = code[at]
    if head == "A":  # A<n>_<element>
        digits = re.match(r"A\d+_", code[at:])
        if not digits:
            raise ValueError("bad array")
        return _type_end(code, at + digits.end())
    if head == "F":  # F<parameters>_<return>
        at += 1
        while code[at] != "_":
            at = _type_end(code, at)
        return _type_end(code, at + 1)
    if head == "M":  # M<class><member type>
        return _type_end(code, _type_end(code, at + 1))
    if head == "Q" or head.isdigit():
        parsed = _qualifiers(code[at:])
        if parsed is None:
            raise ValueError("bad name")
        return len(code) - len(parsed[1])
    if head in "vcsilxfdrbwe":
        return at + 1
    raise ValueError(f"unsupported type code {head!r}")


def parameters(symbol: str) -> tuple[bool, tuple[str, ...]] | None:
    """(const member, parameter type codes) of a function symbol, or None."""
    split = _split(symbol)
    if split is None:
        return None
    _name, const, code = split
    result, at = [], 0
    try:
        while at < len(code):
            end = _type_end(code, at)
            result.append(code[at:end])
            at = end
    except (ValueError, IndexError):
        return None
    return const, tuple(item for item in result if item != "v")


BUILTINS = {
    "void": "v", "char": "c", "signed char": "Sc", "unsigned char": "Uc", "short": "s",
    "unsigned short": "Us", "int": "i", "unsigned int": "Ui", "unsigned": "Ui", "long": "l",
    "unsigned long": "Ul", "long long": "x", "__int64": "x", "unsigned long long": "Ux",
    "float": "f", "double": "d", "long double": "r", "bool": "b", "_Bool": "b", "wchar_t": "w",
}


def encode(spelling: str) -> str | None:
    """ARM code of a clang parameter spelling; None when the form is not modelled."""
    text = re.sub(r"\b(?:struct|class|enum|union)\s+", "", spelling).strip()
    if "(" in text or "[" in text or "<" in text:
        return None
    split = re.search(r"[*&]", text)
    base = (text[:split.start()] if split else text).split()
    declarators = text[split.start():] if split else ""
    if not base or re.sub(r"[*&\s]|\bconst\b", "", declarators):
        return None
    const = "const" in base
    base = " ".join(word for word in base if word not in ("const", "volatile"))
    code = BUILTINS.get(base)
    if code is None:
        if not re.fullmatch(r"\w+(?:::\w+)*", base) or base.startswith("std::"):
            return None
        parts = base.split("::")
        names = "".join(f"{len(part)}{part}" for part in parts)
        code = names if len(parts) == 1 else f"Q{len(parts)}{names}"
    for token in re.findall(r"\*|&|const", declarators):
        if token == "const":
            const = True
            continue
        code = ("P" if token == "*" else "R") + ("C" if const else "") + code
        const = False
    return code  # a by-value parameter's top-level const is not mangled


def _template_hint(spelling: str) -> str | None:
    """`<first-argument-code,` of a template spelling such as std::vector<army *> &."""
    match = re.search(r"<([^<>,]+)[,>]", spelling)
    code = encode(match.group(1)) if match else None
    return f"<{code}," if code else None


def fit(definition, symbol: str) -> int | None:
    """How many parameters of `symbol` spell `definition`'s; None if incompatible."""
    decoded = parameters(symbol)
    if decoded is None:
        return None
    const, codes = decoded
    wanted = [encode(spelling) for spelling in definition.argument_types]
    if definition.variadic:
        wanted.append("e")
    if len(codes) != len(wanted) or (definition.member and const != definition.const):
        return None
    if any(code is not None and code != actual and code[:1] not in "0123456789Q"
           and not code.startswith(("P", "R")) for code, actual in zip(wanted, codes)):
        return None  # a written builtin must mangle as itself
    hints = [_template_hint(spelling) for spelling in definition.argument_types]
    return (sum(code == actual for code, actual in zip(wanted, codes))
            + sum(bool(hint) and hint in actual for hint, actual in zip(hints, codes)))


def select(definition, symbols) -> str | None:
    """The one symbol among same-named overloads whose parameters fit `definition`."""
    symbols = sorted(set(symbols))
    if len(symbols) == 1:
        return symbols[0]
    scored = [(score, symbol) for symbol in symbols
              if (score := fit(definition, symbol)) is not None]
    if not scored:
        return None
    best = max(score for score, _ in scored)
    winners = [symbol for score, symbol in scored if score == best]
    return winners[0] if len(winners) == 1 else None


def owner(symbol: str, definitions) -> object | None:
    """The one same-named definition whose parameters `symbol` spells."""
    if len(definitions) == 1:
        return definitions[0] if parameters(symbol) is not None else None
    scored = [(score, index) for index, definition in enumerate(definitions)
              if (score := fit(definition, symbol)) is not None]
    if not scored:
        return None
    best = max(score for score, _ in scored)
    winners = [index for score, index in scored if score == best]
    return definitions[winners[0]] if len(winners) == 1 else None


def key(name: str) -> str:
    """Join key: template arguments and whitespace removed."""
    previous = None
    while previous != name:
        previous, name = name, re.sub(r"<[^<>]*>", "", name)
    return re.sub(r"\s+", "", name)


IMPLICIT_MEMBER_RE = re.compile(r"^\.?(?:__(?:ct|dt|as)__(?:\d|Q\d)|__sinit_\w+)")


def classify(qualified: str, owners: list, unit_source: str, symbol: str = "") -> str:
    """Owner of one emitted symbol.

    vendor: the unit itself is a vendored library source. library_template:
    an MSL std/Metrowerks name (also when the listing truncated it).
    compiler_generated: a constructor, destructor or assignment with no
    written definition, or a file's static initializer (`__sinit_`).
    """
    if unit_source.startswith("vendor/"):
        return "vendor"
    if owners:
        files = {owner.file for owner in owners}
        if unit_source and (unit_source in files
                            or any(owner.source_owner == unit_source for owner in owners)):
            return "unit_source"
        if all(path.startswith("include/") for path in files):
            return "header"
        return "other_source"
    if qualified.startswith(("std::", "Metrowerks::")) or re.search(
            r"(?<![A-Za-z])3std(?![a-z])|Metrowerks", symbol):
        return "library_template"
    if IMPLICIT_MEMBER_RE.match(symbol):
        return "compiler_generated"
    return "no_source_owner"


def report(root: Path, definitions) -> dict:
    from homm3 import manifest
    by_key = defaultdict(list)
    for definition in definitions:
        by_key[key(definition.name)].append(definition)
    units, emitted_rows, missing_rows = [], [], []
    for unit in manifest.units(root / "config/units.toml"):
        name, source = unit["unit"], unit["source"]
        index = root / "build/mac/obj" / f"{name}.hunks.json"
        if not index.is_file():
            units.append(dict(unit=name, state="no_object"))
            continue
        hunks = json.loads(index.read_text())["code"]
        seen, counts = set(), Counter()
        for hunk in hunks:
            qualified = demangle(hunk["symbol"])
            owners = by_key.get(key(qualified), [])
            state = classify(qualified, owners, source, hunk["symbol"])
            counts[state] += 1
            seen.add(key(qualified))
            emitted_rows.append(dict(unit=name, symbol=hunk["symbol"], qualified=qualified,
                                     size=hunk["size"], owner=state, definitions=len(owners),
                                     windows_va=",".join(f"0x{owner.va:08x}" for owner in owners if owner.va)))
        authored = [definition for definition in definitions
                    if (definition.file == source or definition.source_owner == source)
                    and not definition.template]
        missing = [definition for definition in authored if key(definition.name) not in seen]
        for definition in missing:
            missing_rows.append(dict(unit=name, name=definition.name, file=definition.file,
                                     line=definition.line, inline=int(definition.inline),
                                     windows_va=f"0x{definition.va:08x}" if definition.va else ""))
        units.append(dict(unit=name, state="compiled", emitted=len(hunks), authored=len(authored),
                          not_emitted=len(missing), **{f"emitted_{k}": v for k, v in counts.items()}))
    return {"units": units, "emitted": emitted_rows, "not_emitted": missing_rows}


def write(root: Path, result: dict) -> Path:
    folder = root / "build/gen/mac"
    write_tsv(folder / "emitted-symbols.tsv", ["# GENERATED by `homm3 mac emitted`; do not edit."],
              ["unit", "symbol", "qualified", "size", "owner", "definitions", "windows_va"],
              result["emitted"])
    write_tsv(folder / "not-emitted.tsv", ["# GENERATED by `homm3 mac emitted`; do not edit."],
              ["unit", "name", "file", "line", "inline", "windows_va"], result["not_emitted"])
    path = folder / "emitted-units.json"
    path.write_text(json.dumps(result["units"], indent=2) + "\n")
    return path


def identity_leads(root: Path, definitions, objects: list[dict], claimed_vas: set[int]) -> list[dict]:
    from homm3 import manifest
    sources = {unit["unit"]: unit["source"] for unit in manifest.units(root / "config/units.toml")}
    """Object leads that name exactly one source definition.

    `suggest` marks a definition with a Windows VA and no Mac claim yet:
    the masked bytes of its own compiled body occupy the whole proven span.
    """
    by_key = defaultdict(list)
    for definition in definitions:
        by_key[key(definition.name)].append(definition)
    rows = []
    for lead in objects:
        qualified = demangle(lead["symbol"])
        owners = by_key.get(key(qualified), [])
        row = dict(offset=lead["offset"], size=lead["size"], unit=lead["unit"], symbol=lead["symbol"],
                   qualified=qualified, exact_row=lead["exact_row"], definitions=len(owners),
                   owner=classify(qualified, owners, sources.get(lead["unit"], ""), lead["symbol"]),
                   windows_va="", file="", line=0, suggest=False)
        if len(owners) == 1:
            owner = owners[0]
            row.update(windows_va=f"0x{owner.va:08x}" if owner.va else "", file=owner.file,
                       line=owner.line,
                       suggest=bool(owner.va) and owner.va not in claimed_vas and lead["exact_row"])
        rows.append(row)
    return rows


def admit_library(root: Path, identities: list[dict]) -> dict[str, int]:
    """Label unowned rows filled exactly by an MSL template or vendored zlib hunk.

    Only exact-row leads on rows with no source, runtime, glue or zlib owner
    are admitted; game-owned bodies stay leads for a reviewed MAC_ADDRESS.
    """
    from homm3.mac import addresses, tables
    spans = tables.read_functions(root)
    runtime = tables.read_runtime(root)
    aliases = tables.read_aliases(root)
    glue = tables.read_glue(root)
    zlib = tables.read_zlib(root)
    claims, _windows, _problems = addresses.scan(root)
    taken = ({claim.offset for claim in claims} | {row.offset for row in runtime}
             | {stub.offset for stub in glue} | {row.offset for row in zlib})
    names = {row.name for row in runtime} | {alias.name for alias in aliases} | {row.name for row in zlib}
    added = Counter()
    labels: dict[int, tables.RuntimeLabel] = {}
    for row in identities:
        if (not row["exact_row"] or row["offset"] in taken or spans.get(row["offset"]) != row["size"]
                or "\ufffd" in row["symbol"]):
            continue  # a truncated listing name is not a linkage name
        evidence = (f"Full-TU {row['unit']} CodeWarrior hunk {row['symbol']} ({row['size']:#x} bytes) "
                    "matches these bytes outside relocated words and fills the proven span "
                    "(homm3 mac inventory object lead).")
        if row["owner"] == "library_template" and row["symbol"] not in names:
            if row["offset"] in labels:
                if labels[row["offset"]].name != row["symbol"]:
                    aliases.append(tables.Alias(row["offset"], row["symbol"], evidence))
                    names.add(row["symbol"])
                    added["aliases"] += 1
                continue
            labels[row["offset"]] = tables.RuntimeLabel(row["offset"], row["symbol"], "msl_cxx",
                                                        "direct", evidence)
            names.add(row["symbol"])
            added["runtime"] += 1
        elif row["owner"] == "vendor" and row["symbol"] not in names:
            zlib.append(tables.VendorLabel(row["offset"], row["symbol"], row["unit"]))
            names.add(row["symbol"])
            taken.add(row["offset"])
            added["zlib"] += 1
    tables.write(root, spans, [*runtime, *labels.values()], aliases, glue, zlib)
    return dict(added)


def bind(definitions, claims) -> list[tuple[object, object | None]]:
    """Each non-compiler-generated claim with its definition, or None.

    A claim binds through the analysis arm's annotations: the VA for paired
    claims, the `mac:` attribute otherwise.
    """
    by_va = {definition.va: definition for definition in definitions if definition.va is not None}
    for definition in definitions:
        for va, _selector, _mangled in definition.additional_instances:
            by_va.setdefault(va, definition)
    by_mac = {(definition.file, definition.mac_offset): definition
              for definition in definitions if definition.mac_offset is not None}
    by_file_name = defaultdict(list)
    for definition in definitions:
        by_file_name[(definition.file, key(definition.name).split("::")[-1])].append(definition)
    result = []
    for claim in claims:
        if claim.compgen is not None:
            continue  # compiler-generated bodies have no written definition
        definition = (by_va.get(claim.windows_va) if claim.windows_va is not None
                      else by_mac.get((claim.path, claim.offset)))
        if definition is None and claim.windows_va is not None and claim.label:
            # A VA on a forward declaration: the one same-named body in its file.
            named = [candidate for candidate in by_file_name.get(
                         (claim.path, key(claim.label).split("::")[-1]), [])
                     if key(candidate.name).endswith(key(claim.label))]
            definition = named[0] if len(named) == 1 else None
        result.append((claim, definition))
    return result


def object_hunks(root: Path) -> dict[str, dict[str, list[dict]]]:
    """Full-TU code hunks by unit, then by qualified-name join key."""
    objects: dict[str, dict[str, list[dict]]] = {}
    for index in sorted((root / "build/mac/obj").glob("*.hunks.json")):
        if not index.with_name(index.name.replace(".hunks.json", ".o")).is_file():
            continue  # a stale index beside a failed compile
        by_key = defaultdict(list)
        for hunk in json.loads(index.read_text())["code"]:
            by_key[key(demangle(hunk["symbol"]))].append(hunk)
        objects[index.name.removesuffix(".hunks.json")] = by_key
    return objects


def claim_bodies(root: Path, definitions, claims, objects=None) -> list[dict]:
    """Does each MAC_ADDRESS claim have an emitted body in a full-TU object?

    The body is looked up by qualified name in the owning unit's object, or in
    any compiled object for a header definition; overloads are told apart by
    their mangled parameters. States: emitted, ambiguous (no one overload
    fits), not_emitted (inlined, folded or conditional), no_object (the
    owning unit does not compile) and no_definition.
    """
    from homm3 import manifest
    units = {unit["source"]: unit["unit"] for unit in manifest.units(root / "config/units.toml")}
    if objects is None:
        objects = object_hunks(root)
    rows = []
    for claim, definition in bind(definitions, claims):
        row = dict(identity=claim.identity, file=claim.path, line=claim.line,
                   mac_offset=f"0x{claim.offset:06x}", mac_size=f"0x{claim.size:x}",
                   name="", unit="", state="no_definition", symbol="", symbol_size="",
                   object="")
        if definition is not None:
            owner = definition.source_owner or definition.file
            unit = units.get(owner, "")
            row.update(name=definition.name, unit=unit)
            searched = [unit] if unit else sorted(objects)
            if unit and unit not in objects:
                row["state"] = "no_object"
            else:
                hunks = {}
                for name in searched:
                    for hunk in objects.get(name, {}).get(key(definition.name), []):
                        hunks.setdefault(hunk["symbol"], (name, hunk))
                symbol = select(definition, hunks) if hunks else None
                row["state"] = "emitted" if symbol else "ambiguous" if hunks else "not_emitted"
                if symbol:
                    name, hunk = hunks[symbol]
                    row.update(symbol=symbol, symbol_size=f"0x{hunk['size']:x}", object=name)
        rows.append(row)
    return rows
