"""C11 CodeView types used by the Dreamcast structure exporter.

Layouts: https://github.com/microsoft/microsoft-pdb/blob/master/include/cvinfo.h
This is a declarator renderer, not a compiler: unresolved types keep their IDs.
"""
from __future__ import annotations

from functools import lru_cache

from homm3.core.nb11 import NB11Error, _View, numeric


PRIMITIVES = {
    0: ("<no type>", None), 3: ("void", 0), 8: ("HRESULT", 4),
    0x10: ("signed char", 1), 0x11: ("short", 2), 0x12: ("long", 4),
    0x13: ("__int64", 8), 0x20: ("unsigned char", 1),
    0x21: ("unsigned short", 2), 0x22: ("unsigned long", 4),
    0x23: ("unsigned __int64", 8), 0x30: ("bool", 1),
    0x31: ("__bool16", 2), 0x32: ("__bool32", 4),
    0x40: ("float", 4), 0x41: ("double", 8), 0x42: ("long double", 10),
    0x68: ("signed char", 1), 0x69: ("unsigned char", 1),
    0x70: ("char", 1), 0x71: ("wchar_t", 2),
    0x72: ("short", 2), 0x73: ("unsigned short", 2),
    0x74: ("int", 4), 0x75: ("unsigned int", 4),
    0x76: ("__int64", 8), 0x77: ("unsigned __int64", 8),
}
CALLS = {0: "__cdecl", 1: "__far __cdecl", 2: "__pascal", 4: "__fastcall",
         7: "__stdcall", 11: "__thiscall", 16: "__shcall"}
ACCESS = {0: "unspecified", 1: "private", 2: "protected", 3: "public"}
METHODS = {0: "ordinary", 1: "virtual", 2: "static", 3: "friend",
           4: "introducing virtual", 5: "pure virtual", 6: "pure introducing virtual"}


def _unqualified(name: str) -> str:
    depth, start = 0, 0
    for i, char in enumerate(name):
        if char == "<":
            depth += 1
        elif char == ">":
            depth = max(0, depth - 1)
        elif char == ":" and not depth and name[i:i + 2] == "::":
            start = i + 2
    return name[start:]


class Types:
    def __init__(self, records: dict[int, bytes], aliases: dict[int, set[str]] | None = None):
        self.records = records
        self.aliases = aliases or {}

    @classmethod
    def from_symbols(cls, symbols):
        aliases: dict[int, set[str]] = {}
        for rows in symbols.module_info.values():
            for row in rows:
                if row["kind"] == "typedef":
                    aliases.setdefault(row["type_index"], set()).add(row["name"])
        return cls(symbols.type_records, aliases)

    def _name(self, v: _View, at: int, index: int) -> dict:
        length, = v.unpack("<B", at)
        if at + 1 + length <= len(v.data):
            return {"name": v.string(at)}
        # Three linked class records in H3.EXE have truncated names following
        # a large numeric size leaf. S_UDT independently names the same type ID.
        prefix = v.data[at + 1:].decode("latin1")
        aliases = sorted(name for name in self.aliases.get(index, ())
                         if len(name.encode("latin1")) == length and name.startswith(prefix))
        return {"name": aliases[0] if len(aliases) == 1 else f"cv_type_{index:04x}",
                "name_truncated": True, "recorded_name_prefix": prefix,
                "name_source": "S_UDT" if len(aliases) == 1 else "unresolved"}

    @lru_cache(maxsize=None)
    def get(self, index: int) -> dict:
        if index < 0x1000:
            mode, base = (index >> 8) & 7, index & 0xff
            if mode:
                return {"kind": "pointer", "target": base, "mode": 0,
                        "pointer_kind": mode, "qualifiers": [],
                        "size": {1: 2, 2: 4, 3: 4, 4: 4, 5: 6, 6: 8}.get(mode)}
            name, size = PRIMITIVES.get(index, (f"cv_type_{index:04x}", None))
            return {"kind": "primitive", "name": name, "size": size}
        data = self.records.get(index)
        if data is None:
            return {"kind": "unresolved", "name": f"cv_type_{index:04x}"}
        v = _View(data)
        leaf, = v.unpack("<H", 2)
        item = {"leaf": leaf}
        if leaf == 0x1001:
            target, flags = v.unpack("<IH", 4)
            item.update(kind="modifier", target=target, flags=flags,
                        qualifiers=[name for bit, name in ((1, "const"), (2, "volatile"),
                                                           (4, "__unaligned")) if flags & bit])
        elif leaf == 0x1002:
            target, flags = v.unpack("<II", 4)
            mode = (flags >> 5) & 7
            item.update(kind="pointer", target=target, mode=mode,
                        pointer_kind=flags & 31, flags=flags,
                        qualifiers=[name for bit, name in ((0x400, "const"), (0x200, "volatile"),
                                                           (0x800, "__unaligned"), (0x1000, "__restrict"))
                                    if flags & bit],
                        size={0: 2, 1: 4, 2: 4, 10: 4, 11: 6, 12: 8}.get(flags & 31))
            if mode in (2, 3):
                owner, representation = v.unpack("<IH", 12)
                item.update(owner=owner, representation=representation, size=None)
        elif leaf == 0x1003:
            target, subscript = v.unpack("<II", 4)
            size, end = numeric(v, 12)
            item.update(kind="array", target=target, subscript=subscript,
                        size=size, name=v.string(end))
        elif leaf in (0x1004, 0x1005, 0x1006, 0x1007):
            count, flags = v.unpack("<HH", 4)
            item.update(kind={0x1004: "class", 0x1005: "struct", 0x1006: "union",
                              0x1007: "enum"}[leaf], count=count, flags=flags,
                        forward=bool(flags & 0x80))
            if leaf == 0x1007:
                underlying, fields = v.unpack("<II", 8)
                item.update(underlying=underlying, fields=fields, name=v.string(16))
            else:
                fields, = v.unpack("<I", 8)
                if leaf != 0x1006:
                    derived, vshape = v.unpack("<II", 12)
                    item.update(derived=derived, vshape=vshape)
                size, end = numeric(v, 12 if leaf == 0x1006 else 20)
                item.update(fields=fields, size=size, **self._name(v, end, index))
        elif leaf in (0x1008, 0x1009):
            ret, = v.unpack("<I", 4)
            at = 8
            item.update(kind="function", returns=ret)
            if leaf == 0x1009:
                owner, this = v.unpack("<II", 8)
                item.update(owner=owner, this=this)
                at = 16
            call, flags, count, arguments = v.unpack("<BBHI", at)
            item.update(call=call, calling_convention=CALLS.get(call, f"cv_call_{call}"),
                        flags=flags, count=count, arguments=arguments)
            if leaf == 0x1009:
                item["this_adjust"], = v.unpack("<i", 24)
        elif leaf in (0x1201, 0x1204):
            count, = v.unpack("<I", 4)
            item.update(kind="arguments" if leaf == 0x1201 else "derived",
                        types=list(v.unpack(f"<{count}I", 8)))
        elif leaf == 0x1205:
            target, length, position = v.unpack("<IBB", 4)
            item.update(kind="bitfield", target=target, length=length, position=position)
        elif leaf == 0x1203:
            item.update(kind="fields", entries=self._fields(v))
        elif leaf == 0x1206:
            entries, at = [], 4
            while at < len(data) and data[at] < 0xf0:
                attr, _pad, typ = v.unpack("<HHI", at)
                entry = self._method(attr, typ)
                at += 8
                if (attr >> 2) & 7 in (4, 6):
                    entry["vtable_offset"], = v.unpack("<i", at)
                    at += 4
                entries.append(entry)
            item.update(kind="methods", entries=entries)
        elif leaf == 0xa:
            count, = v.unpack("<H", 4)
            packed = v.part(6, (count + 1) // 2)
            item.update(kind="vtable_shape", slots=[
                (packed[i // 2] >> (4 if i % 2 == 0 else 0)) & 15 for i in range(count)])
        else:
            item.update(kind="unresolved", name=f"cv_type_{index:04x}")
        return item

    @staticmethod
    def _method(attr: int, typ: int) -> dict:
        return {"kind": "method", "type": typ, "attributes": attr,
                "access": ACCESS[attr & 3], "property": METHODS.get((attr >> 2) & 7, "unknown")}

    def _fields(self, v: _View) -> list[dict]:
        at, entries = 4, []
        while at < len(v.data):
            if v.data[at] >= 0xf0:
                pad = v.data[at] & 15
                if not pad:
                    raise NB11Error("invalid field-list padding")
                v.part(at, pad)
                at += pad
                continue
            leaf, = v.unpack("<H", at)
            start = at
            if leaf == 0x403:  # LF_ENUMERATE_ST
                attr, = v.unpack("<H", at + 2)
                value, at = numeric(v, at + 4)
                name = v.string(at)
                at += 1 + len(name.encode("latin1"))
                entry = {"kind": "enumerator", "name": name, "value": value, "attributes": attr}
            elif leaf in (0x1400, 0x1401, 0x1402, 0x1405, 0x1406, 0x140b):
                attr, typ = v.unpack("<HI", at + 2)
                at += 8
                entry = {"attributes": attr, "access": ACCESS[attr & 3], "type": typ}
                if leaf == 0x1400:
                    value, at = numeric(v, at)
                    entry.update(kind="base", offset=value)
                elif leaf in (0x1401, 0x1402):
                    vbptr, = v.unpack("<I", at)
                    offset, at = numeric(v, at + 4)
                    slot, at = numeric(v, at)
                    entry.update(kind="virtual_base", indirect=leaf == 0x1402,
                                 vbptr=vbptr, vbptr_offset=offset, vbtable_index=slot)
                else:
                    if leaf == 0x1405:
                        value, at = numeric(v, at)
                        entry.update(kind="member", offset=value)
                    elif leaf == 0x1406:
                        entry["kind"] = "static_member"
                    else:
                        entry = self._method(attr, typ)
                        if (attr >> 2) & 7 in (4, 6):
                            entry["vtable_offset"], = v.unpack("<i", at)
                            at += 4
                    entry["name"] = v.string(at)
                    at += 1 + len(entry["name"].encode("latin1"))
            elif leaf in (0x1404, 0x1408, 0x1409):
                _pad, typ = v.unpack("<HI", at + 2)
                at += 8
                entry = {"kind": {0x1404: "continuation", 0x1408: "nested_type", 0x1409: "vfptr"}[leaf],
                         "type": typ}
                if leaf == 0x1408:
                    entry["name"] = v.string(at)
                    at += 1 + len(entry["name"].encode("latin1"))
            elif leaf == 0x1407:
                count, typ = v.unpack("<HI", at + 2)
                name = v.string(at + 8)
                at += 9 + len(name.encode("latin1"))
                entry = {"kind": "overloads", "name": name, "count": count, "type": typ}
            else:
                # Preserve the location of an undecoded tail; never guess its length.
                entries.append({"kind": "unresolved", "leaf": leaf, "record_offset": at})
                break
            entry.update(leaf=leaf, record_offset=start)
            entries.append(entry)
        return entries

    def fields(self, index: int, seen: frozenset[int] = frozenset()) -> list[dict]:
        if not index or index in seen:
            return []
        out = []
        for entry in self.get(index).get("entries", []):
            if entry["kind"] == "continuation":
                out.extend(self.fields(entry["type"], seen | {index}))
            elif entry["kind"] == "overloads":
                for method in self.get(entry["type"]).get("entries", []):
                    out.append(dict(method, name=entry["name"]))
            else:
                out.append(entry)
        return out

    def size(self, index: int, seen: frozenset[int] = frozenset()) -> int | None:
        if index in seen:
            return None
        item = self.get(index)
        if item["kind"] in ("modifier", "bitfield"):
            return self.size(item["target"], seen | {index})
        if item["kind"] == "enum":
            return self.size(item["underlying"], seen | {index})
        return item.get("size")

    def declaration(self, index: int, name: str = "", *, qualifiers: tuple[str, ...] = (),
                    seen: frozenset[int] = frozenset()) -> str:
        if index in seen:
            return f"cv_type_{index:04x} {name}".strip()
        item = self.get(index)
        seen = seen | {index}
        kind = item["kind"]
        if kind == "modifier":
            return self.declaration(item["target"], name,
                                    qualifiers=tuple(item["qualifiers"]) + qualifiers, seen=seen)
        if kind == "pointer":
            mode = item["mode"]
            token = "&" if mode == 1 else "*"
            if mode in (2, 3):
                token = self.declaration(item["owner"]) + "::*"
            q = " ".join((*item["qualifiers"], *qualifiers))
            declarator = token + (" " + q + " " if q else "") + name
            target = self.get(item["target"])
            if target["kind"] in ("array", "function"):
                declarator = "(" + declarator + ")"
            return self.declaration(item["target"], declarator, seen=seen)
        if kind == "array":
            size = self.size(item["target"])
            count = str(item["size"] // size) if size and item["size"] % size == 0 else ""
            return self.declaration(item["target"], f"{name}[{count}]", qualifiers=qualifiers, seen=seen)
        if kind == "bitfield":
            return self.declaration(item["target"], f"{name} : {item['length']}", seen=seen)
        if kind == "function":
            return self.function(index, name, seen=seen)
        prefix = " ".join(qualifiers)
        return " ".join(part for part in (prefix, item.get("name", f"cv_type_{index:04x}"), name) if part)

    def function(self, index: int, name: str, parameters: list[str] | None = None,
                 *, seen: frozenset[int] = frozenset()) -> str:
        item = self.get(index)
        if item["kind"] != "function":
            return f"cv_type_{index:04x} {name}(/* signature unavailable */)"
        arguments = self.get(item["arguments"]).get("types", [])
        args = ["..." if typ == 0 else self.declaration(
            typ, parameters[i] if parameters is not None and i < len(parameters) else "", seen=seen)
                for i, typ in enumerate(arguments)]
        suffix = ""
        this = self.get(item.get("this", 0))
        if this["kind"] == "pointer":
            target = self.get(this["target"])
            if target["kind"] == "modifier":
                suffix = " " + " ".join(target["qualifiers"])
        call = item["calling_convention"]
        head = "(" + call + " " + name[1:] if name.startswith("(") else f"{call} {name}"
        declarator = f"{head}({', '.join(args)}){suffix}"
        method = _unqualified(name).split("<", 1)[0]
        owner = _unqualified(self.get(item.get("owner", 0)).get("name", "")).split("<", 1)[0]
        special = bool(owner and method in (owner, "~" + owner))
        if item["returns"] == 0 or special:
            return declarator
        return self.declaration(item["returns"], declarator, seen=seen | {index})

    def procedure_signature(self, procedure) -> str:
        parameters = [v for v in procedure.variables
                      if v.kind == "param" and v.name not in ("this", "__$ReturnUdt")]
        function = self.get(procedure.type_index)
        if function["kind"] != "function":
            args = ", ".join(self.declaration(v.type_index, v.name) for v in parameters)
            return f"/* return type / calling convention unavailable */ {procedure.name}({args})"
        arguments = self.get(function["arguments"]).get("types", [])
        # Absent optimized-out parameter records supply no names. For a partial
        # inventory, positional names would be guesses; keep the inventory in
        # its own section instead. Hidden return storage is an ABI parameter.
        names = [v.name for v in parameters] if len(parameters) == sum(t != 0 for t in arguments) else None
        return self.function(procedure.type_index, procedure.name, names)
