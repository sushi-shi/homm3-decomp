"""homm3.verify.layout - the FIELD-OFFSET oracle (i386/MSVC record layout).

verify/alloc_size.py harvests class SIZES; the retail data-access map needs
the other half - where each member of a claimed datum starts and how wide it
is - because "retail touches +0x14 with a 2-byte store" only becomes a
verdict once a declared field covers (or fails to cover) that offset.

The layout comes from pylibclang under the TU's own compdb flags
(`--target=i386-pc-windows-msvc`), so it is clang's real MSVC record layout,
not an arithmetic model of one: base subobject offsets via
`clang_getOffsetOfBase`, member offsets via `clang_Cursor_getOffsetOfField`.

HONESTY FLAGS. Every leaf carries `resolved`; a verdict may only fire through
a resolved leaf.
  * a polymorphic class's vptr has no FieldDecl - it is synthesised at the
    offset MSVC puts it (0, pushing bases up) and tagged `vptr`, so an
    unmodelled-member verdict never fires on a vtable stamp;
  * a union is laid out but its members are marked UNRESOLVED (overlapping
    readings of one storage are a modelling question, not a width defect);
  * bitfields, virtual bases, incomplete types and anything past the
    recursion cap are opaque - sized where possible, never adjudicated.

NAMES. The join key is cl 5.0's own spelling, derived from clang's mangled
name by `core.msvc_names` (the same rewrite extraction applies), because that
is what the Model binds an rva to - cl 5.0 spells a file-scope static
`_kScrollRate$S<n>` where clang says `kScrollRate`, and the volatile CodeView
ordinal is dropped on both sides.

CACHING. Incremental, keyed on two fingerprints: a hash over every header
(a header edit relays to every TU) and each unit's own source file. Editing
one .cpp re-parses one TU (~1 s), not all 300 (~20 s); `--rebuild` forces the
whole harvest.

    python3 -m homm3.verify.layout [--unit U] [--var NAME] [--rebuild]
"""

from __future__ import annotations

import hashlib
import json
import os
from typing import NamedTuple

from homm3.core.paths import BUILD

CACHE = BUILD / "gen/data_layout.json"

#: how deep a member chain is laid out before a leaf becomes opaque
MAX_DEPTH = 8

#: Cache schema. Bumped whenever the harvest's KEY derivation changes - the
#: fingerprints track source, not this module, so a stale cache would answer
#: with the old spellings for every unchanged TU.
SCHEMA = 4


class Field(NamedTuple):
    """One resolved byte offset inside a declared type.

    `kind` is the leaf's node kind (prim/ptr/arr/rec/opaque) - a verdict asks
    "is this a pointer / a float", never the spelling. `tag` is '' for a real
    leaf, or one of `out` (past the type), `hole` (a gap between members),
    `vptr` (the compiler's own pointer)."""
    off: int
    size: int
    path: str
    type: str
    resolved: bool
    tag: str = ""
    kind: str = "prim"

    @property
    def is_ptr(self) -> bool:
        return self.kind == "ptr"

    @property
    def is_float(self) -> bool:
        return self.kind == "prim" and self.type.replace("const ", "") in (
            "float", "double", "long double")


# --------------------------------------------------------------------------- #
# the type tree (what the cache stores)                                       #
# --------------------------------------------------------------------------- #
# node = {"k": kind, "t": spelling, "sz": bytes, ...}
#   prim   a scalar (builtin or enum)
#   ptr    any pointer / member pointer / function pointer
#   arr    {"n": count, "el": <id>}
#   rec    {"m": [[offset, name, <id>], ...], "u": 1 if union}
#   opaque unresolvable (incomplete, bitfield storage, past the depth cap)
# Every nested type is stored once in `types` and referenced by integer id.


class Layout:
    """The harvested tree: types + per-unit variables."""

    def __init__(self, doc: dict, reparsed: int = 0):
        self.types: list[dict] = doc["types"]
        self.units: dict[str, dict] = doc["units"]
        self.header_hash: str = doc.get("header_hash", "")
        #: units this harvest actually re-parsed (0 = fully cached)
        self.reparsed = reparsed

    def node(self, ref) -> dict | None:
        """A type by id, or an INLINE node (what a synthetic/injected layout
        carries - the selftest builds trees the cache never saw)."""
        if isinstance(ref, dict):
            return ref
        return self.types[ref] if isinstance(ref, int) \
            and 0 <= ref < len(self.types) else None

    def vars_of(self, unit: str) -> dict:
        return (self.units.get(unit) or {}).get("vars", {})

    def var(self, unit: str, name: str) -> dict | None:
        """The declaration by cl 5.0's spelling. The Model may carry a per-rva
        discriminated spelling for a name several units share; the harvest is
        keyed by the family, so the lookup masks."""
        from homm3.core.msvc_names import mask
        vars_ = self.vars_of(unit)
        found = vars_.get(name) or vars_.get(mask(name))
        if found is not None:
            return found
        # DATA claims use the owning VC6 object's checked spelling. The
        # layout harvest keeps Clang's spelling, including anonymous types
        # in local statics. Use the model's unique, module-checked bridge;
        # erasing namespace hashes would also join unrelated declarations.
        from homm3.model import data_spelling
        wanted = mask(name)
        matches = [var for key, var in vars_.items()
                   if mask(data_spelling(key, unit)) == wanted]
        return matches[0] if len(matches) == 1 else None

    def var_node(self, unit: str, name: str) -> dict | None:
        v = self.var(unit, name)
        return self.node(v["t"]) if v else None

    # --- queries ------------------------------------------------------------
    def spelling(self, node) -> str:
        return (node or {}).get("t", "?")

    def sizeof(self, node) -> int | None:
        sz = (node or {}).get("sz")
        return sz if sz and sz > 0 else None

    def element(self, node) -> tuple[int, dict | None]:
        """(count, element node) for an array node, else (1, node)."""
        if node and node["k"] == "arr":
            return node["n"], self.node(node["el"])
        return 1, node

    def field_at(self, node, off: int, path: str = "", depth: int = 0) -> Field:
        """Resolve a byte offset inside a declared type, arithmetically.

        Arrays are indexed, not flattened, so a 196610-byte table resolves at
        any offset without materialising 196610 rows (the flattening cap the
        frozen tool carried was itself a false-positive source: every offset
        past the cap read as unmodelled)."""
        if node is None:
            return Field(off, 0, path, "?", False, "out", "")
        sz = node.get("sz") or 0
        if off < 0 or (sz > 0 and off >= sz):
            return Field(off, 0, path, node.get("t", "?"), True, "out", "")
        if depth > MAX_DEPTH:
            return Field(off, sz, path, node.get("t", "?"), False, "",
                         node["k"])
        if node["k"] == "arr":
            el = self.node(node["el"])
            esz = (el or {}).get("sz") or 0
            if esz <= 0:
                return Field(off, sz, path, node.get("t", "?"), False, "",
                             node["k"])
            i, rem = divmod(off, esz)
            sub = self.field_at(el, rem, f"{path}[{i}]", depth + 1)
            return sub._replace(off=sub.off + i * esz)
        if node["k"] == "rec":
            members = node.get("m") or []
            if not members:
                return Field(off, sz, path or ".", node.get("t", "?"),
                             not node.get("u"), "", node["k"])
            hit = None
            for moff, mname, mid in members:
                if moff <= off:
                    msz = (self.node(mid) or {}).get("sz") or 0
                    if node.get("u") or off < moff + max(msz, 1):
                        hit = (moff, mname, mid)
            if hit is None:
                first = min(m[0] for m in members)
                if off < first:
                    tag = "vptr" if node.get("poly") and off < 4 else "hole"
                    return Field(off - off % max(first, 1), first, path,
                                 node.get("t", "?"), True, tag, "")
                return Field(off, 0, path, node.get("t", "?"), True, "hole",
                             "")
            moff, mname, mid = hit
            sub = self.field_at(self.node(mid), off - moff,
                                f"{path}{mname}", depth + 1)
            if sub.tag == "out":
                return Field(off, 0, path, node.get("t", "?"), True, "hole",
                             "")
            resolved = sub.resolved and not node.get("u")
            return sub._replace(off=sub.off + moff, resolved=resolved)
        return Field(0, sz, path or ".", node.get("t", "?"),
                     node["k"] != "opaque", "", node["k"])

    def flatten(self, node, base_off: int = 0, path: str = "", depth: int = 0,
                cap: int = 4096) -> list[Field]:
        """[(offset, size, path, type, resolved)] - for DISPLAY (`--var`) and
        for the per-claim field table; never a verdict input (`field_at` is)."""
        out: list[Field] = []
        if node is None or depth > MAX_DEPTH:
            return out
        if node["k"] == "arr":
            el = self.node(node["el"])
            esz = (el or {}).get("sz") or 0
            if esz <= 0:
                return [Field(base_off, node.get("sz") or 0, path or ".",
                              node.get("t", "?"), False, "", node["k"])]
            for i in range(node["n"]):
                if len(out) >= cap:
                    break
                out += self.flatten(el, base_off + i * esz, f"{path}[{i}]",
                                    depth + 1, cap - len(out))
            return out
        if node["k"] == "rec" and node.get("m"):
            for moff, mname, mid in node["m"]:
                if len(out) >= cap:
                    break
                out += self.flatten(self.node(mid), base_off + moff,
                                    f"{path}{mname}", depth + 1, cap - len(out))
            return out
        return [Field(base_off, node.get("sz") or 0, path or ".",
                      node.get("t", "?"), node["k"] != "opaque"
                      and not node.get("u"), "", node["k"])]


# --------------------------------------------------------------------------- #
# the harvest                                                                 #
# --------------------------------------------------------------------------- #
def _worker(job):
    """(unit, {obj symbol: {'t': inline node, 'sz': n, 'clang': name}}, err)."""
    unit, source, flags = job
    try:
        return _harvest_unit(unit, source, flags)
    except Exception as exc:                            # noqa: BLE001
        return unit, {}, f"{unit}: {type(exc).__name__}: {exc}"


def _harvest_unit(unit: str, source: str, flags: list[str] | None):
    import clang.cindex as cidx

    from homm3.core import msvc_names

    args = flags
    if args is None:
        return unit, {}, f"{unit}: no compdb entry for {source}"
    try:
        tu = cidx.Index.create().parse(source, args=args)
    except cidx.LibclangError as exc:
        return unit, {}, f"{unit}: libclang: {exc}"
    diagnostic = None
    if any(d.severity >= cidx.Diagnostic.Error for d in tu.diagnostics):
        first = next(d for d in tu.diagnostics
                     if d.severity >= cidx.Diagnostic.Error)
        diagnostic = f'{unit}: local layouts unavailable: {first}'
        tu = cidx.Index.create().parse(source, args=args,
                    options=cidx.TranslationUnit.PARSE_SKIP_FUNCTION_BODIES)
        if any(d.severity >= cidx.Diagnostic.Error for d in tu.diagnostics):
            return unit, {}, diagnostic

    builder = _Builder(cidx)
    main_real = os.path.realpath(source)
    out: dict[str, dict] = {}
    pending = list(tu.cursor.get_children())
    while pending:
        cur = pending.pop()
        if cur.location.file is None or os.path.realpath(cur.location.file.name) != main_real:
            continue
        pending.extend(cur.get_children())
        if cur.kind != cidx.CursorKind.VAR_DECL or cur.location.file is None:
            continue
        if os.path.realpath(cur.location.file.name) != main_real:
            continue
        clang_name = cur.mangled_name
        if not clang_name:
            continue
        got = msvc_names.data(clang_name, decorated=True,
                              internal=cur.linkage != cidx.LinkageKind.EXTERNAL)
        node = builder.node_of(cur.type)
        if got in out and out[got]["t"] != node:
            continue                          # conflicting decls: state nothing
        out[got] = {"t": node, "sz": node.get("sz") or 0, "clang": clang_name}
    return unit, out, diagnostic


class _Builder:
    """clang Type -> the inline node tree (ids are resolved by the parent)."""

    def __init__(self, cidx):
        self.c = cidx
        self.memo: dict[str, dict] = {}

    def node_of(self, ty, depth: int = 0) -> dict:
        c = self.c
        ty = ty.get_canonical()
        sz = ty.get_size()
        sz = sz if isinstance(sz, int) and sz > 0 else 0
        spell = ty.spelling
        if depth > MAX_DEPTH:
            return {"k": "opaque", "t": spell, "sz": sz}
        k = ty.kind
        if k == c.TypeKind.CONSTANTARRAY:
            el = self.node_of(ty.element_type, depth + 1)
            return {"k": "arr", "t": spell, "sz": sz,
                    "n": int(ty.element_count), "el": el}
        if k in (c.TypeKind.POINTER, c.TypeKind.MEMBERPOINTER,
                 c.TypeKind.BLOCKPOINTER, c.TypeKind.LVALUEREFERENCE,
                 c.TypeKind.RVALUEREFERENCE):
            return {"k": "ptr", "t": spell, "sz": sz or 4}
        if k == c.TypeKind.RECORD:
            key = f"{spell}|{sz}"
            if key in self.memo:
                return self.memo[key]
            node = self._record(ty, spell, sz, depth)
            self.memo[key] = node
            return node
        if k in (c.TypeKind.ENUM, c.TypeKind.BOOL, c.TypeKind.CHAR_S,
                 c.TypeKind.CHAR_U, c.TypeKind.SCHAR, c.TypeKind.UCHAR,
                 c.TypeKind.SHORT, c.TypeKind.USHORT, c.TypeKind.INT,
                 c.TypeKind.UINT, c.TypeKind.LONG, c.TypeKind.ULONG,
                 c.TypeKind.LONGLONG, c.TypeKind.ULONGLONG,
                 c.TypeKind.FLOAT, c.TypeKind.DOUBLE, c.TypeKind.LONGDOUBLE,
                 c.TypeKind.WCHAR):
            return {"k": "prim", "t": spell, "sz": sz}
        return {"k": "opaque", "t": spell, "sz": sz}

    def _record(self, ty, spell, sz, depth):
        c = self.c
        decl = ty.get_declaration()
        node = {"k": "rec", "t": spell, "sz": sz, "m": []}
        if decl.kind == c.CursorKind.UNION_DECL:
            node["u"] = 1
        try:
            bases = list(ty.get_bases())
        except Exception:                                # noqa: BLE001
            bases = []
        if any(b.is_virtual_base() for b in bases):
            return {"k": "opaque", "t": spell, "sz": sz}   # vbptr layout
        members = []
        base_at_zero = False
        for b in bases:
            off = b.get_base_offsetof(decl)
            if off < 0 or off % 8:
                return {"k": "opaque", "t": spell, "sz": sz}
            base_at_zero = base_at_zero or off == 0
            members.append((off // 8, f"::{b.type.spelling}",
                            self.node_of(b.type, depth + 1)))
        poly = any(m.is_virtual_method() for m in ty.get_methods())
        if poly and not base_at_zero:
            # MSVC puts the introducing vptr at 0 and pushes the bases up
            members.append((0, ".__vfptr", {"k": "ptr", "t": "void *",
                                            "sz": 4}))
            node["poly"] = 1
        for f in ty.get_fields():
            off = f.get_field_offsetof()
            if off < 0:
                return {"k": "opaque", "t": spell, "sz": sz}
            if f.is_bitfield() or off % 8:
                width = max(f.get_bitfield_width(), 1) if f.is_bitfield() else 8
                members.append((off // 8, f".{f.spelling}",
                                {"k": "opaque", "t": f"{f.type.spelling}:"
                                                     f"{width}",
                                 "sz": (width + 7) // 8}))
                continue
            members.append((off // 8, f".{f.spelling}",
                            self.node_of(f.type, depth + 1)))
        node["m"] = sorted(members, key=lambda m: m[0])
        return node


def _intern(node: dict, types: list[dict], index: dict[str, int]) -> int:
    """Store `node` (inline tree) in the shared table, returning its id."""
    n = dict(node)
    if n["k"] == "arr":
        n["el"] = _intern(n["el"], types, index)
    elif n["k"] == "rec":
        n["m"] = [[off, name, _intern(sub, types, index)]
                  for off, name, sub in n.get("m", [])]
    key = json.dumps(n, sort_keys=True)
    tid = index.get(key)
    if tid is None:
        tid = len(types)
        types.append(n)
        index[key] = tid
    return tid


def _header_hash() -> str:
    from homm3.core import msvc_names
    """A fingerprint of every header a TU can see. Any header edit relays out
    to every unit, so it invalidates the whole harvest; a .cpp edit does not."""
    from homm3.core.paths import REPO
    h = hashlib.sha1()
    h.update(__import__('pathlib').Path(__file__).read_bytes())
    h.update(__import__('pathlib').Path(msvc_names.__file__).read_bytes())
    h.update((REPO / 'config/units.toml').read_bytes())
    for root in ("src", "include", "vendor", "build/gen/msvc-include"):
        for p in sorted((REPO / root).rglob("*")):
            if p.is_file() and (p.suffix in (".h", ".hpp", ".inl") or root.endswith('msvc-include')):
                h.update(str(p.relative_to(REPO)).encode())
                h.update(p.read_bytes())
    return h.hexdigest()[:16]


def _tu_fingerprint(path: str) -> str:
    try:
        payload = __import__('pathlib').Path(path).read_bytes()
    except OSError:
        return ""
    return hashlib.sha256(payload).hexdigest()


def harvest(rebuild: bool = False, jobs: int | None = None,
            units: list[str] | None = None) -> tuple[Layout, list[str]]:
    """(Layout, problems) - INCREMENTAL: a unit is re-parsed only when a header
    changed (headers relay to everyone) or its own TU file did. A full harvest
    is ~20 s over 300 TUs; the one-.cpp case is ~1 s, which is what a matcher
    editing one file pays on the next full tier."""
    import concurrent.futures as cf

    from homm3 import manifest
    from homm3.core.paths import REPO
    from homm3.core.compiler_profile import Profiles
    from homm3.core.project import Project
    header = _header_hash()
    cached: dict = {}
    types: list[dict] = []
    index: dict[str, int] = {}
    if not rebuild and CACHE.is_file():
        try:
            doc = json.loads(CACHE.read_text())
            if doc.get("header_hash") == header \
                    and doc.get("schema") == SCHEMA:
                cached = doc.get("units", {})
                types = doc.get("types", [])
                index = {json.dumps(n, sort_keys=True): i
                         for i, n in enumerate(types)}
        except (json.JSONDecodeError, KeyError, OSError):
            cached, types, index = {}, [], {}

    profiles = Profiles(Project(REPO))
    todo, out_units = [], {}
    for u in manifest.units():
        if units and u["unit"] not in units:
            if u["unit"] in cached:
                out_units[u["unit"]] = cached[u["unit"]]
            continue
        src = os.path.realpath(str(REPO / u["source"]))
        fp = _tu_fingerprint(src)
        old = cached.get(u["unit"])
        if old is not None and old.get("fp") == fp and not rebuild:
            out_units[u["unit"]] = old
            continue
        todo.append((u["unit"], src, profiles.for_source(__import__("pathlib").Path(src))))

    problems: list[str] = [u['error'] for u in out_units.values() if u.get('error')]
    if todo:
        fps = {unit: _tu_fingerprint(src) for unit, src, _flags in todo}
        jobs_ = jobs or min(os.cpu_count() or 4, 16)
        with cf.ProcessPoolExecutor(max_workers=jobs_) as pool:
            for unit, vars_, err in pool.map(_worker, todo, chunksize=1):
                if err:
                    problems.append(err)
                out_units[unit] = {
                    "fp": fps[unit],
                    "error": err,
                    "vars": {name: {"t": _intern(v["t"], types, index),
                                    "sz": v["sz"], "clang": v["clang"]}
                             for name, v in vars_.items()}}
    doc = {"schema": SCHEMA, "header_hash": header, "types": types,
           "units": out_units}
    if units is None:
        CACHE.parent.mkdir(parents=True, exist_ok=True)
        CACHE.write_text(json.dumps(doc))
    return Layout(doc, len(todo)), problems


def main(argv=None) -> int:
    import argparse
    ap = argparse.ArgumentParser(prog="homm3 verify layout",
                                 description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--unit",
                    help="restrict to one unit (with --rebuild, re-parse only that unit)")
    ap.add_argument("--var", help="one variable's flattened field map")
    ap.add_argument("--rebuild", action="store_true",
                    help="re-parse every TU (ignore the incremental cache)")
    ap.add_argument("--limit", type=int, default=64,
                    help="cap the printed problem/field listing")
    a = ap.parse_args(argv)

    lay, problems = harvest(rebuild=a.rebuild,
                            units=[a.unit] if a.unit else None)
    nvar = sum(len(lay.vars_of(u)) for u in lay.units)
    print(f"[layout] {len(lay.units)} unit(s), {nvar} variable(s), "
          f"{len(lay.types)} distinct type(s), {lay.reparsed} re-parsed "
          f"-> {CACHE}")
    for p in problems[:a.limit]:
        print(f"  ! {p}")
    if problems and len(problems) > a.limit:
        print(f"  ... {len(problems) - a.limit} more")
    if a.var:
        for unit in sorted(lay.units):
            if a.unit and unit != a.unit:
                continue
            for name, v in sorted(lay.vars_of(unit).items()):
                if a.var not in name:
                    continue
                node = lay.node(v["t"])
                print(f"\n{name}  [{unit}]  {lay.spelling(node)}  "
                      f"{v['sz']} B")
                for f in lay.flatten(node)[:a.limit]:
                    print(f"  +0x{f.off:<5x} {f.size:<4} {f.type:<24} "
                          f"{f.path or '.'}{'' if f.resolved else '  (?)'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
